

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
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

#define MSGBUF_MEM (65536*2)
#define ERROR_MEM 4096
#define DEFAULT_TIMEOUT 3000
#define DEFAULT_TOR_TIMEOUT 10000

// Tor socks define
#define SOCKS_NOAUTH		0
#define SOCKS_ERROR 	 0xff
#define SOCKS_CONNECT		1
#define SOCKS_TYP_IPV4		1
#define SOCKS_DOMAIN		3
#define SOCKS_TYP_IPV6		4
#define SOCKS_V5            5

#define MAX_SIZE_OF_SOCKS5_REQ_OR_RESP 255
#define SIZE_OF_RESPONSE 		4
#define SIZE_OF_REQUEST 		3
#define SIZE_OF_IPV4_RESPONSE 	6
#define SIZE_OF_IPV6_RESPONSE 	18
#define SOCK_REQ_METH_LEN		3
#define SOCK_REQ_V5_LEN			5
#define SOCK_REQ_V5_HEADER_LEN	7

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

int EXPORT lnsocket_make_default_initmsg(unsigned char *msgbuf, int buflen)
{
	u8 global_features[2] = {0};
	u8 features[5] = {0};
	u16 len;

	/*
	struct tlv network_tlv;
	u8 tlvbuf[1024];
	const u8 genesis_block[]  = {
	  0x6f, 0xe2, 0x8c, 0x0a, 0xb6, 0xf1, 0xb3, 0x72, 0xc1, 0xa6, 0xa2, 0x46,
	  0xae, 0x63, 0xf7, 0x4f, 0x93, 0x1e, 0x83, 0x65, 0xe1, 0x5a, 0x08, 0x9c,
	  0x68, 0xd6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	const u8 *blockids[] = { genesis_block };

	if (!lnsocket_make_network_tlv(tlvbuf, sizeof(tlvbuf), blockids, 1,
				&network_tlv))
		return 0;


	const struct tlv *init_tlvs[] = { &network_tlv } ;
	*/

	const struct tlv *init_tlvs[] = { } ;

	if (!lnsocket_make_init_msg(msgbuf, buflen,
			global_features, sizeof(global_features),
			features, sizeof(features),
			init_tlvs, 0,
			&len))
		return 0;

	return (int)len;
}

/*
static void print_hex(u8 *bytes, int len) {
	int i;
	for (i = 0; i < len; ++i) {
		printf("%02x", bytes[i]);
	}
}
*/

int lnsocket_perform_init(struct lnsocket *ln)
{
	u8 msgbuf[1024];
	u16 len;
	u8 *buf;

	// read the init message from the other side and ignore it
	if (!lnsocket_read(ln, &buf, &len))
		return 0;

	if (!(len = lnsocket_make_default_initmsg(msgbuf, sizeof(msgbuf))))
		return 0;

	if (!lnsocket_write(ln, msgbuf, len))
		return 0;

	return 1;
}

// simple helper that pushes a message type and payload
int lnsocket_send(struct lnsocket *ln, unsigned short msg_type, const unsigned char *payload, unsigned short payload_len)
{
	reset_cursor(&ln->msgbuf);

	if (!cursor_push_u16(&ln->msgbuf, msg_type))
		return note_error(&ln->errs, "could not write type to msgbuf?");

	if (!cursor_push(&ln->msgbuf, payload, payload_len))
		return note_error(&ln->errs, "payload too big");

	return lnsocket_write(ln, ln->msgbuf.start, ln->msgbuf.p - ln->msgbuf.start);
}

// simple helper that receives a message type and payload
int lnsocket_recv(struct lnsocket *ln, u16 *msg_type, unsigned char **payload, u16 *payload_len)
{
	struct cursor cur;
	u8 *msg;
	u16 msglen;

	if (!lnsocket_read(ln, &msg, &msglen))
		return 0;

	make_cursor(msg, msg + msglen, &cur);

	if (!cursor_pull_u16(&cur, msg_type))
		return note_error(&ln->errs, "could not read msgtype");

	*payload_len = msglen - 2;
	*payload = cur.p;

	if (*payload + *payload_len > cur.end)
		return note_error(&ln->errs, "recv buffer overflow?");

	return 1;
}

