CC ?= gcc
CFLAGS ?= -O0 -g -Wall -Wextra

all: slow_leak

slow_leak: main.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f slow_leak

.PHONY: all clean
