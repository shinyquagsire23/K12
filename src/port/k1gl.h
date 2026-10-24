#pragma once
#include "k2stdint.h"

// KOTOR 1 only. KOTOR 1 has no C++ GL layer: the engine calls the C imports `android_port_glX`, plus
// six bare OpenGL ES framebuffer entry points, directly. Each translation unit includes this header
// in place of its own `ASLgl` block, and every `ASLgl::glX` below forwards to the K1 import, so call
// sites are shared between the two engines. The forwarders are `static`: a host build links
// KOTOR II's ASLgl layer as well, and its exported `ASLgl::glX` definitions would otherwise
// replace these.

extern "C" {
void android_port_glActiveTexture(unsigned int);
void android_port_glActiveTextureARB(unsigned int);
void android_port_glAlphaFunc(unsigned int, float);
void android_port_glBegin(unsigned int);
void android_port_glBindBuffer(unsigned int, unsigned int);
void android_port_glBindProgramARB(unsigned int, unsigned int);
void android_port_glBindTexture(unsigned int, unsigned int);
void android_port_glBlendFunc(unsigned int, unsigned int);
void android_port_glBufferData(unsigned int, int64_t, const void*, unsigned int);
void android_port_glBufferSubData(unsigned int, int64_t, int64_t, const void*);
void android_port_glCallLists(int, unsigned int, const void*);
void android_port_glClear(unsigned int);
void android_port_glClearColor(float, float, float, float);
void android_port_glClearDepth(double);
void android_port_glClearStencil(int);
void android_port_glClientActiveTexture(unsigned int);
void android_port_glColor3f(float, float, float);
void android_port_glColor4f(float, float, float, float);
void android_port_glColor4fv(const float*);
void android_port_glColor4ubv(const unsigned char*);
void android_port_glColorMask(unsigned char, unsigned char, unsigned char, unsigned char);
void android_port_glColorPointer(int, unsigned int, int, const void*);
void android_port_glCompressedTexImage2D(unsigned int, int, unsigned int, int, int, int, int, const void*);
void android_port_glCompressedTexImage2DARB(unsigned int, int, unsigned int, int, int, int, int, const void*);
void android_port_glCopyTexImage2D(unsigned int, int, unsigned int, int, int, int, int, int);
void android_port_glCopyTexSubImage2D(unsigned int, int, int, int, int, int, int, int);
void android_port_glDeleteBuffers(int, const unsigned int*);
void android_port_glDeleteLists(unsigned int, int);
void android_port_glDeleteProgramsARB(int, const unsigned int*);
void android_port_glDeleteTextures(int, const unsigned int*);
void android_port_glDepthFunc(unsigned int);
void android_port_glDepthMask(unsigned char);
void android_port_glDisable(unsigned int);
void android_port_glDisableClientState(unsigned int);
void android_port_glDisableVertexAttribArrayARB(unsigned int);
void android_port_glDrawArrays(unsigned int, int, int);
void android_port_glDrawElements(unsigned int, int, unsigned int, const void*);
void android_port_glEnable(unsigned int);
void android_port_glEnableClientState(unsigned int);
void android_port_glEnableVertexAttribArrayARB(unsigned int);
void android_port_glEnd();
void android_port_glFlush();
void android_port_glFogf(unsigned int, float);
void android_port_glFogfv(unsigned int, const float*);
void android_port_glFogi(unsigned int, int);
void android_port_glFrontFace(unsigned int);
void android_port_glGenBuffers(int, unsigned int*);
void android_port_glGenProgramsARB(int, unsigned int*);
void android_port_glGenTextures(int, unsigned int*);
void android_port_glGetBufferParameteriv(unsigned int, unsigned int, int*);
unsigned int android_port_glGetError();
void android_port_glGetFloatv(unsigned int, float*);
void android_port_glGetIntegerv(unsigned int, int*);
const unsigned char* android_port_glGetString(unsigned int);
void android_port_glGetTexParameteriv(unsigned int, unsigned int, int*);
unsigned char android_port_glIsEnabled(unsigned int);
unsigned char android_port_glIsTexture(unsigned int);
void android_port_glLightModelfv(unsigned int, const float*);
void android_port_glLightfv(unsigned int, unsigned int, const float*);
void android_port_glLineWidth(float);
void android_port_glListBase(unsigned int);
void android_port_glLoadIdentity();
void android_port_glMaterialfv(unsigned int, unsigned int, const float*);
void android_port_glMatrixMode(unsigned int);
void android_port_glMultMatrixf(const float*);
void android_port_glNormal3f(float, float, float);
void android_port_glNormal3fv(const float*);
void android_port_glNormalPointer(unsigned int, int, const void*);
void android_port_glOrtho(double, double, double, double, double, double);
void android_port_glPointSize(float);
void android_port_glPolygonMode(unsigned int, unsigned int);
void android_port_glPopAttrib();
void android_port_glPopClientAttrib();
void android_port_glPopMatrix();
void android_port_glProgramEnvParameter4fARB(unsigned int, unsigned int, float, float, float, float);
void android_port_glProgramEnvParameter4fvARB(unsigned int, unsigned int, const float*);
void android_port_glProgramStringARB(unsigned int, unsigned int, int, const void*);
void android_port_glPushAttrib(unsigned int);
void android_port_glPushClientAttrib(unsigned int);
void android_port_glPushMatrix();
void android_port_glRasterPos2f(float, float);
void android_port_glReadPixels(int, int, int, int, unsigned int, unsigned int, void*);
void android_port_glRotatef(float, float, float, float);
void android_port_glScalef(float, float, float);
void android_port_glScissor(int, int, int, int);
void android_port_glStencilFunc(unsigned int, int, unsigned int);
void android_port_glStencilMask(unsigned int);
void android_port_glStencilOp(unsigned int, unsigned int, unsigned int);
void android_port_glTexCoord2f(float, float);
void android_port_glTexCoordPointer(int, unsigned int, int, const void*);
void android_port_glTexEnvi(unsigned int, unsigned int, int);
void android_port_glTexGeni(unsigned int, unsigned int, int);
void android_port_glTexImage2D(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void*);
void android_port_glTexParameterf(unsigned int, unsigned int, float);
void android_port_glTexParameteri(unsigned int, unsigned int, int);
void android_port_glTexSubImage2D(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void*);
void android_port_glTranslatef(float, float, float);
void android_port_glVertex2f(float, float);
void android_port_glVertex3f(float, float, float);
void android_port_glVertex3fv(const float*);
void android_port_glVertex4f(float, float, float, float);
void android_port_glVertexAttribPointerARB(unsigned int, int, unsigned int, unsigned char, int, const void*);
void android_port_glVertexPointer(int, unsigned int, int, const void*);
void android_port_glViewport(int, int, int, int);
void android_port_glViewportRaw(int, int, int, int);

// the bare OpenGL ES framebuffer imports
void glBindRenderbuffer(unsigned int, unsigned int);
unsigned int glCheckFramebufferStatus(unsigned int);
void glFramebufferRenderbuffer(unsigned int, unsigned int, unsigned int, unsigned int);
void glGenFramebuffers(int, unsigned int*);
void glGenRenderbuffers(int, unsigned int*);
void glRenderbufferStorage(unsigned int, unsigned int, int, int);

// the OES framebuffer twins, used by every other K1 framebuffer site
void glBindFramebufferOES(unsigned int, unsigned int);
void glBindRenderbufferOES(unsigned int, unsigned int);
unsigned int glCheckFramebufferStatusOES(unsigned int);
void glFramebufferRenderbufferOES(unsigned int, unsigned int, unsigned int, unsigned int);
void glFramebufferTexture2DOES(unsigned int, unsigned int, unsigned int, unsigned int, int);
void glGenFramebuffersOES(int, unsigned int*);
void glGenRenderbuffersOES(int, unsigned int*);
void glRenderbufferStorageOES(unsigned int, unsigned int, int, int);
}

