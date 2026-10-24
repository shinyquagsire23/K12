#ifndef LIBCMATH_H
#define LIBCMATH_H

// The engine's <math.h>, declaring once the math functions it calls.
// Nounwind (throw()) where bionic's declarations are not, so this header comes after every system
// header in a TU (the exception specifications differ, an error in either order); port/libcbase.h
// explains the scheme.

#if defined(K2_HOST_BUILD)
// A host takes its own C library's declarations.
#include <math.h>
#else  // bionic's declarations, without its headers

#if !defined(_MATH_H_)
extern "C" {
float  sqrtf(float x) throw();
float  sinf(float x) throw();
float  cosf(float x) throw();
float  tanf(float x) throw();
float  asinf(float x) throw();
float  acosf(float x) throw();
float  atanf(float x) throw();
float  atan2f(float y, float x) throw();
float  powf(float x, float y) throw();
float  logf(float x) throw();
float  log10f(float x) throw();
float  exp2f(float x) throw();
float  floorf(float x) throw();
float  ceilf(float x) throw();
float  fabsf(float x) throw();
float  fminf(float x, float y) throw();
float  fmaxf(float x, float y) throw();
double sqrt(double x) throw();
double sin(double x) throw();
double cos(double x) throw();
double tan(double x) throw();
double asin(double x) throw();
double acos(double x) throw();
double atan(double x) throw();
double atan2(double y, double x) throw();
double pow(double x, double y) throw();
double log(double x) throw();
double floor(double x) throw();
double fabs(double x) throw();
}
#endif

#endif  // K2_HOST_BUILD

#endif // LIBCMATH_H
