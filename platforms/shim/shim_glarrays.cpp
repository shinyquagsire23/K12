// shim_glarrays.cpp -- a shadow of the GL client-array state, kept by the generated GL forwarders.
//
// NVIDIA's desktop driver fetches every enabled vertex array at a draw, whether the draw (or its
// program) reads it or not, and the engine leaves arrays enabled across draws that never set them.
// Apple's GL skips the unused ones, so a stale pointer only faults on NVIDIA, inside the driver's
// generated vertex-fetch code, where the crash report has no stack. This file keeps, per array:
// whether it is enabled, its pointer, the buffer bound when it was set and who set it; and a ring
// of the last draws with their engine callers. Then:
//   * operator delete (shim_new.cpp) marks every enabled client array whose pointer lies in the
//     block being freed, and the next draw disables it first (logging the array, the setter and the
//     draw once) -- the next gl*Pointer for that array enables it again, so the engine sees the
//     state it set;
//   * glPushClientAttrib / glPopClientAttrib save and restore the shadow with the native stack;
//   * the Windows crash handler prints the ring and the enabled arrays (k2shim_glarr_dump).
// The GL calls this makes go to the native entry points, never back through the hooks.
#include "k2shim.h"

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <atomic>
#include <set>
#include <string>
#include <vector>

void* k2shim_glarr_caller;

namespace {

enum { kFixed = 5, kTex = 8, kAttrib = 16, kSlots = kFixed + kTex + kAttrib };

const struct { const char* name; unsigned int cap; } kFixedArrays[kFixed] = {
    { "vertex", 0x8074 }, { "normal", 0x8075 }, { "colour", 0x8076 },
    { "secondary colour", 0x845E }, { "fog coordinate", 0x8457 },
};

struct Slot {
    bool on, autoOff;
    const void* ptr;
    int size, stride;
    unsigned int type, buffer;
    void* setter;
    unsigned int serial;              // when it was set (s_nSerial)
    std::atomic<bool> freed;
    Slot() : on(false), autoOff(false), ptr(0), size(0), stride(0), type(0), buffer(0), setter(0), serial(0),
             freed(false) {}
};

struct Saved {
    bool on, autoOff, freed;
    const void* ptr;
    int size, stride;
    unsigned int type, buffer;
    void* setter;
    unsigned int serial;
};

struct Draw {
    const char* name;
    unsigned int mode;
    int count;
    void* caller;
};

Slot s_slots[kSlots];
unsigned int s_nClientUnit;          // glClientActiveTexture, 0-based
unsigned int s_nArrayBuffer;         // GL_ARRAY_BUFFER binding
std::atomic<uintptr_t> s_lo{UINTPTR_MAX}, s_hi{0};   // enabled client pointers' range (for operator delete)
std::atomic<int> s_nFreed;
Draw s_ring[16];
unsigned int s_nRing;
unsigned int s_nSerial;
std::vector<std::pair<std::vector<Saved>, unsigned int>> s_stack;

int FixedSlot(unsigned int cap)
{
    if (cap == 0x8078) return kFixed + static_cast<int>(s_nClientUnit < kTex ? s_nClientUnit : kTex - 1);
    for (int i = 0; i < kFixed; i++)
        if (kFixedArrays[i].cap == cap) return i;
    return -1;
}

const char* SlotName(int i, char* buf, size_t n)
{
    if (i < kFixed) snprintf(buf, n, "%s", kFixedArrays[i].name);
    else if (i < kFixed + kTex) snprintf(buf, n, "texcoord %d", i - kFixed);
    else snprintf(buf, n, "attribute %d", i - kFixed - kTex);
    return buf;
}

const char* Where(void* p, char* buf, size_t n)
{
    Dl_info info;
    if (p && dladdr(p, &info) && info.dli_fname) {
        const char* base = strrchr(info.dli_fname, '/');
        const char* base2 = strrchr(info.dli_fname, '\\');
        if (base2 > base) base = base2;
        snprintf(buf, n, "%s+0x%llx", base ? base + 1 : info.dli_fname,
                 static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(p) - reinterpret_cast<uintptr_t>(info.dli_fbase)));
    } else {
        snprintf(buf, n, "%p", p);
    }
    return buf;
}

// Recomputes the range operator delete compares against, every enabled array sourced from client
// memory.
void Recompute()
{
    uintptr_t lo = UINTPTR_MAX, hi = 0;
    for (Slot& s : s_slots)
        if (s.on && s.buffer == 0 && s.ptr) {
            uintptr_t p = reinterpret_cast<uintptr_t>(s.ptr);
            if (p < lo) lo = p;
            if (p > hi) hi = p;
        }
    s_lo = lo;
    s_hi = hi;
}

typedef void (K2_GLAPI* CapFn)(unsigned int);

CapFn Native(const char* name)
{
    return reinterpret_cast<CapFn>(k2shim_gl_proc(name));
}

