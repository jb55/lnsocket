

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

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
	secp256k1_context *secp;
};


static bool char_to_hex(unsigned char *val, char c)
{
	if (c >= '0' && c <= '9') {
		*val = c - '0';
		return true;
	}
 	if (c >= 'a' && c <= 'f') {
		*val = c - 'a' + 10;
		return true;
	}
 	if (c >= 'A' && c <= 'F') {
		*val = c - 'A' + 10;
		return true;
	}
	return false;
}

static bool hex_decode(const char *str, size_t slen, void *buf, size_t bufsize)
{
	unsigned char v1, v2;
	unsigned char *p = buf;

	while (slen > 1) {
		if (!char_to_hex(&v1, str[0]) || !char_to_hex(&v2, str[1]))
			return false;
		if (!bufsize)
			return false;
		*(p++) = (v1 << 4) | v2;
		str += 2;
		slen -= 2;
		bufsize--;
	}
	return slen == 0 && bufsize == 0;
}

int parse_node_id(const char *str, struct node_id *dest)
{
	return hex_decode(str, strlen(str), dest->k, sizeof(*dest));
}

int pubkey_from_node_id(secp256k1_context *secp, struct pubkey *key,
		const struct node_id *id)
{
	return secp256k1_ec_pubkey_parse(secp, &key->pubkey,
					 memcheck(id->k, sizeof(id->k)),
					 sizeof(id->k));
}


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

	lnsocket->secp = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY |
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

static inline void check_act_one(const struct act_one *act1)
{
	/* BOLT #8:
	 *
	 * : 1 byte for the handshake version, 33 bytes for the compressed
	 * ephemeral public key of the initiator, and 16 bytes for the
	 * `poly1305` tag.
	 */
	BUILD_ASSERT(sizeof(act1->v) == 1);
	BUILD_ASSERT(sizeof(act1->pubkey) == 33);
	BUILD_ASSERT(sizeof(act1->tag) == 16);
}

static void print_hex(u8 *bytes, int len) {
	int i;
	for (i = 0; i < len; ++i) {
		printf("%02x", bytes[i]);
	}
}

static void new_handshake(secp256k1_context *secp, struct handshake *handshake,
		const struct pubkey *responder_id)
{
	/* BOLT #8:
	 *
	 * Before the start of Act One, both sides initialize their
	 * per-sessions state as follows:
	 *
	 *  1. `h = SHA-256(protocolName)`
	 *   *  where `protocolName = "Noise_XK_secp256k1_ChaChaPoly_SHA256"`
	 *      encoded as an ASCII string
	 */
	sha256(&handshake->h, "Noise_XK_secp256k1_ChaChaPoly_SHA256",
	       strlen("Noise_XK_secp256k1_ChaChaPoly_SHA256"));

	/* BOLT #8:
	 *
	 * 2. `ck = h`
	 */
	BUILD_ASSERT(sizeof(handshake->h) == sizeof(handshake->ck));
	memcpy(&handshake->ck, &handshake->h, sizeof(handshake->ck));

	/* BOLT #8:
	 *
	 * 3. `h = SHA-256(h || prologue)`
	 *    *  where `prologue` is the ASCII string: `lightning`
	 */
	sha_mix_in(&handshake->h, "lightning", strlen("lightning"));

	/* BOLT #8:
	 *
	 * As a concluding step, both sides mix the responder's public key
	 * into the handshake digest:
	 *
	 * * The initiating node mixes in the responding node's static public
	 *    key serialized in Bitcoin's compressed format:
	 *    * `h = SHA-256(h || rs.pub.serializeCompressed())`
	 *
	 * * The responding node mixes in their local static public key
	 *   serialized in Bitcoin's compressed format:
	 *    * `h = SHA-256(h || ls.pub.serializeCompressed())`
	 */
	sha_mix_in_key(secp, &handshake->h, responder_id);
}

