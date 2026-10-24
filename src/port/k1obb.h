// K1 only: reader for the Android OBB expansion archives. A file the filesystem does not have is
// looked up in the patch OBB, then the main OBB.
#ifndef K1OBB_H
#define K1OBB_H

#if KOTOR1
#include <string>
#include <vector>
#include "sdlrwops.h"

struct mz_zip_archive;
extern "C" int mz_zip_reader_locate_file(mz_zip_archive* pZip, const char* pName,
                                         const char* pComment, unsigned int flags);

struct ObbEntry {
    unsigned char m_pad00[0x30];
    unsigned long long m_nTime;
    unsigned char m_pad38[0x470 - 0x38];
    int m_bDirectory;
    unsigned char m_pad474[0x498 - 0x474];
    std::string m_sName;
    unsigned char m_pad4b0[0x4c8 - 0x4b0];
    std::vector<ObbEntry*> m_aChildren;
};

class ObbFile {
public:
    ObbFile(const char* pPath, bool bPatch);
    ~ObbFile();
    SDL_RWops* RWFromFile(const char* pName);

    mz_zip_archive* GetZip() { return reinterpret_cast<mz_zip_archive*>(this); }

    unsigned char m_zip[0xa0];
    ObbEntry** m_ppEntries;
    unsigned char m_padA8[0xc0 - 0xa8];
};
K2_LAYOUT_ASSERT(sizeof(ObbFile) == 0xc0, "K1 ObbFile size");

extern ObbFile* g_mainObb;
extern ObbFile* g_patchObb;
extern volatile bool g_obbMounted;
extern volatile bool g_patchObbMounted;
#endif

#endif
