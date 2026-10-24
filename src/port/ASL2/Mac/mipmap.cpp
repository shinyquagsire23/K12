// GLU's 2D mipmap builder, a copy of SGI's `mipmap.c` (the sample implementation Mesa's libGLU
// ships), as far as the engine's own entry point needs it. KOTOR 1 has no GLU of its own: it calls
// `gluBuild2DMipmaps` directly.
#include "k2stdint.h"
#include "port/aslglapi.h"
#include <limits.h>

#if !KOTOR1

// The tail callee is internal to this file and defined at its end. It takes the size pair twice (the
// raw width/height and again as the power-of-two pair) before format and type, and computes its own
// logs from the second pair, which is why this caller passes the pair twice. It stays out of line so
// the builder and the core remain separate functions.
namespace {
    __attribute__((noinline)) int
    gluBuild2DMipmapLevelsCore(unsigned int nTarget, int nInternalFormat, int nWidth, int nHeight,
                               int nWidthPow2, int nHeightPow2, unsigned int nFormat,
                               unsigned int nType, int nUserLevel, int nBaseLevel, int nMaxLevel,
                               const void* pData);
}

// GLU's two error codes. The format and type guards return INVALID_ENUM, every size and level guard
// returns INVALID_VALUE. The shared return block re-materialises whichever one is live.
enum {
    GLU_INVALID_ENUM  = 0x18a24,
    GLU_INVALID_VALUE = 0x18a25,
};

// A power-of-two log2 answering -1 for anything that is not a power of two. The loop is written
// test-first, and both halves must stay as they are.
namespace {
    int gluMipmapLog2(unsigned int n)
    {
        if (n == 0)
            return -1;
        int nLog = 0;
        while ((n & 1) == 0) {
            n >>= 1;
            nLog++;
        }
        return n == 1 ? nLog : -1;
    }
}

int gluBuild2DMipmapLevels(unsigned int nTarget, int nInternalFormat, int nWidth, int nHeight,
                           unsigned int nFormat, unsigned int nType, int nUserLevel, int nBaseLevel,
                           int nMaxLevel, const void* pData)
{
    if (nFormat - GL_COLOR_INDEX > GL_LUMINANCE_ALPHA - GL_COLOR_INDEX)
        return GLU_INVALID_ENUM;
    // The two guards share one format test: the type half is a negated range test short-circuited
    // with the second type value, and only then is the format re-tested. Seven types are accepted,
    // not eight: the eighth constant is rejected like any other unknown type.
    if (static_cast<unsigned int>(nType - GL_BYTE) >= 7) {
        if (nType != GL_BITMAP)
            return GLU_INVALID_ENUM;
    }
    if (nFormat == GL_STENCIL_INDEX)
        return GLU_INVALID_ENUM;
    if (nWidth < 1 || nHeight < 1)
        return GLU_INVALID_VALUE;
    int nWidthLog = gluMipmapLog2(static_cast<unsigned int>(nWidth));
    int nHeightLog = gluMipmapLog2(static_cast<unsigned int>(nHeight));
    // The larger of the two logs bounds the level count.
    int nMaxLog = nHeightLog > nWidthLog ? nHeightLog : nWidthLog;
    if (nBaseLevel < 0)
        return GLU_INVALID_VALUE;
    if (nBaseLevel < nUserLevel)
        return GLU_INVALID_VALUE;
    if (nMaxLevel < nBaseLevel)
        return GLU_INVALID_VALUE;
    if (nMaxLog + nUserLevel < nMaxLevel)
        return GLU_INVALID_VALUE;
    return gluBuild2DMipmapLevelsCore(nTarget, nInternalFormat, nWidth, nHeight, nWidth, nHeight,
                                      nFormat, nType, nUserLevel, nBaseLevel, nMaxLevel, pData);
}

// GLU mipmap builder behind the two entry points, SGI's `mipmap.c` (the sample implementation
// Mesa's libGLU ships), cut down to the seven plain pixel types and four colour formats. It never
// reads the GL's pixel-store state: `retrieveStoreModes` answers the tightest unpack modes, so rows
// are never padded, skipped or byte-swapped, and no GL_UNPACK_* state is set or restored.
// `gluScaleImage` and `gluBuild2DMipmaps` come from the same source; the platforms layer supplies
// its own `gluBuild2DMipmaps`.
#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include "port/libcmath.h"

typedef unsigned int GLenum;
typedef int GLint;
typedef unsigned int GLuint;
typedef int GLsizei;
typedef signed char GLbyte;
typedef unsigned char GLubyte;
typedef short GLshort;
typedef unsigned short GLushort;
typedef float GLfloat;
typedef double GLdouble;
typedef unsigned char GLboolean;

namespace {
    enum {
        GLU_OUT_OF_MEMORY = 0x18a26,
    };

    struct PixelStorageModes {
        GLint unpack_alignment;
        GLint unpack_row_length;
        GLint unpack_skip_rows;
        GLint unpack_skip_pixels;
        GLint unpack_lsb_first;
        GLint unpack_swap_bytes;
    };

    void retrieveStoreModes(PixelStorageModes* psm)
    {
        psm->unpack_alignment = 1;
        psm->unpack_row_length = 0;
        psm->unpack_skip_rows = 0;
        psm->unpack_skip_pixels = 0;
        psm->unpack_lsb_first = 0;
        psm->unpack_swap_bytes = 0;
    }

    // Only the four colour formats have more than one element per group; the two index formats,
    // alpha and luminance fall through to one.
    GLint elements_per_group(GLenum format)
    {
        switch (format) {
        case GL_RGB:
            return 3;
        case GL_LUMINANCE_ALPHA:
            return 2;
        case GL_RGBA:
            return 4;
        default:
            return 1;
        }
    }

    GLfloat bytes_per_element(GLenum type)
    {
        switch (type) {
        case GL_BITMAP:
            return 1.0 / 8.0;
        case GL_UNSIGNED_SHORT:
            return sizeof(GLushort);
        case GL_SHORT:
            return sizeof(GLshort);
        case GL_UNSIGNED_BYTE:
            return sizeof(GLubyte);
        case GL_BYTE:
            return sizeof(GLbyte);
        case GL_INT:
            return sizeof(GLint);
        case GL_UNSIGNED_INT:
            return sizeof(GLuint);
        case GL_FLOAT:
            return sizeof(GLfloat);
        default:
            return 4;
        }
    }

    GLint is_index(GLenum format)
    {
        return format == GL_COLOR_INDEX || format == GL_STENCIL_INDEX;
    }

    // The memory one image of the given type and format takes, rows unpadded.
    GLint image_size(GLint width, GLint height, GLenum format, GLenum type)
    {
        int bytes_per_row;
        int components = elements_per_group(format);
        if (type == GL_BITMAP)
            bytes_per_row = (width + 7) / 8;
        else
            bytes_per_row = static_cast<int>(bytes_per_element(type) * static_cast<GLfloat>(width));
        return bytes_per_row * height * components;
    }

    // The power of two nearest `value` (a 3 rounds up to 4); -1 for zero.
    int nearestPower(GLuint value)
    {
        int i;

        i = 1;

        if (value == 0)
            return -1;

        for (;;) {
            if (value == 1) {
                return i;
            } else if (value == 3) {
                return i * 4;
            }
            value = value >> 1;
            i *= 2;
        }
    }

// Byte-swapped reads, each a 2- or 4-byte value assembled from its bytes, low byte first.
#define GLU_SWAP_2_BYTES(s)                                                                        \
    static_cast<GLushort>((static_cast<GLushort>((reinterpret_cast<const GLubyte*>(s))[1])) << 8 | (reinterpret_cast<const GLubyte*>(s))[0])
#define GLU_SWAP_4_BYTES(s)                                                                        \
    static_cast<GLuint>((static_cast<GLuint>((reinterpret_cast<const GLubyte*>(s))[3])) << 24 | (static_cast<GLuint>((reinterpret_cast<const GLubyte*>(s))[2])) << 16 | \
             (static_cast<GLuint>((reinterpret_cast<const GLubyte*>(s))[1])) << 8 | (reinterpret_cast<const GLubyte*>(s))[0])

    void halveImage(GLint components, GLuint width, GLuint height, const GLushort* datain,
                    GLushort* dataout)
    {
        int i, j, k;
        int newwidth, newheight;
        int delta;
        GLushort* s;
        const GLushort* t;

        newwidth = width / 2;
        newheight = height / 2;
        delta = width * components;
        s = dataout;
        t = datain;

        for (i = 0; i < newheight; i++) {
            for (j = 0; j < newwidth; j++) {
                for (k = 0; k < components; k++) {
                    s[0] = (t[0] + t[components] + t[delta] + t[delta + components] + 2) / 4;
                    s++;
                    t++;
                }
                t += components;
            }
            t += delta;
        }
    }

    void halve1Dimage_ubyte(GLint components, GLuint width, GLuint height, const GLubyte* dataIn,
                            GLubyte* dataOut, GLint element_size, GLint ysize, GLint group_size)
    {
        GLint halfWidth = width / 2;
        GLint halfHeight = height / 2;
        const char* src = reinterpret_cast<const char*>(dataIn);
        GLubyte* dest = dataOut;
        int jj;

        if (height == 1) { // 1 row
            halfHeight = 1;

            for (jj = 0; jj < halfWidth; jj++) {
                int kk;
                for (kk = 0; kk < components; kk++) {
                    *dest = (*reinterpret_cast<const GLubyte*>(src) + *reinterpret_cast<const GLubyte*>(src + group_size)) / 2;

                    src += element_size;
                    dest++;
                }
                src += group_size;
            }
        } else if (width == 1) { // 1 column
            int padBytes = ysize - (width * group_size);
            halfWidth = 1;

            for (jj = 0; jj < halfHeight; jj++) {
                int kk;
                for (kk = 0; kk < components; kk++) {
                    *dest = (*reinterpret_cast<const GLubyte*>(src) + *reinterpret_cast<const GLubyte*>(src + ysize)) / 2;

                    src += element_size;
                    dest++;
                }
                src += padBytes;
                src += ysize;
            }
        }
    }

    void halveImage_ubyte(GLint components, GLuint width, GLuint height, const GLubyte* datain,
                          GLubyte* dataout, GLint element_size, GLint ysize, GLint group_size)
    {
        int i, j, k;
        int newwidth, newheight;
        GLubyte* s;
        const char* t;

        // handle case where there is only 1 column/row
        if (width == 1 || height == 1) {
            halve1Dimage_ubyte(components, width, height, datain, dataout, element_size, ysize,
                               group_size);
            return;
        }

        newwidth = width / 2;
        newheight = height / 2;
        s = dataout;
        t = reinterpret_cast<const char*>(datain);

        for (i = 0; i < newheight; i++) {
            for (j = 0; j < newwidth; j++) {
                for (k = 0; k < components; k++) {
                    s[0] = (*reinterpret_cast<const GLubyte*>(t) + *reinterpret_cast<const GLubyte*>(t + group_size) +
                            *reinterpret_cast<const GLubyte*>(t + ysize) +
                            *reinterpret_cast<const GLubyte*>(t + ysize + group_size) + 2) /
                           4;
                    s++;
                    t += element_size;
                }
                t += group_size;
            }
            t += ysize;
        }
    }

    void halve1Dimage_byte(GLint components, GLuint width, GLuint height, const GLbyte* dataIn,
                           GLbyte* dataOut, GLint element_size, GLint ysize, GLint group_size)
    {
        GLint halfWidth = width / 2;
        GLint halfHeight = height / 2;
        const char* src = reinterpret_cast<const char*>(dataIn);
        GLbyte* dest = dataOut;
        int jj;

        if (height == 1) { // 1 row
            halfHeight = 1;

            for (jj = 0; jj < halfWidth; jj++) {
                int kk;
                for (kk = 0; kk < components; kk++) {
                    *dest = (*reinterpret_cast<const GLbyte*>(src) + *reinterpret_cast<const GLbyte*>(src + group_size)) / 2;

                    src += element_size;
                    dest++;
                }
                src += group_size;
            }
        } else if (width == 1) { // 1 column
            int padBytes = ysize - (width * group_size);
            halfWidth = 1;

            for (jj = 0; jj < halfHeight; jj++) {
                int kk;
                for (kk = 0; kk < components; kk++) {
                    *dest = (*reinterpret_cast<const GLbyte*>(src) + *reinterpret_cast<const GLbyte*>(src + ysize)) / 2;

                    src += element_size;
                    dest++;
                }
                src += padBytes;
                src += ysize;
            }
        }
    }

