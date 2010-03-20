linenoise_example: linenoise.c example.c
	$(CC) -Wall -W -Os -o linenoise_example linenoise.c example.c

clean:
	rm -f linenoise_example
