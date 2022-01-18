#ifndef LNSOCKET_BIGSIZE_H
#define LNSOCKET_BIGSIZE_H
#include "config.h"
#include "cursor.h"
#include <stddef.h>

/* typedef for clarity. */
typedef u64 bigsize_t;

#define BIGSIZE_MAX_LEN 9

/* Returns length of buf used. */
size_t bigsize_put(unsigned char buf[BIGSIZE_MAX_LEN], bigsize_t v);

/* Returns 0 on failure, otherwise length (<= max) used.  */
size_t bigsize_get(const unsigned char *p, size_t max, bigsize_t *val);

/* How many bytes does it take to encode v? */
size_t bigsize_len(bigsize_t v);

/* Used for wire generation */
typedef bigsize_t bigsize;

/* marshal/unmarshal functions */
void towire_bigsize(unsigned char **pptr, const bigsize_t val);
bigsize_t fromwire_bigsize(const unsigned char **cursor, size_t *max);

int cursor_pull_bigsize(struct cursor *cur, bigsize_t *out);
int cursor_push_bigsize(struct cursor *cur, const bigsize_t val);

#endif /* LNSOCKET_BIGSIZE_H */
