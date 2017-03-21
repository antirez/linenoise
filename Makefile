linenoise_example: linenoise.h linenoise.c

linenoise_example: linenoise.c example.c
	$(CC) -Wall -W -Os -g -o linenoise_example linenoise.c example.c

lib: liblinenoise.a

liblinenoise.a: linenoise.o
	$(AR) -rcs liblinenoise.a linenoise.o

.o:
	$(CC) -Wall -W -Os -c $<

clean:
	rm -f linenoise_example liblinenoise.a linenoise.o
