linenoise_example: linenoise.h linenoise.c

linenoise_example: linenoise.c example.c
	$(CC) -Wall -W -Os -g -o linenoise_example linenoise.c example.c

lib: linenoise.c
	$(CC) -Wall  -g -o linenoise.o -c linenoise.c
	$(AR) rcs linenoise.a linenoise.o
clean:
	rm -f linenoise_example
