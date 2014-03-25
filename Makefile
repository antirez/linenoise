CFLAGS = -g -Wall
LDFLAGS = -g

all : linenoise_example

linenoise.o : linenoise.h

liblinenoise.a : linenoise.o lnCore.o lnTermPosix.o lnHist.o
	$(AR) rcs $@ $^

linenoise_example: example.c liblinenoise.a
	$(CC) $(CFLAGS) -o $@ $< -L. -llinenoise

clean:
	-rm -f linenoise_example
	-rm -f *.a *.o
