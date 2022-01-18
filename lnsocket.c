

#include <sys/socket.h>
#include <sys/types.h>
#include <inttypes.h>
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
#include "crypto.h"
#include "endian.h"
#include "bigsize.h"
#include "compiler.h"
#include "lnsocket_internal.h"
#include "lnsocket.h"

#define array_len(x) (sizeof(x)/sizeof(x[0]))

#define MSGBUF_MEM 65536
#define ERROR_MEM 4096

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


static int read_all(int fd, void *data, size_t size)
{
	while (size) {
		ssize_t done;

		done = read(fd, data, size);
		if (done < 0 && errno == EINTR)
			continue;
		if (done <= 0)
			return 0;
		data = (char *)data + done;
		size -= done;
	}

	return 1;
}


int lnsocket_read(struct lnsocket *ln, unsigned char **buf, int *len)
{
	struct cursor enc, dec;
	u8 hdr[18];
	u16 size;

	reset_cursor(&ln->errs.cur);
	reset_cursor(&ln->msgbuf);

	if (!read_all(ln->socket, hdr, sizeof(hdr)))
		return note_error(&ln->errs,"Failed reading header: %s",
				strerror(errno));

	if (!cryptomsg_decrypt_header(&ln->crypto_state, hdr, &size))
		return note_error(&ln->errs,
				"Failed hdr decrypt with rn=%"PRIu64,
				ln->crypto_state.rn-1);

	if (!cursor_slice(&ln->msgbuf, &enc, size + 16))
		return note_error(&ln->errs, "out of memory");

	if (!cursor_slice(&ln->msgbuf, &dec, size))
		return note_error(&ln->errs, "out of memory");

	if (!read_all(ln->socket, enc.p, enc.end - enc.start))
		return note_error(&ln->errs, "Failed reading body: %s",
				strerror(errno));

	if (!cryptomsg_decrypt_body(&ln->crypto_state,
				enc.start, enc.end - enc.start,
				dec.start, dec.end - dec.start))
		return note_error(&ln->errs, "error decrypting body");

	*buf = dec.start;
	*len = dec.end - dec.start;

	return 1;
}

static int highest_byte(unsigned char *buf, int buflen)
{
	int i, highest;
	for (i = 0, highest = 0; i < buflen; i++) {
		if (buf[i] != 0)
			highest = i;
	}
	return highest;
}

#define max(a,b) ((a) > (b) ? (a) : (b))
int lnsocket_set_feature_bit(unsigned char *buf, int buflen, int *newlen, unsigned int bit)
{
	if (newlen == NULL)
		return 0;

	if (bit / 8 >= buflen)
		return 0;

	*newlen = max(highest_byte(buf, buflen), (bit / 8) + 1);
	buf[*newlen - 1 - bit / 8] |= (1 << (bit % 8));

	return 1;
}
#undef max

int cursor_push_tlv(struct cursor *cur, const struct tlv *tlv)
{
	/* BOLT #1:
	 *
	 * The sending node:
	 ...
	 *  - MUST minimally encode `type` and `length`.
	 */
	return cursor_push_bigsize(cur, tlv->type) &&
	       cursor_push_bigsize(cur, tlv->length) &&
	       cursor_push(cur, tlv->value, tlv->length);
}

int cursor_push_tlvs(struct cursor *cur, const struct tlv **tlvs, int n_tlvs)
{
	int i;
	for (i = 0; i < n_tlvs; i++) {
		if (!cursor_push_tlv(cur, tlvs[i]))
			return 0;
	}

	return 1;
}

int lnsocket_make_network_tlv(unsigned char *buf, int buflen,
		const unsigned char **blockids, int num_blockids,
		struct tlv *tlv_out)
{
	struct cursor cur;

	if (!tlv_out)
		return 0;

	tlv_out->type = 1;
	tlv_out->value = buf;

	make_cursor(buf, buf + buflen, &cur);

	for (size_t i = 0; i < num_blockids; i++) {
		if (!cursor_push(&cur, memcheck(blockids[i], 32), 32))
			return 0;
	}

	tlv_out->length = cur.p - cur.start;
	return 1;
}

int lnsocket_make_ping_msg(unsigned char *buf, int buflen, u16 num_pong_bytes, u16 ignored_bytes, int *outlen)
{
	struct cursor msg;
	int i;

	make_cursor(buf, buf + buflen, &msg);

	if (!cursor_push_u16(&msg, WIRE_PING))
		return 0;
	if (!cursor_push_u16(&msg, num_pong_bytes))
		return 0;
	if (!cursor_push_u16(&msg, ignored_bytes))
		return 0;
	for (i = 0; i < ignored_bytes; i++) {
		if (!cursor_push_byte(&msg, 0))
			return 0;
	}

	*outlen = msg.p - msg.start;

	return 1;
}

int lnsocket_make_init_msg(unsigned char *buf, int buflen,
	const unsigned char *globalfeatures, u16 gflen,
	const unsigned char *features, u16 flen,
	const struct tlv **tlvs,
	unsigned short num_tlvs,
	int *outlen)
{
	struct cursor msg;

	make_cursor(buf, buf + buflen, &msg);

	if (!cursor_push_u16(&msg, WIRE_INIT))
		return 0;

	if (!cursor_push_u16(&msg, gflen))
		return 0;

	if (!cursor_push(&msg, globalfeatures, gflen))
		return 0;

	if (!cursor_push_u16(&msg, flen))
		return 0;

	if (!cursor_push(&msg, features, flen))
		return 0;

	if (!cursor_push_tlvs(&msg, tlvs, num_tlvs))
		return 0;

	*outlen = msg.p - msg.start;

	return 1;
}

int lnsocket_write(struct lnsocket *ln, const u8 *msg, int msglen)
{
	ssize_t writelen, outcap;
	size_t outlen;
	u8 *out;

	// this is just temporary so we don't need to move the memory cursor
	reset_cursor(&ln->msgbuf);

	out = ln->msgbuf.start;
	outcap = ln->msgbuf.end - ln->msgbuf.start;

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

struct lnsocket *lnsocket_create()
{
	struct cursor mem;
	int memory = MSGBUF_MEM + ERROR_MEM + sizeof(struct lnsocket);

	void *arena = malloc(memory);

	if (!arena)
		return NULL;

	make_cursor(arena, arena + memory, &mem);
	struct lnsocket *lnsocket = cursor_alloc(&mem, sizeof(*lnsocket));

	if (!lnsocket)
		return NULL;

	lnsocket->secp = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY |
						  SECP256K1_CONTEXT_SIGN);

	if (!cursor_slice(&mem, &lnsocket->msgbuf, MSGBUF_MEM))
		return NULL;

	if (!cursor_slice(&mem, &lnsocket->errs.cur, ERROR_MEM))
		return NULL;

	lnsocket->errs.enabled = 1;

	lnsocket->mem = mem;
	return lnsocket;
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


