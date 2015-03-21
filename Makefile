
CXX_FLAGS = -std=c++0x -Wall -W -g

linenoise_example: lib example.cpp
	$(CXX) $(CXX_FLAGS)  -o example.o -c example.cpp
	$(CXX)  -o linenoise_example example.o linenoise.o ./linenoise.a

lib: linenoise.cpp string_fmt.cpp
	$(CXX) $(CXX_FLAGS) -o linenoise.o -c linenoise.cpp
	$(CXX) $(CXX_FLAGS) -o string_fmt.o -c string_fmt.cpp
	$(AR) rcs linenoise.a linenoise.o string_fmt.o


test: string_fmt.cpp
	$(CXX) $(CXX_FLAGS) -D_TEST -o test_string_fmt string_fmt.cpp

clean:
	rm -f linenoise_example
