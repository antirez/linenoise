CXX=g++
CXXFLAGS=-Wall

.PHONY: all

all: linenoise_example

cpp: linenoise.h linenoise.c example.c
	$(CXX) $(CXXFLAGS) -o linenoise_example linenoise.c example.c

linenoise_example: linenoise.c example.c
	$(CC) -Wall -W -Os -g -o linenoise_example linenoise.c example.c

clean:
	rm -f linenoise_example
