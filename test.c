
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
	static u8 tlvbuf[1024];
	static u8 msgbuf[4096];
	u8 *buf;
	u8 global_features[2] = {0};
	u8 features[5] = {0};
	struct tlv network_tlv;
	struct lnsocket *ln;
	int len;

	int ok = 1;

	ln = lnsocket_create();
	assert(ln);

	lnsocket_genkey(ln);

	const char *nodeid = "03f3c108ccd536b8526841f0a5c58212bb9e6584a1eb493080e7c1cc34f82dad71";
	if (!(ok = lnsocket_connect(ln, nodeid, "24.84.152.187")))
		goto done;

	if (!(ok = lnsocket_read(ln, &buf, &len)))
		goto done;

	printf("got "); print_data(buf, len);

	const u8 genesis_block[]  = { 
	  0x6f, 0xe2, 0x8c, 0x0a, 0xb6, 0xf1, 0xb3, 0x72, 0xc1, 0xa6, 0xa2, 0x46, 
	  0xae, 0x63, 0xf7, 0x4f, 0x93, 0x1e, 0x83, 0x65, 0xe1, 0x5a, 0x08, 0x9c, 
	  0x68, 0xd6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00 
	};

	const u8 *blockids[] = { genesis_block };

	if (!(ok = lnsocket_make_network_tlv(tlvbuf, sizeof(tlvbuf), blockids, 1, &network_tlv)))
		goto done;

	const struct tlv *init_tlvs[] = { &network_tlv } ;

	if (!(ok = lnsocket_make_init_msg(msgbuf, sizeof(msgbuf),
			global_features, sizeof(global_features),
			features, sizeof(features),
			init_tlvs, 1,
			&len)))
		goto done;

	if (!(ok = lnsocket_write(ln, msgbuf, len)))
		goto done;

	printf("sent init ");
	print_data(msgbuf, len);

	if (!(ok = lnsocket_make_ping_msg(msgbuf, sizeof(msgbuf), 1, 1, &len)))
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
