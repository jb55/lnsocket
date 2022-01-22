
#ifndef LNSOCKET_COMMANDO
#define LNSOCKET_COMMANDO

#include <inttypes.h>

int commando_make_rpc_msg(const char *method, const char *params, const char *rune, uint64_t req_id, unsigned char *buf, int buflen, unsigned short *outlen);

#endif /* LNSOCKET_COMMANDO */
