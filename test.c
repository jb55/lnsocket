
#include "lnsocket.h"
#include <stdio.h>

int main(int argc, const char *argv[])
{
	struct lnsocket *ln = lnsocket_create();
	int ok = 1;

	const char *nodeid = "03f3c108ccd536b8526841f0a5c58212bb9e6584a1eb493080e7c1cc34f82dad71";

	lnsocket_genkey(ln);
	if (!(ok = lnsocket_connect(ln, nodeid, "24.84.152.187"))) {
		lnsocket_print_errors(ln);
		goto done;
	}

	printf("connected!\n");
done:
	lnsocket_destroy(ln);
	return !ok;
}