    void halveImage_byte(GLint components, GLuint width, GLuint height, const GLbyte* datain,
                         GLbyte* dataout, GLint element_size, GLint ysize, GLint group_size)
    {
        int i, j, k;
        int newwidth, newheight;
        GLbyte* s;
        const char* t;

        // handle case where there is only 1 column/row
        if (width == 1 || height == 1) {
            halve1Dimage_byte(components, width, height, datain, dataout, element_size, ysize,
                              group_size);
            return;
        }

        newwidth = width / 2;
        newheight = height / 2;
        s = dataout;
        t = reinterpret_cast<const char*>(datain);

        for (i = 0; i < newheight; i++) {
            for (j = 0; j < newwidth; j++) {
                for (k = 0; k < components; k++) {
                    s[0] = (*reinterpret_cast<const GLbyte*>(t) + *reinterpret_cast<const GLbyte*>(t + group_size) +
                            *reinterpret_cast<const GLbyte*>(t + ysize) + *reinterpret_cast<const GLbyte*>(t + ysize + group_size) +
                            2) /
                           4;
                    s++;
                    t += element_size;
                }
                t += group_size;
            }
            t += ysize;
        }
    }

    void halve1Dimage_ushort(GLint components, GLuint width, GLuint height, const GLushort* dataIn,
                             GLushort* dataOut, GLint element_size, GLint ysize, GLint group_size,
                             GLint myswap_bytes)
    {
        GLint halfWidth = width / 2;
        GLint halfHeight = height / 2;
        const char* src = reinterpret_cast<const char*>(dataIn);
        GLushort* dest = dataOut;
        int jj;

        if (height == 1) { // 1 row
            halfHeight = 1;

            for (jj = 0; jj < halfWidth; jj++) {
                int kk;
                for (kk = 0; kk < components; kk++) {
                    GLushort ushort[2];
                    if (myswap_bytes) {
                        ushort[0] = GLU_SWAP_2_BYTES(src);
                        ushort[1] = GLU_SWAP_2_BYTES(src + group_size);
                    } else {
                        ushort[0] = *reinterpret_cast<const GLushort*>(src);
                        ushort[1] = *reinterpret_cast<const GLushort*>(src + group_size);
                    }

                    *dest = (ushort[0] + ushort[1]) / 2;
                    src += element_size;
                    dest++;
                }
                src += group_size;
            }
        } else if (width == 1) { // 1 column
            int padBytes = ysize - (width * group_size);
            halfWidth = 1;

            for (jj = 0; jj < halfHeight; jj++) {
                int kk;
                for (kk = 0; kk < components; kk++) {
                    GLushort ushort[2];
                    if (myswap_bytes) {
                        ushort[0] = GLU_SWAP_2_BYTES(src);
                        ushort[1] = GLU_SWAP_2_BYTES(src + ysize);
                    } else {
                        ushort[0] = *reinterpret_cast<const GLushort*>(src);
                        ushort[1] = *reinterpret_cast<const GLushort*>(src + ysize);
                    }
                    *dest = (ushort[0] + ushort[1]) / 2;

                    src += element_size;
                    dest++;
                }
                src += padBytes;
                src += ysize;
            }
        }
    }

    void halveImage_ushort(GLint components, GLuint width, GLuint height, const GLushort* datain,
                           GLushort* dataout, GLint element_size, GLint ysize, GLint group_size,
                           GLint myswap_bytes)
    {
        int i, j, k;
        int newwidth, newheight;
        GLushort* s;
        const char* t;

        // handle case where there is only 1 column/row
        if (width == 1 || height == 1) {
            halve1Dimage_ushort(components, width, height, datain, dataout, element_size, ysize,
                                group_size, myswap_bytes);
            return;
        }

        newwidth = width / 2;
        newheight = height / 2;
        s = dataout;
        t = reinterpret_cast<const char*>(datain);

        if (!myswap_bytes)
            for (i = 0; i < newheight; i++) {
                for (j = 0; j < newwidth; j++) {
                    for (k = 0; k < components; k++) {
                        s[0] = (*reinterpret_cast<const GLushort*>(t) + *reinterpret_cast<const GLushort*>(t + group_size) +
                                *reinterpret_cast<const GLushort*>(t + ysize) +
                                *reinterpret_cast<const GLushort*>(t + ysize + group_size) + 2) /
                               4;
                        s++;
                        t += element_size;
                    }
                    t += group_size;
                }
                t += ysize;
            }
        else
            for (i = 0; i < newheight; i++) {
                for (j = 0; j < newwidth; j++) {
                    for (k = 0; k < components; k++) {
                        s[0] = (GLU_SWAP_2_BYTES(t) + GLU_SWAP_2_BYTES(t + group_size) +
                                GLU_SWAP_2_BYTES(t + ysize) +
                                GLU_SWAP_2_BYTES(t + ysize + group_size) + 2) /
                               4;
                        s++;
                        t += element_size;
                    }
                    t += group_size;
                }
                t += ysize;
            }
    }

    void halve1Dimage_short(GLint components, GLuint width, GLuint height, const GLshort* dataIn,
                            GLshort* dataOut, GLint element_size, GLint ysize, GLint group_size,
                            GLint myswap_bytes)
    {
        GLint halfWidth = width / 2;
        GLint halfHeight = height / 2;
        const char* src = reinterpret_cast<const char*>(dataIn);
        GLshort* dest = dataOut;
        int jj;

        if (height == 1) { // 1 row
            halfHeight = 1;

            for (jj = 0; jj < halfWidth; jj++) {
                int kk;
                for (kk = 0; kk < components; kk++) {
                    GLshort sshort[2];
                    if (myswap_bytes) {
                        sshort[0] = GLU_SWAP_2_BYTES(src);
                        sshort[1] = GLU_SWAP_2_BYTES(src + group_size);
                    } else {
                        sshort[0] = *reinterpret_cast<const GLshort*>(src);
                        sshort[1] = *reinterpret_cast<const GLshort*>(src + group_size);
                    }

                    *dest = (sshort[0] + sshort[1]) / 2;
                    src += element_size;
                    dest++;
                }
                src += group_size;
            }
        } else if (width == 1) { // 1 column
            int padBytes = ysize - (width * group_size);
            halfWidth = 1;

            for (jj = 0; jj < halfHeight; jj++) {
                int kk;
                for (kk = 0; kk < components; kk++) {
                    GLshort sshort[2];
                    if (myswap_bytes) {
                        sshort[0] = GLU_SWAP_2_BYTES(src);
                        sshort[1] = GLU_SWAP_2_BYTES(src + ysize);
                    } else {
                        sshort[0] = *reinterpret_cast<const GLshort*>(src);
                        sshort[1] = *reinterpret_cast<const GLshort*>(src + ysize);
                    }
                    *dest = (sshort[0] + sshort[1]) / 2;

                    src += element_size;
                    dest++;
                }
                src += padBytes;
                src += ysize;
            }
        }
    }

    void halveImage_short(GLint components, GLuint width, GLuint height, const GLshort* datain,
                          GLshort* dataout, GLint element_size, GLint ysize, GLint group_size,
                          GLint myswap_bytes)
    {
        int i, j, k;
        int newwidth, newheight;
        GLshort* s;
        const char* t;

        // handle case where there is only 1 column/row
        if (width == 1 || height == 1) {
            halve1Dimage_short(components, width, height, datain, dataout, element_size, ysize,
                               group_size, myswap_bytes);
            return;
        }

        newwidth = width / 2;
        newheight = height / 2;
        s = dataout;
        t = reinterpret_cast<const char*>(datain);

        if (!myswap_bytes)
            for (i = 0; i < newheight; i++) {
                for (j = 0; j < newwidth; j++) {
                    for (k = 0; k < components; k++) {
                        s[0] = (*reinterpret_cast<const GLshort*>(t) + *reinterpret_cast<const GLshort*>(t + group_size) +
                                *reinterpret_cast<const GLshort*>(t + ysize) +
                                *reinterpret_cast<const GLshort*>(t + ysize + group_size) + 2) /
                               4;
                        s++;
                        t += element_size;
                    }
                    t += group_size;
                }
                t += ysize;
            }
        else
            for (i = 0; i < newheight; i++) {
                for (j = 0; j < newwidth; j++) {
                    for (k = 0; k < components; k++) {
                        GLushort b;
                        GLint buf;
                        b = GLU_SWAP_2_BYTES(t);
                        buf = *reinterpret_cast<const GLshort*>(&b);
                        b = GLU_SWAP_2_BYTES(t + group_size);
                        buf += *reinterpret_cast<const GLshort*>(&b);
                        b = GLU_SWAP_2_BYTES(t + ysize);
                        buf += *reinterpret_cast<const GLshort*>(&b);
                        b = GLU_SWAP_2_BYTES(t + ysize + group_size);
                        buf += *reinterpret_cast<const GLshort*>(&b);
                        s[0] = static_cast<GLshort>((buf + 2) / 4);
                        s++;
                        t += element_size;
                    }
                    t += group_size;
                }
                t += ysize;
            }
    }

    void halve1Dimage_uint(GLint components, GLuint width, GLuint height, const GLuint* dataIn,
                           GLuint* dataOut, GLint element_size, GLint ysize, GLint group_size,
                           GLint myswap_bytes)
    {
        GLint halfWidth = width / 2;
        GLint halfHeight = height / 2;
        const char* src = reinterpret_cast<const char*>(dataIn);
        GLuint* dest = dataOut;
        int jj;

        if (height == 1) { // 1 row
            halfHeight = 1;

            for (jj = 0; jj < halfWidth; jj++) {
                int kk;
                for (kk = 0; kk < components; kk++) {
                    GLuint uint[2];
                    if (myswap_bytes) {
                        uint[0] = GLU_SWAP_4_BYTES(src);
                        uint[1] = GLU_SWAP_4_BYTES(src + group_size);
                    } else {
                        uint[0] = *reinterpret_cast<const GLuint*>(src);
                        uint[1] = *reinterpret_cast<const GLuint*>(src + group_size);
                    }
                    *dest = static_cast<GLuint>((static_cast<double>(uint[0]) + static_cast<double>(uint[1])) / 2.0);

                    src += element_size;
                    dest++;
                }
                src += group_size;
            }
        } else if (width == 1) { // 1 column
            int padBytes = ysize - (width * group_size);
            halfWidth = 1;

            for (jj = 0; jj < halfHeight; jj++) {
                int kk;
                for (kk = 0; kk < components; kk++) {
                    GLuint uint[2];
                    if (myswap_bytes) {
                        uint[0] = GLU_SWAP_4_BYTES(src);
                        uint[1] = GLU_SWAP_4_BYTES(src + ysize);
                    } else {
                        uint[0] = *reinterpret_cast<const GLuint*>(src);
                        uint[1] = *reinterpret_cast<const GLuint*>(src + ysize);
                    }
                    *dest = static_cast<GLuint>((static_cast<double>(uint[0]) + static_cast<double>(uint[1])) / 2.0);

                    src += element_size;
                    dest++;
                }
                src += padBytes;
                src += ysize;
            }
        }
    }

    void halveImage_uint(GLint components, GLuint width, GLuint height, const GLuint* datain,
                         GLuint* dataout, GLint element_size, GLint ysize, GLint group_size,
                         GLint myswap_bytes)
    {
        int i, j, k;
        int newwidth, newheight;
        GLuint* s;
        const char* t;

        // handle case where there is only 1 column/row
        if (width == 1 || height == 1) {
            halve1Dimage_uint(components, width, height, datain, dataout, element_size, ysize,
                              group_size, myswap_bytes);
            return;
        }

        newwidth = width / 2;
        newheight = height / 2;
        s = dataout;
        t = reinterpret_cast<const char*>(datain);

        if (!myswap_bytes)
            for (i = 0; i < newheight; i++) {
                for (j = 0; j < newwidth; j++) {
                    for (k = 0; k < components; k++) {
                        // need to cast to double to hold large unsigned ints
                        s[0] =
                            static_cast<GLuint>((static_cast<double>(*reinterpret_cast<const GLuint*>(t)) + static_cast<double>(*reinterpret_cast<const GLuint*>(t + group_size)) +
                             static_cast<double>(*reinterpret_cast<const GLuint*>(t + ysize)) +
                             static_cast<double>(*reinterpret_cast<const GLuint*>(t + ysize + group_size))) /
                                4 +
                            0.5);
                        s++;
                        t += element_size;
                    }
                    t += group_size;
                }
                t += ysize;
            }
        else
            for (i = 0; i < newheight; i++) {
                for (j = 0; j < newwidth; j++) {
                    for (k = 0; k < components; k++) {
                        // need to cast to double to hold large unsigned ints
                        GLdouble buf;
                        buf = static_cast<GLdouble>(GLU_SWAP_4_BYTES(t)) +
                              static_cast<GLdouble>(GLU_SWAP_4_BYTES(t + group_size)) +
                              static_cast<GLdouble>(GLU_SWAP_4_BYTES(t + ysize)) +
                              static_cast<GLdouble>(GLU_SWAP_4_BYTES(t + ysize + group_size));
                        s[0] = static_cast<GLuint>(buf / 4 + 0.5);

                        s++;
                        t += element_size;
                    }
                    t += group_size;
                }
                t += ysize;
            }
    }

