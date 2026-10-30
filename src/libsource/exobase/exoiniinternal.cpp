#include "k2stdint.h"
#include "exoiniinternal.h"

#include <stddef.h>
#include <stdio.h>
#if KOTOR1
#include "sdlrwops.h"
#endif

#define EXOINI_FILE const_cast<char*>("C:/Users/Vu/Documents/AndroidStudioProjects/KOTOR1-2/KOTOR2_Android/app/src/main/cpp/kotor2/CODE/libsource/exobase/exoini.cpp")

#include "port/libcbase.h"
#include "port/libcstring.h"

// An ini line's buffer: the longest line the reader and writer handle, terminator included.
enum {
    INI_LINE_SIZE = 512,
};

template class CExoLinkedList<CFileSection>;

CExoIniInternal::CExoIniInternal()
{
    m_sCachedFileName = "";
}

CExoIniInternal::~CExoIniInternal()
{
    Destroy();
}

void CExoIniInternal::Destroy()
{
    while (m_sections.m_pInternal->m_pHead != 0) {
        CFileSection* pSection = m_sections.RemoveHead();
        while (pSection->m_keys.m_pInternal->m_pHead != 0) {
            CFileKey* pKey = pSection->m_keys.RemoveHead();
            delete pKey;
        }
        delete pSection;
    }
    m_sCachedFileName = "";
}

int CExoIniInternal::ReadIniEntry(CExoString& result, const CExoString& fileName, const CExoString& section, const CExoString& key)
{
#if KOTOR1  // K1 has no port key overrides and keeps the line buffer on the stack
    char lineBuf[INI_LINE_SIZE];
#else
    if (strcmp(key.CStr(), "AllowWindowedMode") == 0 ||
        strcmp(key.CStr(), "AllowHighMonitorFrequency") == 0) {
        result = "1";
        return 1;
    }
    if (strcmp(key.CStr(), "FullScreen") == 0) {
        result = CExoString(static_cast<int>(ASL::GetPrefAsBool("DisplayFullScreen", true)));
        return 1;
    }

    char* lineBuf = new char[INI_LINE_SIZE];  // sic: never freed
#endif
    char fileNameBuf[96];  sprintf(fileNameBuf, "%s", fileName.CStr());
    char sectionBuf[96];   sprintf(sectionBuf, "%s", section.CStr());
    char keyBuf[96];       sprintf(keyBuf, "%s", key.CStr());

    if (!m_sCachedFileName.CompareNoCase(fileNameBuf))
        Destroy();

    if (m_sections.m_pInternal->m_pHead == 0 || m_sections.m_pInternal->m_pHead->m_pData == 0) {
        m_sCachedFileName = fileName;
#if KOTOR1  // K1 uses an SDL stream, read through its stdio FILE
        SDL_RWops* rw = SDL_RWFromFile(fileNameBuf, "r");
        if (rw == 0)
            return 0;
        FILE* f = static_cast<FILE*>(rw->hidden.stdio.fp);
        if (f == 0)
            return 0;
#else
        FILE* f = fopen(fileNameBuf, "r");
        if (f == 0)
            return 0;
#endif

        while (fgets(lineBuf, INI_LINE_SIZE, f) != 0) {
            if (lineBuf[0] == '[') {
                CFileSection* pSection = new CFileSection;
                pSection->m_line = lineBuf;
                m_sections.AddTail(pSection);
                while (fgets(lineBuf, INI_LINE_SIZE, f) != 0) {
                    if (lineBuf[0] == '[') {
                        // sic: the rewind goes one byte further back than the line is long, so the
                        // '[' is re-read by the outer fgets.
                        int len = static_cast<int>(__strlen_chk(lineBuf, INI_LINE_SIZE));
                        fseek(f, ~static_cast<long>(len), SEEK_CUR);
                        break;
                    }
                    CFileKey* pEntry = new CFileKey;
                    pEntry->m_line = lineBuf;
                    pSection->m_keys.AddTail(pEntry);
                }
            }
        }
#if KOTOR1
        SDL_RWclose(rw);
#else
        fclose(f);
#endif
        if (m_sections.m_pInternal->m_pHead == 0)
            return 0;
    }

    CFileSection* pSection = m_sections.m_pInternal->m_pHead ? static_cast<CFileSection*>(m_sections.m_pInternal->m_pHead->m_pData) : 0;
    CExoLinkedListNode* secPos = m_sections.m_pInternal->m_pHead;
    while (pSection != 0) {
        const char* line = pSection->m_line.CStr();
        __strcpy_chk(lineBuf, line, INI_LINE_SIZE);
        char* pEnd = __strchr_chk(lineBuf, ']', INI_LINE_SIZE);
        if (pEnd)
            *pEnd = 0;
        char* pStart = __strchr_chk(lineBuf, '[', INI_LINE_SIZE);
        if (!pStart)
            pStart = lineBuf;
        if (strcmp(pStart + 1, sectionBuf) == 0) {
            CExoLinkedListNode* entPos = pSection->m_keys.m_pInternal->m_pHead;
            if (entPos == 0 || entPos->m_pData == 0)
                return 0;
            CFileKey* pEntry = static_cast<CFileKey*>(entPos->m_pData);
            while (pEntry != 0) {
                const char* entryLine = pEntry->m_line.CStr();
                __strcpy_chk(lineBuf, entryLine, INI_LINE_SIZE);
                char* pEq = __strchr_chk(lineBuf, '=', INI_LINE_SIZE);
                if (pEq)
                    *pEq = 0;
                if (strcmp(lineBuf, keyBuf) == 0) {
                    const char* fullLine = pEntry->m_line.CStr();
                    const char* pRealEq = strchr(fullLine, '=');
                    result = pRealEq + 1;
                    // sic: an unnamed temporary, destroyed before the comparison runs.
                    if (result.Right(1) == "\n") {
#if KOTOR1  // K1's GetLength is out of line
                        result = result.Left(result.GetLength() - 1);
#else
                        int len = 0;
                        if (result.m_sString != 0)
                            len = static_cast<int>(strlen(result.m_sString));
                        result = result.Left(len - 1);
#endif
                    }
                    return 1;
                }
                pEntry = pSection->m_keys.GetNext(entPos);
            }
            return 0;
        }
        pSection = static_cast<CFileSection*>(m_sections.GetNext(secPos));
    }
    return 0;
}

