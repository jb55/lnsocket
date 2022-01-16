

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

#include "handshake.h"
#include "error.h"
#include "compiler.h"
#include "lnsocket_internal.h"

#define array_len(x) (sizeof(x)/sizeof(x[0]))

int push_error(struct lnsocket *lnsocket, const char *err);

static int char_to_hex(unsigned char *val, char c)
{
	if (c >= '0' && c <= '9') {
		*val = c - '0';
		return 1;
	}
 	if (c >= 'a' && c <= 'f') {
		*val = c - 'a' + 10;
		return 1;
	}
 	if (c >= 'A' && c <= 'F') {
		*val = c - 'A' + 10;
		return 1;
	}
	return 0;
}


static int hex_decode(const char *str, size_t slen, void *buf, size_t bufsize)
{
	unsigned char v1, v2;
	unsigned char *p = buf;

	while (slen > 1) {
		if (!char_to_hex(&v1, str[0]) || !char_to_hex(&v2, str[1]))
			return 0;
		if (!bufsize)
			return 0;
		*(p++) = (v1 << 4) | v2;
		str += 2;
		slen -= 2;
		bufsize--;
	}
	return slen == 0 && bufsize == 0;
}


static int parse_node_id(const char *str, struct node_id *dest)
{
	return hex_decode(str, strlen(str), dest->k, sizeof(*dest));
}

static int pubkey_from_node_id(secp256k1_context *secp, struct pubkey *key,
		const struct node_id *id)
{
	return secp256k1_ec_pubkey_parse(secp, &key->pubkey,
					 memcheck(id->k, sizeof(id->k)),
					 sizeof(id->k));
}

int lnsocket_write(struct lnsocket *ln, const u8 *msg, int msglen)
{
	// this is just temporary so we don't need to move the memory cursor
	u8 *out = ln->mem.p;
	ssize_t outcap = ln->mem.end - ln->mem.p;
	ssize_t writelen;
	size_t outlen;

	if (!ln->socket)
		return note_error(&ln->errs, "not connected");

	if (outcap <= 0)
		return note_error(&ln->errs, "out of memory");

	if (!cryptomsg_encrypt_msg(&ln->crypto_state, msg, msglen, out, &outlen, (size_t)outcap))
		return note_error(&ln->errs, "encrypt message failed, out of memory");

	if ((writelen = write(ln->socket, out, outlen)) != outlen)
		return note_error(&ln->errs,
				"write failed. wrote %ld bytes, expected %ld %s",
				writelen, outlen,
				writelen < 0 ? strerror(errno) : "");

	return 1;
}

struct lnsocket *lnsocket_create_with(int memory)
{
	struct cursor mem;
	void *arena = malloc(memory);

	if (!arena)
		return NULL;

	make_cursor(arena, arena + memory, &mem);
	struct lnsocket *lnsocket = cursor_alloc(&mem, sizeof(*lnsocket));

	if (!lnsocket)
		return NULL;

	lnsocket->secp = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY |
						 SECP256K1_CONTEXT_SIGN);

	if (!cursor_slice(&mem, &lnsocket->errs.cur, memory / 2)) {
		return NULL;
	}
	lnsocket->errs.enabled = 1;

	lnsocket->mem = mem;
	return lnsocket;
}

struct lnsocket *lnsocket_create()
{
	return lnsocket_create_with(16384);
}

void lnsocket_destroy(struct lnsocket *lnsocket)
{
	if (!lnsocket)
		return;

	secp256k1_context_destroy(lnsocket->secp);
	free(lnsocket->mem.start);
}

static int is_zero(void *vp, int size)
{
	u8 *p = (u8*)vp;
	const u8 *start = p;

	for (; p < start+size; p++) {
		if (*p != 0)
			return 0;
	}
	return 1;
}

int lnsocket_connect(struct lnsocket *ln, const char *node_id, const char *host)
{
	int ret;
	struct addrinfo *addrs = NULL;
	struct handshake h;
	struct pubkey their_id;
	struct node_id their_node_id;

	if (is_zero(&ln->key, sizeof(ln->key)))
		return note_error(&ln->errs, "key not initialized, use lnsocket_set_key() or lnsocket_genkey()");

	// convert node_id string to bytes
	if (!parse_node_id(node_id, &their_node_id))
		return note_error(&ln->errs, "failed to parse node id");

	// encode node_id bytes to secp pubkey
	if (!pubkey_from_node_id(ln->secp, &their_id, &their_node_id))
		return note_error(&ln->errs, "failed to convert node_id to pubkey");

	// parse ip into addrinfo
	if ((ret = getaddrinfo(host, "9735", NULL, &addrs)) || !addrs)
		return note_error(&ln->errs, "%s", gai_strerror(ret));

	// create our network socket for comms
	if (!(ln->socket = socket(AF_INET, SOCK_STREAM, 0)))
		return note_error(&ln->errs, "creating socket failed");

	// connect to the node!
	if (connect(ln->socket, addrs->ai_addr, addrs->ai_addrlen) == -1)
		return note_error(&ln->errs, "%s", strerror(errno));

	// prepare some data for ACT1
	new_handshake(ln->secp, &h, &their_id);

	h.side = INITIATOR;
	h.their_id = their_id;

	// let's do this!
	return act_one_initiator(ln, &h);
}

void lnsocket_genkey(struct lnsocket *ln)
{
	ln->key = generate_key(ln->secp);
}

void lnsocket_print_errors(struct lnsocket *ln)
{
	print_error_backtrace(&ln->errs);
}


