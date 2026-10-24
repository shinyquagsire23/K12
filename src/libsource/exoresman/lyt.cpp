#include "k2stdint.h"
#include "lyt.h"
#include "exostring.h"

#include "port/libcbase.h"
#include "port/libcstring.h"

// m_pReserved1 and m_bDemanded are deliberately left uninitialised; m_bDemanded is read by
// UnloadLayout and the destructor.
CLYT::CLYT()
{
    m_nRoomCount = 0;
    m_nTrackCount = 0;
    m_nObstacleCount = 0;
    m_nDoorHookCount = 0;
    m_paRoomNames = 0;
    m_paRoomPositions = 0;
    m_paTrackNames = 0;
    m_paTrackPositions = 0;
    m_paObstacleNames = 0;
    m_paObstaclePositions = 0;
    m_pasDoorHookRoomNames = 0;
    m_pasDoorHookNames = 0;
    m_paDoorHookPositions = 0;
    m_paDoorHookOrientations = 0;
    m_panDoorHookUniqueTypes = 0;
}

CLYT::~CLYT()
{
    UnloadLayout();

    delete[] m_paRoomNames;
    m_paRoomNames = 0;
    delete[] m_paRoomPositions;
    m_paRoomPositions = 0;
    delete[] m_paTrackNames;
    m_paTrackNames = 0;
    delete[] m_paTrackPositions;
    m_paTrackPositions = 0;
    delete[] m_paObstacleNames;
    m_paObstacleNames = 0;
    delete[] m_paObstaclePositions;
    m_paObstaclePositions = 0;
    delete[] m_pasDoorHookRoomNames;
    m_pasDoorHookRoomNames = 0;
    delete[] m_pasDoorHookNames;
    m_pasDoorHookNames = 0;
    delete[] m_paDoorHookPositions;
    m_paDoorHookPositions = 0;
    delete[] m_paDoorHookOrientations;
    m_paDoorHookOrientations = 0;
    delete[] m_panDoorHookUniqueTypes;
    m_panDoorHookUniqueTypes = 0;
}

void CLYT::UnloadLayout()
{
    if (m_bDemanded) {
        m_pRes->Release();
        m_bDemanded = 0;
    }
}

int CLYT::LoadLayout(const CResRef& resRef)
{
    CExoString sResRef;
    resRef.CopyToString(sResRef);
    return LoadLayout(sResRef);
}

