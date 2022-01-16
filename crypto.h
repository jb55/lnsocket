
#ifndef LNSOCKET_CRYPTO_H
#define LNSOCKET_CRYPTO_H

#include <secp256k1.h>

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


#endif /* LNSOCKET_CRYPTO_H */
