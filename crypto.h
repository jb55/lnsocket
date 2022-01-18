
#ifndef LNSOCKET_CRYPTO_H
#define LNSOCKET_CRYPTO_H

#include <secp256k1.h>
#include "typedefs.h"

#define PUBKEY_CMPR_LEN 33

struct secret {
	u8 data[32];
};

struct node_id {
	u8 k[PUBKEY_CMPR_LEN];
};

struct pubkey {
	secp256k1_pubkey pubkey;
};

struct privkey {
	struct secret secret;
};

struct keypair {
	struct pubkey pub;
	struct privkey priv;
};

struct crypto_state {
	/* Received and sent nonces. */
	u64 rn, sn;
	/* Sending and receiving keys. */
	struct secret sk, rk;
	/* Chaining key for re-keying */
	struct secret s_ck, r_ck;
};

void le64_nonce(unsigned char *npub, u64 nonce);

void hkdf_two_keys(struct secret *out1, struct secret *out2,
			  const struct secret *in1,
			  const struct secret *in2);

int cryptomsg_decrypt_body(struct crypto_state *cs, const u8 *in, size_t inlen, u8 *out, size_t outcap);
int cryptomsg_decrypt_header(struct crypto_state *cs, u8 hdr[18], u16 *lenp);
unsigned char *cryptomsg_encrypt_msg(struct crypto_state *cs, const u8 *msg, unsigned long long mlen, u8 *out, size_t *outlen, size_t outcap);

static inline int cryptomsg_decrypt_size(size_t inlen)
{
	return inlen - 16;
}

#endif /* LNSOCKET_CRYPTO_H */