// Parse the layout. A line cursor walks a NUL-padded copy of the resource; a scratch buffer of the
// same size is refilled by sscanf at each step and advanced past by strlen(szLine) + 2 (the CR and
// the LF).
int CLYT::LoadLayout(const CExoString& sResRef)
{
    SetResRef(CResRef(sResRef), 0);

    if (!m_pRes->Demand())
        return 0;

    char* pLYTData = static_cast<char*>(m_pRes->GetLYTDataPtr());
    int nLYTSize = m_pRes->GetLYTSize();

    char* pBuffer = new char[nLYTSize + 4];
    char* szLine = new char[nLYTSize + 4];
    memset(pBuffer, 0, nLYTSize + 4);      // idiom-exempt: runtime byte count, not a sizeof
    memcpy(pBuffer, pLYTData, nLYTSize);   // idiom-exempt: runtime byte count, not a sizeof

    char* p = pBuffer;
    static_cast<void>(sscanf(p, "%[^\015\n]", szLine));
    while (strcmp("beginlayout", szLine) != 0) {
        p += strlen(szLine) + 2;
        static_cast<void>(sscanf(p, "%[^\015\n]", szLine));
    }
    p += strlen(szLine) + 2;
    static_cast<void>(sscanf(p, "%[^\015\n]", szLine));

    static_cast<void>(sscanf(szLine, "%*s%d", &m_nRoomCount));
    p += strlen(szLine) + 2;

    if (m_nRoomCount > 0) {
        char szName[32];
        m_paRoomNames = new CResRef[m_nRoomCount];
        m_paRoomPositions = new Vector[m_nRoomCount];
        for (int i = 0; i < m_nRoomCount; i++) {
            static_cast<void>(sscanf(p, "%[^\015\n]", szLine));
            static_cast<void>(sscanf(szLine, "%s%f%f%f", szName, &m_paRoomPositions[i].x,
                   &m_paRoomPositions[i].y, &m_paRoomPositions[i].z));
            m_paRoomNames[i] = szName;
            p += strlen(szLine) + 2;
        }
    }

    static_cast<void>(sscanf(p, "%[^\015\n]", szLine));
    if (strcmp("donelayout", szLine) != 0) {
        static_cast<void>(sscanf(szLine, "%*s%d", &m_nTrackCount));
        p += strlen(szLine) + 2;

        if (m_nTrackCount > 0) {
            char szName[32];
            m_paTrackNames = new CResRef[m_nTrackCount];
            m_paTrackPositions = new Vector[m_nTrackCount];
            for (int i = 0; i < m_nTrackCount; i++) {
                static_cast<void>(sscanf(p, "%[^\015\n]", szLine));
                static_cast<void>(sscanf(szLine, "%s%f%f%f", szName, &m_paTrackPositions[i].x,
                       &m_paTrackPositions[i].y, &m_paTrackPositions[i].z));
                m_paTrackNames[i] = szName;
                p += strlen(szLine) + 2;
            }
        }

        static_cast<void>(sscanf(p, "%[^\015\n]", szLine));
        if (strcmp("donelayout", szLine) != 0) {
            static_cast<void>(sscanf(szLine, "%*s%d", &m_nObstacleCount));
            p += strlen(szLine) + 2;

            if (m_nObstacleCount > 0) {
                char szName[32];
                m_paObstacleNames = new CResRef[m_nObstacleCount];
                m_paObstaclePositions = new Vector[m_nObstacleCount];
                for (int i = 0; i < m_nObstacleCount; i++) {
                    static_cast<void>(sscanf(p, "%[^\015\n]", szLine));
                    static_cast<void>(sscanf(szLine, "%s%f%f%f", szName, &m_paObstaclePositions[i].x,
                           &m_paObstaclePositions[i].y, &m_paObstaclePositions[i].z));
                    m_paObstacleNames[i] = szName;
                    p += strlen(szLine) + 2;
                }
            }

            static_cast<void>(sscanf(p, "%[^\015\n]", szLine));
            if (strcmp("donelayout", szLine) != 0) {
                static_cast<void>(sscanf(szLine, "%*s%d", &m_nDoorHookCount));
                p += strlen(szLine) + 2;

                if (m_nDoorHookCount > 0) {
                    char szRoomName[32];
                    char szName[32];
                    m_pasDoorHookRoomNames = new CExoString[m_nDoorHookCount];
                    m_pasDoorHookNames = new CExoString[m_nDoorHookCount];
                    m_paDoorHookPositions = new Vector[m_nDoorHookCount];
                    m_paDoorHookOrientations = new Quaternion[m_nDoorHookCount];
                    m_panDoorHookUniqueTypes = new int[m_nDoorHookCount];
                    for (int i = 0; i < m_nDoorHookCount; i++) {
                        static_cast<void>(sscanf(p, "%[^\015\n]", szLine));
                        static_cast<void>(sscanf(szLine, "%s%s%d%f%f%f%f%f%f%f", szRoomName, szName,
                               &m_panDoorHookUniqueTypes[i],
                               &m_paDoorHookPositions[i].x, &m_paDoorHookPositions[i].y,
                               &m_paDoorHookPositions[i].z,
                               &m_paDoorHookOrientations[i].w, &m_paDoorHookOrientations[i].x,
                               &m_paDoorHookOrientations[i].y, &m_paDoorHookOrientations[i].z));
                        m_pasDoorHookRoomNames[i] = szRoomName;
                        m_pasDoorHookNames[i] = szName;
                        p += strlen(szLine) + 2;
                    }
                }
            }
        }
    }

    m_bDemanded = 1;
    delete[] szLine;
    delete[] pBuffer;
    return m_bDemanded;
}

// Bind the handle, demand it, and return the resource blob instead of parsing it.
int CLYT::LoadLayout(const CExoString& sResRef, char** ppData, int* pnSize)
{
    SetResRef(CResRef(sResRef), 0);

    if (!m_pRes->Demand())
        return 0;

    *ppData = static_cast<char*>(m_pRes->GetLYTDataPtr());
    *pnSize = m_pRes->GetLYTSize();
    m_bDemanded = 1;
    return 1;
}

CResRef CLYT::GetRoomName(int nRoom)
{
    return m_paRoomNames[nRoom];
}

Vector CLYT::GetRoomPosition(int nRoom)
{
    return m_paRoomPositions[nRoom];
}

int CLYT::GetRoomCount()
{
    return m_nRoomCount;
}

CResRef CLYT::GetTrackName(int nTrack)
{
    return m_paTrackNames[nTrack];
}

Vector CLYT::GetTrackPosition(int nTrack)
{
    return m_paTrackPositions[nTrack];
}

int CLYT::GetTrackCount()
{
    return m_nTrackCount;
}

CResRef CLYT::GetObstacleName(int nObstacle)
{
    return m_paObstacleNames[nObstacle];
}

Vector CLYT::GetObstaclePosition(int nObstacle)
{
    return m_paObstaclePositions[nObstacle];
}

int CLYT::GetObstacleCount()
{
    return m_nObstacleCount;
}

int CLYT::GetDoorHookCount()
{
    return m_nDoorHookCount;
}

void CLYT::GetDoorHookRoomName(int nDoorHook, CExoString& sRoomName)
{
    sRoomName = m_pasDoorHookRoomNames[nDoorHook];
}

void CLYT::GetDoorHookName(int nDoorHook, CExoString& sName)
{
    sName = m_pasDoorHookNames[nDoorHook];
}

int CLYT::GetDoorHookUniqueType(int nDoorHook)
{
    return m_panDoorHookUniqueTypes[nDoorHook];
}

Vector CLYT::GetDoorHookPosition(int nDoorHook)
{
    return m_paDoorHookPositions[nDoorHook];
}

Quaternion CLYT::GetDoorHookOrientation(int nDoorHook)
{
    return m_paDoorHookOrientations[nDoorHook];
}
