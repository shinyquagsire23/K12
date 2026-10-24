// Template members an engine TU calls out of line that no engine TU is guaranteed to emit. Whether
// the compiler emits one depends on the optimizer: a Debug build emits these out of line where they
// are used, an optimized one may inline every use and emit none, leaving the caller on a generated
// abort stub (the iOS presets are RelWithDebInfo). Compiled with the engine's flags
// (K2P_CONSOLE_SOURCE). Delete an entry when src/ instantiates it.
#include "CExoArrayListImpl.h"

class CSWGuiControl;

// swguinamechargen.cpp (the name page) assigns a control list.
template CExoArrayList<CSWGuiControl*>& CExoArrayList<CSWGuiControl*>::operator=(const CExoArrayList<CSWGuiControl*>&);
