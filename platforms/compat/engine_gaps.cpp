// Engine helpers src/ declares but does not define yet, which the host run reaches early. Each is a
// small, fully understood body whose real home is the TU named in its comment. Delete an entry when
// src/ defines it (the link will say so).
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

// funcholder.h: the snprintf-into-returnvalue helper.
extern "C" int FormatReturnValue(char* buf, uint64_t size, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return r;
}
