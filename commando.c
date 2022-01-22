
#include "lnsocket.h"
#include "cursor.h"
#include "endian.h"

#define COMMANDO_CMD 0x4c4f
#define COMMANDO_REPLY_CONTINUES 0x594b
#define COMMANDO_REPLY_TERM 0x594d

int commando_make_rpc_msg(const char *method, const char *params,
		const char *rune, uint64_t req_id, unsigned char *buf,
		int buflen, unsigned short *outlen)
{
	struct cursor msgbuf;
	int ok;

	make_cursor(buf, buf+buflen, &msgbuf);

	params = (params == NULL || params[0] == '\0') ? "[]" : params;

	if (!cursor_push_u16(&msgbuf, COMMANDO_CMD))
		return 0;

	if (!cursor_push_u64(&msgbuf, req_id))
		return 0;

	ok = cursor_push_str(&msgbuf, "{\"method\":\"") &&
	cursor_push_str(&msgbuf, method) &&
	cursor_push_str(&msgbuf, "\",\"params\":") &&
	cursor_push_str(&msgbuf, params) &&
	cursor_push_str(&msgbuf, ",\"rune\":\"") &&
	cursor_push_str(&msgbuf, rune) &&
	cursor_push_str(&msgbuf, "\"}");

	if (!ok)
		return 0;

	*outlen = msgbuf.p - msgbuf.start;

	return 1;
}
