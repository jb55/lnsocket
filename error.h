
#ifndef LNSOCKET_ERROR_H
#define LNSOCKET_ERROR_H

#include "cursor.h"

struct error {
	const char *msg;
};

struct errors {
	struct cursor cur;
	int enabled;
};

#define note_error(errs, fmt, ...) note_error_(errs, "%s:%s:%d: " fmt, __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)

static inline int cursor_push_error(struct cursor *cur, struct error *err)
{
	return cursor_push_c_str(cur, err->msg);
}

static inline int cursor_pull_error(struct cursor *cur, struct error *err)
{
	return cursor_pull_c_str(cur, &err->msg);
}

int note_error_(struct errors *errs, const char *fmt, ...);

void print_error_backtrace(struct errors *errors);

#endif /* LNSOCKET_ERROR_H */
