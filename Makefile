CFLAGS = -g
LDFLAGS = -g

all : linenoise_example

linenoise.o : linenoise.h

liblinenoise.a : linenoise.o
	$(AR) rcs $@ $^

linenoise_example: elinenoise_example.c liblinenoise.a
	$(CC) $(CFLAGS) -o $@ $< -L. -llinenoise

clean:
	-rm -f linenoise_example
	-rm -f *.a *.o
