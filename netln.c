

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "handshake.h"

#define array_len(x) (sizeof(x)/sizeof(x[0]))

struct netln {
	const char *errors[8];
	int num_errors;
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

void act_one_initiator(int fd)
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
