PREFIX = /usr/local

linenoise_example: linenoise.h linenoise.c

linenoise_example: linenoise.c example.c
	$(CC) -Wall -W -Os -g -o linenoise_example linenoise.c example.c

lib: liblinenoise.a

liblinenoise.a: linenoise.o
	$(AR) -rcs liblinenoise.a linenoise.o

.o:
	$(CC) -Wall -W -Os -c $<

install: liblinenoise.a linenoise.h
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	cp liblinenoise.a $(DESTDIR)$(PREFIX)/lib/liblinenoise.a
	mkdir -p $(DESTDIR)$(PREFIX)/include
	cp linenoise.h $(DESTDIR)$(PREFIX)/include/linenoise.h

clean:
	rm -f linenoise_example liblinenoise.a linenoise.o
