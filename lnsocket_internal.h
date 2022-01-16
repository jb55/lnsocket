
struct lnsocket {
	const char *errors[8];
	struct cursor mem;
	struct errors errs;
	int socket;
	int num_errors;
	struct keypair key;
	secp256k1_context *secp;
};

