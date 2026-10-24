// mingw-w64's <fcntl.h> plus O_CLOEXEC (Windows builds only): a descriptor the child processes do
// not inherit is the CRT's _O_NOINHERIT. Every descriptor the shim opens holds binary data, so it
// carries _O_BINARY too: without it the CRT reads in text mode.
#include_next <fcntl.h>
#ifndef O_CLOEXEC
#define O_CLOEXEC (_O_NOINHERIT | _O_BINARY)
#endif
