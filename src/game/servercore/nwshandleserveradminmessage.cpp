
#include "serverexoappinternal.h"
#include "restype.h"
#include <string.h>
#include "exostringinline.h"

static int GetTokenPair(unsigned char* pBuffer, unsigned int nSize,
                        CExoString& sToken0, CExoString& sToken1, CExoString& sToken2);   // below

// Dispatches an 'S'-prefixed admin-console message through a long if-chain of ParseToken /
// ParseGetString calls, one arm per command. An unhandled command answers 0, every handled one
// answers 1.
int CSWSMessage::HandleServerAdminToServerMessage(unsigned int nPlayerId, unsigned char* pMessage,
                                                  unsigned int nLength)
{
    CExoString sOut;
    if (*pMessage != 's') return 0;

    CNetLayerPlayerInfo* pPlayerInfo = g_pAppManager->m_pServerExoApp->GetNetLayer()
                                           ->GetPlayerInfo(nPlayerId);
    if (pPlayerInfo->m_isAdminPermitted == 0) return 0;

    unsigned int nSize = nLength - 1;
    unsigned char* pBuffer = pMessage + 1;

    // Echo: echoes the message body back to the sender.
    if (ParseToken("Echo", &pBuffer, nSize, 0))
    {
        CExoString sEcho(reinterpret_cast<const char*>(pMessage + strlen("Echo") + 2), static_cast<int>(nLength - strlen("Echo") - 2));
        SendServerToServerAdminMessage(nPlayerId, sEcho);
    }

    // ServerStatus.<GetStatus|GetModuleList|GetPlayerList>: exact-length tokens only, a token with
    // leftover text returns 0.
    if (ParseToken("ServerStatus", &pBuffer, nSize, 1))
    {
        if (ParseToken("GetStatus", &pBuffer, nSize, 0))
        {
            if (nSize == 0)
            {
                SendServerToServerAdminServerStatus(nPlayerId);
                return 1;
            }
            return 0;
        }
        if (ParseToken("GetModuleList", &pBuffer, nSize, 0))
        {
            if (nSize == 0)
            {
                SendServerToServerAdminModuleList(nPlayerId);
                return 1;
            }
            return 0;
        }
        if (ParseToken("GetPlayerList", &pBuffer, nSize, 0))
        {
            if (nSize == 0)
            {
                SendServerToServerAdminPlayerList(nPlayerId);
                return 1;
            }
            return 0;
        }
    }

    // Module.Load / Module.Run / Module.Save; the Load and Save arguments are parsed strings.
    if (ParseToken("Module", &pBuffer, nSize, 1))
    {
        if (ParseToken("Load", &pBuffer, nSize, 0))
        {
            if (nSize == 0) return 0;
            CExoString sModuleName;
            if (ParseGetString(pBuffer, nSize, sModuleName, 256))
            {
                CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(g_pAppManager->m_pServerExoApp
                                          ->GetClientObjectByPlayerId(nPlayerId, 0));
                g_pAppManager->m_pServerExoApp->LoadModule(sModuleName, pPlayer->AsSWSPlayer());
            }
            return 1;
        }
        if (ParseToken("Run", &pBuffer, nSize, 0))
        {
            if (nSize != 0) return 0;
            g_pAppManager->m_pServerExoApp->RunModule();
            return 1;
        }
        if (ParseToken("Save", &pBuffer, nSize, 0))
        {
            if (nSize == 0) return 0;
            if (ParseGetString(pBuffer, nSize, sOut, 256))
            {
                // "Save <slot> <name>": the slot must be 2 or above and must not overwrite an
                // existing save.
                CExoString sSaveName;   // the SubString result (the save's new name)
                CExoString sScrap;      // never read
                int nSpace = sOut.Find(' ', 0);
                if (nSpace >= 1)
                {
                    int nSlot;
                    nSlot = sOut.Left(nSpace).AsINT();
                    sSaveName = sOut.SubString(nSpace + 1, -1);
                    if (nSlot >= 2 && sSaveName.GetLength() >= 1)
                    {
                        CExoArrayList<CExoString> saveList;
                        g_pExoBase->GetDirectoryList(&saveList, CExoString("SAVES:"), RESTYPE_INVALID, 1, 0);
                        int nSlots = static_cast<int>(saveList.m_count);
                        CExoString* pScan = saveList.m_pArray;
                        if (nSlots >= 1)
                        {
                            do
                            {
#if KOTOR1
                                if (pScan->AsINT() == nSlot)
                                    goto slotFound;
#else
                                if (pScan->m_sString != 0)
                                {
                                    if (atoi(pScan->m_sString) == nSlot)
                                        goto slotFound;
                                }
#endif
                                --nSlots;
                                ++pScan;
                            } while (nSlots != 0);
                        }
#if KOTOR1  // K1 SaveGame has no temp-save argument
                        g_pAppManager->m_pServerExoApp->SaveGame(nSlot, sSaveName, 0, 0);
#else
                        g_pAppManager->m_pServerExoApp->SaveGame(nSlot, sSaveName, 0, 0, 0);
#endif
                    slotFound: ;
                    }
                }
            }
            return 1;
        }
    }

    // Control.Boot / Control.SAPass / Control.Say: Boot and SAPass parse an argument they then
    // discard, Say's text is broadcast.
    if (ParseToken("Control", &pBuffer, nSize, 1))
    {
        if (ParseToken("Boot", &pBuffer, nSize, 1))
        {
            if (nSize == 0) return 0;
            if (ParseGetString(pBuffer, nSize, sOut, 256))
                sOut.AsINT();
        }
        else if (ParseToken("SAPass", &pBuffer, nSize, 1))
        {
            if (nSize == 0) return 0;
            ParseGetString(pBuffer, nSize, sOut, 256);
        }
        else if (ParseToken("Say", &pBuffer, nSize, 0))
        {
            if (nSize == 0) return 0;
            if (ParseGetString(pBuffer, nSize, sOut, 256))
            {
                // Say: broadcast the text to every server admin, then to every regular player.
                CExoLinkedListNode* pNode = g_pAppManager->m_pServerExoApp
                                                 ->GetSysAdminList()->m_pInternal->m_pHead;
                while (pNode != 0)
                {
                    CSWSPlayer* pAdmin = static_cast<CSWSPlayer*>(g_pAppManager->m_pServerExoApp
                                                          ->GetPlayerList()->m_pInternal->GetAtPos(pNode));
                    CSWSMessage* pSWSMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
                    pSWSMessage->SendServerToPlayerChatMessage(5, OBJECT_INVALID, CExoString(sOut),
                                                               pAdmin->m_nPlayerId, CExoString(""));
                    g_pAppManager->m_pServerExoApp->GetPlayerList()->m_pInternal->GetNext(pNode);
                }
                pNode = g_pAppManager->m_pServerExoApp
                             ->GetPlayerList()->m_pInternal->m_pHead;
                while (pNode != 0)
                {
                    CSWSPlayer* pAdmin = static_cast<CSWSPlayer*>(g_pAppManager->m_pServerExoApp
                                                          ->GetPlayerList()->m_pInternal->GetAtPos(pNode));
                    CSWSMessage* pSWSMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
                    pSWSMessage->SendServerToPlayerChatMessage(5, OBJECT_INVALID, CExoString(sOut),
                                                               pAdmin->m_nPlayerId, CExoString(""));
                    g_pAppManager->m_pServerExoApp->GetPlayerList()->m_pInternal->GetNext(pNode);
                }
                g_pAppManager->m_pServerExoApp->GetPlayerList();
            }
        }
    }

    // default: the name=value option editor. GetTokenPair splits the remaining text into up to
    // three tokens; with exactly three, the first names a server-info option and the third sets
    // it, and the resulting line goes to every server admin. Multi-line values are re-split
    // line by line. The accumulator is load-bearing: the first failure returns 0, a later
    // line's failure returns 1.
    CExoString sToken0;
    CExoString sToken1;
    CExoString sToken2;
    int nTokens = GetTokenPair(pBuffer, nSize, sToken0, sToken1, sToken2);
    CServerInfo* pServerInfo = g_pAppManager->m_pServerExoApp->GetServerInfo();
    unsigned int nAllAdmins = CNetLayer::PLAYERID_ALL_SERVERADMINS;
    // Declared outside the if on purpose: both joins write it.
    int nReturnAccum = 0;
    if (nTokens == 3)
    {
        do
        {
            int nOption = g_pAppManager->m_pServerExoApp->GetServerInfo()
                              ->FindOptionIndex(sToken0, sToken1);
            if (nOption == 0) break;   // return nReturnAccum (0 first, 1 after any success)
            g_pAppManager->m_pServerExoApp->GetServerInfo()->m_nUpdatedFlags |= 2;
            int nType = pServerInfo->m_Options[nOption].m_nType;
            int nValue;
            if (nType == 2)
            {
                nValue = atoi(sToken2.CStr());
                *pServerInfo->m_Options[nOption].m_pValue = nValue;
            }
            else if (nType == 1)
            {
                nValue = (sToken2[0] == '1') ? 1 : 0;
                *pServerInfo->m_Options[nOption].m_pValue = nValue;
            }

            {
                CExoString sDot(".");
                CExoString sResp1 = sToken0 + sDot;
                CExoString sResp2 = sResp1 + sToken1;
                CExoString sSpace(" ");
                CExoString sResp3 = sResp2 + sSpace;
                CExoString sResp4 = sResp3 + sToken2;
                SendServerToServerAdminMessage(nAllAdmins, sResp4);
            }

            int nPos = 0;
            while (sToken2[nPos] != 0 && sToken2[nPos] != '\n')
                nPos++;
            if (sToken2[nPos] != '\n')
            {
                nReturnAccum = 1;
                break;
            }
            nReturnAccum = 1;
            int nLen = sToken2.GetLength();
            nTokens = GetTokenPair(reinterpret_cast<unsigned char*>(sToken2.CStr() + nPos + 1),
                                   nLen - (nPos + 1), sToken0, sToken1, sToken2);
            if (nTokens != 3)
                nReturnAccum = 0;
        } while (nTokens == 3);
    }
    return nReturnAccum;
}

