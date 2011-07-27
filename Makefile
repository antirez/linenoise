OPTS=-Wall -W -Os -g

linenoise_example: linenoise.h linenoise.c

linenoise_example: linenoise.c example.c
	$(CC) $(OPTS) -o linenoise_example linenoise.c example.c

clean:
	rm -f linenoise_example
