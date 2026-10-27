#include "k2stdint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#if KOTOR1
#include "port/k1obb.h"
#endif

#include "port/asllanguage.h"

class IosLocalizedStringsImpl
{
public:
    IosLocalizedStringsImpl(const char* pFileName, bool bDecodeUtf8);
    const char* GetString(int nIndex, int nStringsPerLanguage);

private:
    std::vector<std::string> m_aStrings;
};

#include "port/ios/IosLocalizedStrings.h"

// UTF-8: the smallest lead byte, a continuation byte's payload bits, and the byte-order mark.
enum {
    UTF8_LEAD_MIN = 0xc0,
    UTF8_PAYLOAD_MASK = 0x3f,
    UTF8_BOM_0 = 0xef,
    UTF8_BOM_1 = 0xbb,
    UTF8_BOM_2 = 0xbf,
};

// The longest line the table format allows, terminator included.
enum {
    LOCALIZED_LINE_SIZE = 512,
};

// Two-byte UTF-8 to single-byte: a lead byte starts a sequence whose low 6 bits are spliced in
// under it; every other byte is copied through unchanged.
void ConvertUtfToAscii(const unsigned char* pSrc, unsigned char* pDst)
{
    unsigned char c = *pSrc;
    while (c != 0)
    {
        if (c >= UTF8_LEAD_MIN)
        {
            pSrc++;
            c = static_cast<unsigned char>((*pSrc & UTF8_PAYLOAD_MASK) | (c << 6));
        }
        *pDst = c;
        pSrc++;
        pDst++;
        c = *pSrc;
    }
    *pDst = 0;
}

#if KOTOR1
// K1 reads the table out of the patch OBB, else the main OBB, 512 bytes at a time from a running
// offset, one line per pass. The read buffer is never NUL-terminated, so a line ends only on a
// short read.
IosLocalizedStringsImpl::IosLocalizedStringsImpl(const char* pFileName, bool bDecodeUtf8)
{
    std::string sPath = std::string("./") + pFileName;
    SDL_RWops* pFile;
    if ((g_patchObbMounted && (pFile = g_patchObb->RWFromFile(sPath.c_str())) != 0) ||
        (g_obbMounted && (pFile = g_mainObb->RWFromFile(sPath.c_str())) != 0))
    {
        int nOffset = 0;
        bool bDone;
        do
        {
            char szBuffer[512];
            int nRead;
            if (SDL_RWseek(pFile, nOffset, RW_SEEK_SET) == -1)
                break;
            nRead = static_cast<int>(SDL_RWread(pFile, szBuffer, 1, LOCALIZED_LINE_SIZE));
            std::string sLine;
#ifdef K2_FIX_ENGINE_BUGS
            // Only the bytes just read are the line buffer.
            std::string sBuffer(szBuffer, nRead > 0 ? nRead : 0);
#else
            std::string sBuffer(szBuffer);
#endif
            int nEnd = static_cast<int>(sBuffer.find('\n'));
            if (nEnd != -1)
            {
                sLine = sBuffer.substr(0, nEnd);
                const unsigned char* p = reinterpret_cast<const unsigned char*>(sLine.c_str());
                if (*p == UTF8_BOM_0) p++;
                if (*p == UTF8_BOM_1) p++;
                if (*p == UTF8_BOM_2) p++;
                if (bDecodeUtf8)
                {
                    char szDecoded[512];
                    ConvertUtfToAscii(p, reinterpret_cast<unsigned char*>(szDecoded));
                    m_aStrings.push_back(std::string(szDecoded));
                }
                else
                {
                    m_aStrings.push_back(std::string(reinterpret_cast<const char*>(p)));
                }
                bDone = false;
                nOffset += nEnd + 1;
            }
            else
            {
                bDone = nRead != LOCALIZED_LINE_SIZE;
            }
        } while (!bDone);
        SDL_RWclose(pFile);
    }
}
#else
IosLocalizedStringsImpl::IosLocalizedStringsImpl(const char* pFileName, bool bDecodeUtf8)
{
    FILE* pFile = fopen(pFileName, "r");
    if (pFile == 0) return;

    fflush(pFile);
    long nStart = ftell(pFile);
    int nSize;
    if (static_cast<int>(nStart) < 0)
    {
        nSize = 0;
    }
    else
    {
        fseek(pFile, 0, SEEK_END);
        nSize = static_cast<int>(ftell(pFile));
        fseek(pFile, static_cast<int>(nStart), SEEK_SET);
    }
#ifdef K2_FIX_ENGINE_BUGS
    if (nSize < 0)
        nSize = 0;
    char* pszBuffer = static_cast<char*>(malloc(nSize + 1));
    if (pszBuffer == 0)
    {
        fclose(pFile);
        return;
    }
    size_t nRead = fread(pszBuffer, 1, static_cast<size_t>(nSize), pFile);
    K2_ASSUME(nRead <= static_cast<size_t>(nSize));   // fread returns at most the count it was given
    pszBuffer[nRead] = 0;
#else
    char* pszBuffer = static_cast<char*>(malloc(nSize));
    fread(pszBuffer, 1, nSize, pFile);
#endif

    char* pszLine = strtok(pszBuffer, "\n");
    while (pszLine != 0)
    {
        const unsigned char* p = reinterpret_cast<const unsigned char*>(pszLine);
        if (*p == UTF8_BOM_0) p++;
        if (*p == UTF8_BOM_1) p++;
        if (*p == UTF8_BOM_2) p++;
        // Clamps the token to the longest line the format allows.
#ifdef K2_FIX_ENGINE_BUGS
        if (strlen(pszLine) > LOCALIZED_LINE_SIZE - 1)
#endif
        pszLine[LOCALIZED_LINE_SIZE - 1] = 0;

        char szDecoded[512];
        if (bDecodeUtf8)
        {
            ConvertUtfToAscii(p, reinterpret_cast<unsigned char*>(szDecoded));
            m_aStrings.push_back(std::string(szDecoded));
        }
        else
        {
            m_aStrings.push_back(std::string(reinterpret_cast<const char*>(p)));
        }

        pszLine = strtok(0, "\n");
    }

    free(pszBuffer);
    fclose(pFile);
}
#endif

// The file is one flat run of strings per language, and the language comes straight from the
// platform, clamped to the first four; nStringsPerLanguage is the stride.
const char* IosLocalizedStringsImpl::GetString(int nIndex, int nStringsPerLanguage)
{
    // `<=` rather than `>`: only that spelling keeps the keep-arm on the true edge.
    int nLanguage = ASLPlat_GetCurrentLanguage();
    nLanguage = (static_cast<unsigned int>(nLanguage - ASL_LANGUAGE_FRENCH)
                 <= ASL_LANGUAGE_SPANISH - ASL_LANGUAGE_FRENCH) ? nLanguage : ASL_LANGUAGE_ENGLISH;

    uint64_t nWhich = static_cast<uint64_t>(nIndex + nLanguage * nStringsPerLanguage);
    if (nWhich >= m_aStrings.size()) return "";
    return m_aStrings[nWhich].c_str();
}

const char* IosLocalizedStrings::GetString(int nIndex, int nStringsPerLanguage)
{
    return m_pImpl->GetString(nIndex, nStringsPerLanguage);
}

IosLocalizedStrings::IosLocalizedStrings(const char* pFileName, bool bLoadNow)
{
    m_pImpl = new IosLocalizedStringsImpl(pFileName, bLoadNow);
}

IosLocalizedStrings::~IosLocalizedStrings()
{
    delete m_pImpl;
}
