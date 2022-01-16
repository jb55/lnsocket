#ifndef LNSOCKET_INTERNAL_H
#define LNSOCKET_INTERNAL_H

#include "crypto.h"

struct lnsocket {
	const char *errors[8];
	struct cursor mem;
	struct errors errs;
	int num_errors;
	int socket;
	struct keypair key;
	struct crypto_state crypto_state;
	secp256k1_context *secp;
};

#endif /* LNSOCKET_INTERNAL_H */
