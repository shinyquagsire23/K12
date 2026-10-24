// Reports from the K2_FIX_ENGINE_BUGS branches: each one is a state the engine would mishandle or
// silently drop, so it goes to stderr (logcat tag k2 on Android) instead of passing silently. src/
// declares these itself, as it includes no system headers.
#include <stdio.h>

// A moved object that is not in its area's X-sorted index. The engine swaps index[-1] into slot 0,
// and a creature missing from the index is never found by another creature's perception scan, so the
// player is never sent it: an invisible, untargetable creature. Logged once per object id in a row,
// since moves are per frame.
extern "C" void k2_fix_object_not_indexed(unsigned long long objectId, int nObjectType, float fX,
                                          float fY)
{
    static unsigned long long s_lastId = ~0ull;
    if (objectId == s_lastId)
        return;
    s_lastId = objectId;
    fprintf(stderr, "k2: fix: object %llx (type %d) at (%.2f, %.2f) moved but is not in its area's "
                    "X index\n", objectId, nObjectType, fX, fY);
}

// No room was found for a point, so the engine reads m_paRoomTableIndex[-1] and uses that byte to
// index the room rating and weather tables. Logged once: repopulating the action bar reaches this
// path, so a standing spot without a room would otherwise report on every repopulation.
extern "C" void k2_fix_room_index_missing(float fX, float fY)
{
    static int s_bLogged = 0;
    if (s_bLogged)
        return;
    s_bLogged = 1;
    fprintf(stderr, "k2: fix: no room at (%.2f, %.2f) -- the orig indexes m_paRoomTableIndex[-1] "
                    "here (docs/engine-bugs.md); reporting rating 0/weather 0\n", fX, fY);
}

// A record tag the game-object update loop does not know. The batch is a run of tagged records whose
// handlers' results are ignored, so a record read with the wrong field widths leaves the cursor
// mid-record and the next tag is garbage; the rest of the batch is then dropped silently. A dropped
// 'A' leaves the server holding a snapshot of an object the client never created. The record before
// the bad tag is the one whose reader and writer disagree; its bytes are dumped to decode by hand.
extern "C" void k2_diag_bad_update_record(unsigned char nTag, unsigned int nRecords,
                                          const unsigned char* aTag, const unsigned char* aType,
                                          const unsigned long long* aId, const unsigned int* aPos,
                                          const unsigned int* aMask, unsigned int nPos,
                                          unsigned int nSize, const unsigned char* pMessage)
{
    fprintf(stderr, "k2: diag: game-object update desynced: tag 0x%02x at record %u (cursor %u of "
                    "%u); the rest of the batch is dropped. Last records:\n",
            nTag, nRecords, nPos, nSize);
    unsigned int nFirst = nRecords > 4 ? nRecords - 4 : 0;
    for (unsigned int i = nFirst; i < nRecords; i++) {
        unsigned int k = i & 3;
        unsigned char c = aTag[k];
        fprintf(stderr, "k2: diag:   #%u '%c' (0x%02x) start %u type %u id %llx mask %08x\n", i,
                c >= 0x20 && c < 0x7f ? c : '?', c, aPos[k], aType[k], aId[k], aMask[k]);
    }
    if (pMessage == 0 || nRecords < 2)
        return;
    // The suspect record runs from its own start to the bad tag; show 16 bytes past the tag as well.
    unsigned int nFrom = aPos[(nRecords - 2) & 3];
    unsigned int nTo = nPos + 16 < nSize ? nPos + 16 : nSize;
    for (unsigned int nRow = nFrom; nRow < nTo; nRow += 32) {
        char line[32 * 3 + 1];
        unsigned int n = 0;
        for (unsigned int i = nRow; i < nTo && i < nRow + 32; i++)
            n += snprintf(line + n, sizeof(line) - n, "%02x ", pMessage[i]);
        line[n] = 0;
        fprintf(stderr, "k2: diag:   %5u: %s\n", nRow, line);
    }
}