void NativeDisable(int i)
{
    static CapFn pDisable = Native("glDisableClientState");
    static CapFn pDisableAttrib = Native("glDisableVertexAttribArray");
    static CapFn pClientActive = Native("glClientActiveTexture");
    if (i < kFixed) {
        if (pDisable) pDisable(kFixedArrays[i].cap);
    } else if (i < kFixed + kTex) {
        if (pDisable && pClientActive) {
            pClientActive(0x84C0 + static_cast<unsigned int>(i - kFixed));
            pDisable(0x8078);
            pClientActive(0x84C0 + s_nClientUnit);
        }
    } else if (pDisableAttrib) {
        pDisableAttrib(static_cast<unsigned int>(i - kFixed - kTex));
    }
}

void NativeEnable(int i)
{
    static CapFn pEnable = Native("glEnableClientState");
    static CapFn pEnableAttrib = Native("glEnableVertexAttribArray");
    static CapFn pClientActive = Native("glClientActiveTexture");
    if (i < kFixed) {
        if (pEnable) pEnable(kFixedArrays[i].cap);
    } else if (i < kFixed + kTex) {
        if (pEnable && pClientActive) {
            pClientActive(0x84C0 + static_cast<unsigned int>(i - kFixed));
            pEnable(0x8078);
            pClientActive(0x84C0 + s_nClientUnit);
        }
    } else if (pEnableAttrib) {
        pEnableAttrib(static_cast<unsigned int>(i - kFixed - kTex));
    }
}

void SetOn(int i, bool on)
{
    if (i < 0) return;
    Slot& s = s_slots[i];
    s.on = on;
    s.autoOff = false;
    Recompute();
}

void SetPointer(int i, int size, unsigned int type, int stride, const void* p, void* caller)
{
    if (k2shim_glarr_caller) {
        caller = k2shim_glarr_caller;
        k2shim_glarr_caller = 0;
    }
    if (i < 0) return;
    Slot& s = s_slots[i];
    s.ptr = p;
    s.size = size;
    s.type = type;
    s.stride = stride;
    s.buffer = s_nArrayBuffer;
    s.setter = caller;
    s.serial = ++s_nSerial;
    s.freed = false;
    if (s.autoOff) {   // disabled by a draw below while the engine still had it on
        s.autoOff = false;
        s.on = true;
        NativeEnable(i);
    }
    Recompute();
}

} // namespace

void k2shim_glarr_enable(unsigned int cap, bool on) { SetOn(FixedSlot(cap), on); }

void k2shim_glarr_client_active(unsigned int unit) { s_nClientUnit = unit >= 0x84C0 ? unit - 0x84C0 : unit; }

void k2shim_glarr_attrib_enable(unsigned int index, bool on)
{
    if (index < kAttrib) SetOn(kFixed + kTex + static_cast<int>(index), on);
}

// A KotorProgram stops being used (unbound, or another one bound): the generic attribute arrays it
// switched on go off. NVIDIA fetches every enabled array at a draw, a fixed-function one included,
// and a skinned program's weight / index arrays live in its own buffer, shorter than the next draw's
// vertex count. The next program's SetupShader switches on what it reads. Attribute 0 aliases the
// fixed-function vertex array and is left alone.
void k2shim_glarr_attribs_off()
{
    for (int i = kFixed + kTex + 1; i < kSlots; i++) {
        if (!s_slots[i].on) continue;
        NativeDisable(i);
        s_slots[i].on = false;
    }
    Recompute();
}

void k2shim_glarr_bind(unsigned int target, unsigned int buffer)
{
    if (target == 0x8892) s_nArrayBuffer = buffer;   // GL_ARRAY_BUFFER
}

void k2shim_glarr_buffers_deleted(int n, const unsigned int* ids)
{
    for (int k = 0; k < n; k++) {
        if (ids[k] == 0) continue;
        if (ids[k] == s_nArrayBuffer) s_nArrayBuffer = 0;
        for (Slot& s : s_slots)
            if (s.on && s.buffer == ids[k]) s.on = s.autoOff = false;   // k2shim_gl_unbind_buffers disables it
    }
    Recompute();
}

void k2shim_glarr_pointer(unsigned int cap, int size, unsigned int type, int stride, const void* p, void* caller)
{
    SetPointer(FixedSlot(cap), size, type, stride, p, caller);
}

void k2shim_glarr_attrib_pointer(unsigned int index, int size, unsigned int type, int stride, const void* p,
                                 void* caller)
{
    if (index < kAttrib) SetPointer(kFixed + kTex + static_cast<int>(index), size, type, stride, p, caller);
}

void k2shim_glarr_push()
{
    std::vector<Saved> v;
    for (Slot& s : s_slots)
        v.push_back({ s.on, s.autoOff, s.freed.load(), s.ptr, s.size, s.stride, s.type, s.buffer, s.setter, s.serial });
    s_stack.push_back({ v, s_nClientUnit });
}

void k2shim_glarr_pop()
{
    if (s_stack.empty()) return;
    const std::vector<Saved>& v = s_stack.back().first;
    for (int i = 0; i < kSlots; i++) {
        Slot& s = s_slots[i];
        const Saved& o = v[static_cast<size_t>(i)];
        // A block freed since the push is still freed.
        bool bFreed = o.freed || (s.freed && s.ptr == o.ptr);
        s.on = o.on; s.autoOff = o.autoOff; s.ptr = o.ptr; s.size = o.size; s.stride = o.stride;
        s.type = o.type; s.buffer = o.buffer; s.setter = o.setter; s.serial = o.serial; s.freed = bFreed;
    }
    s_nClientUnit = s_stack.back().second;
    s_stack.pop_back();
    Recompute();
}

