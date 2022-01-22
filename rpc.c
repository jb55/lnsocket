
#include <stdlib.h>

#include "lnsocket.h"
#include "endian.h"
#include "typedefs.h"
#include "commando.h"

#include <stdio.h>
#include <assert.h>

int usage()
{
	printf("lnrpc <nodeid> <ip/hostname> <method> <params> <rune>\n\n");
	printf("currently supports commando for clightning, but potentially more rpc types in the future!");
	return 0;
}

int main(int argc, const char *argv[])
{
	static u8 msgbuf[4096];
	u8 *buf;
	struct lnsocket *ln;
	u16 len, msgtype;
	int ok = 1;
	int verbose = getenv("VERBOSE") != 0;
	//int verbose = 1;

	if (argc < 6)
		return usage();

	ln = lnsocket_create();
	assert(ln);

	lnsocket_genkey(ln);

	const char *nodeid = argv[1];
	const char *host = argv[2];
	const char *method = argv[3];
	const char *params = argv[4];
	const char *rune = argv[5];

	if (!(ok = lnsocket_connect(ln, nodeid, host)))
		goto done;

	if (!(ok = lnsocket_perform_init(ln)))
		goto done;

	if (verbose)
		fprintf(stderr, "init success\n");

	if (!(ok = commando_make_rpc_msg(method, params, rune, 1, msgbuf, sizeof(msgbuf), &len)))
		goto done;

	if (!(ok = lnsocket_write(ln, msgbuf, len)))
		goto done;

	if (verbose)
		fprintf(stderr, "waiting for response...\n");

	while (1) {
		if (!(ok = lnsocket_recv(ln, &msgtype, &buf, &len)))
			goto done;

		printf("%.*s", len - 8, buf + 8);

		switch (msgtype) {
		case COMMANDO_REPLY_TERM:
			printf("\n");
			goto done;
		case COMMANDO_REPLY_CONTINUES:
			continue;
		default:
			printf("\n");
			fprintf(stderr, "unknown msgtype %d\n", msgtype);
			ok = 0;
			goto done;
		}
	}

done:
	lnsocket_print_errors(ln);
	lnsocket_destroy(ln);
	return !ok;
}