    void halve1Dimage_int(GLint components, GLuint width, GLuint height, const GLint* dataIn,
                          GLint* dataOut, GLint element_size, GLint ysize, GLint group_size,
                          GLint myswap_bytes)
    {
        GLint halfWidth = width / 2;
        GLint halfHeight = height / 2;
        const char* src = reinterpret_cast<const char*>(dataIn);
        GLint* dest = dataOut;
        int jj;

        if (height == 1) { // 1 row
            halfHeight = 1;

            for (jj = 0; jj < halfWidth; jj++) {
                int kk;
                for (kk = 0; kk < components; kk++) {
                    GLuint uint[2];
                    if (myswap_bytes) {
                        uint[0] = GLU_SWAP_4_BYTES(src);
                        uint[1] = GLU_SWAP_4_BYTES(src + group_size);
                    } else {
                        uint[0] = *reinterpret_cast<const GLuint*>(src);
                        uint[1] = *reinterpret_cast<const GLuint*>(src + group_size);
                    }
                    *dest = static_cast<GLint>((static_cast<float>(uint[0]) + static_cast<float>(uint[1])) / 2.0);

                    src += element_size;
                    dest++;
                }
                src += group_size;
            }
        } else if (width == 1) { // 1 column
            int padBytes = ysize - (width * group_size);
            halfWidth = 1;

            for (jj = 0; jj < halfHeight; jj++) {
                int kk;
                for (kk = 0; kk < components; kk++) {
                    GLuint uint[2];
                    if (myswap_bytes) {
                        uint[0] = GLU_SWAP_4_BYTES(src);
                        uint[1] = GLU_SWAP_4_BYTES(src + ysize);
                    } else {
                        uint[0] = *reinterpret_cast<const GLuint*>(src);
                        uint[1] = *reinterpret_cast<const GLuint*>(src + ysize);
                    }
                    *dest = static_cast<GLint>((static_cast<float>(uint[0]) + static_cast<float>(uint[1])) / 2.0);

                    src += element_size;
                    dest++;
                }
                src += padBytes;
                src += ysize;
            }
        }
    }

    void halveImage_int(GLint components, GLuint width, GLuint height, const GLint* datain,
                        GLint* dataout, GLint element_size, GLint ysize, GLint group_size,
                        GLint myswap_bytes)
    {
        int i, j, k;
        int newwidth, newheight;
        GLint* s;
        const char* t;

        // handle case where there is only 1 column/row
        if (width == 1 || height == 1) {
            halve1Dimage_int(components, width, height, datain, dataout, element_size, ysize,
                             group_size, myswap_bytes);
            return;
        }

        newwidth = width / 2;
        newheight = height / 2;
        s = dataout;
        t = reinterpret_cast<const char*>(datain);

        if (!myswap_bytes)
            for (i = 0; i < newheight; i++) {
                for (j = 0; j < newwidth; j++) {
                    for (k = 0; k < components; k++) {
                        s[0] = static_cast<GLint>((static_cast<float>(*reinterpret_cast<const GLint*>(t)) + static_cast<float>(*reinterpret_cast<const GLint*>(t + group_size)) +
                                static_cast<float>(*reinterpret_cast<const GLint*>(t + ysize)) +
                                static_cast<float>(*reinterpret_cast<const GLint*>(t + ysize + group_size))) /
                                   4 +
                               0.5);
                        s++;
                        t += element_size;
                    }
                    t += group_size;
                }
                t += ysize;
            }
        else
            for (i = 0; i < newheight; i++) {
                for (j = 0; j < newwidth; j++) {
                    for (k = 0; k < components; k++) {
                        GLuint b;
                        GLfloat buf;
                        b = GLU_SWAP_4_BYTES(t);
                        buf = static_cast<GLfloat>(*reinterpret_cast<GLint*>(&b));
                        b = GLU_SWAP_4_BYTES(t + group_size);
                        buf += static_cast<GLfloat>(*reinterpret_cast<GLint*>(&b));
                        b = GLU_SWAP_4_BYTES(t + ysize);
                        buf += static_cast<GLfloat>(*reinterpret_cast<GLint*>(&b));
                        b = GLU_SWAP_4_BYTES(t + ysize + group_size);
                        buf += static_cast<GLfloat>(*reinterpret_cast<GLint*>(&b));
                        s[0] = static_cast<GLint>(buf / 4 + 0.5);

                        s++;
                        t += element_size;
                    }
                    t += group_size;
                }
                t += ysize;
            }
    }

    void halve1Dimage_float(GLint components, GLuint width, GLuint height, const GLfloat* dataIn,
                            GLfloat* dataOut, GLint element_size, GLint ysize, GLint group_size,
                            GLint myswap_bytes)
    {
        GLint halfWidth = width / 2;
        GLint halfHeight = height / 2;
        const char* src = reinterpret_cast<const char*>(dataIn);
        GLfloat* dest = dataOut;
        int jj;

        if (height == 1) { // 1 row
            halfHeight = 1;

            for (jj = 0; jj < halfWidth; jj++) {
                int kk;
                for (kk = 0; kk < components; kk++) {
                    GLfloat sfloat[2];
                    if (myswap_bytes) {
                        sfloat[0] = static_cast<GLfloat>(GLU_SWAP_4_BYTES(src));
                        sfloat[1] = static_cast<GLfloat>(GLU_SWAP_4_BYTES(src + group_size));
                    } else {
                        sfloat[0] = *reinterpret_cast<const GLfloat*>(src);
                        sfloat[1] = *reinterpret_cast<const GLfloat*>(src + group_size);
                    }

                    *dest = static_cast<GLfloat>((sfloat[0] + sfloat[1]) / 2.0);
                    src += element_size;
                    dest++;
                }
                src += group_size;
            }
        } else if (width == 1) { // 1 column
            int padBytes = ysize - (width * group_size);
            halfWidth = 1;

            for (jj = 0; jj < halfHeight; jj++) {
                int kk;
                for (kk = 0; kk < components; kk++) {
                    GLfloat sfloat[2];
                    if (myswap_bytes) {
                        sfloat[0] = static_cast<GLfloat>(GLU_SWAP_4_BYTES(src));
                        sfloat[1] = static_cast<GLfloat>(GLU_SWAP_4_BYTES(src + ysize));
                    } else {
                        sfloat[0] = *reinterpret_cast<const GLfloat*>(src);
                        sfloat[1] = *reinterpret_cast<const GLfloat*>(src + ysize);
                    }
                    *dest = static_cast<GLfloat>((sfloat[0] + sfloat[1]) / 2.0);

                    src += element_size;
                    dest++;
                }
                src += padBytes;
                src += ysize;
            }
        }
    }

    void halveImage_float(GLint components, GLuint width, GLuint height, const GLfloat* datain,
                          GLfloat* dataout, GLint element_size, GLint ysize, GLint group_size,
                          GLint myswap_bytes)
    {
        int i, j, k;
        int newwidth, newheight;
        GLfloat* s;
        const char* t;

        // handle case where there is only 1 column/row
        if (width == 1 || height == 1) {
            halve1Dimage_float(components, width, height, datain, dataout, element_size, ysize,
                               group_size, myswap_bytes);
            return;
        }

        newwidth = width / 2;
        newheight = height / 2;
        s = dataout;
        t = reinterpret_cast<const char*>(datain);

        if (!myswap_bytes)
            for (i = 0; i < newheight; i++) {
                for (j = 0; j < newwidth; j++) {
                    for (k = 0; k < components; k++) {
                        s[0] = (*reinterpret_cast<const GLfloat*>(t) + *reinterpret_cast<const GLfloat*>(t + group_size) +
                                *reinterpret_cast<const GLfloat*>(t + ysize) +
                                *reinterpret_cast<const GLfloat*>(t + ysize + group_size)) /
                               4;
                        s++;
                        t += element_size;
                    }
                    t += group_size;
                }
                t += ysize;
            }
        else
            for (i = 0; i < newheight; i++) {
                for (j = 0; j < newwidth; j++) {
                    for (k = 0; k < components; k++) {
                        union {
                            GLuint b;
                            GLfloat f;
                        } swapbuf;
                        swapbuf.b = GLU_SWAP_4_BYTES(t);
                        s[0] = swapbuf.f;
                        swapbuf.b = GLU_SWAP_4_BYTES(t + group_size);
                        s[0] += swapbuf.f;
                        swapbuf.b = GLU_SWAP_4_BYTES(t + ysize);
                        s[0] += swapbuf.f;
                        swapbuf.b = GLU_SWAP_4_BYTES(t + ysize + group_size);
                        s[0] += swapbuf.f;
                        s[0] /= 4;
                        s++;
                        t += element_size;
                    }
                    t += group_size;
                }
                t += ysize;
            }
    }

    __attribute__((noinline)) void scale_internal(GLint components, GLint widthin, GLint heightin,
                                                  const GLushort* datain, GLint widthout,
                                                  GLint heightout, GLushort* dataout)
    {
        float x, lowx, highx, convx, halfconvx;
        float y, lowy, highy, convy, halfconvy;
        float xpercent, ypercent;
        float percent;

        float totals[4];
        float area;
        int i, j, k, yint, xint, xindex, yindex;
        int temp;

        if (widthin == widthout * 2 && heightin == heightout * 2) {
            halveImage(components, widthin, heightin, datain, dataout);
            return;
        }
        convy = static_cast<float>(heightin) / static_cast<float>(heightout);
        convx = static_cast<float>(widthin) / static_cast<float>(widthout);
        halfconvx = convx / 2;
        halfconvy = convy / 2;
        for (i = 0; i < heightout; i++) {
            y = static_cast<float>(convy * (i + 0.5));
            if (heightin > heightout) {
                highy = y + halfconvy;
                lowy = y - halfconvy;
            } else {
                highy = static_cast<float>(y + 0.5);
                lowy = static_cast<float>(y - 0.5);
            }
            for (j = 0; j < widthout; j++) {
                x = static_cast<float>(convx * (j + 0.5));
                if (widthin > widthout) {
                    highx = x + halfconvx;
                    lowx = x - halfconvx;
                } else {
                    highx = static_cast<float>(x + 0.5);
                    lowx = static_cast<float>(x - 0.5);
                }

                // Box-filter the input from (lowx, lowy) to (highx, highy) into this output pixel.
                totals[0] = totals[1] = totals[2] = totals[3] = 0.0;
                area = 0.0;

                y = lowy;
                yint = static_cast<int>(floor(y));
                while (y < highy) {
                    yindex = (yint + heightin) % heightin;
                    if (highy < static_cast<float>(yint + 1)) {
                        ypercent = highy - y;
                    } else {
                        ypercent = static_cast<float>(yint + 1) - y;
                    }

                    x = lowx;
                    xint = static_cast<int>(floor(x));

                    while (x < highx) {
                        xindex = (xint + widthin) % widthin;
                        if (highx < static_cast<float>(xint + 1)) {
                            xpercent = highx - x;
                        } else {
                            xpercent = static_cast<float>(xint + 1) - x;
                        }

                        percent = xpercent * ypercent;
                        area += percent;
                        temp = (xindex + (yindex * widthin)) * components;
                        for (k = 0; k < components; k++) {
                            totals[k] += datain[temp + k] * percent;
                        }

                        xint++;
                        x = static_cast<float>(xint);
                    }
                    yint++;
                    y = static_cast<float>(yint);
                }

                temp = (j + (i * widthout)) * components;
                for (k = 0; k < components; k++) {
                    dataout[temp + k] = static_cast<GLushort>((totals[k] + 0.5) / area);
                }
            }
        }
    }

