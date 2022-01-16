#ifndef LNSOCKET_H
#define LNSOCKET_H

struct lnsocket;

int lnsocket_connect(struct lnsocket *, const char *node_id, const char *host);
void lnsocket_genkey(struct lnsocket *);
struct lnsocket *lnsocket_create();
void lnsocket_destroy(struct lnsocket *);
void lnsocket_print_errors(struct lnsocket *);

#endif /* LNSOCKET_H */