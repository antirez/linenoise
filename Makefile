.PHONY: all

all: linenoise_example

linenoise_example: linenoise.c example.c
	$(CC) -Wall -W -Os -g -o linenoise_example linenoise.c example.c

clean:
	rm -f linenoise_example
