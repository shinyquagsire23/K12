#pragma once

// One per-creature visibility-list entry, shared by the client and server halves of the engine.

struct CSWVisibilityNode {
    unsigned long long m_objectId;
    unsigned char      m_flags;

    CSWVisibilityNode() : m_objectId(OBJECT_INVALID), m_flags(0) {}
};
