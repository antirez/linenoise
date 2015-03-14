#ifndef STRING_FMT_H
#define STRING_FMT_H

#include <string>

class string_fmt_c : public std::string
{
public:
    string_fmt_c() : std::string() {}
    string_fmt_c(const char *s) : std::string(s) {}

    void vformat(const char *fmt, va_list);
    void format(const char *fmt, ...);
    void append(const char *fmt, ...);
};

#endif
