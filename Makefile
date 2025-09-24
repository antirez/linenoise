CFLAGS := -Wall 
TARGET  = linenoise_example
  
all: *.c *.h $(TARGET)
  
$(TARGET): *.c
	$(CC) $(CFLAGS) -o $(TARGET) $^

$(TARGET)_objc: objc/*.m linenoise.c
	$(CC) $(CFLAGS) $^ -fobjc-arc -I$(CURDIR) -o $@
	
objc: $(TARGET)_objc *.h objc/*.h
	
clean:
	rm -rvf $(TARGET)* **.dSYM **.a **.out **.gch