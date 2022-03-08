
#include "lnsocket.h"
#include "endian.h"
#include "typedefs.h"
#include <stdio.h>
#include <assert.h>

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
	struct lnsocket *ln;

	u16 len;
	int ok = 1;

	ln = lnsocket_create();
	assert(ln);

	lnsocket_genkey(ln);

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

	if (!(ok = lnsocket_read(ln, &buf, &len)))
		goto done;

	printf("got ");
	print_data(buf, len);
done:
	lnsocket_print_errors(ln);
	lnsocket_destroy(ln);
	return !ok;
}
