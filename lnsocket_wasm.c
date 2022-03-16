
#include "handshake.h"
#include "lnsocket_internal.h"
#include <emscripten.h>

struct act_three* lnsocket_act_two(struct lnsocket *ln, struct act_two *act2);

void* EMSCRIPTEN_KEEPALIVE lnsocket_act_one(struct lnsocket *ln, const char *node_id)
{
	struct pubkey their_id;
	struct node_id their_node_id;

	if (is_zero(&ln->key, sizeof(ln->key))) {
		note_error(&ln->errs, "key not initialized, use lnsocket_set_key() or lnsocket_genkey()");
		return NULL;
	}

	if (!parse_node_id(node_id, &their_node_id)) {
		note_error(&ln->errs, "failed to parse node id");
		return NULL;
	}

	if (!pubkey_from_node_id(ln->secp, &their_id, &their_node_id)) {
		note_error(&ln->errs, "failed to convert node_id to pubkey");
		return NULL;
	}
	
	new_handshake(ln->secp, &ln->handshake, &their_id);

	ln->handshake.side = INITIATOR;
	ln->handshake.their_id = their_id;

	if (!act_one_initiator_prep(ln)) {
		note_error(&ln->errs, "failed to build initial handshake data (act1)");
		return NULL;
	}

	return &ln->handshake.act1;
}
