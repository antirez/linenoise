CFLAGS+=$(shell pkg-config termkey --cflags)
LIBS+=$(shell pkg-config termkey --libs)

linenoise_example: linenoise.h linenoise.c

linenoise_example: linenoise.c example.c
	$(CC) -Wall -W -Os -g -o linenoise_example linenoise.c example.c $(CFLAGS) $(LIBS)

clean:
	rm -f linenoise_example
