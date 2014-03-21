
all : linenoise_example

linenoise.o : linenoise.h

liblinenoise.a : linenoise.o
	$(AR) rcs $@ $^

linenoise_example: elinenoise_example.c liblinenoise.a
	$(CC) $(CFLAGS) -L. -llinenoise -o $@ $<

clean:
	-rm -f linenoise_example
	-rm -f *.a *.o
