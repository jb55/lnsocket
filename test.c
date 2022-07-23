
#include "lnsocket.h"
#include "endian.h"
#include "typedefs.h"
#include <stdio.h>
#include <assert.h>
#include <sodium/randombytes.h>

static void print_data(unsigned char *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		printf("%02x", buf[i]);
	}
	printf("\n");
}

int main(int argc, const char *argv[])
{
	static u8 msgbuf[4096];
	u8 *buf;
	u16 msgtype;
	struct lnsocket *ln;

	static unsigned char key[32] = {0};

	u16 len;
	int ok = 1;

	ln = lnsocket_create();
	assert(ln);

	if (lnsocket_setkey(ln, key)) {
		// key with 0s should be an error
		ok = 0;
		goto done;
	}

	randombytes_buf(key, sizeof(key));
	if (!(ok = lnsocket_setkey(ln, key)))
		goto done;

	const char *nodeid = "03f3c108ccd536b8526841f0a5c58212bb9e6584a1eb493080e7c1cc34f82dad71";
	if (!(ok = lnsocket_connect(ln, nodeid, "24.84.152.187")))
		goto done;

	if (!(ok = lnsocket_perform_init(ln)))
		goto done;

	printf("init ok!\n");

	if (!(ok = len = lnsocket_make_ping_msg(msgbuf, sizeof(msgbuf), 1, 1)))
		goto done;

	if (!(ok = lnsocket_write(ln, msgbuf, len)))
		goto done;

	printf("sent ping ");
	print_data(msgbuf, len);

	for (int packets = 0; packets < 3; packets++) {
		if (!(ok = lnsocket_recv(ln, &msgtype, &buf, &len)))
			goto done;

		if (msgtype == WIRE_PONG) {
			printf("got pong! ");
			print_data(buf, len);
			break;
		}
	}

done:
	lnsocket_print_errors(ln);
	lnsocket_destroy(ln);
	return !ok;
}