int CSWSMessage::ParseToken(char const* pToken, unsigned char** ppBuffer, unsigned int& nSize, int nDelim)
{
    unsigned int nTokenLen = static_cast<unsigned int>(strlen(pToken));
    if (nSize < nTokenLen) return 0;
    unsigned char* pBuffer = *ppBuffer;
    if (strncmp(reinterpret_cast<char const*>(pBuffer), pToken, nTokenLen) != 0) return 0;
    if (nDelim != 0) {
        if (pBuffer[nTokenLen] != '.') return 0;
        *ppBuffer = pBuffer + (nTokenLen + 1);
        nSize = nSize - (nTokenLen + 1);
        return 1;
    } else {
        *ppBuffer = pBuffer + nTokenLen;
        nSize = nSize - nTokenLen;
        return 1;
    }
}

int CSWSMessage::ParseGetString(unsigned char* pBuffer, unsigned int nSize, CExoString& sResult, unsigned int nMaxLen)
{
    if (pBuffer[0] == ' ') {
        if (nMaxLen != 0) {
            if (nSize < 2) return 0;
            if (nSize - 1 > nMaxLen) return 0;
        }
        CExoString sTemp(reinterpret_cast<const char*>(&pBuffer[1]), nSize - 1);
        sResult = sTemp;
        return 1;
    }
    return 0;
}

