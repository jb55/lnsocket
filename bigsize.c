/* Copyright Rusty Russell (Blockstream) 2015.
	     William Casarin 2022

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#include <assert.h>
#include "config.h"
#include "bigsize.h"

#ifndef SUPERVERBOSE
#define SUPERVERBOSE(...)
#endif

size_t bigsize_len(bigsize_t v)
{
	if (v < 0xfd) {
		return 1;
	} else if (v <= 0xffff) {
		return 3;
	} else if (v <= 0xffffffff) {
		return 5;
	} else {
		return 9;
	}
}

size_t bigsize_put(u8 buf[BIGSIZE_MAX_LEN], bigsize_t v)
{
	u8 *p = buf;

	if (v < 0xfd) {
		*(p++) = v;
	} else if (v <= 0xffff) {
		(*p++) = 0xfd;
		(*p++) = v >> 8;
		(*p++) = v;
	} else if (v <= 0xffffffff) {
		(*p++) = 0xfe;
		(*p++) = v >> 24;
		(*p++) = v >> 16;
		(*p++) = v >> 8;
		(*p++) = v;
	} else {
		(*p++) = 0xff;
		(*p++) = v >> 56;
		(*p++) = v >> 48;
		(*p++) = v >> 40;
		(*p++) = v >> 32;
		(*p++) = v >> 24;
		(*p++) = v >> 16;
		(*p++) = v >> 8;
		(*p++) = v;
	}
	return p - buf;
}

size_t bigsize_get(const u8 *p, size_t max, bigsize_t *val)
{
	if (max < 1) {
		SUPERVERBOSE("EOF");
		return 0;
	}

	switch (*p) {
	case 0xfd:
		if (max < 3) {
			SUPERVERBOSE("unexpected EOF");
			return 0;
		}
		*val = ((u64)p[1] << 8) + p[2];
		if (*val < 0xfd) {
			SUPERVERBOSE("decoded bigsize is not canonical");
			return 0;
		}
		return 3;
	case 0xfe:
		if (max < 5) {
			SUPERVERBOSE("unexpected EOF");
			return 0;
		}
		*val = ((u64)p[1] << 24) + ((u64)p[2] << 16)
			+ ((u64)p[3] << 8) + p[4];
		if ((*val >> 16) == 0) {
			SUPERVERBOSE("decoded bigsize is not canonical");
			return 0;
		}
		return 5;
	case 0xff:
		if (max < 9) {
			SUPERVERBOSE("unexpected EOF");
			return 0;
		}
		*val = ((u64)p[1] << 56) + ((u64)p[2] << 48)
			+ ((u64)p[3] << 40) + ((u64)p[4] << 32)
			+ ((u64)p[5] << 24) + ((u64)p[6] << 16)
			+ ((u64)p[7] << 8) + p[8];
		if ((*val >> 32) == 0) {
			SUPERVERBOSE("decoded bigsize is not canonical");
			return 0;
		}
		return 9;
	default:
		*val = *p;
		return 1;
	}
}

int cursor_pull_bigsize(struct cursor *cur, bigsize_t *out)
{
	return bigsize_get(cur->p, cur->end - cur->p, out);
}

int cursor_push_bigsize(struct cursor *cur, const bigsize_t val)
{
	u8 buf[BIGSIZE_MAX_LEN];
	size_t len;

	len = bigsize_put(buf, val);

	return cursor_push(cur, buf, len);
}