bool k2shim_glarr_watching(const void* p)
{
    uintptr_t u = reinterpret_cast<uintptr_t>(p), hi = s_hi;
    return hi && u <= hi && u + (static_cast<uintptr_t>(1) << 28) > s_lo;
}

void k2shim_glarr_freed(const void* p, size_t n)
{
    uintptr_t lo = reinterpret_cast<uintptr_t>(p), hi = lo + n;
    for (Slot& s : s_slots)
        if (s.on && s.buffer == 0 && reinterpret_cast<uintptr_t>(s.ptr) >= lo && reinterpret_cast<uintptr_t>(s.ptr) < hi && !s.freed) {
            s.freed = true;
            s_nFreed++;
        }
}

void k2shim_glarr_draw(const char* name, unsigned int mode, int count, void* caller)
{
    if (k2shim_glarr_caller) {
        caller = k2shim_glarr_caller;
        k2shim_glarr_caller = 0;
    }
    s_ring[s_nRing++ % 16] = { name, mode, count, caller };
    static unsigned int s_nDrawSerial;   // s_nSerial at the previous draw
    if (k2shim_gl_checking && s_slots[0].on && s_slots[0].serial > s_nDrawSerial) {
        // K2_GL_CHECK: this draw set its vertex array; an array on from client memory that was set
        // before the previous draw is left over from another one.
        static std::set<std::string> s_stale;
        for (int i = 1; i < kSlots; i++) {
            const Slot& s = s_slots[i];
            if (!s.on || s.buffer != 0 || s.serial > s_nDrawSerial) continue;
            char szName[32], szSetter[96], szVertex[96], szCaller[96];
            SlotName(i, szName, sizeof szName);
            Where(s.setter, szSetter, sizeof szSetter);
            Where(s_slots[0].setter, szVertex, sizeof szVertex);
            Where(caller, szCaller, sizeof szCaller);
            if (s_stale.insert(std::string(szName) + szSetter + szVertex).second)
                fprintf(stderr, "k2: gl: %s array on from client memory %p (set at %s) under a vertex array %s "
                        "(buffer %u, set at %s): %s count %d from %s\n", szName, s.ptr, szSetter,
                        s_slots[0].buffer ? "in a buffer" : "in client memory", s_slots[0].buffer, szVertex, name,
                        count, szCaller);
        }
    }
    s_nDrawSerial = s_nSerial;
    if (s_nFreed == 0) return;
    s_nFreed = 0;
    static std::set<std::string> s_reported;
    for (int i = 0; i < kSlots; i++) {
        Slot& s = s_slots[i];
        if (!s.freed) continue;
        if (!s.on || s.buffer != 0) { s.freed = false; continue; }
        NativeDisable(i);
        s.on = false;
        s.autoOff = true;
        s.freed = false;
        char szName[32], szSetter[96], szCaller[96];
        SlotName(i, szName, sizeof szName);
        Where(s.setter, szSetter, sizeof szSetter);
        Where(caller, szCaller, sizeof szCaller);
        std::string key = std::string(szName) + szSetter + szCaller;
        if (s_reported.insert(key).second)
            fprintf(stderr, "k2: gl: %s array enabled on freed memory %p (set at %s); disabled for the %s from %s\n",
                    szName, s.ptr, szSetter, name, szCaller);
    }
    Recompute();
}

void k2shim_glarr_dump(uintptr_t fault)
{
    char szName[32], szWhere[96];
    fprintf(stderr, "k2:   last GL draws (newest first):\n");
    for (unsigned int k = 0; k < 16 && k < s_nRing; k++) {
        const Draw& d = s_ring[(s_nRing - 1 - k) % 16];
        fprintf(stderr, "k2:     %s mode %u count %d from %s\n", d.name, d.mode, d.count,
                Where(d.caller, szWhere, sizeof szWhere));
    }
    fprintf(stderr, "k2:   enabled arrays (array buffer %u, client unit %u):\n", s_nArrayBuffer, s_nClientUnit);
    for (int i = 0; i < kSlots; i++) {
        const Slot& s = s_slots[i];
        if (!s.on) continue;
        uintptr_t p = reinterpret_cast<uintptr_t>(s.ptr);
        fprintf(stderr, "k2:     %s %p size %d type 0x%x stride %d buffer %u%s set at %s", SlotName(i, szName, sizeof szName),
                s.ptr, s.size, s.type, s.stride, s.buffer, s.freed ? " (freed)" : "",
                Where(s.setter, szWhere, sizeof szWhere));
        if (s.buffer == 0 && fault >= p && fault - p < (static_cast<uintptr_t>(1) << 24))
            fprintf(stderr, " <- fault at +0x%llx", static_cast<unsigned long long>(fault - p));
        fprintf(stderr, "\n");
    }
}
