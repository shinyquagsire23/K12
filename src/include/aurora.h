#ifndef CAURORA_H
#define CAURORA_H

// The renderer front end. NewCAurora() builds the concrete CAurInternal, which the game keeps in
// g_pAurora. Calls through it need aurinternal.h.
class CAurInternal;

CAurInternal* NewCAurora();
void DestroyCAurora();

extern CAurInternal* g_pAurora;

#endif // CAURORA_H
