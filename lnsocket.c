

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <secp256k1.h>
#include <secp256k1_ecdh.h>
#include <sodium/crypto_aead_chacha20poly1305.h>
#include <sodium/randombytes.h>

#include "sha256.h"
#include "compiler.h"
#include "hkdf.h"
#include "handshake.h"
#include "endian.h"

#define array_len(x) (sizeof(x)/sizeof(x[0]))

struct lnsocket {
	const char *errors[8];
	int socket;
	int num_errors;
	secp256k1_context *secp_ctx;
};


static struct keypair generate_key(secp256k1_context *ctx)
{
	struct keypair k;

	do {
		randombytes_buf(k.priv.secret.data, sizeof(k.priv.secret.data));
	} while (!secp256k1_ec_pubkey_create(ctx, &k.pub.pubkey,
				k.priv.secret.data));
	return k;
}


static void push_error(struct lnsocket *lnsocket, const char *err)
{
	if (lnsocket->num_errors >= array_len(lnsocket->errors)) {
		// TODO: push out old errors instead
		return;
	}

	lnsocket->errors[lnsocket->num_errors++] = err;
}

static void lnsocket_init(struct lnsocket *lnsocket)
{
	memset(lnsocket, 0, sizeof(*lnsocket));

	lnsocket->secp_ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY |
						 SECP256K1_CONTEXT_SIGN);
}

/* h = SHA-256(h || data) */
static void sha_mix_in(struct sha256 *h, const void *data, size_t len)
{
	struct sha256_ctx shactx;

	sha256_init(&shactx);
	sha256_update(&shactx, h->u.u8, sizeof(*h));
	sha256_update(&shactx, data, len);
	sha256_done(&shactx, h);
}

/* h = SHA-256(h || pub.serializeCompressed()) */
static void sha_mix_in_key(secp256k1_context *ctx, struct sha256 *h,
		const struct pubkey *key)
{
	u8 der[PUBKEY_CMPR_LEN];
	size_t len = sizeof(der);

	secp256k1_ec_pubkey_serialize(ctx, der, &len, &key->pubkey,
				      SECP256K1_EC_COMPRESSED);
	assert(len == sizeof(der));
	sha_mix_in(h, der, sizeof(der));
}

static int handshake_failed(struct lnsocket *ln, struct handshake *h)
{
	push_error(ln, "handshake failed");
	return 0;
}

/* out1, out2 = HKDF(in1, in2)` */
static void hkdf_two_keys(struct secret *out1, struct secret *out2,
			  const struct secret *in1,
			  const void *in2, size_t in2_size)
{
	/* BOLT #8:
	 *
	 *   * `HKDF(salt,ikm)`: a function defined in `RFC 5869`<sup>[3](#reference-3)</sup>,
	 *      evaluated with a zero-length `info` field
	 *      * All invocations of `HKDF` implicitly return 64 bytes
	 *        of cryptographic randomness using the extract-and-expand
	 *        component of the `HKDF`.
	 */
	struct secret okm[2];

	hkdf_sha256(okm, sizeof(okm), in1, sizeof(*in1), in2, in2_size,
		    NULL, 0);
	*out1 = okm[0];
	*out2 = okm[1];
}


static void le64_nonce(unsigned char *npub, u64 nonce)
{
	/* BOLT #8:
	 *
	 * ...with nonce `n` encoded as 32 zero bits, followed by a
	 * *little-endian* 64-bit value.  Note: this follows the Noise Protocol
	 * convention, rather than our normal endian
	 */
	le64 le_nonce = cpu_to_le64(nonce);
	const size_t zerolen = crypto_aead_chacha20poly1305_ietf_NPUBBYTES - sizeof(le_nonce);

	BUILD_ASSERT(crypto_aead_chacha20poly1305_ietf_NPUBBYTES >= sizeof(le_nonce));
	/* First part is 0, followed by nonce. */
	memset(npub, 0, zerolen);
	memcpy(npub + zerolen, &le_nonce, sizeof(le_nonce));
}

/* BOLT #8:
 *   * `encryptWithAD(k, n, ad, plaintext)`: outputs `encrypt(k, n, ad,
 *      plaintext)`
 *      * Where `encrypt` is an evaluation of `ChaCha20-Poly1305` (IETF
 *	  variant) with the passed arguments, with nonce `n`
 */
