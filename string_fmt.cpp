#include <alloca.h>
#include <stdarg.h>
#include <assert.h>

#include "string_fmt.h"

void string_fmt_c::
vformat (const char *fmt, va_list ap)
{
    int buf_sz = 256;
			
    while (1) {  
	char *buf = (char *) alloca(buf_sz);
	
	va_list apc;
	va_copy(apc, ap);
        int n = vsnprintf(buf, buf_sz, fmt, apc);
	
        if (n > -1 && n < buf_sz) {
	    *this = buf;
	    return;
        }
        if (n > -1)  // Needed size returned
            buf_sz = n + 1;   // For null char
        else {
	    assert(0);
	}
    }
}

void string_fmt_c::
format (const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vformat(fmt, ap);
    va_end(ap);
}
	
void string_fmt_c::
append (const char *fmt, ...)
{
    string_fmt_c tmp;
    
    va_list ap;
    va_start(ap, fmt);
    tmp.vformat(fmt, ap);

    *this += tmp;
}

#ifdef _TEST

#define TEST(x) if (!(x)) assert(0)

int
main ()
{
    string_fmt_c s, s1;

    s = "abc";
    s1.format("this '%s' is %d\n", "thing", 99);

    TEST(s1 == "this 'thing' is 99\n");

    s1.append("some %d", 55);
    TEST(s1 == "this 'thing' is 99\nsome 55");

    printf("all test passed\n");
    return 0;
}
#endif