int EXPORT lnsocket_decrypt(struct lnsocket *ln, unsigned char *packet, int size)
{
	struct cursor enc, dec;

	make_cursor(packet, packet + size, &enc);
	reset_cursor(&ln->msgbuf);
	if (!cursor_slice(&ln->msgbuf, &dec, size - 16))
		return note_error(&ln->errs, "out of memory: %d + %d = %d > %d",
				ln->msgbuf.end - ln->msgbuf.p, size,
				ln->msgbuf.end - ln->msgbuf.p + size,
				MSGBUF_MEM
				);

	if (!cryptomsg_decrypt_body(&ln->crypto_state,
				enc.start, enc.end - enc.start,
				dec.start, dec.end - dec.start))
		return note_error(&ln->errs, "error decrypting body");

	return dec.end - dec.start;
}

// this is used in js
int EXPORT lnsocket_decrypt_header(struct lnsocket *ln, unsigned char *hdr)
{
	u16 size = 0;
	if (!cryptomsg_decrypt_header(&ln->crypto_state, hdr, &size))
		return note_error(&ln->errs,
			"Failed hdr decrypt with rn=%"PRIu64,
			ln->crypto_state.rn-1);
	return size;
}

int lnsocket_read(struct lnsocket *ln, unsigned char **buf, unsigned short *len)
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
                return note_error(&ln->errs, "out of memory: %d + %d = %d > %d",
                                ln->msgbuf.end - ln->msgbuf.p, size,
                                ln->msgbuf.end - ln->msgbuf.p + size,
                                MSGBUF_MEM
                                );

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

int lnsocket_make_pong_msg(unsigned char *buf, int buflen, u16 num_pong_bytes)
{
	struct cursor msg;

	make_cursor(buf, buf + buflen, &msg);

	if (!cursor_push_u16(&msg, WIRE_PONG))
		return 0;

	// don't include itself
	num_pong_bytes = num_pong_bytes <= 4 ? 0 : num_pong_bytes - 4;

	if (!cursor_push_u16(&msg, num_pong_bytes))
		return 0;

	if (msg.p + num_pong_bytes > msg.end)
		return 0;

	memset(msg.p, 0, num_pong_bytes);
	msg.p += num_pong_bytes;

	return msg.p - msg.start;
}

static int lnsocket_decode_ping_payload(const unsigned char *payload, int payload_len, u16 *pong_bytes)
{
	struct cursor msg;

	make_cursor((u8*)payload, (u8*)payload + payload_len, &msg);

	if (!cursor_pull_u16(&msg, pong_bytes))
		return 0;

	return 1;
}

int lnsocket_make_pong_from_ping(unsigned char *buf, int buflen, const unsigned char *ping, u16 ping_len)
{
	u16 pong_bytes;

	if (!lnsocket_decode_ping_payload(ping, ping_len, &pong_bytes))
		return 0;

	return lnsocket_make_pong_msg(buf, buflen, pong_bytes);
}

int lnsocket_pong(struct lnsocket *ln, const unsigned char *ping, u16 ping_len)
{
	unsigned char pong[0xFFFF];
	u16 len;

	if (!(len = lnsocket_make_pong_from_ping(pong, sizeof(pong), ping, ping_len)))
		return 0;

	return lnsocket_write(ln, pong, len);
}

int lnsocket_make_ping_msg(unsigned char *buf, int buflen, u16 num_pong_bytes, u16 ignored_bytes)
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

	return msg.p - msg.start;
}

int lnsocket_make_init_msg(unsigned char *buf, int buflen,
	const unsigned char *globalfeatures, u16 gflen,
	const unsigned char *features, u16 flen,
	const struct tlv **tlvs,
	unsigned short num_tlvs,
	unsigned short *outlen)
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

unsigned char* EXPORT lnsocket_msgbuf(struct lnsocket *ln)
{
	return ln->msgbuf.start;
}