namespace ASLgl {
// same name
static inline void glActiveTexture(unsigned int a) { ::android_port_glActiveTexture(a); }
static inline void glAlphaFunc(unsigned int a, float b) { ::android_port_glAlphaFunc(a, b); }
static inline void glBegin(unsigned int a) { ::android_port_glBegin(a); }
static inline void glBindProgramARB(unsigned int a, unsigned int b) { ::android_port_glBindProgramARB(a, b); }
static inline void glBindTexture(unsigned int a, unsigned int b) { ::android_port_glBindTexture(a, b); }
static inline void glBlendFunc(unsigned int a, unsigned int b) { ::android_port_glBlendFunc(a, b); }
static inline void glCallLists(int a, unsigned int b, const void* c) { ::android_port_glCallLists(a, b, c); }
static inline void glClear(unsigned int a) { ::android_port_glClear(a); }
static inline void glClearColor(float a, float b, float c, float d) { ::android_port_glClearColor(a, b, c, d); }
static inline void glClearDepth(double a) { ::android_port_glClearDepth(a); }
static inline void glClearStencil(int a) { ::android_port_glClearStencil(a); }
static inline void glColor3f(float a, float b, float c) { ::android_port_glColor3f(a, b, c); }
static inline void glColor4f(float a, float b, float c, float d) { ::android_port_glColor4f(a, b, c, d); }
static inline void glColor4fv(const float* a) { ::android_port_glColor4fv(a); }
static inline void glColor4ubv(const unsigned char* a) { ::android_port_glColor4ubv(a); }
static inline void glColorMask(unsigned char a, unsigned char b, unsigned char c, unsigned char d) { ::android_port_glColorMask(a, b, c, d); }
static inline void glColorPointer(int a, unsigned int b, int c, const void* d) { ::android_port_glColorPointer(a, b, c, d); }
static inline void glCopyTexImage2D(unsigned int a, int b, unsigned int c, int d, int e, int f, int g, int h) { ::android_port_glCopyTexImage2D(a, b, c, d, e, f, g, h); }
static inline void glCopyTexSubImage2D(unsigned int a, int b, int c, int d, int e, int f, int g, int h) { ::android_port_glCopyTexSubImage2D(a, b, c, d, e, f, g, h); }
static inline void glDeleteLists(unsigned int a, int b) { ::android_port_glDeleteLists(a, b); }
static inline void glDeleteProgramsARB(int a, const unsigned int* b) { ::android_port_glDeleteProgramsARB(a, b); }
static inline void glDeleteTextures(int a, const unsigned int* b) { ::android_port_glDeleteTextures(a, b); }
static inline void glDepthFunc(unsigned int a) { ::android_port_glDepthFunc(a); }
static inline void glDepthMask(unsigned char a) { ::android_port_glDepthMask(a); }
static inline void glDisable(unsigned int a) { ::android_port_glDisable(a); }
static inline void glDisableClientState(unsigned int a) { ::android_port_glDisableClientState(a); }
static inline void glDisableVertexAttribArrayARB(unsigned int a) { ::android_port_glDisableVertexAttribArrayARB(a); }
static inline void glDrawArrays(unsigned int a, int b, int c) { ::android_port_glDrawArrays(a, b, c); }
static inline void glDrawElements(unsigned int a, int b, unsigned int c, const void* d) { ::android_port_glDrawElements(a, b, c, d); }
static inline void glEnable(unsigned int a) { ::android_port_glEnable(a); }
static inline void glEnableClientState(unsigned int a) { ::android_port_glEnableClientState(a); }
static inline void glEnableVertexAttribArrayARB(unsigned int a) { ::android_port_glEnableVertexAttribArrayARB(a); }
static inline void glEnd() { ::android_port_glEnd(); }
static inline void glFlush() { ::android_port_glFlush(); }
static inline void glFogf(unsigned int a, float b) { ::android_port_glFogf(a, b); }
static inline void glFogfv(unsigned int a, const float* b) { ::android_port_glFogfv(a, b); }
static inline void glFogi(unsigned int a, int b) { ::android_port_glFogi(a, b); }
static inline void glFrontFace(unsigned int a) { ::android_port_glFrontFace(a); }
static inline void glGenProgramsARB(int a, unsigned int* b) { ::android_port_glGenProgramsARB(a, b); }
static inline void glGenTextures(int a, unsigned int* b) { ::android_port_glGenTextures(a, b); }
static inline unsigned int glGetError() { return ::android_port_glGetError(); }
static inline void glGetFloatv(unsigned int a, float* b) { ::android_port_glGetFloatv(a, b); }
static inline void glGetIntegerv(unsigned int a, int* b) { ::android_port_glGetIntegerv(a, b); }
static inline const unsigned char* glGetString(unsigned int a) { return ::android_port_glGetString(a); }
static inline void glGetTexParameteriv(unsigned int a, unsigned int b, int* c) { ::android_port_glGetTexParameteriv(a, b, c); }
static inline unsigned char glIsEnabled(unsigned int a) { return ::android_port_glIsEnabled(a); }
static inline unsigned char glIsTexture(unsigned int a) { return ::android_port_glIsTexture(a); }
static inline void glLightModelfv(unsigned int a, const float* b) { ::android_port_glLightModelfv(a, b); }
static inline void glLightfv(unsigned int a, unsigned int b, const float* c) { ::android_port_glLightfv(a, b, c); }
static inline void glLineWidth(float a) { ::android_port_glLineWidth(a); }
static inline void glListBase(unsigned int a) { ::android_port_glListBase(a); }
static inline void glLoadIdentity() { ::android_port_glLoadIdentity(); }
static inline void glMaterialfv(unsigned int a, unsigned int b, const float* c) { ::android_port_glMaterialfv(a, b, c); }
static inline void glMatrixMode(unsigned int a) { ::android_port_glMatrixMode(a); }
static inline void glMultMatrixf(const float* a) { ::android_port_glMultMatrixf(a); }
static inline void glNormal3f(float a, float b, float c) { ::android_port_glNormal3f(a, b, c); }
static inline void glNormal3fv(const float* a) { ::android_port_glNormal3fv(a); }
static inline void glNormalPointer(unsigned int a, int b, const void* c) { ::android_port_glNormalPointer(a, b, c); }
static inline void glOrtho(double a, double b, double c, double d, double e, double f) { ::android_port_glOrtho(a, b, c, d, e, f); }
static inline void glPointSize(float a) { ::android_port_glPointSize(a); }
static inline void glPolygonMode(unsigned int a, unsigned int b) { ::android_port_glPolygonMode(a, b); }
static inline void glPopAttrib() { ::android_port_glPopAttrib(); }
static inline void glPopClientAttrib() { ::android_port_glPopClientAttrib(); }
static inline void glPopMatrix() { ::android_port_glPopMatrix(); }
static inline void glProgramEnvParameter4fARB(unsigned int a, unsigned int b, float c, float d, float e, float f) { ::android_port_glProgramEnvParameter4fARB(a, b, c, d, e, f); }
static inline void glProgramEnvParameter4fvARB(unsigned int a, unsigned int b, const float* c) { ::android_port_glProgramEnvParameter4fvARB(a, b, c); }
static inline void glProgramStringARB(unsigned int a, unsigned int b, int c, const void* d) { ::android_port_glProgramStringARB(a, b, c, d); }
static inline void glPushAttrib(unsigned int a) { ::android_port_glPushAttrib(a); }
static inline void glPushClientAttrib(unsigned int a) { ::android_port_glPushClientAttrib(a); }
static inline void glPushMatrix() { ::android_port_glPushMatrix(); }
static inline void glRasterPos2f(float a, float b) { ::android_port_glRasterPos2f(a, b); }
static inline void glReadPixels(int a, int b, int c, int d, unsigned int e, unsigned int f, void* g) { ::android_port_glReadPixels(a, b, c, d, e, f, g); }
static inline void glRotatef(float a, float b, float c, float d) { ::android_port_glRotatef(a, b, c, d); }
static inline void glScalef(float a, float b, float c) { ::android_port_glScalef(a, b, c); }
static inline void glScissor(int a, int b, int c, int d) { ::android_port_glScissor(a, b, c, d); }
static inline void glStencilFunc(unsigned int a, int b, unsigned int c) { ::android_port_glStencilFunc(a, b, c); }
static inline void glStencilMask(unsigned int a) { ::android_port_glStencilMask(a); }
static inline void glStencilOp(unsigned int a, unsigned int b, unsigned int c) { ::android_port_glStencilOp(a, b, c); }
static inline void glTexCoord2f(float a, float b) { ::android_port_glTexCoord2f(a, b); }
static inline void glTexCoordPointer(int a, unsigned int b, int c, const void* d) { ::android_port_glTexCoordPointer(a, b, c, d); }
static inline void glTexEnvi(unsigned int a, unsigned int b, int c) { ::android_port_glTexEnvi(a, b, c); }
static inline void glTexGeni(unsigned int a, unsigned int b, int c) { ::android_port_glTexGeni(a, b, c); }
static inline void glTexImage2D(unsigned int a, int b, int c, int d, int e, int f, unsigned int g, unsigned int h, const void* i) { ::android_port_glTexImage2D(a, b, c, d, e, f, g, h, i); }
static inline void glTexParameterf(unsigned int a, unsigned int b, float c) { ::android_port_glTexParameterf(a, b, c); }
static inline void glTexParameteri(unsigned int a, unsigned int b, int c) { ::android_port_glTexParameteri(a, b, c); }
static inline void glTexSubImage2D(unsigned int a, int b, int c, int d, int e, int f, unsigned int g, unsigned int h, const void* i) { ::android_port_glTexSubImage2D(a, b, c, d, e, f, g, h, i); }
static inline void glTranslatef(float a, float b, float c) { ::android_port_glTranslatef(a, b, c); }
static inline void glVertex2f(float a, float b) { ::android_port_glVertex2f(a, b); }
static inline void glVertex3f(float a, float b, float c) { ::android_port_glVertex3f(a, b, c); }
static inline void glVertex3fv(const float* a) { ::android_port_glVertex3fv(a); }
static inline void glVertex4f(float a, float b, float c, float d) { ::android_port_glVertex4f(a, b, c, d); }
static inline void glVertexAttribPointerARB(unsigned int a, int b, unsigned int c, unsigned char d, int e, const void* f) { ::android_port_glVertexAttribPointerARB(a, b, c, d, e, f); }
static inline void glVertexPointer(int a, unsigned int b, int c, const void* d) { ::android_port_glVertexPointer(a, b, c, d); }
static inline void glViewport(int a, int b, int c, int d) { ::android_port_glViewport(a, b, c, d); }

// K1 drops the ARB suffix
static inline void glActiveTextureARB(unsigned int a) { ::android_port_glActiveTexture(a); }
static inline void glBindBufferARB(unsigned int a, unsigned int b) { ::android_port_glBindBuffer(a, b); }
static inline void glBufferDataARB(unsigned int a, int64_t b, const void* c, unsigned int d) { ::android_port_glBufferData(a, b, c, d); }
static inline void glBufferSubDataARB(unsigned int a, int64_t b, int64_t c, const void* d) { ::android_port_glBufferSubData(a, b, c, d); }
static inline void glClientActiveTextureARB(unsigned int a) { ::android_port_glClientActiveTexture(a); }
static inline void glCompressedTexImage2DARB(unsigned int a, int b, unsigned int c, int d, int e, int f, int g, const void* h) { ::android_port_glCompressedTexImage2D(a, b, c, d, e, f, g, h); }
static inline void glDeleteBuffersARB(int a, const unsigned int* b) { ::android_port_glDeleteBuffers(a, b); }
static inline void glGenBuffersARB(int a, unsigned int* b) { ::android_port_glGenBuffers(a, b); }
static inline void glGetBufferParameterivARB(unsigned int a, unsigned int b, int* c) { ::android_port_glGetBufferParameteriv(a, b, c); }

// the bare OpenGL ES framebuffer imports
static inline void glBindRenderbuffer(unsigned int a, unsigned int b) { ::glBindRenderbuffer(a, b); }
static inline unsigned int glCheckFramebufferStatus(unsigned int a) { return ::glCheckFramebufferStatus(a); }
static inline void glFramebufferRenderbuffer(unsigned int a, unsigned int b, unsigned int c, unsigned int d) { ::glFramebufferRenderbuffer(a, b, c, d); }
static inline void glGenFramebuffers(int a, unsigned int* b) { ::glGenFramebuffers(a, b); }
static inline void glGenRenderbuffers(int a, unsigned int* b) { ::glGenRenderbuffers(a, b); }
static inline void glRenderbufferStorage(unsigned int a, unsigned int b, int c, int d) { ::glRenderbufferStorage(a, b, c, d); }

// no K1 import: K1 never makes these calls, so they stay as K2 externs
void glActiveStencilFaceEXT(unsigned int);
void glBitmap(int, int, float, float, float, float, const unsigned char*);
void glColorMaterial(unsigned int, unsigned int);
void glDeleteRenderbuffersEXT(int, const unsigned int*);
void glDrawBuffer(unsigned int);
void glEndList();
unsigned int glGenLists(int);
unsigned char glIsProgramARB(unsigned int);
void glMultiTexCoord2fARB(unsigned int, float, float);
void glMultiTexCoord3fARB(unsigned int, float, float, float);
void glNewList(unsigned int, unsigned int);
void glPixelStorei(unsigned int, int);
void glPolygonOffset(float, float);
void glTexImage1D(unsigned int, int, int, int, int, unsigned int, unsigned int, const void*);
void glTexImage3D(unsigned int, int, int, int, int, int, int, unsigned int, unsigned int, const void*);
void glTexSubImage3D(unsigned int, int, int, int, int, int, int, int, unsigned int, unsigned int, const void*);
}
