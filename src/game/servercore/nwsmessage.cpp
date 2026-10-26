#include "k2stdint.h"
#include "nwssendplayermessage.h"
#include "gameobjectarray.h"
#include "appmanager.h"
#include "serverexoapp.h"
#include "exolocstring.h"
#include "exolocstringinternal.h"
#include "exostring.h"

void ByteSwap(unsigned long long* p);
#include "port/libcbase.h"
#include "port/libcstring.h"
#include "exostringinline.h"

// Byte-swaps an object id into network order, setting the list-type bit unless the id is invalid.
void CSWSMessage::WriteOBJECTIDServer(unsigned long long nObjectId)
{
    unsigned long long nValue = nObjectId;
    if (m_nWriteMessageSize + sizeof(nValue) > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(sizeof(nValue));

    if (nObjectId != CGameObjectArray::INVALIDOBJID)
        nValue = nObjectId | CGameObjectArray::LISTTYPE_MASK;
    ByteSwap(&nValue);
    memcpy(&m_pWriteMessage[m_nWriteMessageSize], &nValue, sizeof(nValue));
    m_nWriteMessageSize += sizeof(nValue);
    m_nBytesWritten += sizeof(nValue);
}

// Reads an object id and clears its list-type bit. An overrunning read still advances the cursor
// and yields 0.
unsigned long long CSWSMessage::ReadOBJECTIDServer()
{
    if (m_nReadMessagePos + sizeof(unsigned long long) > m_nReadMessageSize) {
        m_nReadMessagePos += sizeof(unsigned long long);
        return 0;
    }

    unsigned long long nValue;
    memcpy(&nValue, &m_pReadMessage[m_nReadMessagePos], sizeof(nValue));
    ByteSwap(&nValue);
    m_nReadMessagePos += 8;
    nValue = nValue & 0x7fffffff;
    return nValue;
}

// Writes a localized string: picks the first variant that resolves for this player's language and
// writes it as an unlocalized string, or writes the string ref when none does.
void CSWSMessage::WriteCExoLocStringServer(CExoLocString& s, unsigned char nLang)
{
    CExoString sString;

    if (g_pAppManager->m_pServerExoApp != 0)
    {
        int nPlayerLanguage;
        int nModuleLanguage;
        nPlayerLanguage = g_pAppManager->m_pServerExoApp->GetPlayerLanguage(m_nPlayerId);
        nModuleLanguage = g_pAppManager->m_pServerExoApp->GetModuleLanguage();
        if (s.GetStringLoc(nPlayerLanguage, &sString, nLang))
        {
            if (static_cast<uint64_t>(m_nWriteMessageSize) +
                    sString.GetLength() + 4 >
                m_nWriteMessageBufferSize)
                ExtendWriteBuffer(sString.GetLength() + 4);
            WriteBOOL(0);
            WriteCExoString(sString, 32);
        }
        else if (nPlayerLanguage && s.GetStringLoc(0, &sString, nLang))
        {
            if (static_cast<uint64_t>(m_nWriteMessageSize) +
                    sString.GetLength() + 4 >
                m_nWriteMessageBufferSize)
                ExtendWriteBuffer(sString.GetLength() + 4);
            WriteBOOL(0);
            WriteCExoString(sString, 32);
        }
        else if (nModuleLanguage != 0 && nModuleLanguage != nPlayerLanguage &&
                 s.GetStringLoc(nModuleLanguage, &sString, nLang))
        {
            if (static_cast<uint64_t>(m_nWriteMessageSize) +
                    sString.GetLength() + 4 >
                m_nWriteMessageBufferSize)
                ExtendWriteBuffer(sString.GetLength() + 4);
            WriteBOOL(0);
            WriteCExoString(sString, 32);
        }
        else
        {
            unsigned char nFlag;
            if (s.GetString(0, &nPlayerLanguage, &sString, &nFlag))
            {
                if (static_cast<uint64_t>(m_nWriteMessageSize) +
                        sString.GetLength() + 4 >
                    m_nWriteMessageBufferSize)
                    ExtendWriteBuffer(sString.GetLength() + 4);
                WriteBOOL(0);
                WriteCExoString(sString, 32);
            }
            else if (s.m_nStrRef == 0xFFFFFFFF)
            {
                if (sString.GetLength() +
                    m_nWriteMessageSize > m_nWriteMessageBufferSize)
                    ExtendWriteBuffer(sString.GetLength());
                WriteBOOL(0);
                WriteCExoString(sString, 32);
            }
            else
            {
                if (static_cast<uint64_t>(m_nWriteMessageSize) + 9 > m_nWriteMessageBufferSize)
                    ExtendWriteBuffer(9);
                WriteBOOL(1);
                WriteBYTE(nLang, 8);
                WriteDWORD(static_cast<unsigned int>(s.m_nStrRef), 32);
            }
        }
    }
}

// Joins a creature's first and last name in one player's language.
bool CSWSMessage::GetLocStringServer(unsigned int playerId, CExoLocString& in,
                                     CExoLocString& out, CExoString& outStr, unsigned char flag)
{
    CExoString s1;
    CExoString s2;
    outStr = "";
    if (g_pAppManager->m_pServerExoApp != 0)
    {
        int nLanguage = g_pAppManager->m_pServerExoApp->GetPlayerLanguage(playerId);
        in.GetString(nLanguage, &s1, flag);
        out.GetString(nLanguage, &s2, flag);
        if (!s2.IsEmpty() && !s1.IsEmpty())
        {
            outStr = s1 + " " + s2;
        }
        else
        {
            outStr = s1 + s2;
        }
        return true;
    }
    return false;
}

// Resolves a localized string in one player's language, falling back to the talk table, and adds
// the resolved entry's sound length to fTime.
bool CSWSMessage::GetLocStringServerTime(unsigned int playerId, CExoLocString& in,
                                         CExoString& outStr, float& fTime, unsigned char flag)
{
    CExoString s1;
    CExoString s2;
    STR_RES sres;
    int nPlayerLanguage;
    int nModuleLanguage;
    outStr = "";
    fTime = 0;
    if (g_pAppManager->m_pServerExoApp != 0)
    {
        nPlayerLanguage = g_pAppManager->m_pServerExoApp->GetPlayerLanguage(playerId);
        nModuleLanguage = g_pAppManager->m_pServerExoApp->GetModuleLanguage();
        if (in.GetStringLoc(nPlayerLanguage, &outStr, flag) ||
            (nPlayerLanguage && in.GetStringLoc(0, &outStr, flag)) ||
            (nModuleLanguage && nModuleLanguage != nPlayerLanguage &&
             in.GetStringLoc(nModuleLanguage, &outStr, flag)))
        {
            // resolved directly from the locstring, nothing more to do
        }
        else
        {
            unsigned char nFlag;
            if (in.GetString(0, &nPlayerLanguage, &outStr, &nFlag))
            {
                // resolved directly from the locstring, nothing more to do
            }
            else if (in.m_nStrRef != 0xFFFFFFFF)
            {
                g_pTlkTable->Fetch(static_cast<unsigned int>(in.m_nStrRef), sres, nFlag);
                outStr = sres.text;
                fTime += sres.fSoundLength;
            }
        }
        return true;
    }
    return false;
}