static void print_act_two(struct act_two *two)
{
	printf("ACT2 v %d pubkey ", two->v);
	print_hex(two->pubkey, sizeof(two->pubkey));
	printf("tag ");
	print_hex(two->tag, sizeof(two->tag));
	printf("\n");
}

static int act_two_initiator(struct lnsocket *ln, struct handshake *h)
{
	/* BOLT #8:
	 *
	 * 1. Read _exactly_ 50 bytes from the network buffer.
	 *
	 * 2. Parse the read message (`m`) into `v`, `re`, and `c`:
	 *    * where `v` is the _first_ byte of `m`, `re` is the next 33
	 *      bytes of `m`, and `c` is the last 16 bytes of `m`.
	 */
	ssize_t size;

	if ((size = read(ln->socket, &h->act2, ACT_TWO_SIZE)) != ACT_TWO_SIZE) {
		printf("read %ld bytes, expected %d\n", size, ACT_TWO_SIZE);
		push_error(ln, strerror(errno));
		return 0;
	}

	print_act_two(&h->act2);
	return 0;
}

int act_one_initiator(struct lnsocket *ln, struct handshake *h)
{
	h->e = generate_key(ln->secp);

	/* BOLT #8:
	 *
	 * 2. `h = SHA-256(h || e.pub.serializeCompressed())`
	 *     * The newly generated ephemeral key is accumulated into the
	 *       running handshake digest.
	 */
	sha_mix_in_key(ln->secp, &h->h, &h->e.pub);

	/* BOLT #8:
	 *
	 * 3. `es = ECDH(e.priv, rs)`
	 *      * The initiator performs an ECDH between its newly generated ephemeral
	 *        key and the remote node's static public key.
	 */
	if (!secp256k1_ecdh(ln->secp, h->ss.data,
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

	/* BOLT #8:
	 * 6. `h = SHA-256(h || c)`
	 *     * Finally, the generated ciphertext is accumulated into the
	 *       authenticating handshake digest.
	 */
	sha_mix_in(&h->h, h->act1.tag, sizeof(h->act1.tag));

	/* BOLT #8:
	 *
	 * 7. Send `m = 0 || e.pub.serializeCompressed() || c` to the responder over the network buffer.
	 */
	h->act1.v = 0;
	size_t len = sizeof(h->act1.pubkey);
	secp256k1_ec_pubkey_serialize(ln->secp, h->act1.pubkey, &len,
				      &h->e.pub.pubkey,
				      SECP256K1_EC_COMPRESSED);

	check_act_one(&h->act1);

	if (write(ln->socket, &h->act1, ACT_ONE_SIZE) != ACT_ONE_SIZE)
		return handshake_failed(ln, h);

	return act_two_initiator(ln, h);
}

int connect_node(struct lnsocket *ln, const char *node_id, const char *host)
{
	int ret;
	struct addrinfo *addrs = NULL;
	struct handshake h;
	struct keypair my_id;
	struct pubkey their_id;
	struct node_id their_node_id;

	if (!parse_node_id(node_id, &their_node_id)) {
		push_error(ln, "failed to parse node id");
		return 0;
	}

	if (!pubkey_from_node_id(ln->secp, &their_id, &their_node_id)) {
		push_error(ln, "failed to convert node_id to pubkey");
		return 0;
	}

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

	my_id = generate_key(ln->secp);
	new_handshake(ln->secp, &h, &their_id);

	h.side = INITIATOR;
	h.my_id = my_id.pub;
	h.their_id = their_id;

	return act_one_initiator(ln, &h);
}

int main(int argc, const char *argv[])
{
	struct lnsocket ln;
	lnsocket_init(&ln);

	const char *nodeid = "03f3c108ccd536b8526841f0a5c58212bb9e6584a1eb493080e7c1cc34f82dad71";

	if (!connect_node(&ln, nodeid, "24.84.152.187")) {
		printf("connection failed: %s\n", ln.errors[0]);
		return 1;
	}

	printf("connected!\n");

	return 0;
}
