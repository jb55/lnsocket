
#include "error.h"

#include <stdlib.h>
#include <stdarg.h>

int note_error_(struct errors *errs_, const char *fmt, ...)
{
	static char buf[512];
	struct error err;
	struct cursor *errs;
	va_list ap;

	errs = &errs_->cur;

	if (errs_->enabled == 0)
		return 0;

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	err.msg = buf;

	if (!cursor_push_error(errs, &err)) {
		fprintf(stderr, "arena OOM when recording error, ");
		fprintf(stderr, "errs->p at %ld, remaining %ld, strlen %ld\n",
				errs->p - errs->start, errs->end - errs->p, strlen(buf));
	}

	return 0;
}

void print_error_backtrace(struct errors *errors)
{
	struct cursor errs;
	struct error err;

	copy_cursor(&errors->cur, &errs);
	errs.p = errs.start;

	while (errs.p < errors->cur.p) {
		if (!cursor_pull_error(&errs, &err)) {
			fprintf(stderr, "backtrace: couldn't pull error\n");
			return;
		}
		fprintf(stderr, "%s\n", err.msg);
	}
}
