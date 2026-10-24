#pragma once

// The shared root of the animation-base hierarchy holds a vptr and four indexed floats, reachable
// only through Get and Set. Both CSWCAnimBase, the client's rendering and animation object, and
// CSWSAnimBase, the server-side shell, derive from it directly, and its whole vtable is the
// destructor pair.

class CSWAnimBase {
public:
    CSWAnimBase();

    virtual ~CSWAnimBase();

    // Indexed 0 to 3 by Get and Set. Their purpose is unknown: nothing in the animation-base family
    // reads them off this object. The only known writers are CSWCCreature::LoadModel and
    // CSWCCreature::RequestModel, which stamp 1850.0f, 6600.0f, 0.925f and 3.3f into a freshly
    // allocated animation base in index order. The constructor zeroes only the first two.
    float m_fParam0;
    float m_fParam1;
    float m_fParam2;
    float m_fParam3;

    // Both return an int that no caller uses.
    int Get(int nIndex, float* pfValue);
    int Set(int nIndex, float fValue);
};
