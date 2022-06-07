/* Copyright Rusty Russell (Blockstream) 2015.
             William Casarin 2022

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "compiler.h"
#include "endian.h"
#include "error.h"
#include "handshake.h"
#include "hkdf.h"
#include "lnsocket_internal.h"
#include "sha256.h"
#include <errno.h>
#include <secp256k1_ecdh.h>
#include <unistd.h>
#include <sodium/randombytes.h>
#include "export.h"

struct keypair generate_key(secp256k1_context *ctx)
{
	struct keypair k;

	do {
		randombytes_buf(k.priv.secret.data, sizeof(k.priv.secret.data));
	} while (!secp256k1_ec_pubkey_create(ctx, &k.pub.pubkey,
				k.priv.secret.data));
	return k;
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

/*
static void print_hex(u8 *bytes, int len) {
	int i;
	for (i = 0; i < len; ++i) {
		printf("%02x", bytes[i]);
	}
}
*/

void new_handshake(secp256k1_context *secp, struct handshake *handshake,
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

/*
static void print_act_two(struct act_two *two)
{
	printf("ACT2 v %d pubkey ", two->v);
	print_hex(two->pubkey, sizeof(two->pubkey));
	printf(" tag ");
	print_hex(two->tag, sizeof(two->tag));
	printf("\n");
}
*/

/* BOLT #8:
 *    * `decryptWithAD(k, n, ad, ciphertext)`: outputs `decrypt(k, n, ad,
 *       ciphertext)`
 *      * Where `decrypt` is an evaluation of `ChaCha20-Poly1305` (IETF
 *        variant) with the passed arguments, with nonce `n`
 */
static int decrypt(const struct secret *k, u64 nonce,
		const void *additional_data, size_t additional_data_len,
		const void *ciphertext, size_t ciphertext_len,
		void *output, size_t outputlen)
{
	unsigned char npub[crypto_aead_chacha20poly1305_ietf_NPUBBYTES];
	unsigned long long mlen;

	assert(outputlen == ciphertext_len - crypto_aead_chacha20poly1305_ietf_ABYTES);

	le64_nonce(npub, nonce);
	BUILD_ASSERT(sizeof(*k) == crypto_aead_chacha20poly1305_ietf_KEYBYTES);
	if (crypto_aead_chacha20poly1305_ietf_decrypt(output, &mlen, NULL,
						 memcheck(ciphertext, ciphertext_len),
						 ciphertext_len,
						 additional_data, additional_data_len,
						 npub, k->data) != 0) {
		return 0;
	}

	assert(mlen == ciphertext_len - crypto_aead_chacha20poly1305_ietf_ABYTES);
	return 1;
}

static int handshake_success(struct lnsocket *ln, struct handshake *h)
{
	struct crypto_state *cs = &ln->crypto_state;

	/* BOLT #8:
	 *
	 * 9. `rk, sk = HKDF(ck, zero)`
	 *      * where `zero` is a zero-length plaintext, `rk` is the key to
	 *        be used by the responder to decrypt the messages sent by the
	 *        initiator, and `sk` is the key to be used by the responder
	 *        to encrypt messages to the initiator
	 *
	 *      * The final encryption keys, to be used for sending and
	 *        receiving messages for the duration of the session, are
	 *        generated.
	 */
	if (h->side == RESPONDER)
		hkdf_two_keys(&cs->rk, &cs->sk, &h->ck, NULL);
	else
		hkdf_two_keys(&cs->sk, &cs->rk, &h->ck, NULL);

	cs->rn = cs->sn = 0;
	cs->r_ck = cs->s_ck = h->ck;

	return 1;
}

static struct act_three *build_act_three(struct lnsocket *ln)
{
	u8 spub[PUBKEY_CMPR_LEN];
	size_t len = sizeof(spub);
	struct handshake *h = &ln->handshake;

	/* BOLT #8:
	 * 1. `c = encryptWithAD(temp_k2, 1, h, s.pub.serializeCompressed())`
	 *     * where `s` is the static public key of the initiator
	 */
	secp256k1_ec_pubkey_serialize(ln->secp, spub, &len,
				      &ln->key.pub.pubkey,
				      SECP256K1_EC_COMPRESSED);
	encrypt_ad(&h->temp_k, 1, &h->h, sizeof(h->h), spub, sizeof(spub),
		   h->act3.ciphertext, sizeof(h->act3.ciphertext));

	/* BOLT #8:
	 * 2. `h = SHA-256(h || c)`
	 */
	sha_mix_in(&h->h, h->act3.ciphertext, sizeof(h->act3.ciphertext));

	/* BOLT #8:
	 *
	 * 3. `se = ECDH(s.priv, re)`
	 *     * where `re` is the ephemeral public key of the responder
	 */
	if (!secp256k1_ecdh(ln->secp, h->ss.data, &h->re.pubkey,
			    ln->key.priv.secret.data, NULL, NULL)) {
		note_error(&ln->errs, "act3 ecdh handshake failed");
		return NULL;
	}

	/* BOLT #8:
	 *
	 * 4. `ck, temp_k3 = HKDF(ck, se)`
	 *     * The final intermediate shared secret is mixed into the running chaining key.
	 */
	hkdf_two_keys(&h->ck, &h->temp_k, &h->ck, &h->ss);

	/* BOLT #8:
	 *
	 * 5. `t = encryptWithAD(temp_k3, 0, h, zero)`
	 *      * where `zero` is a zero-length plaintext
	 *
	 */
	encrypt_ad(&h->temp_k, 0, &h->h, sizeof(h->h), NULL, 0,
		   h->act3.tag, sizeof(h->act3.tag));

	/* BOLT #8:
	 *
	 * 8.  Send `m = 0 || c || t` over the network buffer.
	 *
	 */
	h->act3.v = 0;

        handshake_success(ln, &ln->handshake);

	return &h->act3;
}


// act2: handle the response to the message sent in act1
struct act_three* EXPORT lnsocket_act_two(struct lnsocket *ln, struct act_two *act2)
{
	struct handshake *h = &ln->handshake;

	//print_act_two(act2);

	/* BOLT #8:
	 *
	 * 3. If `v` is an unrecognized handshake version, then the responder
	 *     MUST abort the connection attempt.
	 */
	if (act2->v != 0) {
		note_error(&ln->errs, "unrecognized handshake version");
		return NULL;
	}

	//print_hex()

	/* BOLT #8:
	 *
	 *     * The raw bytes of the remote party's ephemeral public key
	 *       (`re`) are to be deserialized into a point on the curve using
	 *       affine coordinates as encoded by the key's serialized
	 *       composed format.
	 */
	if (secp256k1_ec_pubkey_parse(ln->secp, &h->re.pubkey, act2->pubkey,
				sizeof(act2->pubkey)) != 1) {
		note_error(&ln->errs, "failed to parse remote pubkey");
		return NULL;
	}

	/* BOLT #8:
	 *
	 * 4. `h = SHA-256(h || re.serializeCompressed())`
	 */
	sha_mix_in_key(ln->secp, &h->h, &h->re);

	/* BOLT #8:
	 *
	 * 5. `es = ECDH(s.priv, re)`
	 */
	if (!secp256k1_ecdh(ln->secp, h->ss.data, &h->re.pubkey,
			    h->e.priv.secret.data, NULL, NULL)) {
		note_error(&ln->errs, "act2 ecdh failed");
		return NULL;
	}

	/* BOLT #8:
	 *
	 * 6. `ck, temp_k2 = HKDF(ck, ee)`
	 *      * A new temporary encryption key is generated, which is
	 *        used to generate the authenticating MAC.
	 */
	hkdf_two_keys(&h->ck, &h->temp_k, &h->ck, &h->ss);

	/* BOLT #8:
	 *
	 * 7. `p = decryptWithAD(temp_k2, 0, h, c)`
	 *     * If the MAC check in this operation fails, then the initiator
	 *       MUST terminate the connection without any further messages.
	 */
	if (!decrypt(&h->temp_k, 0, &h->h, sizeof(h->h),
		     act2->tag, sizeof(act2->tag), NULL, 0)) {
		note_error(&ln->errs, "handshake decrypt failed");
		return NULL;
	}

	/* BOLT #8:
	 *
	 * 8. `h = SHA-256(h || c)`
	 *     * The received ciphertext is mixed into the handshake digest.
	 *       This step serves to ensure the payload wasn't modified by a
	 *       MITM.
	 */
	sha_mix_in(&h->h, act2->tag, sizeof(act2->tag));

	return build_act_three(ln);
}

// Prepare the very first message and send it the connected node
// Wait for a response in act2
int act_one_initiator_prep(struct lnsocket *ln)
{
	struct handshake *h = &ln->handshake;

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
			    NULL, NULL)) {
		return note_error(&ln->errs, "handshake failed, secp256k1_ecdh error");
	}

	/* BOLT #8:
	 *
	 * 4. `ck, temp_k1 = HKDF(ck, es)`
	 *      * A new temporary encryption key is generated, which is
	 *        used to generate the authenticating MAC.
	 */
	hkdf_two_keys(&h->ck, &h->temp_k, &h->ck, &h->ss);

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

	return 1;
}

// act2: read the response to the message sent in act1
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
		fprintf(stderr, "read %ld bytes, expected %d\n", size, ACT_TWO_SIZE);
		return note_error(&ln->errs, "%s", strerror(errno));
	}

	struct act_three *act3 = lnsocket_act_two(ln, &h->act2);
	if (act3 == NULL)
		return 0;

	if (write(ln->socket, act3, ACT_THREE_SIZE) != ACT_THREE_SIZE) {
		return note_error(&ln->errs, "handshake failed on initial send");
	}

	return 1;
}

int act_one_initiator(struct lnsocket *ln)
{
	if (!act_one_initiator_prep(ln))
		return 0;

	if (write(ln->socket, &ln->handshake.act1, ACT_ONE_SIZE) != ACT_ONE_SIZE) {
		return note_error(&ln->errs, "handshake failed on initial send");
	}

	return act_two_initiator(ln, &ln->handshake);
}
