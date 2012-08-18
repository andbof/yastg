CC=gcc
COMMIT=$(shell ./getcommit.sh)
CFLAGS=-g -O0 -Wall -D__COMMIT__=$(COMMIT) -Wformat -Wformat-security -Wformat-nonliteral -Wformat=2 -ftrapv -std=gnu99 -Wno-unused-variable -Wno-unused-parameter -Wtype-limits
# Optimisation flags: -O3 -funroll-loops

OUTPUT = yastg
SRCS = $(wildcard *.c util/*.c civs/*.c)
OBJECTS = $(patsubst %.c,%.o,$(SRCS))
DEPS = $(patsubst %.c,%.P,$(SRCS))

all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	cc -o yastg $(OBJECTS) -lm -pthread -lmcheck

clean:
	rm -f $(OBJECTS)
	rm -f $(DEPS)
	rm -f $(OUTPUT)

%.o: %.c
	$(COMPILE.c) -MD -o $@ $<
	@cp $*.d $*.P; \
		sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	rm -f $*.d

-include $(DEPS)