// File-local token splitter for HandleServerAdminToServerMessage's option editor: splits the
// buffer on '.' and ' ' into up to three tokens and returns the token count.
//   "a"            one token
//   "a.b"           two
//   "a.b "          three (the second token is the trailing text)
//   leading space   none, all three tokens cleared
static int GetTokenPair(unsigned char* pBuffer, unsigned int nSize,
                        CExoString& sToken0, CExoString& sToken1, CExoString& sToken2)
{
    unsigned int nEnd0 = 0;
    // Intentionally uninitialised: the size-0 path never writes it.
    unsigned char c0;
#ifdef K2_FIX_ENGINE_BUGS
    c0 = 0;
#endif
    if (nSize != 0)
    {
        do
        {
            c0 = pBuffer[nEnd0];
            if (c0 == '.') break;
            if (c0 == ' ')
            {
                sToken0 = "";
                sToken1 = "";
                sToken2 = "";
                return 0;
            }
            nEnd0++;
        } while (nEnd0 != nSize);
    }

    {
        CExoString sTmp0(reinterpret_cast<const char*>(pBuffer), nEnd0);
        sToken0 = sTmp0;
    }

    if (c0 == '.')
    {
        unsigned int nEnd1 = nEnd0 + 1;
        unsigned char c1;
        if (nEnd1 >= nSize)
        {
            c1 = '.';
        }
        else
        {
            unsigned int nScan = nEnd1;
            do
            {
                c1 = pBuffer[nScan];
                if (c1 == ' ') break;
                if (c1 == '.')
                {
                    sToken1 = "";
                    sToken2 = "";
                    return 0;
                }
                nScan++;
            } while (nScan != nSize);
            nEnd1 = (c1 == ' ') ? nScan : nSize;
        }
        {
            CExoString sTmp1(reinterpret_cast<const char*>(pBuffer + nEnd0 + 1), nEnd1 - (nEnd0 + 1));
            sToken1 = sTmp1;
        }
        if (c1 == ' ')
        {
            if (nEnd1 + 1 >= nSize)
            {
                sToken2 = "";
                return 0;
            }
            {
                CExoString sTmp2(reinterpret_cast<const char*>(pBuffer + nEnd1 + 1), nSize - (nEnd1 + 1));
                sToken2 = sTmp2;
            }
            return 3;
        }
        sToken2 = "";
        return (nEnd1 > nEnd0 + 1) ? 2 : 0;
    }

    sToken1 = "";
    sToken2 = "";
    return nEnd0 != 0 ? 1 : 0;
}

// '0' and '1' are listed in that order on purpose: the compiled comparison chain tests '1'
// first and lets the '0' arm fall through.
int CSWSMessage::ParseGetBool(unsigned char* pBuffer, unsigned int nSize, int& nResult)
{
    if (nSize == 1) return 0;
    if (pBuffer[0] != ' ') return 0;
    if (pBuffer[1] == '0') { nResult = 0; return 1; }
    if (pBuffer[1] == '1') { nResult = 1; return 1; }
    return 0;
}
