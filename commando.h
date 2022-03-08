
#ifndef LNSOCKET_COMMANDO
#define LNSOCKET_COMMANDO

#include <inttypes.h>
#include "export.h"

#define COMMANDO_CMD 0x4c4f
#define COMMANDO_REPLY_CONTINUES 0x594b
#define COMMANDO_REPLY_TERM 0x594d

int EXPORT commando_make_rpc_msg(const char *method, const char *params, const char *rune, unsigned int req_id, unsigned char *buf, int buflen);

#endif /* LNSOCKET_COMMANDO */
