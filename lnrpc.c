
#include <stdlib.h>

#include "lnsocket.h"
#include "endian.h"
#include "typedefs.h"
#include "commando.h"

#include <stdio.h>
#include <assert.h>

#include <sys/select.h>

int usage()
{
	printf("lnrpc <nodeid> <ip/hostname> <rune> <method>  [params (json string)]\n\n");
	printf("currently supports commando for clightning, but potentially more rpc types in the future!\n");
	return 0;
}

int main(int argc, const char *argv[])
{
	static u8 msgbuf[4096];
	u8 *buf;
	struct lnsocket *ln;
	fd_set set;
	struct timeval timeout = {0};

	char *timeout_str;
	u16 len, msgtype;
	int ok = 1;
	int socket, rv;
	int verbose = getenv("VERBOSE") != 0;
	//int verbose = 1;

	timeout_str = getenv("LNRPC_TIMEOUT");
	int timeout_ms = timeout_str ? atoi(timeout_str) : 50000000;

	timeout.tv_sec = timeout_ms / 1000;
	timeout.tv_usec = (timeout_ms % 1000) * 1000;

	FD_ZERO(&set); /* clear the set */

	if (argc < 5)
		return usage();

	ln = lnsocket_create();
	assert(ln);

	lnsocket_genkey(ln);

	const char *nodeid = argv[1];
	const char *host = argv[2];
	const char *rune = argv[3];
	const char *method = argv[4];
	const char *params = argc < 7 ? argv[5] : NULL;

	if (!(ok = lnsocket_connect(ln, nodeid, host)))
		goto done;

	if (!(ok = lnsocket_fd(ln, &socket)))
		goto done;

	FD_SET(socket, &set); /* add our file descriptor to the set */

	if (!(ok = lnsocket_perform_init(ln)))
		goto done;

	if (verbose)
		fprintf(stderr, "init success\n");

	if (!(ok = len = commando_make_rpc_msg(method, params, rune, 1, msgbuf, sizeof(msgbuf))))
		goto done;

	if (!(ok = lnsocket_write(ln, msgbuf, len)))
		goto done;

	if (verbose)
		fprintf(stderr, "waiting for response...\n");

	while (1) {
		rv = select(socket + 1, &set, NULL, NULL, &timeout);

		if (rv == -1) {
			perror("select");
			ok = 0;
			goto done;
		} else if (rv == 0) {
			fprintf(stderr, "error: rpc request timeout\n");
			ok = 0;
			goto done;
		}

		if (!(ok = lnsocket_recv(ln, &msgtype, &buf, &len)))
			goto done;

		switch (msgtype) {
		case COMMANDO_REPLY_TERM:
			printf("%.*s\n", len - 8, buf + 8);
			goto done;
		case COMMANDO_REPLY_CONTINUES:
			printf("%.*s", len - 8, buf + 8);
			continue;
		case WIRE_PING:
			if (!lnsocket_pong(ln, buf, len)) {
				fprintf(stderr, "pong error...\n");
			}
		default:
			// ignore extra interleaved messages which can happen
			continue;
		}
	}

done:
	lnsocket_print_errors(ln);
	lnsocket_destroy(ln);
	return !ok;
}
