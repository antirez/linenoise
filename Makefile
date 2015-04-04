CXX=g++
CXXFLAGS=-Wall -W -Os -g
CCFLAGS=-Wall -W -Os -g

all: linenoise_example linenoise_example++11 linenoise_example++98

linenoise_example: linenoise.c example.c
	$(CC) $(CCFLAGS) -o linenoise_example linenoise.c example.c

linenoise_example++11: linenoise.h linenoise.c example.c
	$(CXX) $(CXXFLAGS) -std=c++11 -o linenoise_example++11 linenoise.c example.c

linenoise_example++98: linenoise.h linenoise.c example.c
	$(CXX) $(CXXFLAGS) -std=c++98 -o linenoise_example++98 linenoise.c example.c

clean:
	rm -f linenoise_example
	rm -f linenoise_example++11
	rm -f linenoise_example++98
