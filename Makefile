
OBJC_FLAGS = -fobjc-arc -framework Foundation
OBJC_FILES = Interference.m example.m

linenoise_example: linenoise.h linenoise.c

linenoise_example: linenoise.c example.c
	$(CC) -Wall -W -Os -g -o linenoise_example linenoise.c example.c

objc: linenoise.c Interference.h $(OBJC_FILES)
	$(CC) -w -g -o linenoise_objc_example linenoise.c $(OBJC_FILES) $(OBJC_FLAGS)

clean:
	rm -f linenoise_example linenoise_objc_example