static void encrypt_ad(const struct secret *k, u64 nonce,
		       const void *additional_data, size_t additional_data_len,
		       const void *plaintext, size_t plaintext_len,
		       void *output, size_t outputlen)
{
	unsigned char npub[crypto_aead_chacha20poly1305_ietf_NPUBBYTES];
	unsigned long long clen;
	int ret;

	assert(outputlen == plaintext_len + crypto_aead_chacha20poly1305_ietf_ABYTES);
	le64_nonce(npub, nonce);
	BUILD_ASSERT(sizeof(*k) == crypto_aead_chacha20poly1305_ietf_KEYBYTES);

	ret = crypto_aead_chacha20poly1305_ietf_encrypt(
			output, &clen, memcheck(plaintext, plaintext_len),
			plaintext_len, additional_data, additional_data_len,
			NULL, npub, k->data);

	assert(ret == 0);
	assert(clen == plaintext_len + crypto_aead_chacha20poly1305_ietf_ABYTES);
}


int act_one_initiator(struct lnsocket *ln, struct handshake *h)
{
	h->e = generate_key(ln->secp_ctx);

	/* BOLT #8:
	 *
	 * 2. `h = SHA-256(h || e.pub.serializeCompressed())`
	 *     * The newly generated ephemeral key is accumulated into the
	 *       running handshake digest.
	 */
	sha_mix_in_key(ln->secp_ctx, &h->h, &h->e.pub);

	/* BOLT #8:
	 *
	 * 3. `es = ECDH(e.priv, rs)`
	 *      * The initiator performs an ECDH between its newly generated ephemeral
	 *        key and the remote node's static public key.
	 */
	if (!secp256k1_ecdh(ln->secp_ctx, h->ss.data,
			    &h->their_id.pubkey, h->e.priv.secret.data,
			    NULL, NULL))
		return handshake_failed(ln, h);

	/* BOLT #8:
	 *
	 * 4. `ck, temp_k1 = HKDF(ck, es)`
	 *      * A new temporary encryption key is generated, which is
	 *        used to generate the authenticating MAC.
	 */
	hkdf_two_keys(&h->ck, &h->temp_k, &h->ck, &h->ss, sizeof(h->ss));

	/* BOLT #8:
	 * 5. `c = encryptWithAD(temp_k1, 0, h, zero)`
	 *     * where `zero` is a zero-length plaintext
	 */
	encrypt_ad(&h->temp_k, 0, &h->h, sizeof(h->h), NULL, 0,
		   h->act1.tag, sizeof(h->act1.tag));

	return 1;
}

int connect_node(struct lnsocket *ln, const struct pubkey pubkey, const char *host)
{
	int ret;
	struct addrinfo *addrs = NULL;
	struct handshake h;
	
	h.their_id = pubkey;

	if ((ret = getaddrinfo(host, "9735", NULL, &addrs)) || !addrs) {
		push_error(ln, gai_strerror(ret));
		return 0;
	}

	if (!(ln->socket = socket(AF_INET, SOCK_STREAM, 0))) {
		push_error(ln, "creating socket failed");
		return 0;
	}

	if (connect(ln->socket, addrs->ai_addr, addrs->ai_addrlen) == -1) {
		push_error(ln, strerror(errno));
		return 0;
	}

	return act_one_initiator(ln, &h);
}

static struct pubkey nodeid = {
	.pubkey = {
		.data = {
  0x03, 0xf3, 0xc1, 0x08, 0xcc, 0xd5, 0x36, 0xb8, 0x52, 0x68, 0x41, 0xf0,
  0xa5, 0xc5, 0x82, 0x12, 0xbb, 0x9e, 0x65, 0x84, 0xa1, 0xeb, 0x49, 0x30,
  0x80, 0xe7, 0xc1, 0xcc, 0x34, 0xf8, 0x2d, 0xad, 0x71 } }
};

int main(int argc, const char *argv[])
{
	struct lnsocket ln;
	lnsocket_init(&ln);

	if (!connect_node(&ln, nodeid, "24.84.152.187")) {
		printf("connection failed: %s\n", ln.errors[0]);
		return 1;
	}

	printf("connected!\n");

	return 0;
}