    // Unlike the other six types, the unsigned-byte filter runs its box ends one texel behind
    // (`convx_int - 1`, `convy_int - 1`, the row end clamped at zero, as is the column offset), so
    // the first box of each row and column is one texel narrower and every later one sits a texel
    // earlier.
    void scale_internal_ubyte(GLint components, GLint widthin, GLint heightin,
                              const GLubyte* datain, GLint widthout, GLint heightout,
                              GLubyte* dataout, GLint element_size, GLint ysize, GLint group_size)
    {
        float convx;
        float convy;
        float percent;

        float totals[4];
        float area;
        int i, j, k, xindex;

        const char *temp, *temp0;
        const char* temp_index;
        int outindex;

        int lowx_int, highx_int, lowy_int, highy_int;
        float x_percent, y_percent;
        float lowx_float, highx_float, lowy_float, highy_float;
        float convy_float, convx_float;
        int convy_int, convx_int;
        int l, m;
        const char *left, *right;

        if (widthin == widthout * 2 && heightin == heightout * 2) {
            halveImage_ubyte(components, widthin, heightin, static_cast<const GLubyte*>(datain),
                             static_cast<GLubyte*>(dataout), element_size, ysize, group_size);
            return;
        }
        convy = static_cast<float>(heightin) / static_cast<float>(heightout);
        convx = static_cast<float>(widthin) / static_cast<float>(widthout);
        convy_int = static_cast<int>(floor(convy));
        convy_float = convy - static_cast<float>(convy_int);
        convx_int = static_cast<int>(floor(convx));
        convx_float = convx - static_cast<float>(convx_int);

        area = convx * convy;

        lowy_int = 0;
        lowy_float = 0;
        highy_int = convy_int - 1;
        highy_float = convy_float;

        for (i = 0; i < heightout; i++) {
            if (highy_int < 0)
                highy_int = 0;
            lowx_int = 0;
            lowx_float = 0;
            highx_int = convx_int - 1;
            highx_float = convx_float;

            for (j = 0; j < widthout; j++) {

                // Box-filter the input from (lowx, lowy) to (highx, highy) into this output pixel.
                totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

                // calculate the value for pixels in the 1st row
                xindex = lowx_int * group_size;
                if (xindex < 0)
                    xindex = 0;
                if ((highy_int > lowy_int) && (highx_int > lowx_int)) {

                    y_percent = 1 - lowy_float;
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    percent = y_percent * (1 - lowx_float);
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLubyte>(*(temp_index)) * percent;
                    }
                    left = temp;
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            totals[k] += static_cast<GLubyte>(*(temp_index)) * y_percent;
                        }
                    }
                    temp += group_size;
                    right = temp;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLubyte>(*(temp_index)) * percent;
                    }

