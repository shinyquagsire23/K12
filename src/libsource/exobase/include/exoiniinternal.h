#ifndef CEXOINI_H
#define CEXOINI_H

#include "exostring.h"
#include "CExoLinkedList.h"
#include "exolinkedlistinternal.h"

// One line held in a section's key sublist.
struct CFileKey {
    CExoString m_line;
};

struct CFileSection {
    CExoString               m_line;
    CExoLinkedList<CFileKey> m_keys;
};

// The game's settings file.
#if KOTOR1
#define EXO_GAME_INI "swkotor.ini"
#else
#define EXO_GAME_INI ".\\swkotor2.ini"
#endif

class CExoIniInternal {
public:
    CExoLinkedList<CFileSection> m_sections;
    CExoString                   m_sCachedFileName;

    CExoIniInternal();
    ~CExoIniInternal();
    void Destroy();

    // Both return success: 1 = the key was handled or found (result written / entry written out),
    // 0 = not found or the ini file could not be opened.
    int ReadIniEntry(CExoString& result, const CExoString& fileName, const CExoString& section, const CExoString& key);
    int WriteIniEntry(const CExoString& value, const CExoString& fileName, const CExoString& section, const CExoString& key);
};

class CExoIni {
public:
    CExoIni();
    ~CExoIni();

    int ReadIniEntry(CExoString& result, const CExoString& fileName, const CExoString& section, const CExoString& key);
    int WriteIniEntry(const CExoString& value, const CExoString& fileName, const CExoString& section, const CExoString& key);

private:
    CExoIniInternal* m_pInternal;
};

// Platform preference store (Steam/Android native settings), used for the ini keys that do not live
// in the flat file.
namespace ASL {
    bool GetPrefAsBool(const char* key, bool defaultValue);
}

#endif
