#ifndef LNSOCKET_INTERNAL_H
#define LNSOCKET_INTERNAL_H

#include "crypto.h"
#include "error.h"
#include "handshake.h"

struct lnsocket {
	const char *errors[8];
	struct cursor mem;
	struct cursor msgbuf;
	struct errors errs;
	int num_errors;
	int socket;
	struct keypair key;
	struct pubkey responder_id;
	struct handshake handshake;
	struct crypto_state crypto_state;
	secp256k1_context *secp;
};

int parse_node_id(const char *str, struct node_id *dest);
int pubkey_from_node_id(secp256k1_context *secp, struct pubkey *key, const struct node_id *id);
int is_zero(void *vp, int size);

#endif /* LNSOCKET_INTERNAL_H */
