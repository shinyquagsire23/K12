// The four GLU calls the engine makes directly (aurcamera/aurwindow/aurglcore/auraticore/aurora),
// for Android, where there is no system GLU; the macOS host takes OpenGL.framework's. Each is
// written on the GL 1.x calls gl4es provides, so the matrices and mipmaps land in gl4es's state
// like everything else the engine draws.
#include <math.h>

extern "C" {
void glOrtho(double l, double r, double b, double t, double n, double f);
void glFrustum(double l, double r, double b, double t, double n, double f);
void glTexImage2D(unsigned int target, int level, int internalformat, int width, int height, int border,
                  unsigned int format, unsigned int type, const void* pixels);
void glTexParameteri(unsigned int target, unsigned int pname, int param);
void glGetIntegerv(unsigned int pname, int* params);
void glPixelStorei(unsigned int pname, int param);

void gluOrtho2D(double fLeft, double fRight, double fBottom, double fTop)
{
    glOrtho(fLeft, fRight, fBottom, fTop, -1.0, 1.0);
}

void gluPerspective(double fovy, double aspect, double zNear, double zFar)
{
    double ymax = zNear * tan(fovy * M_PI / 360.0);
    double xmax = ymax * aspect;
    glFrustum(-xmax, xmax, -ymax, ymax, zNear, zFar);
}

const char* gluErrorString(unsigned int error)
{
    switch (error) {
    case 0: return "no error";
    case 0x0500: return "invalid enumerant";
    case 0x0501: return "invalid value";
    case 0x0502: return "invalid operation";
    case 0x0503: return "stack overflow";
    case 0x0504: return "stack underflow";
    case 0x0505: return "out of memory";
    case 0x0506: return "invalid framebuffer operation";
    default: return "unknown error";
    }
}

// Level 0 with GL_GENERATE_MIPMAP on: gl4es builds the chain. The rows are read tightly packed
// whatever the unpack alignment says, which is what the engine writes (see the shim's
// wglMakeCurrent).
int gluBuild2DMipmaps(unsigned int target, int components, int width, int height, unsigned int format,
                      unsigned int type, const void* data)
{
    enum { GL_UNPACK_ALIGNMENT = 0x0cf5, GL_GENERATE_MIPMAP = 0x8191 };
    static const int kInternal[] = { 0, 0x1909 /*LUMINANCE*/, 0x190a /*LUMINANCE_ALPHA*/, 0x1907 /*RGB*/,
                                     0x1908 /*RGBA*/ };
    int internal = components >= 1 && components <= 4 ? kInternal[components] : components;
    int align = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &align);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(target, GL_GENERATE_MIPMAP, 1);
    glTexImage2D(target, 0, internal, width, height, 0, format, type, data);
    glTexParameteri(target, GL_GENERATE_MIPMAP, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, align);
    return 0;
}
} // extern "C"
