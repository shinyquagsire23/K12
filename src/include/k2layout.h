// K2_LAYOUT_ASSERT: a pin on an engine record's layout. The layout is a contract only where the
// engine's ABI is the build's, that is on the Android targets. A host may lay the same class out
// otherwise -- the MSVC ABI never packs a derived class into its base's tail padding -- and nothing
// there depends on the numbers. A layout that is a contract on every build (an on-disk or network
// record, a record the shim fills for Win32, SDL's structs) keeps a plain static_assert.
#ifndef K2LAYOUT_H
#define K2LAYOUT_H

#if defined(__ANDROID__) && defined(__aarch64__)
#define K2_LAYOUT_ASSERT(...) static_assert(__VA_ARGS__)
#else
#define K2_LAYOUT_ASSERT(...) static_assert(true, "")
#endif

#endif
