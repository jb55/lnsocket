
CFLAGS=-Wall -g -Og

all: netln

netln: netln.c
	$(CC) $(CFLAGS) $^ -o $@

clean: fake
	rm -f netln

.PHONY: fake
