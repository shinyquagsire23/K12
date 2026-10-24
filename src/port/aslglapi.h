#ifndef ASLGLAPI_H
#define ASLGLAPI_H

// ASLgl, Aspyr's GL layer: the engine calls GL 1.x/ARB through it and, on Android, it translates
// down to GLES 2. Declared once; the bodies are platforms/shim (aslgl.gen.cpp, shim_kotorgl.cpp).
// KOTOR 1 has no such layer: port/k1gl.h forwards the same names to KOTOR 1's android_port_gl*
// imports, and only the names it does not cover (the GLES 2 shader calls) are declared for KOTOR 1.
#include "k2stdint.h"
#include "port/glconst.h"

#if KOTOR1
#include "port/k1gl.h"
#endif

namespace ASLgl {
#if !KOTOR1
void glActiveStencilFaceEXT(unsigned int);
void glActiveTexture(unsigned int);
void glActiveTextureARB(unsigned int);
void glAlphaFunc(unsigned int, float);
void glBegin(unsigned int);
void glBindBufferARB(unsigned int, unsigned int);
void glBindProgramARB(unsigned int, unsigned int);
void glBindRenderbuffer(unsigned int, unsigned int);
void glBindTexture(unsigned int, unsigned int);
void glBitmap(int, int, float, float, float, float, const unsigned char*);
void glBlendFunc(unsigned int, unsigned int);
void glBufferDataARB(unsigned int, int64_t, const void*, unsigned int);
void glBufferSubDataARB(unsigned int, int64_t, int64_t, const void*);
void glCallLists(int, unsigned int, const void*);
unsigned int glCheckFramebufferStatus(unsigned int);
void glClear(unsigned int);
void glClearColor(float, float, float, float);
void glClearDepth(double);
void glClearStencil(int);
void glClientActiveTextureARB(unsigned int);
void glColor3f(float, float, float);
void glColor4f(float, float, float, float);
void glColor4fv(const float*);
void glColor4ubv(const unsigned char*);
void glColorMask(unsigned char, unsigned char, unsigned char, unsigned char);
void glColorMaterial(unsigned int, unsigned int);
void glColorPointer(int, unsigned int, int, const void*);
void glCompressedTexImage2DARB(unsigned int, int, unsigned int, int, int, int, int, const void*);
void glCopyTexImage2D(unsigned int, int, unsigned int, int, int, int, int, int);
void glCopyTexSubImage2D(unsigned int, int, int, int, int, int, int, int);
void glDeleteBuffersARB(int, const unsigned int*);
void glDeleteLists(unsigned int, int);
void glDeleteProgramsARB(int, const unsigned int*);
void glDeleteRenderbuffersEXT(int, const unsigned int*);
void glDeleteTextures(int, const unsigned int*);
void glDepthFunc(unsigned int);
void glDepthMask(unsigned char);
void glDisable(unsigned int);
void glDisableClientState(unsigned int);
void glDisableVertexAttribArrayARB(unsigned int);
void glDrawArrays(unsigned int, int, int);
void glDrawBuffer(unsigned int);
void glDrawElements(unsigned int, int, unsigned int, const void*);
void glEnable(unsigned int);
void glEnableClientState(unsigned int);
void glEnableVertexAttribArrayARB(unsigned int);
void glEnd();
void glEndList();
void glFlush();
void glFogf(unsigned int, float);
void glFogfv(unsigned int, const float*);
void glFogi(unsigned int, int);
void glFramebufferRenderbuffer(unsigned int, unsigned int, unsigned int, unsigned int);
void glFrontFace(unsigned int);
void glGenBuffersARB(int, unsigned int*);
void glGenFramebuffers(int, unsigned int*);
unsigned int glGenLists(int);
void glGenProgramsARB(int, unsigned int*);
void glGenRenderbuffers(int, unsigned int*);
void glGenTextures(int, unsigned int*);
void glGetBufferParameterivARB(unsigned int, unsigned int, int*);
unsigned int glGetError();
void glGetFloatv(unsigned int, float*);
void glGetIntegerv(unsigned int, int*);
const unsigned char* glGetString(unsigned int);
void glGetTexParameteriv(unsigned int, unsigned int, int*);
unsigned char glIsEnabled(unsigned int);
unsigned char glIsProgramARB(unsigned int);
unsigned char glIsTexture(unsigned int);
void glLightModelfv(unsigned int, const float*);
void glLightfv(unsigned int, unsigned int, const float*);
void glLineWidth(float);
void glListBase(unsigned int);
void glLoadIdentity();
void glMaterialfv(unsigned int, unsigned int, const float*);
void glMatrixMode(unsigned int);
void glMultMatrixf(const float*);
void glMultiTexCoord2fARB(unsigned int, float, float);
void glMultiTexCoord3fARB(unsigned int, float, float, float);
void glNewList(unsigned int, unsigned int);
void glNormal3f(float, float, float);
void glNormal3fv(const float*);
void glNormalPointer(unsigned int, int, const void*);
void glOrtho(double, double, double, double, double, double);
void glPixelStorei(unsigned int, int);
void glPointSize(float);
void glPolygonMode(unsigned int, unsigned int);
void glPolygonOffset(float, float);
void glPopAttrib();
void glPopClientAttrib();
void glPopMatrix();
void glProgramEnvParameter4fARB(unsigned int, unsigned int, float, float, float, float);
void glProgramEnvParameter4fvARB(unsigned int, unsigned int, const float*);
void glProgramStringARB(unsigned int, unsigned int, int, const void*);
void glPushAttrib(unsigned int);
void glPushClientAttrib(unsigned int);
void glPushMatrix();
void glRasterPos2f(float, float);
void glReadPixels(int, int, int, int, unsigned int, unsigned int, void*);
void glRenderbufferStorage(unsigned int, unsigned int, int, int);
void glRotatef(float, float, float, float);
void glScalef(float, float, float);
void glScissor(int, int, int, int);
void glStencilFunc(unsigned int, int, unsigned int);
void glStencilMask(unsigned int);
void glStencilOp(unsigned int, unsigned int, unsigned int);
void glTexCoord2f(float, float);
void glTexCoordPointer(int, unsigned int, int, const void*);
void glTexEnvi(unsigned int, unsigned int, int);
void glTexGeni(unsigned int, unsigned int, int);
void glTexImage1D(unsigned int, int, int, int, int, unsigned int, unsigned int, const void*);
void glTexImage2D(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void*);
void glTexImage3D(unsigned int, int, int, int, int, int, int, unsigned int, unsigned int, const void*);
void glTexParameteri(unsigned int, unsigned int, int);
void glTexSubImage2D(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void*);
void glTexSubImage3D(unsigned int, int, int, int, int, int, int, int, unsigned int, unsigned int, const void*);
void glTranslatef(float, float, float);
void glVertex2f(float, float);
void glVertex3f(float, float, float);
void glVertex3fv(const float*);
void glVertex4f(float, float, float, float);
void glVertexAttribPointerARB(unsigned int, int, unsigned int, unsigned char, int, const void*);
void glVertexPointer(int, unsigned int, int, const void*);
void glViewport(int, int, int, int);
#endif

void glAttachShader(unsigned int, unsigned int);
void glCompileShader(unsigned int);
unsigned int glCreateProgram();
unsigned int glCreateShader(unsigned int);
void glDeleteProgram(unsigned int);
void glDeleteShader(unsigned int);
void glDisableVertexAttribArray(unsigned int);
void glEnableVertexAttribArray(unsigned int);
int glGetAttribLocation(unsigned int, const char*);
void glGetProgramInfoLog(unsigned int, int, int*, char*);
void glGetProgramiv(unsigned int, unsigned int, int*);
void glGetShaderInfoLog(unsigned int, int, int*, char*);
void glGetShaderiv(unsigned int, unsigned int, int*);
int glGetUniformLocation(unsigned int, const char*);
void glLinkProgram(unsigned int);
void glShaderSource(unsigned int, int, const char* const*, const int*);
void glUniform1f(int, float);
void glUniform1i(int, int);
void glUniform3fv(int, int, const float*);
void glUniform4fv(int, int, const float*);
void glUniformMatrix4fv(int, int, unsigned char, const float*);
void glUseProgram(unsigned int);
void glVertexAttrib1f(unsigned int, float);
void glVertexAttrib2f(unsigned int, float, float);
void glVertexAttrib3f(unsigned int, float, float, float);
void glVertexAttrib4f(unsigned int, float, float, float, float);
void glVertexAttribPointer(unsigned int, int, unsigned int, unsigned char, int, const void*);
}  // namespace ASLgl

#endif // ASLGLAPI_H
