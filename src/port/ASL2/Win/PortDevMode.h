#ifndef PORTDEVMODE_H
#define PORTDEVMODE_H
#include "k2layout.h"

// The port's display-mode descriptor plus the enumerator that fills it. Both consumers
// live in different modules, the aurora video-mode facade and clientcore's
// ReadAndSetVideoMode/GetVideoModeSpec, so the type has its own header.
//
// The field offsets match the Win32 display-mode structure, which is also the value every caller
// stamps into m_size. Only these fields are ever read back.
#if KOTOR1
typedef struct _devicemodeA PortDevMode;   // K1's own name for the type, which its exported symbols carry
struct _devicemodeA {
#else
struct PortDevMode {
#endif
    char          _pad00[0x24];
    unsigned short m_size;
    char          _pad26[0x28-0x26];
    unsigned int   m_fields;
    char          _pad2c[0x68-0x2c];
    unsigned int   m_bitsPerPel;
    unsigned int   m_pelsWidth;
    unsigned int   m_pelsHeight;
    unsigned int   m_displayFlags;
    unsigned int   m_displayFrequency;
#if !KOTOR1  // K1's DEVMODEA ends here
    char          _pad7c[0x9c-0x7c];
#endif
};
#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(PortDevMode) == 0x7c, "K1 DEVMODEA");
#endif

#include "port/win32api.h"

#endif // PORTDEVMODE_H
