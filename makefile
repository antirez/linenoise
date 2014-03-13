# Install directory
INSTALL_PREFIX = /usr/local

# OS_Depenedent library vars
UNAME_S = $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	LXTENSION = so
	LFLAGS   += -shared
endif
ifeq ($(UNAME_S),Darwin)
	LXTENSION = dylib
	LFLAGS   += -dynamiclib
endif

# Example program
linenoise_example: linenoise.h linenoise.c

linenoise_example: linenoise.c example.c
	$(CC) -Wall -W -Os -g -o linenoise_example linenoise.c example.c

# Library
library: linenoise.$(LXTENSION)

linenoise.$(LXTENSION): linenoise.o
	$(CC) $(LFLAGS) -o linenoise.$(LXTENSION) linenoise.o

linenoise.o:
	$(CC) -fPIC -Os -g -c -o linenoise.o linenoise.c

install: linenoise.$(LXTENSION)
	install linenoise.$(LXTENSION) $(INSTALL_PREFIX)/lib

uninstall:
	rm -vf $(INSTALL_PREFIX)/lib/linenoise.$(LXTENSION)

clean:
	rm -vf linenoise_example linenoise.o linenoise.$(LXTENSION)
