// The "no name" sentinel compared against model, texture and part names. The data spells it "NULL"
// (a binary model's untextured mesh, a text model's root `parent`), as do K1 and the PC games; the
// K2 port's NULL -> nullptr rename reached this string, so its build compares against "nullptr".
#ifndef NULLNAME_H
#define NULLNAME_H

#if KOTOR1 || defined(K2_FIX_ENGINE_BUGS)
#define NULL_NAME "NULL"
#else
#define NULL_NAME "nullptr"
#endif

#endif
