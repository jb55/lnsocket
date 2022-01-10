

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define array_len(x) (sizeof(x)/sizeof(x[0]))

struct netln {
	const char *errors[8];
	int num_errors;
};


struct pubkey {
	/* Unpacked pubkey (as used by libsecp256k1 internally) */
	secp256k1_pubkey pubkey;
};

/* BOLT #8:
 *
 * Act One is sent from initiator to responder. During Act One, the
 * initiator attempts to satisfy an implicit challenge by the responder. To
 * complete this challenge, the initiator must know the static public key of
 * the responder.
 */
struct act_one {
	u8 v;
	u8 pubkey[PUBKEY_CMPR_LEN];
	u8 tag[crypto_aead_chacha20poly1305_ietf_ABYTES];
};

/* BOLT #8:
 *
 * Throughout the handshake process, each side maintains these variables:
 *
 *  * `ck`: the **chaining key**. This value is the accumulated hash of all
 *    previous ECDH outputs. At the end of the handshake, `ck` is used to derive
 *    the encryption keys for Lightning messages.
 *
 *  * `h`: the **handshake hash**. This value is the accumulated hash of _all_
 *    handshake data that has been sent and received so far during the handshake
 *    process.
 *
 *  * `temp_k1`, `temp_k2`, `temp_k3`: the **intermediate keys**. These are used to
 *    encrypt and decrypt the zero-length AEAD payloads at the end of each handshake
 *    message.
 *
 *  * `e`: a party's **ephemeral keypair**. For each session, a node MUST generate a
 *    new ephemeral key with strong cryptographic randomness.
 *
 *  * `s`: a party's **static keypair** (`ls` for local, `rs` for remote)
 */
struct handshake {
	struct secret ck;
	struct secret temp_k;
	struct sha256 h;
	struct keypair e;
	struct secret *ss;

	/* Used between the Acts */
	struct pubkey re;
	struct act_one act1;
	struct act_two act2;
	struct act_three act3;

	/* Where is connection from/to */
	struct wireaddr_internal addr;

	/* Who we are */
	struct pubkey my_id;
	/* Who they are: set already if we're initiator. */
	struct pubkey their_id;

	/* Are we initiator or responder. */
	enum bolt8_side side;

	/* Function to call once handshake complete. */
	struct io_plan *(*cb)(struct io_conn *conn,
			      const struct pubkey *their_id,
			      const struct wireaddr_internal *wireaddr,
			      struct crypto_state *cs,
			      void *cbarg);
	void *cbarg;
};


static void push_error(struct netln *netln, const char *err)
{
	if (netln->num_errors >= array_len(netln->errors)) {
		// TODO: push out old errors instead
		return;
	}

	netln->errors[netln->num_errors++] = err;
}

static void netln_init(struct netln *netln)
{
	memset(netln, 0, sizeof(*netln));
}

int connect_node(struct netln *netln, unsigned char *pubkey, const char *host)
{
	int fd, ret;
	struct addrinfo *addrs = NULL;

	if ((ret = getaddrinfo(host, "9735", NULL, &addrs)) || !addrs) {
		push_error(netln, gai_strerror(ret));
		return 0;
	}

	if (!(fd = socket(AF_INET, SOCK_STREAM, 0))) {
		push_error(netln, "creating socket failed");
		return 0;
	}

	if (connect(fd, addrs->ai_addr, addrs->ai_addrlen) == -1) {
		push_error(netln, strerror(errno));
		return 0;
	}

	return 1;
}

int act_one_initiator(int fd)
{

}

static unsigned char nodeid[] = {
  0x03, 0xf3, 0xc1, 0x08, 0xcc, 0xd5, 0x36, 0xb8, 0x52, 0x68, 0x41, 0xf0,
  0xa5, 0xc5, 0x82, 0x12, 0xbb, 0x9e, 0x65, 0x84, 0xa1, 0xeb, 0x49, 0x30,
  0x80, 0xe7, 0xc1, 0xcc, 0x34, 0xf8, 0x2d, 0xad, 0x71 };

int main(int argc, const char *argv[])
{
	struct netln netln;
	netln_init(&netln);

	if (!connect_node(&netln, nodeid, "24.84.152.187")) {
		printf("connection failed: %s\n", netln.errors[0]);
		return 1;
	}

	printf("connected!\n");

	return 0;
}
