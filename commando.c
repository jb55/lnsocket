
#include "lnsocket.h"
#include "cursor.h"
#include "endian.h"
#include "commando.h"
#include "export.h"

int EXPORT commando_make_rpc_msg(const char *method, const char *params,
		const char *rune, unsigned int req_id, unsigned char *buf, int buflen)
{
	struct cursor msgbuf;
	int ok;

	make_cursor(buf, buf+buflen, &msgbuf);

	params = (params == NULL || params[0] == '\0') ? "[]" : params;

	if (!cursor_push_u16(&msgbuf, COMMANDO_CMD))
		return 0;

	if (!cursor_push_u64(&msgbuf, (u64)req_id))
		return 0;

	ok = cursor_push_str(&msgbuf, "{\"method\":\"") &&
	cursor_push_str(&msgbuf, method) &&
	cursor_push_str(&msgbuf, "\",\"params\":") &&
	cursor_push_str(&msgbuf, params) &&
	cursor_push_str(&msgbuf, ",\"rune\":\"") &&
	cursor_push_str(&msgbuf, rune) &&
	cursor_push_str(&msgbuf, "\",\"id\":\"d0\",\"jsonrpc\":\"2.0\"}");

	if (!ok)
		return 0;

	return msgbuf.p - msgbuf.start;
}
