#ifndef BINDING_H
#define BINDING_H

#include "List.h"

// One key-to-console-command binding for the Aurora debug console.
//
// The class self-registers. Its ctor appends `this` to the file-scope `bindings` list and its dtor
// removes it again, so the global list is the binding table (there is no separate Add/Remove API).
// DoBindings polls every entry once per frame and runs the bound command.
class Binding {
public:
    Binding();
    ~Binding();

    char m_cKey;
    char m_szCommand[256];
    int m_bRepeat;
    int m_bDown;
};

Binding* FindBinding(char cKey);
void DoBindings();
void unbindall();

#endif
