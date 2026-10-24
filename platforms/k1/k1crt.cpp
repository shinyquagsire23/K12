// Aspyr's MSVC CRT extensions, which KOTOR 1 declares with C++ linkage. They forward to the C ones
// (k1shimc.cpp). Kept out of k1port.cpp because the Windows CRT headers declare these three names
// with C linkage, and this unit includes no header at all.
extern "C" {
char* k2c_strlwr(char* s);
char* k2c_strupr(char* s);
char* k2c_itoa(int value, char* buf, int radix);
}

char* _strlwr(char* s) { return k2c_strlwr(s); }
char* _strupr(char* s) { return k2c_strupr(s); }
char* _itoa(int value, char* buf, int radix) { return k2c_itoa(value, buf, radix); }
