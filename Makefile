CC=gcc
CFLAGS=-Wall -W -Os -g
CPPFLAGS=

all: linenoise-example linenoise-test

linenoise-example: linenoise.h linenoise.c example.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o linenoise-example linenoise.c example.c

linenoise-test: linenoise-test.c linenoise-example
	$(CC) $(CFLAGS) $(CPPFLAGS) -o linenoise-test linenoise-test.c

test: linenoise-test linenoise-example
	./linenoise-test

clean:
	rm -f linenoise-example linenoise-test
