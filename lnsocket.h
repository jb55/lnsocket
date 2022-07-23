#ifndef LNSOCKET_H
#define LNSOCKET_H

#include <stdint.h>
#include <stdlib.h>

struct lnsocket;

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
	#define EXPORT EMSCRIPTEN_KEEPALIVE
#else
	#define EXPORT
#endif

enum peer_wire {
        WIRE_INIT = 16,
        WIRE_ERROR = 17,
        WIRE_WARNING = 1,
        WIRE_PING = 18,
        WIRE_PONG = 19,
        WIRE_TX_ADD_INPUT = 66,
        WIRE_TX_ADD_OUTPUT = 67,
        WIRE_TX_REMOVE_INPUT = 68,
        WIRE_TX_REMOVE_OUTPUT = 69,
        WIRE_TX_COMPLETE = 70,
        WIRE_TX_SIGNATURES = 71,
        WIRE_OPEN_CHANNEL = 32,
        WIRE_ACCEPT_CHANNEL = 33,
        WIRE_FUNDING_CREATED = 34,
        WIRE_FUNDING_SIGNED = 35,
        WIRE_FUNDING_LOCKED = 36,
        WIRE_OPEN_CHANNEL2 = 64,
        WIRE_ACCEPT_CHANNEL2 = 65,
        WIRE_INIT_RBF = 72,
        WIRE_ACK_RBF = 73,
        WIRE_SHUTDOWN = 38,
        WIRE_CLOSING_SIGNED = 39,
        WIRE_UPDATE_ADD_HTLC = 128,
        WIRE_UPDATE_FULFILL_HTLC = 130,
        WIRE_UPDATE_FAIL_HTLC = 131,
        WIRE_UPDATE_FAIL_MALFORMED_HTLC = 135,
        WIRE_COMMITMENT_SIGNED = 132,
        WIRE_REVOKE_AND_ACK = 133,
        WIRE_UPDATE_FEE = 134,
        WIRE_UPDATE_BLOCKHEIGHT = 137,
        WIRE_CHANNEL_REESTABLISH = 136,
        WIRE_ANNOUNCEMENT_SIGNATURES = 259,
        WIRE_CHANNEL_ANNOUNCEMENT = 256,
        WIRE_NODE_ANNOUNCEMENT = 257,
        WIRE_CHANNEL_UPDATE = 258,
        WIRE_QUERY_SHORT_CHANNEL_IDS = 261,
        WIRE_REPLY_SHORT_CHANNEL_IDS_END = 262,
        WIRE_QUERY_CHANNEL_RANGE = 263,
        WIRE_REPLY_CHANNEL_RANGE = 264,
        WIRE_GOSSIP_TIMESTAMP_FILTER = 265,
        WIRE_OBS2_ONION_MESSAGE = 387,
        WIRE_ONION_MESSAGE = 513,
};

/* A single TLV field, consisting of the data and its associated metadata. */
struct tlv {
	uint64_t type;
	size_t length;
	unsigned char *value;
};

struct lnsocket EXPORT *lnsocket_create();

/* messages */

int lnsocket_make_network_tlv(unsigned char *buf, int buflen, const unsigned char **blockids, int num_blockids, struct tlv *tlv_out);
int lnsocket_make_init_msg(unsigned char *buf, int buflen, const unsigned char *globalfeatures, unsigned short gflen, const unsigned char *features, unsigned short flen, const struct tlv **tlvs, unsigned short num_tlvs, unsigned short *outlen);

int lnsocket_perform_init(struct lnsocket *ln);

int lnsocket_connect(struct lnsocket *, const char *node_id, const char *host);
int lnsocket_connect_tor(struct lnsocket *, const char *node_id, const char *host, const char *tor_proxy);

int lnsocket_fd(struct lnsocket *, int *fd);
int lnsocket_write(struct lnsocket *, const unsigned char *msg, unsigned short msg_len);
int lnsocket_read(struct lnsocket *, unsigned char **buf, unsigned short *len);

int lnsocket_send(struct lnsocket *, unsigned short msg_type, const unsigned char *payload, unsigned short payload_len);
int lnsocket_recv(struct lnsocket *, unsigned short *msg_type, unsigned char **payload, unsigned short *payload_len);
int lnsocket_pong(struct lnsocket *, const unsigned char *ping, unsigned short ping_len);


int EXPORT lnsocket_make_pong_from_ping(unsigned char *buf, int buflen, const unsigned char *ping, unsigned short ping_len);
int EXPORT lnsocket_make_ping_msg(unsigned char *buf, int buflen, unsigned short num_pong_bytes, unsigned short ignored_bytes);
int EXPORT lnsocket_make_pong_msg(unsigned char *buf, int buflen, unsigned short num_pong_bytes);
void* EXPORT lnsocket_secp(struct lnsocket *);
void EXPORT lnsocket_genkey(struct lnsocket *);
int EXPORT lnsocket_setkey(struct lnsocket *, const unsigned char key[32]);
void EXPORT lnsocket_destroy(struct lnsocket *);
void EXPORT lnsocket_print_errors(struct lnsocket *);
int EXPORT lnsocket_make_default_initmsg(unsigned char *msgbuf, int buflen);
int EXPORT lnsocket_encrypt(struct lnsocket *ln, const unsigned char *msg, unsigned short msglen);
int EXPORT lnsocket_decrypt_header(struct lnsocket *ln, unsigned char *hdr);

#endif /* LNSOCKET_H */