int EXPORT lnsocket_encrypt(struct lnsocket *ln, const u8 *msg, unsigned short msglen)
{
	ssize_t outcap;
	size_t outlen;

	// this is just temporary so we don't need to move the memory cursor
	reset_cursor(&ln->msgbuf);

	u8 *out = ln->msgbuf.start;
	outcap = ln->msgbuf.end - ln->msgbuf.start;

#ifndef __EMSCRIPTEN__
	if (!ln->socket)
		return note_error(&ln->errs, "not connected");
#endif

	if (outcap <= 0)
		return note_error(&ln->errs, "out of memory");

	if (!cryptomsg_encrypt_msg(&ln->crypto_state, msg, msglen, out, &outlen, (size_t)outcap))
		return note_error(&ln->errs, "encrypt message failed, out of memory");

	return outlen;
}

int lnsocket_write(struct lnsocket *ln, const u8 *msg, unsigned short msglen)
{
	ssize_t writelen, outlen;
	u8 *out = ln->msgbuf.start;

	if (!(outlen = lnsocket_encrypt(ln, msg, msglen)))
		return 0;

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

int is_zero(void *vp, int size)
{
	u8 *p = (u8*)vp;
	const u8 *start = p;

	for (; p < start+size; p++) {
		if (*p != 0)
			return 0;
	}
	return 1;
}

static int io_fd_block(int fd, int block)
{
	int flags = fcntl(fd, F_GETFL);

	if (flags == -1)
		return 0;

	if (block)
		flags &= ~O_NONBLOCK;
	else
		flags |= O_NONBLOCK;

	return fcntl(fd, F_SETFL, flags) != -1;
}

const char *parse_port(char *host)
{
	int i, len;

	len = strlen(host);

	for (i = 0; i < len; i++) {
		if (host[i] == ':') {
			host[i] = 0;
			return &host[i+1];
		}
	}


	return NULL;
}

int lnsocket_handshake_with_tor(struct lnsocket *ln, const char *host, const char *port)
{
	// make the init request
	u8 request[SIZE_OF_REQUEST] = { SOCKS_V5, 0x01, SOCKS_NOAUTH };
	if (write(ln->socket, request, sizeof(request)) != sizeof(request)) {
		return note_error(&ln->errs, "Failed writing request: %s",
				  strerror(errno));
	}

	// read proxy response
	u8 hdr[2];
	if (!read(ln->socket, hdr, 2))
		return note_error(&ln->errs,"Failed reading header");

	if (hdr[1] == SOCKS_ERROR)
		return note_error(&ln->errs,"Failed proxy authentication required");

	// make the V5 request
	size_t hlen = strlen(host);
	u16 uport = htons(atoi(port));

	u8 buffer[SOCK_REQ_V5_HEADER_LEN + hlen];
	buffer[0] = SOCKS_V5;
	buffer[1] = SOCKS_CONNECT;
	buffer[2] = 0;
	buffer[3] = SOCKS_DOMAIN;
	buffer[4] = hlen;
	memcpy(buffer + SOCK_REQ_V5_LEN, host, hlen);
	memcpy(buffer + SOCK_REQ_V5_LEN + hlen, &uport, sizeof(uport));

	if (write(ln->socket, buffer, sizeof(buffer)) != sizeof(buffer)) {
		return note_error(&ln->errs,"Failed writing buffer: %s", strerror(errno));
	}

	// read completion
	u8 inbuffer[SIZE_OF_IPV4_RESPONSE + SIZE_OF_RESPONSE];
	if (!read_all(ln->socket, inbuffer, sizeof(inbuffer)))
		return note_error(&ln->errs,"Failed reading inbuffer: %s", strerror(errno));
	if (inbuffer[1] != '\0')
		return note_error(&ln->errs,"Failed proxy connection: %d", inbuffer[1]);
	if (inbuffer[3] == SOCKS_TYP_IPV6)
		return note_error(&ln->errs,"Failed ipv6 not supported yet");
	if (inbuffer[3] != SOCKS_TYP_IPV4)
		return note_error(&ln->errs,"Failed only ipv4 supported");

	return 1;
}

int lnsocket_connect_with(struct lnsocket *ln, const char *node_id, const char *host, const char *tor_proxy, int timeout_ms)
{
	int ret;
	struct addrinfo *addrs = NULL;
	struct pubkey their_id;
	struct node_id their_node_id;
	struct timeval timeout = {0};
	char onlyhost[strlen(host)+1];
	strncpy(onlyhost, host, sizeof(onlyhost));
	fd_set set;

	timeout.tv_sec = timeout_ms / 1000;
	timeout.tv_usec = (timeout_ms % 1000) * 1000;

	FD_ZERO(&set); /* clear the set */

	if (is_zero(&ln->key, sizeof(ln->key)))
		return note_error(&ln->errs, "key not initialized, use lnsocket_set_key() or lnsocket_genkey()");

	// convert node_id string to bytes
	if (!parse_node_id(node_id, &their_node_id))
		return note_error(&ln->errs, "failed to parse node id");

	// encode node_id bytes to secp pubkey
	if (!pubkey_from_node_id(ln->secp, &their_id, &their_node_id))
		return note_error(&ln->errs, "failed to convert node_id to pubkey");

	// create our network socket for comms
	if (!(ln->socket = socket(AF_INET, SOCK_STREAM, 0)))
		return note_error(&ln->errs, "creating socket failed");

	FD_SET(ln->socket, &set); /* add our file descriptor to the set */

	if (!io_fd_block(ln->socket, 0))
		return note_error(&ln->errs, "failed setting socket to non-blocking");

	if (tor_proxy != NULL) {
		// connect to the tor proxy!
		char proxyhost[strlen(tor_proxy)+1];
		strncpy(proxyhost, tor_proxy, sizeof(proxyhost));
		const char *port = parse_port(proxyhost);
		struct sockaddr_in proxy_server;
		proxy_server.sin_family = AF_INET;
		proxy_server.sin_addr.s_addr = inet_addr(proxyhost);
		proxy_server.sin_port = htons(port ? atoi(port) : 9050);
		if (!connect(ln->socket, (struct sockaddr*) &proxy_server, sizeof(proxy_server)))
			return note_error(&ln->errs, "Failed to connect");
	} else {
		// parse ip into addrinfo
		const char *port = parse_port(onlyhost);
		if ((ret = getaddrinfo(onlyhost, port ? port : "9735", NULL, &addrs)) || !addrs)
			return note_error(&ln->errs, "%s", gai_strerror(ret));
		// connect to the node!
		connect(ln->socket, addrs->ai_addr, addrs->ai_addrlen);
	}

	if (!io_fd_block(ln->socket, 1))
		return note_error(&ln->errs, "failed setting socket to blocking");

	ret = select(ln->socket + 1, NULL, &set, NULL, &timeout);
	if (ret == -1) {
		return note_error(&ln->errs, "select error");
	} else if (ret == 0) {
		return note_error(&ln->errs, "connection timeout");
	}

	if (tor_proxy != NULL) {
		// connect to the node through tor proxy
		const char *port = parse_port(onlyhost);
		if (!lnsocket_handshake_with_tor(ln, onlyhost, port ? port : "9735"))
			return note_error(&ln->errs, "failed handshake with tor proxy");
	}

	// prepare some data for ACT1
	new_handshake(ln->secp, &ln->handshake, &their_id);

	ln->handshake.side = INITIATOR;
	ln->handshake.their_id = their_id;

	// let's do this!
	return act_one_initiator(ln);
}

int lnsocket_connect(struct lnsocket *ln, const char *node_id, const char *host)
{
	return lnsocket_connect_with(ln, node_id, host, NULL, DEFAULT_TIMEOUT);
}

int lnsocket_connect_tor(struct lnsocket *ln, const char *node_id, const char *host, const char *tor_proxy)
{
	return lnsocket_connect_with(ln, node_id, host, tor_proxy, DEFAULT_TOR_TIMEOUT);
}

int lnsocket_fd(struct lnsocket *ln, int *fd)
{
	*fd = ln->socket;
	return 1;
}

void * lnsocket_secp(struct lnsocket *ln)
{
	return ln->secp;
}

void lnsocket_genkey(struct lnsocket *ln)
{
	ln->key = generate_key(ln->secp);
}

int lnsocket_setkey(struct lnsocket *ln, const unsigned char key[32])
{
	struct keypair k;
	memcpy(k.priv.secret.data, key, 32);

	if (!secp256k1_ec_pubkey_create(ln->secp, &k.pub.pubkey, k.priv.secret.data))
		return 0;

	ln->key = k;

	return 1;
}

void lnsocket_print_errors(struct lnsocket *ln)
{
	print_error_backtrace(&ln->errs);
}