                    // calculate the value for pixels in the last row
                    y_percent = highy_float;
                    percent = y_percent * (1 - lowx_float);
                    temp = reinterpret_cast<const char*>(datain) + xindex + highy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLubyte>(*(temp_index)) * percent;
                    }
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            totals[k] += static_cast<GLubyte>(*(temp_index)) * y_percent;
                        }
                    }
                    temp += group_size;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLubyte>(*(temp_index)) * percent;
                    }

                    // calculate the value for pixels in the 1st and last column
                    for (m = lowy_int + 1; m < highy_int; m++) {
                        left += ysize;
                        right += ysize;
                        for (k = 0; k < components;
                             k++, left += element_size, right += element_size) {
                            totals[k] += static_cast<GLubyte>(*(left)) * (1 - lowx_float) +
                                         static_cast<GLubyte>(*(right)) * highx_float;
                        }
                    }
                } else if (highy_int > lowy_int) {
                    x_percent = highx_float - lowx_float;
                    percent = (1 - lowy_float) * x_percent;
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLubyte>(*(temp_index)) * percent;
                    }
                    for (m = lowy_int + 1; m < highy_int; m++) {
                        temp += ysize;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            totals[k] += static_cast<GLubyte>(*(temp_index)) * x_percent;
                        }
                    }
                    percent = x_percent * highy_float;
                    temp += ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLubyte>(*(temp_index)) * percent;
                    }
                } else if (highx_int > lowx_int) {
                    y_percent = highy_float - lowy_float;
                    percent = (1 - lowx_float) * y_percent;
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLubyte>(*(temp_index)) * percent;
                    }
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            totals[k] += static_cast<GLubyte>(*(temp_index)) * y_percent;
                        }
                    }
                    temp += group_size;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLubyte>(*(temp_index)) * percent;
                    }
                } else {
                    percent = (highy_float - lowy_float) * (highx_float - lowx_float);
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLubyte>(*(temp_index)) * percent;
                    }
                }

                // this is for the pixels in the body
                temp0 = reinterpret_cast<const char*>(datain) + xindex + group_size + (lowy_int + 1) * ysize;
                for (m = lowy_int + 1; m < highy_int; m++) {
                    temp = temp0;
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            totals[k] += static_cast<GLubyte>(*(temp_index));
                        }
                        temp += group_size;
                    }
                    temp0 += ysize;
                }

                outindex = (j + (i * widthout)) * components;
                for (k = 0; k < components; k++) {
                    dataout[outindex + k] = static_cast<GLubyte>(totals[k] / area);
                }
                lowx_int = highx_int;
                lowx_float = highx_float;
                highx_int += convx_int;
                highx_float += convx_float;
                if (highx_float > 1) {
                    highx_float -= static_cast<float>(1.0);
                    highx_int++;
                }
            }
            lowy_int = highy_int;
            lowy_float = highy_float;
            highy_int += convy_int;
            highy_float += convy_float;
            if (highy_float > 1) {
                highy_float -= static_cast<float>(1.0);
                highy_int++;
            }
        }
    }

    void scale_internal_byte(GLint components, GLint widthin, GLint heightin, const GLbyte* datain,
                             GLint widthout, GLint heightout, GLbyte* dataout, GLint element_size,
                             GLint ysize, GLint group_size)
    {
        float convx;
        float convy;
        float percent;

        float totals[4];
        float area;
        int i, j, k, xindex;

        const char *temp, *temp0;
        const char* temp_index;
        int outindex;

        int lowx_int, highx_int, lowy_int, highy_int;
        float x_percent, y_percent;
        float lowx_float, highx_float, lowy_float, highy_float;
        float convy_float, convx_float;
        int convy_int, convx_int;
        int l, m;
        const char *left, *right;

        if (widthin == widthout * 2 && heightin == heightout * 2) {
            halveImage_byte(components, widthin, heightin, static_cast<const GLbyte*>(datain), static_cast<GLbyte*>(dataout),
                            element_size, ysize, group_size);
            return;
        }
        convy = static_cast<float>(heightin) / static_cast<float>(heightout);
        convx = static_cast<float>(widthin) / static_cast<float>(widthout);
        convy_int = static_cast<int>(floor(convy));
        convy_float = convy - static_cast<float>(convy_int);
        convx_int = static_cast<int>(floor(convx));
        convx_float = convx - static_cast<float>(convx_int);

        area = convx * convy;

        lowy_int = 0;
        lowy_float = 0;
        highy_int = convy_int;
        highy_float = convy_float;

        for (i = 0; i < heightout; i++) {
            lowx_int = 0;
            lowx_float = 0;
            highx_int = convx_int;
            highx_float = convx_float;

            for (j = 0; j < widthout; j++) {

                // Box-filter the input from (lowx, lowy) to (highx, highy) into this output pixel.
                totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

                // calculate the value for pixels in the 1st row
                xindex = lowx_int * group_size;
                if ((highy_int > lowy_int) && (highx_int > lowx_int)) {

                    y_percent = 1 - lowy_float;
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    percent = y_percent * (1 - lowx_float);
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLbyte>(*(temp_index)) * percent;
                    }
                    left = temp;
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            totals[k] += static_cast<GLbyte>(*(temp_index)) * y_percent;
                        }
                    }
                    temp += group_size;
                    right = temp;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLbyte>(*(temp_index)) * percent;
                    }

                    // calculate the value for pixels in the last row
                    y_percent = highy_float;
                    percent = y_percent * (1 - lowx_float);
                    temp = reinterpret_cast<const char*>(datain) + xindex + highy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLbyte>(*(temp_index)) * percent;
                    }
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            totals[k] += static_cast<GLbyte>(*(temp_index)) * y_percent;
                        }
                    }
                    temp += group_size;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLbyte>(*(temp_index)) * percent;
                    }

                    // calculate the value for pixels in the 1st and last column
                    for (m = lowy_int + 1; m < highy_int; m++) {
                        left += ysize;
                        right += ysize;
                        for (k = 0; k < components;
                             k++, left += element_size, right += element_size) {
                            totals[k] += static_cast<GLbyte>(*(left)) * (1 - lowx_float) +
                                         static_cast<GLbyte>(*(right)) * highx_float;
                        }
                    }
                } else if (highy_int > lowy_int) {
                    x_percent = highx_float - lowx_float;
                    percent = (1 - lowy_float) * x_percent;
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLbyte>(*(temp_index)) * percent;
                    }
                    for (m = lowy_int + 1; m < highy_int; m++) {
                        temp += ysize;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            totals[k] += static_cast<GLbyte>(*(temp_index)) * x_percent;
                        }
                    }
                    percent = x_percent * highy_float;
                    temp += ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLbyte>(*(temp_index)) * percent;
                    }
                } else if (highx_int > lowx_int) {
                    y_percent = highy_float - lowy_float;
                    percent = (1 - lowx_float) * y_percent;
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLbyte>(*(temp_index)) * percent;
                    }
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            totals[k] += static_cast<GLbyte>(*(temp_index)) * y_percent;
                        }
                    }
                    temp += group_size;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLbyte>(*(temp_index)) * percent;
                    }
                } else {
                    percent = (highy_float - lowy_float) * (highx_float - lowx_float);
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        totals[k] += static_cast<GLbyte>(*(temp_index)) * percent;
                    }
                }

                // this is for the pixels in the body
                temp0 = reinterpret_cast<const char*>(datain) + xindex + group_size + (lowy_int + 1) * ysize;
                for (m = lowy_int + 1; m < highy_int; m++) {
                    temp = temp0;
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            totals[k] += static_cast<GLbyte>(*(temp_index));
                        }
                        temp += group_size;
                    }
                    temp0 += ysize;
                }

                outindex = (j + (i * widthout)) * components;
                for (k = 0; k < components; k++) {
                    dataout[outindex + k] = static_cast<GLbyte>(totals[k] / area);
                }
                lowx_int = highx_int;
                lowx_float = highx_float;
                highx_int += convx_int;
                highx_float += convx_float;
                if (highx_float > 1) {
                    highx_float -= static_cast<float>(1.0);
                    highx_int++;
                }
            }
            lowy_int = highy_int;
            lowy_float = highy_float;
            highy_int += convy_int;
            highy_float += convy_float;
            if (highy_float > 1) {
                highy_float -= static_cast<float>(1.0);
                highy_int++;
            }
        }
    }

    void scale_internal_ushort(GLint components, GLint widthin, GLint heightin,
                               const GLushort* datain, GLint widthout, GLint heightout,
                               GLushort* dataout, GLint element_size, GLint ysize, GLint group_size,
                               GLint myswap_bytes)
    {
        float convx;
        float convy;
        float percent;

        float totals[4];
        float area;
        int i, j, k, xindex;

        const char *temp, *temp0;
        const char* temp_index;
        int outindex;

        int lowx_int, highx_int, lowy_int, highy_int;
        float x_percent, y_percent;
        float lowx_float, highx_float, lowy_float, highy_float;
        float convy_float, convx_float;
        int convy_int, convx_int;
        int l, m;
        const char *left, *right;

        if (widthin == widthout * 2 && heightin == heightout * 2) {
            halveImage_ushort(components, widthin, heightin, static_cast<const GLushort*>(datain),
                              static_cast<GLushort*>(dataout), element_size, ysize, group_size, myswap_bytes);
            return;
        }
        convy = static_cast<float>(heightin) / static_cast<float>(heightout);
        convx = static_cast<float>(widthin) / static_cast<float>(widthout);
        convy_int = static_cast<int>(floor(convy));
        convy_float = convy - static_cast<float>(convy_int);
        convx_int = static_cast<int>(floor(convx));
        convx_float = convx - static_cast<float>(convx_int);

        area = convx * convy;

        lowy_int = 0;
        lowy_float = 0;
        highy_int = convy_int;
        highy_float = convy_float;

        for (i = 0; i < heightout; i++) {
            lowx_int = 0;
            lowx_float = 0;
            highx_int = convx_int;
            highx_float = convx_float;

            for (j = 0; j < widthout; j++) {
                // Box-filter the input from (lowx, lowy) to (highx, highy) into this output pixel.
                totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

                // calculate the value for pixels in the 1st row
                xindex = lowx_int * group_size;
                if ((highy_int > lowy_int) && (highx_int > lowx_int)) {

                    y_percent = 1 - lowy_float;
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    percent = y_percent * (1 - lowx_float);
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += GLU_SWAP_2_BYTES(temp_index) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLushort*>(temp_index) * percent;
                        }
                    }
                    left = temp;
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                totals[k] += GLU_SWAP_2_BYTES(temp_index) * y_percent;
                            } else {
                                totals[k] += *reinterpret_cast<const GLushort*>(temp_index) * y_percent;
                            }
                        }
                    }
                    temp += group_size;
                    right = temp;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += GLU_SWAP_2_BYTES(temp_index) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLushort*>(temp_index) * percent;
                        }
                    }

                    // calculate the value for pixels in the last row
                    y_percent = highy_float;
                    percent = y_percent * (1 - lowx_float);
                    temp = reinterpret_cast<const char*>(datain) + xindex + highy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += GLU_SWAP_2_BYTES(temp_index) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLushort*>(temp_index) * percent;
                        }
                    }
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                totals[k] += GLU_SWAP_2_BYTES(temp_index) * y_percent;
                            } else {
                                totals[k] += *reinterpret_cast<const GLushort*>(temp_index) * y_percent;
                            }
                        }
                    }
                    temp += group_size;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += GLU_SWAP_2_BYTES(temp_index) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLushort*>(temp_index) * percent;
                        }
                    }

                    // calculate the value for pixels in the 1st and last column
                    for (m = lowy_int + 1; m < highy_int; m++) {
                        left += ysize;
                        right += ysize;
                        for (k = 0; k < components;
                             k++, left += element_size, right += element_size) {
                            if (myswap_bytes) {
                                totals[k] += GLU_SWAP_2_BYTES(left) * (1 - lowx_float) +
                                             GLU_SWAP_2_BYTES(right) * highx_float;
                            } else {
                                totals[k] += *reinterpret_cast<const GLushort*>(left) * (1 - lowx_float) +
                                             *reinterpret_cast<const GLushort*>(right) * highx_float;
                            }
                        }
                    }
                } else if (highy_int > lowy_int) {
                    x_percent = highx_float - lowx_float;
                    percent = (1 - lowy_float) * x_percent;
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += GLU_SWAP_2_BYTES(temp_index) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLushort*>(temp_index) * percent;
                        }
                    }
                    for (m = lowy_int + 1; m < highy_int; m++) {
                        temp += ysize;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                totals[k] += GLU_SWAP_2_BYTES(temp_index) * x_percent;
                            } else {
                                totals[k] += *reinterpret_cast<const GLushort*>(temp_index) * x_percent;
                            }
                        }
                    }
                    percent = x_percent * highy_float;
                    temp += ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += GLU_SWAP_2_BYTES(temp_index) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLushort*>(temp_index) * percent;
                        }
                    }
                } else if (highx_int > lowx_int) {
                    y_percent = highy_float - lowy_float;
                    percent = (1 - lowx_float) * y_percent;
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += GLU_SWAP_2_BYTES(temp_index) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLushort*>(temp_index) * percent;
                        }
                    }
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                totals[k] += GLU_SWAP_2_BYTES(temp_index) * y_percent;
                            } else {
                                totals[k] += *reinterpret_cast<const GLushort*>(temp_index) * y_percent;
                            }
                        }
                    }
                    temp += group_size;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += GLU_SWAP_2_BYTES(temp_index) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLushort*>(temp_index) * percent;
                        }
                    }
                } else {
                    percent = (highy_float - lowy_float) * (highx_float - lowx_float);
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += GLU_SWAP_2_BYTES(temp_index) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLushort*>(temp_index) * percent;
                        }
                    }
                }

                // this is for the pixels in the body
                temp0 = reinterpret_cast<const char*>(datain) + xindex + group_size + (lowy_int + 1) * ysize;
                for (m = lowy_int + 1; m < highy_int; m++) {
                    temp = temp0;
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                totals[k] += GLU_SWAP_2_BYTES(temp_index);
                            } else {
                                totals[k] += *reinterpret_cast<const GLushort*>(temp_index);
                            }
                        }
                        temp += group_size;
                    }
                    temp0 += ysize;
                }

                outindex = (j + (i * widthout)) * components;
                for (k = 0; k < components; k++) {
                    dataout[outindex + k] = static_cast<GLushort>(totals[k] / area);
                }
                lowx_int = highx_int;
                lowx_float = highx_float;
                highx_int += convx_int;
                highx_float += convx_float;
                if (highx_float > 1) {
                    highx_float -= static_cast<float>(1.0);
                    highx_int++;
                }
            }
            lowy_int = highy_int;
            lowy_float = highy_float;
            highy_int += convy_int;
            highy_float += convy_float;
            if (highy_float > 1) {
                highy_float -= static_cast<float>(1.0);
                highy_int++;
            }
        }
    }

    void scale_internal_short(GLint components, GLint widthin, GLint heightin,
                              const GLshort* datain, GLint widthout, GLint heightout,
                              GLshort* dataout, GLint element_size, GLint ysize, GLint group_size,
                              GLint myswap_bytes)
    {
        float convx;
        float convy;
        float percent;

        float totals[4];
        float area;
        int i, j, k, xindex;

        const char *temp, *temp0;
        const char* temp_index;
        int outindex;

        int lowx_int, highx_int, lowy_int, highy_int;
        float x_percent, y_percent;
        float lowx_float, highx_float, lowy_float, highy_float;
        float convy_float, convx_float;
        int convy_int, convx_int;
        int l, m;
        const char *left, *right;

        GLushort swapbuf;

        if (widthin == widthout * 2 && heightin == heightout * 2) {
            halveImage_short(components, widthin, heightin, static_cast<const GLshort*>(datain),
                             static_cast<GLshort*>(dataout), element_size, ysize, group_size, myswap_bytes);
            return;
        }
        convy = static_cast<float>(heightin) / static_cast<float>(heightout);
        convx = static_cast<float>(widthin) / static_cast<float>(widthout);
        convy_int = static_cast<int>(floor(convy));
        convy_float = convy - static_cast<float>(convy_int);
        convx_int = static_cast<int>(floor(convx));
        convx_float = convx - static_cast<float>(convx_int);

        area = convx * convy;

        lowy_int = 0;
        lowy_float = 0;
        highy_int = convy_int;
        highy_float = convy_float;

        for (i = 0; i < heightout; i++) {
            lowx_int = 0;
            lowx_float = 0;
            highx_int = convx_int;
            highx_float = convx_float;

            for (j = 0; j < widthout; j++) {
                // Box-filter the input from (lowx, lowy) to (highx, highy) into this output pixel.
                totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

                // calculate the value for pixels in the 1st row
                xindex = lowx_int * group_size;
                if ((highy_int > lowy_int) && (highx_int > lowx_int)) {

                    y_percent = 1 - lowy_float;
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    percent = y_percent * (1 - lowx_float);
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_2_BYTES(temp_index);
                            totals[k] += *reinterpret_cast<const GLshort*>(&swapbuf) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLshort*>(temp_index) * percent;
                        }
                    }
                    left = temp;
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                swapbuf = GLU_SWAP_2_BYTES(temp_index);
                                totals[k] += *reinterpret_cast<const GLshort*>(&swapbuf) * y_percent;
                            } else {
                                totals[k] += *reinterpret_cast<const GLshort*>(temp_index) * y_percent;
                            }
                        }
                    }
                    temp += group_size;
                    right = temp;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_2_BYTES(temp_index);
                            totals[k] += *reinterpret_cast<const GLshort*>(&swapbuf) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLshort*>(temp_index) * percent;
                        }
                    }

                    // calculate the value for pixels in the last row
                    y_percent = highy_float;
                    percent = y_percent * (1 - lowx_float);
                    temp = reinterpret_cast<const char*>(datain) + xindex + highy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_2_BYTES(temp_index);
                            totals[k] += *reinterpret_cast<const GLshort*>(&swapbuf) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLshort*>(temp_index) * percent;
                        }
                    }
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                swapbuf = GLU_SWAP_2_BYTES(temp_index);
                                totals[k] += *reinterpret_cast<const GLshort*>(&swapbuf) * y_percent;
                            } else {
                                totals[k] += *reinterpret_cast<const GLshort*>(temp_index) * y_percent;
                            }
                        }
                    }
                    temp += group_size;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_2_BYTES(temp_index);
                            totals[k] += *reinterpret_cast<const GLshort*>(&swapbuf) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLshort*>(temp_index) * percent;
                        }
                    }

                    // calculate the value for pixels in the 1st and last column
                    for (m = lowy_int + 1; m < highy_int; m++) {
                        left += ysize;
                        right += ysize;
                        for (k = 0; k < components;
                             k++, left += element_size, right += element_size) {
                            if (myswap_bytes) {
                                swapbuf = GLU_SWAP_2_BYTES(left);
                                totals[k] += *reinterpret_cast<const GLshort*>(&swapbuf) * (1 - lowx_float);
                                swapbuf = GLU_SWAP_2_BYTES(right);
                                totals[k] += *reinterpret_cast<const GLshort*>(&swapbuf) * highx_float;
                            } else {
                                totals[k] += *reinterpret_cast<const GLshort*>(left) * (1 - lowx_float) +
                                             *reinterpret_cast<const GLshort*>(right) * highx_float;
                            }
                        }
                    }
                } else if (highy_int > lowy_int) {
                    x_percent = highx_float - lowx_float;
                    percent = (1 - lowy_float) * x_percent;
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_2_BYTES(temp_index);
                            totals[k] += *reinterpret_cast<const GLshort*>(&swapbuf) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLshort*>(temp_index) * percent;
                        }
                    }
                    for (m = lowy_int + 1; m < highy_int; m++) {
                        temp += ysize;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                swapbuf = GLU_SWAP_2_BYTES(temp_index);
                                totals[k] += *reinterpret_cast<const GLshort*>(&swapbuf) * x_percent;
                            } else {
                                totals[k] += *reinterpret_cast<const GLshort*>(temp_index) * x_percent;
                            }
                        }
                    }
                    percent = x_percent * highy_float;
                    temp += ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_2_BYTES(temp_index);
                            totals[k] += *reinterpret_cast<const GLshort*>(&swapbuf) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLshort*>(temp_index) * percent;
                        }
                    }
                } else if (highx_int > lowx_int) {
                    y_percent = highy_float - lowy_float;
                    percent = (1 - lowx_float) * y_percent;

                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_2_BYTES(temp_index);
                            totals[k] += *reinterpret_cast<const GLshort*>(&swapbuf) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLshort*>(temp_index) * percent;
                        }
                    }
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                swapbuf = GLU_SWAP_2_BYTES(temp_index);
                                totals[k] += *reinterpret_cast<const GLshort*>(&swapbuf) * y_percent;
                            } else {
                                totals[k] += *reinterpret_cast<const GLshort*>(temp_index) * y_percent;
                            }
                        }
                    }
                    temp += group_size;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_2_BYTES(temp_index);
                            totals[k] += *reinterpret_cast<const GLshort*>(&swapbuf) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLshort*>(temp_index) * percent;
                        }
                    }
                } else {
                    percent = (highy_float - lowy_float) * (highx_float - lowx_float);
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_2_BYTES(temp_index);
                            totals[k] += *reinterpret_cast<const GLshort*>(&swapbuf) * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLshort*>(temp_index) * percent;
                        }
                    }
                }

                // this is for the pixels in the body
                temp0 = reinterpret_cast<const char*>(datain) + xindex + group_size + (lowy_int + 1) * ysize;
                for (m = lowy_int + 1; m < highy_int; m++) {
                    temp = temp0;
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                swapbuf = GLU_SWAP_2_BYTES(temp_index);
                                totals[k] += *reinterpret_cast<const GLshort*>(&swapbuf);
                            } else {
                                totals[k] += *reinterpret_cast<const GLshort*>(temp_index);
                            }
                        }
                        temp += group_size;
                    }
                    temp0 += ysize;
                }

                outindex = (j + (i * widthout)) * components;
                for (k = 0; k < components; k++) {
                    dataout[outindex + k] = static_cast<GLshort>(totals[k] / area);
                }
                lowx_int = highx_int;
                lowx_float = highx_float;
                highx_int += convx_int;
                highx_float += convx_float;
                if (highx_float > 1) {
                    highx_float -= static_cast<float>(1.0);
                    highx_int++;
                }
            }
            lowy_int = highy_int;
            lowy_float = highy_float;
            highy_int += convy_int;
            highy_float += convy_float;
            if (highy_float > 1) {
                highy_float -= static_cast<float>(1.0);
                highy_int++;
            }
        }
    }

    void scale_internal_uint(GLint components, GLint widthin, GLint heightin, const GLuint* datain,
                             GLint widthout, GLint heightout, GLuint* dataout, GLint element_size,
                             GLint ysize, GLint group_size, GLint myswap_bytes)
    {
        float convx;
        float convy;
        float percent;

        float totals[4];
        float area;
        int i, j, k, xindex;

        const char *temp, *temp0;
        const char* temp_index;
        int outindex;

        int lowx_int, highx_int, lowy_int, highy_int;
        float x_percent, y_percent;
        float lowx_float, highx_float, lowy_float, highy_float;
        float convy_float, convx_float;
        int convy_int, convx_int;
        int l, m;
        const char *left, *right;

        if (widthin == widthout * 2 && heightin == heightout * 2) {
            halveImage_uint(components, widthin, heightin, static_cast<const GLuint*>(datain), static_cast<GLuint*>(dataout),
                            element_size, ysize, group_size, myswap_bytes);
            return;
        }
        convy = static_cast<float>(heightin) / static_cast<float>(heightout);
        convx = static_cast<float>(widthin) / static_cast<float>(widthout);
        convy_int = static_cast<int>(floor(convy));
        convy_float = convy - static_cast<float>(convy_int);
        convx_int = static_cast<int>(floor(convx));
        convx_float = convx - static_cast<float>(convx_int);

        area = convx * convy;

        lowy_int = 0;
        lowy_float = 0;
        highy_int = convy_int;
        highy_float = convy_float;

        for (i = 0; i < heightout; i++) {
            lowx_int = 0;
            lowx_float = 0;
            highx_int = convx_int;
            highx_float = convx_float;

            for (j = 0; j < widthout; j++) {
                // Box-filter the input from (lowx, lowy) to (highx, highy) into this output pixel.
                totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

                // calculate the value for pixels in the 1st row
                xindex = lowx_int * group_size;
                if ((highy_int > lowy_int) && (highx_int > lowx_int)) {

                    y_percent = 1 - lowy_float;
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    percent = y_percent * (1 - lowx_float);
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += static_cast<float>(GLU_SWAP_4_BYTES(temp_index)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLuint*>(temp_index)) * percent;
                        }
                    }
                    left = temp;
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                totals[k] += static_cast<float>(GLU_SWAP_4_BYTES(temp_index)) * y_percent;
                            } else {
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLuint*>(temp_index)) * y_percent;
                            }
                        }
                    }
                    temp += group_size;
                    right = temp;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += static_cast<float>(GLU_SWAP_4_BYTES(temp_index)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLuint*>(temp_index)) * percent;
                        }
                    }

                    // calculate the value for pixels in the last row
                    y_percent = highy_float;
                    percent = y_percent * (1 - lowx_float);
                    temp = reinterpret_cast<const char*>(datain) + xindex + highy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += static_cast<float>(GLU_SWAP_4_BYTES(temp_index)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLuint*>(temp_index)) * percent;
                        }
                    }
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                totals[k] += static_cast<float>(GLU_SWAP_4_BYTES(temp_index)) * y_percent;
                            } else {
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLuint*>(temp_index)) * y_percent;
                            }
                        }
                    }
                    temp += group_size;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += static_cast<float>(GLU_SWAP_4_BYTES(temp_index)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLuint*>(temp_index)) * percent;
                        }
                    }

                    // calculate the value for pixels in the 1st and last column
                    for (m = lowy_int + 1; m < highy_int; m++) {
                        left += ysize;
                        right += ysize;
                        for (k = 0; k < components;
                             k++, left += element_size, right += element_size) {
                            if (myswap_bytes) {
                                totals[k] += static_cast<float>(GLU_SWAP_4_BYTES(left)) * (1 - lowx_float) +
                                             static_cast<float>(GLU_SWAP_4_BYTES(right)) * highx_float;
                            } else {
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLuint*>(left)) * (1 - lowx_float) +
                                             static_cast<float>(*reinterpret_cast<const GLuint*>(right)) * highx_float;
                            }
                        }
                    }
                } else if (highy_int > lowy_int) {
                    x_percent = highx_float - lowx_float;
                    percent = (1 - lowy_float) * x_percent;
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += static_cast<float>(GLU_SWAP_4_BYTES(temp_index)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLuint*>(temp_index)) * percent;
                        }
                    }
                    for (m = lowy_int + 1; m < highy_int; m++) {
                        temp += ysize;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                totals[k] += static_cast<float>(GLU_SWAP_4_BYTES(temp_index)) * x_percent;
                            } else {
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLuint*>(temp_index)) * x_percent;
                            }
                        }
                    }
                    percent = x_percent * highy_float;
                    temp += ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += static_cast<float>(GLU_SWAP_4_BYTES(temp_index)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLuint*>(temp_index)) * percent;
                        }
                    }
                } else if (highx_int > lowx_int) {
                    y_percent = highy_float - lowy_float;
                    percent = (1 - lowx_float) * y_percent;

                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += static_cast<float>(GLU_SWAP_4_BYTES(temp_index)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLuint*>(temp_index)) * percent;
                        }
                    }
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                totals[k] += static_cast<float>(GLU_SWAP_4_BYTES(temp_index)) * y_percent;
                            } else {
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLuint*>(temp_index)) * y_percent;
                            }
                        }
                    }
                    temp += group_size;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += static_cast<float>(GLU_SWAP_4_BYTES(temp_index)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLuint*>(temp_index)) * percent;
                        }
                    }
                } else {
                    percent = (highy_float - lowy_float) * (highx_float - lowx_float);
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            totals[k] += static_cast<float>(GLU_SWAP_4_BYTES(temp_index)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLuint*>(temp_index)) * percent;
                        }
                    }
                }

                // this is for the pixels in the body
                temp0 = reinterpret_cast<const char*>(datain) + xindex + group_size + (lowy_int + 1) * ysize;
                for (m = lowy_int + 1; m < highy_int; m++) {
                    temp = temp0;
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                totals[k] += static_cast<float>(GLU_SWAP_4_BYTES(temp_index));
                            } else {
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLuint*>(temp_index));
                            }
                        }
                        temp += group_size;
                    }
                    temp0 += ysize;
                }

                outindex = (j + (i * widthout)) * components;
                for (k = 0; k < components; k++) {
                    // clamp at UINT_MAX
                    float value = totals[k] / area;
                    if (value >= static_cast<float>(UINT_MAX)) {
                        dataout[outindex + k] = UINT_MAX;
                    } else
                        dataout[outindex + k] = static_cast<GLuint>(value);
                }
                lowx_int = highx_int;
                lowx_float = highx_float;
                highx_int += convx_int;
                highx_float += convx_float;
                if (highx_float > 1) {
                    highx_float -= static_cast<float>(1.0);
                    highx_int++;
                }
            }
            lowy_int = highy_int;
            lowy_float = highy_float;
            highy_int += convy_int;
            highy_float += convy_float;
            if (highy_float > 1) {
                highy_float -= static_cast<float>(1.0);
                highy_int++;
            }
        }
    }

    void scale_internal_int(GLint components, GLint widthin, GLint heightin, const GLint* datain,
                            GLint widthout, GLint heightout, GLint* dataout, GLint element_size,
                            GLint ysize, GLint group_size, GLint myswap_bytes)
    {
        float convx;
        float convy;
        float percent;

        float totals[4];
        float area;
        int i, j, k, xindex;

        const char *temp, *temp0;
        const char* temp_index;
        int outindex;

        int lowx_int, highx_int, lowy_int, highy_int;
        float x_percent, y_percent;
        float lowx_float, highx_float, lowy_float, highy_float;
        float convy_float, convx_float;
        int convy_int, convx_int;
        int l, m;
        const char *left, *right;

        GLuint swapbuf;

        if (widthin == widthout * 2 && heightin == heightout * 2) {
            halveImage_int(components, widthin, heightin, static_cast<const GLint*>(datain), static_cast<GLint*>(dataout),
                           element_size, ysize, group_size, myswap_bytes);
            return;
        }
        convy = static_cast<float>(heightin) / static_cast<float>(heightout);
        convx = static_cast<float>(widthin) / static_cast<float>(widthout);
        convy_int = static_cast<int>(floor(convy));
        convy_float = convy - static_cast<float>(convy_int);
        convx_int = static_cast<int>(floor(convx));
        convx_float = convx - static_cast<float>(convx_int);

        area = convx * convy;

        lowy_int = 0;
        lowy_float = 0;
        highy_int = convy_int;
        highy_float = convy_float;

        for (i = 0; i < heightout; i++) {
            lowx_int = 0;
            lowx_float = 0;
            highx_int = convx_int;
            highx_float = convx_float;

            for (j = 0; j < widthout; j++) {
                // Box-filter the input from (lowx, lowy) to (highx, highy) into this output pixel.
                totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

                // calculate the value for pixels in the 1st row
                xindex = lowx_int * group_size;
                if ((highy_int > lowy_int) && (highx_int > lowx_int)) {

                    y_percent = 1 - lowy_float;
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    percent = y_percent * (1 - lowx_float);
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(&swapbuf)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(temp_index)) * percent;
                        }
                    }
                    left = temp;
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                swapbuf = GLU_SWAP_4_BYTES(temp_index);
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(&swapbuf)) * y_percent;
                            } else {
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(temp_index)) * y_percent;
                            }
                        }
                    }
                    temp += group_size;
                    right = temp;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(&swapbuf)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(temp_index)) * percent;
                        }
                    }

                    // calculate the value for pixels in the last row
                    y_percent = highy_float;
                    percent = y_percent * (1 - lowx_float);
                    temp = reinterpret_cast<const char*>(datain) + xindex + highy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(&swapbuf)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(temp_index)) * percent;
                        }
                    }
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                swapbuf = GLU_SWAP_4_BYTES(temp_index);
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(&swapbuf)) * y_percent;
                            } else {
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(temp_index)) * y_percent;
                            }
                        }
                    }
                    temp += group_size;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(&swapbuf)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(temp_index)) * percent;
                        }
                    }

                    // calculate the value for pixels in the 1st and last column
                    for (m = lowy_int + 1; m < highy_int; m++) {
                        left += ysize;
                        right += ysize;
                        for (k = 0; k < components;
                             k++, left += element_size, right += element_size) {
                            if (myswap_bytes) {
                                swapbuf = GLU_SWAP_4_BYTES(left);
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(&swapbuf)) * (1 - lowx_float);
                                swapbuf = GLU_SWAP_4_BYTES(right);
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(&swapbuf)) * highx_float;
                            } else {
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(left)) * (1 - lowx_float) +
                                             static_cast<float>(*reinterpret_cast<const GLint*>(right)) * highx_float;
                            }
                        }
                    }
                } else if (highy_int > lowy_int) {
                    x_percent = highx_float - lowx_float;
                    percent = (1 - lowy_float) * x_percent;
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(&swapbuf)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(temp_index)) * percent;
                        }
                    }
                    for (m = lowy_int + 1; m < highy_int; m++) {
                        temp += ysize;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                swapbuf = GLU_SWAP_4_BYTES(temp_index);
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(&swapbuf)) * x_percent;
                            } else {
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(temp_index)) * x_percent;
                            }
                        }
                    }
                    percent = x_percent * highy_float;
                    temp += ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(&swapbuf)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(temp_index)) * percent;
                        }
                    }
                } else if (highx_int > lowx_int) {
                    y_percent = highy_float - lowy_float;
                    percent = (1 - lowx_float) * y_percent;

                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(&swapbuf)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(temp_index)) * percent;
                        }
                    }
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                swapbuf = GLU_SWAP_4_BYTES(temp_index);
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(&swapbuf)) * y_percent;
                            } else {
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(temp_index)) * y_percent;
                            }
                        }
                    }
                    temp += group_size;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(&swapbuf)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(temp_index)) * percent;
                        }
                    }
                } else {
                    percent = (highy_float - lowy_float) * (highx_float - lowx_float);
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(&swapbuf)) * percent;
                        } else {
                            totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(temp_index)) * percent;
                        }
                    }
                }

                // this is for the pixels in the body
                temp0 = reinterpret_cast<const char*>(datain) + xindex + group_size + (lowy_int + 1) * ysize;
                for (m = lowy_int + 1; m < highy_int; m++) {
                    temp = temp0;
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                swapbuf = GLU_SWAP_4_BYTES(temp_index);
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(&swapbuf));
                            } else {
                                totals[k] += static_cast<float>(*reinterpret_cast<const GLint*>(temp_index));
                            }
                        }
                        temp += group_size;
                    }
                    temp0 += ysize;
                }

                outindex = (j + (i * widthout)) * components;
                for (k = 0; k < components; k++) {
                    dataout[outindex + k] = static_cast<GLint>(totals[k] / area);
                }
                lowx_int = highx_int;
                lowx_float = highx_float;
                highx_int += convx_int;
                highx_float += convx_float;
                if (highx_float > 1) {
                    highx_float -= static_cast<float>(1.0);
                    highx_int++;
                }
            }
            lowy_int = highy_int;
            lowy_float = highy_float;
            highy_int += convy_int;
            highy_float += convy_float;
            if (highy_float > 1) {
                highy_float -= static_cast<float>(1.0);
                highy_int++;
            }
        }
    }

    void scale_internal_float(GLint components, GLint widthin, GLint heightin,
                              const GLfloat* datain, GLint widthout, GLint heightout,
                              GLfloat* dataout, GLint element_size, GLint ysize, GLint group_size,
                              GLint myswap_bytes)
    {
        float convx;
        float convy;
        float percent;

        float totals[4];
        float area;
        int i, j, k, xindex;

        const char *temp, *temp0;
        const char* temp_index;
        int outindex;

        int lowx_int, highx_int, lowy_int, highy_int;
        float x_percent, y_percent;
        float lowx_float, highx_float, lowy_float, highy_float;
        float convy_float, convx_float;
        int convy_int, convx_int;
        int l, m;
        const char *left, *right;

        union {
            GLuint b;
            GLfloat f;
        } swapbuf;

        if (widthin == widthout * 2 && heightin == heightout * 2) {
            halveImage_float(components, widthin, heightin, static_cast<const GLfloat*>(datain),
                             static_cast<GLfloat*>(dataout), element_size, ysize, group_size, myswap_bytes);
            return;
        }
        convy = static_cast<float>(heightin) / static_cast<float>(heightout);
        convx = static_cast<float>(widthin) / static_cast<float>(widthout);
        convy_int = static_cast<int>(floor(convy));
        convy_float = convy - static_cast<float>(convy_int);
        convx_int = static_cast<int>(floor(convx));
        convx_float = convx - static_cast<float>(convx_int);

        area = convx * convy;

        lowy_int = 0;
        lowy_float = 0;
        highy_int = convy_int;
        highy_float = convy_float;

        for (i = 0; i < heightout; i++) {
            lowx_int = 0;
            lowx_float = 0;
            highx_int = convx_int;
            highx_float = convx_float;

            for (j = 0; j < widthout; j++) {
                // Box-filter the input from (lowx, lowy) to (highx, highy) into this output pixel.
                totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

                // calculate the value for pixels in the 1st row
                xindex = lowx_int * group_size;
                if ((highy_int > lowy_int) && (highx_int > lowx_int)) {

                    y_percent = 1 - lowy_float;
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    percent = y_percent * (1 - lowx_float);
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf.b = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += swapbuf.f * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLfloat*>(temp_index) * percent;
                        }
                    }
                    left = temp;
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                swapbuf.b = GLU_SWAP_4_BYTES(temp_index);
                                totals[k] += swapbuf.f * y_percent;
                            } else {
                                totals[k] += *reinterpret_cast<const GLfloat*>(temp_index) * y_percent;
                            }
                        }
                    }
                    temp += group_size;
                    right = temp;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf.b = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += swapbuf.f * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLfloat*>(temp_index) * percent;
                        }
                    }

                    // calculate the value for pixels in the last row
                    y_percent = highy_float;
                    percent = y_percent * (1 - lowx_float);
                    temp = reinterpret_cast<const char*>(datain) + xindex + highy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf.b = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += swapbuf.f * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLfloat*>(temp_index) * percent;
                        }
                    }
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                swapbuf.b = GLU_SWAP_4_BYTES(temp_index);
                                totals[k] += swapbuf.f * y_percent;
                            } else {
                                totals[k] += *reinterpret_cast<const GLfloat*>(temp_index) * y_percent;
                            }
                        }
                    }
                    temp += group_size;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf.b = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += swapbuf.f * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLfloat*>(temp_index) * percent;
                        }
                    }

                    // calculate the value for pixels in the 1st and last column
                    for (m = lowy_int + 1; m < highy_int; m++) {
                        left += ysize;
                        right += ysize;
                        for (k = 0; k < components;
                             k++, left += element_size, right += element_size) {
                            if (myswap_bytes) {
                                swapbuf.b = GLU_SWAP_4_BYTES(left);
                                totals[k] += swapbuf.f * (1 - lowx_float);
                                swapbuf.b = GLU_SWAP_4_BYTES(right);
                                totals[k] += swapbuf.f * highx_float;
                            } else {
                                totals[k] += *reinterpret_cast<const GLfloat*>(left) * (1 - lowx_float) +
                                             *reinterpret_cast<const GLfloat*>(right) * highx_float;
                            }
                        }
                    }
                } else if (highy_int > lowy_int) {
                    x_percent = highx_float - lowx_float;
                    percent = (1 - lowy_float) * x_percent;
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf.b = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += swapbuf.f * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLfloat*>(temp_index) * percent;
                        }
                    }
                    for (m = lowy_int + 1; m < highy_int; m++) {
                        temp += ysize;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                swapbuf.b = GLU_SWAP_4_BYTES(temp_index);
                                totals[k] += swapbuf.f * x_percent;
                            } else {
                                totals[k] += *reinterpret_cast<const GLfloat*>(temp_index) * x_percent;
                            }
                        }
                    }
                    percent = x_percent * highy_float;
                    temp += ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf.b = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += swapbuf.f * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLfloat*>(temp_index) * percent;
                        }
                    }
                } else if (highx_int > lowx_int) {
                    y_percent = highy_float - lowy_float;
                    percent = (1 - lowx_float) * y_percent;

                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf.b = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += swapbuf.f * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLfloat*>(temp_index) * percent;
                        }
                    }
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        temp += group_size;
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                swapbuf.b = GLU_SWAP_4_BYTES(temp_index);
                                totals[k] += swapbuf.f * y_percent;
                            } else {
                                totals[k] += *reinterpret_cast<const GLfloat*>(temp_index) * y_percent;
                            }
                        }
                    }
                    temp += group_size;
                    percent = y_percent * highx_float;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf.b = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += swapbuf.f * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLfloat*>(temp_index) * percent;
                        }
                    }
                } else {
                    percent = (highy_float - lowy_float) * (highx_float - lowx_float);
                    temp = reinterpret_cast<const char*>(datain) + xindex + lowy_int * ysize;
                    for (k = 0, temp_index = temp; k < components;
                         k++, temp_index += element_size) {
                        if (myswap_bytes) {
                            swapbuf.b = GLU_SWAP_4_BYTES(temp_index);
                            totals[k] += swapbuf.f * percent;
                        } else {
                            totals[k] += *reinterpret_cast<const GLfloat*>(temp_index) * percent;
                        }
                    }
                }

                // this is for the pixels in the body
                temp0 = reinterpret_cast<const char*>(datain) + xindex + group_size + (lowy_int + 1) * ysize;
                for (m = lowy_int + 1; m < highy_int; m++) {
                    temp = temp0;
                    for (l = lowx_int + 1; l < highx_int; l++) {
                        for (k = 0, temp_index = temp; k < components;
                             k++, temp_index += element_size) {
                            if (myswap_bytes) {
                                swapbuf.b = GLU_SWAP_4_BYTES(temp_index);
                                totals[k] += swapbuf.f;
                            } else {
                                totals[k] += *reinterpret_cast<const GLfloat*>(temp_index);
                            }
                        }
                        temp += group_size;
                    }
                    temp0 += ysize;
                }

                outindex = (j + (i * widthout)) * components;
                for (k = 0; k < components; k++) {
                    dataout[outindex + k] = totals[k] / area;
                }
                lowx_int = highx_int;
                lowx_float = highx_float;
                highx_int += convx_int;
                highx_float += convx_float;
                if (highx_float > 1) {
                    highx_float -= static_cast<float>(1.0);
                    highx_int++;
                }
            }
            lowy_int = highy_int;
            lowy_float = highy_float;
            highy_int += convy_int;
            highy_float += convy_float;
            if (highy_float > 1) {
                highy_float -= static_cast<float>(1.0);
                highy_int++;
            }
        }
    }

    // `fill_image`'s GL_BITMAP arm: one unsigned short per bit, 1 for an index format and 65535
    // otherwise.
    void fill_image_bitmap(const PixelStorageModes* psm, GLint width, GLint height, GLenum format,
                           GLboolean index_format, const void* userdata, GLushort* newimage)
    {
        GLint components = elements_per_group(format);
        GLint groups_per_line;
        if (psm->unpack_row_length > 0)
            groups_per_line = psm->unpack_row_length;
        else
            groups_per_line = width;

        GLint rowsize = (groups_per_line * components + 7) / 8;
        GLint padding = rowsize % psm->unpack_alignment;
        if (padding)
            rowsize += psm->unpack_alignment - padding;
        const GLubyte* start = static_cast<const GLubyte*>(userdata) + psm->unpack_skip_rows * rowsize +
                               (psm->unpack_skip_pixels * components / 8);
        GLint elements_per_line = width * components;
        GLushort* iter2 = newimage;
        for (GLint i = 0; i < height; i++) {
            const GLubyte* iter = start;
            GLint bit_offset = (psm->unpack_skip_pixels * components) % 8;
            for (GLint j = 0; j < elements_per_line; j++) {
                GLint current_bit;
                if (psm->unpack_lsb_first)
                    current_bit = iter[0] & (1 << bit_offset);
                else
                    current_bit = iter[0] & (1 << (7 - bit_offset));
                if (current_bit) {
                    if (index_format)
                        *iter2 = 1;
                    else
                        *iter2 = 65535;
                } else {
                    *iter2 = 0;
                }
                bit_offset++;
                if (bit_offset == 8) {
                    bit_offset = 0;
                    iter++;
                }
                iter2++;
            }
            start += rowsize;
        }
    }

    // GL_BITMAP data is expanded to unsigned shorts and box-filtered at the nearest power of two
    // the texture size limit allows; no size-1 tail is padded.
    int bitmapBuild2DMipmaps(GLenum target, GLint internalFormat, GLint width, GLint height,
                             GLenum format, GLenum type, const void* data)
    {
        K2_UNUSED(type);
        PixelStorageModes psm;
        retrieveStoreModes(&psm);

        GLint maxsize;
        ASLgl::glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxsize);
        GLint newwidth = nearestPower(width);
        if (newwidth > maxsize)
            newwidth = maxsize;
        GLint newheight = nearestPower(height);
        if (newheight > maxsize)
            newheight = maxsize;
        GLint levels = gluMipmapLog2(newwidth);
        GLint level = gluMipmapLog2(newheight);
        if (level > levels)
            levels = level;

        GLushort* otherImage = 0;
        GLushort* newImage =
            static_cast<GLushort*>(malloc(image_size(width, height, format, GL_UNSIGNED_SHORT)));
        GLint newImage_width = width;
        GLint newImage_height = height;
        if (newImage == 0)
            return GLU_OUT_OF_MEMORY;

        fill_image_bitmap(&psm, width, height, format, static_cast<GLboolean>(is_index(format)), data, newImage);

        GLint cmpts = elements_per_group(format);
        for (level = 0; level <= levels; level++) {
            if (newImage_width == newwidth && newImage_height == newheight) {
                ASLgl::glTexImage2D(target, level, internalFormat, newImage_width, newImage_height,
                                    0, format, GL_UNSIGNED_SHORT, newImage);
            } else {
                if (otherImage == 0) {
                    GLint memreq = image_size(newwidth, newheight, format, GL_UNSIGNED_SHORT);
                    otherImage = static_cast<GLushort*>(malloc(memreq));
                    if (otherImage == 0)
                        return GLU_OUT_OF_MEMORY;
                }
                scale_internal(cmpts, newImage_width, newImage_height, newImage, newwidth,
                               newheight, otherImage);
                GLushort* imageTemp = otherImage;
                otherImage = newImage;
                newImage = imageTemp;

                newImage_width = newwidth;
                newImage_height = newheight;
                ASLgl::glTexImage2D(target, level, internalFormat, newImage_width, newImage_height,
                                    0, format, GL_UNSIGNED_SHORT, newImage);
            }
            if (newwidth > 1)
                newwidth /= 2;
            if (newheight > 1)
                newheight /= 2;
        }

        free(newImage);
        if (otherImage)
            free(otherImage);
        return 0;
    }

    int gluBuild2DMipmapLevelsCore(GLenum target, GLint internalFormat, GLsizei width,
                                   GLsizei height, GLsizei widthPowerOf2, GLsizei heightPowerOf2,
                                   GLenum format, GLenum type, GLint userLevel, GLint baseLevel,
                                   GLint maxLevel, const void* data)
    {
        if (type == GL_BITMAP)
            return bitmapBuild2DMipmaps(target, internalFormat, width, height, format, type, data);

        void* srcImage = 0;
        void* dstImage = 0;
        void* tmpImage;
        GLint memreq;

        GLint newwidth = widthPowerOf2;
        GLint newheight = heightPowerOf2;
        GLint levels = gluMipmapLog2(newwidth);
        GLint level = gluMipmapLog2(newheight);
        if (level > levels)
            levels = level;
        levels += userLevel;

        PixelStorageModes psm;
        retrieveStoreModes(&psm);
        GLint myswap_bytes = psm.unpack_swap_bytes;
        GLint cmpts = elements_per_group(format);
        GLint groups_per_line;
        if (psm.unpack_row_length > 0)
            groups_per_line = psm.unpack_row_length;
        else
            groups_per_line = width;

        GLint element_size = static_cast<GLint>(bytes_per_element(type));
        GLint group_size = element_size * cmpts;
        if (element_size == 1)
            myswap_bytes = 0;

        GLint rowsize = groups_per_line * group_size;
        GLint padding = rowsize % psm.unpack_alignment;
        if (padding)
            rowsize += psm.unpack_alignment - padding;
        const void* usersImage = static_cast<const GLubyte*>(data) + psm.unpack_skip_rows * rowsize +
                                 psm.unpack_skip_pixels * group_size;

        level = userLevel;

        if (width == newwidth && height == newheight) {
            // Already a power-of-two size: the user's image is level userLevel as it stands.
            if (baseLevel <= level && level <= maxLevel) {
                ASLgl::glTexImage2D(target, level, internalFormat, width, height, 0, format, type,
                                    usersImage);
            }
            if (levels == 0)
                return 0;
            {
                int nextWidth = newwidth / 2;
                int nextHeight = newheight / 2;
                if (nextWidth < 1)
                    nextWidth = 1;
                if (nextHeight < 1)
                    nextHeight = 1;
                memreq = image_size(nextWidth, nextHeight, format, type);
            }

            switch (type) {
            case GL_UNSIGNED_BYTE:
            case GL_BYTE:
            case GL_UNSIGNED_SHORT:
            case GL_SHORT:
            case GL_UNSIGNED_INT:
            case GL_INT:
            case GL_FLOAT:
                dstImage = malloc(memreq);
                break;
            default:
                return GLU_INVALID_ENUM;
            }
            if (dstImage == 0)
                return GLU_OUT_OF_MEMORY;
            switch (type) {
            case GL_UNSIGNED_BYTE:
                halveImage_ubyte(cmpts, width, height, static_cast<const GLubyte*>(usersImage),
                                 static_cast<GLubyte*>(dstImage), element_size, rowsize, group_size);
                break;
            case GL_BYTE:
                halveImage_byte(cmpts, width, height, static_cast<const GLbyte*>(usersImage), static_cast<GLbyte*>(dstImage),
                                element_size, rowsize, group_size);
                break;
            case GL_UNSIGNED_SHORT:
                halveImage_ushort(cmpts, width, height, static_cast<const GLushort*>(usersImage),
                                  static_cast<GLushort*>(dstImage), element_size, rowsize, group_size,
                                  myswap_bytes);
                break;
            case GL_SHORT:
                halveImage_short(cmpts, width, height, static_cast<const GLshort*>(usersImage),
                                 static_cast<GLshort*>(dstImage), element_size, rowsize, group_size,
                                 myswap_bytes);
                break;
            case GL_UNSIGNED_INT:
                halveImage_uint(cmpts, width, height, static_cast<const GLuint*>(usersImage), static_cast<GLuint*>(dstImage),
                                element_size, rowsize, group_size, myswap_bytes);
                break;
            case GL_INT:
                halveImage_int(cmpts, width, height, static_cast<const GLint*>(usersImage), static_cast<GLint*>(dstImage),
                               element_size, rowsize, group_size, myswap_bytes);
                break;
            case GL_FLOAT:
                halveImage_float(cmpts, width, height, static_cast<const GLfloat*>(usersImage),
                                 static_cast<GLfloat*>(dstImage), element_size, rowsize, group_size,
                                 myswap_bytes);
                break;
            }
            newwidth = width / 2;
            newheight = height / 2;
            if (newwidth < 1)
                newwidth = 1;
            if (newheight < 1)
                newheight = 1;

            myswap_bytes = 0;
            rowsize = newwidth * group_size;
            memreq = image_size(newwidth, newheight, format, type);
            tmpImage = srcImage;
            srcImage = dstImage;
            dstImage = tmpImage;
            switch (type) {
            case GL_UNSIGNED_BYTE:
            case GL_BYTE:
            case GL_UNSIGNED_SHORT:
            case GL_SHORT:
            case GL_UNSIGNED_INT:
            case GL_INT:
            case GL_FLOAT:
                dstImage = malloc(memreq);
                break;
            default:
                return GLU_INVALID_ENUM;
            }
            if (dstImage == 0)
                return GLU_OUT_OF_MEMORY;
            // Level userLevel + 1 is in srcImage; level userLevel is already loaded.
            level = userLevel + 1;
        } else {
            // Not a power-of-two size: scale the user's image to one first.
            memreq = image_size(newwidth, newheight, format, type);
            switch (type) {
            case GL_UNSIGNED_BYTE:
            case GL_BYTE:
            case GL_UNSIGNED_SHORT:
            case GL_SHORT:
            case GL_UNSIGNED_INT:
            case GL_INT:
            case GL_FLOAT:
                dstImage = malloc(memreq);
                break;
            default:
                return GLU_INVALID_ENUM;
            }
            if (dstImage == 0)
                return GLU_OUT_OF_MEMORY;

            switch (type) {
            case GL_UNSIGNED_BYTE:
                scale_internal_ubyte(cmpts, width, height, static_cast<const GLubyte*>(usersImage), newwidth,
                                     newheight, static_cast<GLubyte*>(dstImage), element_size, rowsize,
                                     group_size);
                break;
            case GL_BYTE:
                scale_internal_byte(cmpts, width, height, static_cast<const GLbyte*>(usersImage), newwidth,
                                    newheight, static_cast<GLbyte*>(dstImage), element_size, rowsize,
                                    group_size);
                break;
            case GL_UNSIGNED_SHORT:
                scale_internal_ushort(cmpts, width, height, static_cast<const GLushort*>(usersImage), newwidth,
                                      newheight, static_cast<GLushort*>(dstImage), element_size, rowsize,
                                      group_size, myswap_bytes);
                break;
            case GL_SHORT:
                scale_internal_short(cmpts, width, height, static_cast<const GLshort*>(usersImage), newwidth,
                                     newheight, static_cast<GLshort*>(dstImage), element_size, rowsize,
                                     group_size, myswap_bytes);
                break;
            case GL_UNSIGNED_INT:
                scale_internal_uint(cmpts, width, height, static_cast<const GLuint*>(usersImage), newwidth,
                                    newheight, static_cast<GLuint*>(dstImage), element_size, rowsize, group_size,
                                    myswap_bytes);
                break;
            case GL_INT:
                scale_internal_int(cmpts, width, height, static_cast<const GLint*>(usersImage), newwidth,
                                   newheight, static_cast<GLint*>(dstImage), element_size, rowsize, group_size,
                                   myswap_bytes);
                break;
            case GL_FLOAT:
                scale_internal_float(cmpts, width, height, static_cast<const GLfloat*>(usersImage), newwidth,
                                     newheight, static_cast<GLfloat*>(dstImage), element_size, rowsize,
                                     group_size, myswap_bytes);
                break;
            }
            myswap_bytes = 0;
            rowsize = newwidth * group_size;
            tmpImage = srcImage;
            srcImage = dstImage;
            dstImage = tmpImage;

            if (levels != 0) {
                {
                    int nextWidth = newwidth / 2;
                    int nextHeight = newheight / 2;
                    if (nextWidth < 1)
                        nextWidth = 1;
                    if (nextHeight < 1)
                        nextHeight = 1;
                    memreq = image_size(nextWidth, nextHeight, format, type);
                }
                switch (type) {
                case GL_UNSIGNED_BYTE:
                case GL_BYTE:
                case GL_UNSIGNED_SHORT:
                case GL_SHORT:
                case GL_UNSIGNED_INT:
                case GL_INT:
                case GL_FLOAT:
                    dstImage = malloc(memreq);
                    break;
                default:
                    return GLU_INVALID_ENUM;
                }
                if (dstImage == 0)
                    return GLU_OUT_OF_MEMORY;
            }
            // Level userLevel is in srcImage; nothing is loaded yet.
            level = userLevel;
        }

        if (baseLevel <= level && level <= maxLevel) {
            ASLgl::glTexImage2D(target, level, internalFormat, newwidth, newheight, 0, format, type,
                                srcImage);
        }

        level++;
        for (; level <= levels; level++) {
            switch (type) {
            case GL_UNSIGNED_BYTE:
                halveImage_ubyte(cmpts, newwidth, newheight, static_cast<GLubyte*>(srcImage), static_cast<GLubyte*>(dstImage),
                                 element_size, rowsize, group_size);
                break;
            case GL_BYTE:
                halveImage_byte(cmpts, newwidth, newheight, static_cast<GLbyte*>(srcImage), static_cast<GLbyte*>(dstImage),
                                element_size, rowsize, group_size);
                break;
            case GL_UNSIGNED_SHORT:
                halveImage_ushort(cmpts, newwidth, newheight, static_cast<GLushort*>(srcImage),
                                  static_cast<GLushort*>(dstImage), element_size, rowsize, group_size,
                                  myswap_bytes);
                break;
            case GL_SHORT:
                halveImage_short(cmpts, newwidth, newheight, static_cast<GLshort*>(srcImage), static_cast<GLshort*>(dstImage),
                                 element_size, rowsize, group_size, myswap_bytes);
                break;
            case GL_UNSIGNED_INT:
                halveImage_uint(cmpts, newwidth, newheight, static_cast<GLuint*>(srcImage), static_cast<GLuint*>(dstImage),
                                element_size, rowsize, group_size, myswap_bytes);
                break;
            case GL_INT:
                halveImage_int(cmpts, newwidth, newheight, static_cast<GLint*>(srcImage), static_cast<GLint*>(dstImage),
                               element_size, rowsize, group_size, myswap_bytes);
                break;
            case GL_FLOAT:
                halveImage_float(cmpts, newwidth, newheight, static_cast<GLfloat*>(srcImage), static_cast<GLfloat*>(dstImage),
                                 element_size, rowsize, group_size, myswap_bytes);
                break;
            }

            tmpImage = srcImage;
            srcImage = dstImage;
            dstImage = tmpImage;

            if (newwidth > 1) {
                newwidth /= 2;
                rowsize /= 2;
            }
            if (newheight > 1)
                newheight /= 2;
            {
                int rowPad = rowsize % psm.unpack_alignment;
                if (rowPad == 0) {
                    if (baseLevel <= level && level <= maxLevel) {
                        ASLgl::glTexImage2D(target, level, internalFormat, newwidth, newheight, 0,
                                            format, type, srcImage);
                    }
                } else {
                    int newRowLength = rowsize + psm.unpack_alignment - rowPad;
                    void* newMipmapImage = malloc(static_cast<size_t>(newRowLength * newheight));
                    if (newMipmapImage == 0)
                        return GLU_OUT_OF_MEMORY;
                    unsigned char* dstTrav = static_cast<unsigned char*>(newMipmapImage);
                    K2_ASSUME(srcImage != 0);   // this level's halved image: a checked malloc, swapped in above
                    unsigned char* srcTrav = static_cast<unsigned char*>(srcImage);
                    for (int ii = 0; ii < newheight;
                         ii++, dstTrav += newRowLength, srcTrav += rowsize) {
                        memcpy(dstTrav, srcTrav, rowsize);  // idiom-exempt: a row copy, in bytes
                    }
                    if (baseLevel <= level && level <= maxLevel) {
                        ASLgl::glTexImage2D(target, level, internalFormat, newwidth, newheight, 0,
                                            format, type, newMipmapImage);
                    }
                    free(newMipmapImage);
                }
            }
        }

        free(srcImage);
        if (dstImage)
            free(dstImage);
        return 0;
    }
} // namespace
#endif
