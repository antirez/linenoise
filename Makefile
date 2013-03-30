all: linenoise_example linenoise_example_nonblock

linenoise_example: linenoise.h linenoise.c

linenoise_example_nonblock: linenoise.h linenoise.c

linenoise_example: linenoise.c example.c
	$(CC) -Wall -W -Os -g -o linenoise_example linenoise.c example.c

linenoise_example_nonblock: linenoise.c example_nonblock.c
	$(CC) -Wall -W -Os -g -o linenoise_example_nonblock linenoise.c example_nonblock.c

clean:
	rm -f linenoise_example linenoise_example_nonblock