int CExoIniInternal::WriteIniEntry(const CExoString& value, const CExoString& fileName, const CExoString& section, const CExoString& key)
{
    char sectionBuf[96];  sprintf(sectionBuf, "%s", fileName.CStr());
    char keyBuf[96];      sprintf(keyBuf, "%s", section.CStr());
    char valBufSection[96]; sprintf(valBufSection, "%s", key.CStr());
    char valueBuf[INI_LINE_SIZE]; sprintf(valueBuf, "%s", value.CStr());

    if (!m_sCachedFileName.CompareNoCase(sectionBuf))
        Destroy();

    if (m_sections.m_pInternal->m_pHead == 0 || m_sections.m_pInternal->m_pHead->m_pData == 0) {
        m_sCachedFileName = fileName;
#if KOTOR1  // K1 uses SDL streams, read and written through their stdio FILE
        SDL_RWops* rw = SDL_RWFromFile(sectionBuf, "r");
        FILE* f = rw != 0 ? static_cast<FILE*>(rw->hidden.stdio.fp) : 0;
        if (f == 0) {
            rw = SDL_RWFromFile(sectionBuf, "w");
            if (rw == 0)
                return 0;
            f = static_cast<FILE*>(rw->hidden.stdio.fp);
            if (f == 0)
                return 0;
        }
#else
        FILE* f = fopen(sectionBuf, "r");
        if (f == 0) {
            f = fopen(sectionBuf, "w");
            if (f == 0)
                return 0;
        }
#endif

        char lineBuf[INI_LINE_SIZE];
        while (fgets(lineBuf, INI_LINE_SIZE, f) != 0) {
            if (lineBuf[0] == '[') {
                CFileSection* pSection = new CFileSection;
                pSection->m_line = lineBuf;
                m_sections.AddTail(pSection);
                while (fgets(lineBuf, INI_LINE_SIZE, f) != 0) {
                    if (lineBuf[0] == '[') {
                        // sic: the rewind goes one byte further back than the line is long, so the
                        // '[' is re-read by the outer fgets.
                        int len = static_cast<int>(__strlen_chk(lineBuf, INI_LINE_SIZE));
                        fseek(f, ~static_cast<long>(len), SEEK_CUR);
                        break;
                    }
                    CFileKey* pEntry = new CFileKey;
                    pEntry->m_line = lineBuf;
                    pSection->m_keys.AddTail(pEntry);
                }
            }
        }
#if KOTOR1
        SDL_RWclose(rw);
#else
        fclose(f);
#endif
    }

    CFileSection* pSection = m_sections.m_pInternal->m_pHead ? static_cast<CFileSection*>(m_sections.m_pInternal->m_pHead->m_pData) : 0;
    CExoLinkedListNode* secPos = m_sections.m_pInternal->m_pHead;
    char lineBuf[INI_LINE_SIZE];
    while (pSection != 0) {
        const char* line = pSection->m_line.CStr();
        __strcpy_chk(lineBuf, line, INI_LINE_SIZE);
        char* pEnd = __strchr_chk(lineBuf, ']', INI_LINE_SIZE);
        if (pEnd)
            *pEnd = 0;
        char* pStart = __strchr_chk(lineBuf, '[', INI_LINE_SIZE);
        if (!pStart)
            pStart = lineBuf;
        if (strcmp(pStart + 1, keyBuf) == 0) {
            CExoLinkedListNode* entPos = pSection->m_keys.m_pInternal->m_pHead;
            CFileKey* pEntry = entPos ? static_cast<CFileKey*>(entPos->m_pData) : 0;
            while (pEntry != 0) {
                const char* entryLine = pEntry->m_line.CStr();
                __strcpy_chk(lineBuf, entryLine, INI_LINE_SIZE);
                char* pEq = __strchr_chk(lineBuf, '=', INI_LINE_SIZE);
                if (pEq)
                    *pEq = 0;
                if (strcmp(lineBuf, valBufSection) == 0) {
                    if (pEq == 0) {
                        __builtin___strcat_chk(lineBuf, "=", INI_LINE_SIZE);
                        __builtin___strcat_chk(lineBuf, valueBuf, INI_LINE_SIZE);
                    } else {
                        strcpy(pEq + 1, valueBuf);
                        *pEq = '=';
                    }
                    pEntry->m_line = lineBuf;
                    pEntry->m_line = pEntry->m_line + "\n";
                    goto write_out;
                }
                pEntry = pSection->m_keys.GetNext(entPos);
            }
            pEntry = new CFileKey;
            pEntry->m_line = pEntry->m_line + valBufSection;
            pEntry->m_line = pEntry->m_line + "=";
            pEntry->m_line = pEntry->m_line + valueBuf;
            pEntry->m_line = pEntry->m_line + "\n";
            pSection->m_keys.AddHead(pEntry);
            goto write_out;
        }
        pSection = static_cast<CFileSection*>(m_sections.GetNext(secPos));
    }

    {
        CFileSection* pNewSection = new CFileSection;
        pNewSection->m_line = "\n[";
        pNewSection->m_line = pNewSection->m_line + keyBuf;
        pNewSection->m_line = pNewSection->m_line + "]\n";

        CFileKey* pEntry = new CFileKey;
        pEntry->m_line = valBufSection;
        pEntry->m_line = pEntry->m_line + "=";
        pEntry->m_line = pEntry->m_line + valueBuf;
        pEntry->m_line = pEntry->m_line + "\n";

        pNewSection->m_keys.AddHead(pEntry);
        m_sections.AddTail(pNewSection);
    }

write_out:
    {
#if KOTOR1
        SDL_RWops* rw = SDL_RWFromFile(sectionBuf, "w");
        FILE* f = rw != 0 ? static_cast<FILE*>(rw->hidden.stdio.fp) : 0;
#else
        FILE* f = fopen(sectionBuf, "w");
#endif
        if (f == 0) {
            CExoString msg;
            msg.Format(const_cast<char*>("ERROR: %s file is marked read only! Are you sure that you are running from the install location and not from the network?"), sectionBuf);
            // sic: only the string is used; the result is discarded.
            msg.CStr();
            return 0;
        }
        CExoLinkedListNode* wSecPos = m_sections.m_pInternal->m_pHead;
        CFileSection* wSection = wSecPos ? static_cast<CFileSection*>(wSecPos->m_pData) : 0;
        while (wSection != 0) {
            fputs(wSection->m_line.CStr(), f);
            CExoLinkedListNode* wEntPos = wSection->m_keys.m_pInternal->m_pHead;
            CFileKey* wEntry = wEntPos ? static_cast<CFileKey*>(wEntPos->m_pData) : 0;
            while (wEntry != 0) {
                fputs(wEntry->m_line.CStr(), f);
                wEntry = wSection->m_keys.GetNext(wEntPos);
            }
            wSection = static_cast<CFileSection*>(m_sections.GetNext(wSecPos));
        }
#if KOTOR1
        SDL_RWclose(rw);
#else
        fclose(f);
#endif
    }
    return 1;
}
