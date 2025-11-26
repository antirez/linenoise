linenoise_example: linenoise.h linenoise.c encodings/utf8.h encodings/utf8.c

linenoise_example: linenoise.c example.c encodings/utf8.c
	$(CC) -Wall -W -Os -g -o linenoise_example linenoise.c example.c encodings/utf8.c

clean:
	rm -f linenoise_example
