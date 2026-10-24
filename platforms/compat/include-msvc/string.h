// The UCRT's <string.h> plus POSIX's <strings.h> calls, which glibc's, Darwin's and mingw-w64's
// <string.h> also declare (compat/include-mingw/string.h adds strlcpy / strlcat / strcasestr).
#include_next <string.h>
#include <strings.h>
