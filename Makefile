linenoise_example: linenoise.h linenoise.c

linenoise_example: linenoise.c example.c
	$(CC) -Wall -Wextra -Wpedantic -std=c11 -Os -g -o linenoise_example linenoise.c example.c

clean:
	rm -f linenoise_example
