CC ?= gcc
CFLAGS ?= -O0 -g -Wall -Wextra

all: alloc_spike

alloc_spike: main.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f alloc_spike

.PHONY: all clean
