CC=gcc
COMMIT=$(shell ./getcommit.sh)
CFLAGS=-g -O0 -Wall -D__COMMIT__=$(COMMIT) -Wformat -Wformat-security -Wformat-nonliteral -Wformat=2 -ftrapv -std=gnu99 -Wno-unused-variable -Wno-unused-parameter -Wtype-limits
# Optimisation flags: -O3 -funroll-loops

objects := $(patsubst %.c,%.o,$(wildcard *.c util/*.c civs/*.c))

yastg: $(objects)
	cc -o yastg $(objects) -lm -pthread -lmcheck

clean:
	find . -name \*.o | xargs rm -f
	rm -f yastg
