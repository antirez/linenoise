linenoise_example: linenoise.h linenoise.c

linenoise_example: linenoise.c example.c
	$(CC) -Wall -W -Os -g -o linenoise_example linenoise.c example.c

test_cpp_compile: linenoise.h linenoise.c
	g++ -Wall -W -Os -g -c -o linenoise.o linenoise.c 

clean:
	rm -f linenoise_example
