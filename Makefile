linenoise_example: linenoise.h linenoise.c

linenoise_example: linenoise.c example.c
	$(CC) -Wall -W -Os -g -o linenoise_example linenoise.c example.c

lib: linenoise.h linenoise.c
	$(CC) -Wall -W -Os -o linenoise.o linenoise.c
	ar rcs liblinenoise.a linenoise.o

clean:
	rm -f linenoise_example linenoise.o liblinenoise.a
