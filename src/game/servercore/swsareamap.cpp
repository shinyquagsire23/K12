
#include "k2stdint.h"
#include "swsareamap.h"

#include "port/libcstring.h"

CSWSAreaMap::CSWSAreaMap()
{
    m_pData = 0;
    m_nSize = 0;
    m_nResX = 0;
    m_nResY = 0;
    m_fScaleX = 0.0f;
    m_fScaleY = 0.0f;
    m_fOffsetX = 0.0f;
    m_fOffsetY = 0.0f;
    memset(m_anPartyMapX, -1, sizeof(m_anPartyMapX));
    memset(m_anPartyMapY, -1, sizeof(m_anPartyMapY));
    memset(m_afPartyOrientation, 0, sizeof(m_afPartyOrientation));
}

CSWSAreaMap::~CSWSAreaMap()
{
    if (m_pData)
    {
        delete[] m_pData;
        m_pData = 0;
        m_nSize = 0;
    }
}

// The north-axis quadrant rotates the two world anchor points, not the map pixels: 0 identity,
// 1 = 180 degrees, 2 = (x,y) -> (y,-x), 3 = (x,y) -> (-y,x). The same four rotations appear again
// in the transforms below.
//
// nResX is forced to 88 when it is 0 or above 88, and the Y resolution is half of it, floored
// at 1; the data word count is then ((m_nResX + 1) * (m_nResY + 1)) / 32 + 1, zeroed.
void CSWSAreaMap::Initialize(int nMode, int nResX, unsigned int nNorthAxis, float fW1X, float fW1Y,
                             float fW2X, float fW2Y, int nMapPt1X, int nMapPt1Y, int nMapPt2X,
                             int nMapPt2Y, int nZoom)
{
    if (nMode == 0)
        return;

    int nRes = nResX;
    if (nRes > 88 || nRes == 0)
        nRes = 88;
#if KOTOR1  // K1 map: 440 x 256 pixels
    int nResY = static_cast<int>(static_cast<float>(nRes) * 256.0f / 440.0f);
#else
    int nResY = static_cast<int>(static_cast<float>(nRes) * 354.0f / 708.0f);
#endif

    m_fMapScale = 1.0f / static_cast<float>(nZoom);
    m_nResX = nRes;
    m_nResY = nResY > 1 ? nResY : 1;
    m_nNorthAxis = nNorthAxis;

    if (m_pData)
        return;

    m_nSize = ((m_nResX + 1) * (m_nResY + 1)) / 32 + 1;
    m_pData = new unsigned int[m_nSize];
    memset(m_pData, 0, m_nSize * sizeof(unsigned int));

    float fX1 = fW1X;
    float fY1 = fW1Y;
    float fX2 = fW2X;
    float fY2 = fW2Y;
    // Case 3 first.
    if (nNorthAxis == 3)
    {
        float fT1 = fX1;
        fX1 = -fY1;
        fY1 = fT1;
        float fT2 = fX2;
        fX2 = -fY2;
        fY2 = fT2;
    }
    else if (nNorthAxis == 2)
    {
        float fT1 = fX1;
        fX1 = fY1;
        fY1 = -fT1;
        float fT2 = fX2;
        fX2 = fY2;
        fY2 = -fT2;
    }
    else if (nNorthAxis == 1)
    {
        fX1 = -fX1;
        fY1 = -fY1;
        fX2 = -fX2;
        fY2 = -fY2;
    }

    m_fScaleX = (fX2 - fX1) / static_cast<float>(nMapPt2X - nMapPt1X);
    m_fScaleY = (fY2 - fY1) / static_cast<float>(nMapPt2Y - nMapPt1Y);
    m_fOffsetX = fX1 - m_fScaleX * static_cast<float>(nMapPt1X);
    m_fOffsetY = fY1 - m_fScaleY * static_cast<float>(nMapPt1Y);
}

// Returns void; the guard paths never form a result and the body ends in a memcpy.
void CSWSAreaMap::LoadSavedAreaMapData(unsigned int* paData, unsigned int nDataSize, int nResX,
                                      int nResY)
{
    if (m_nSize == 0 || m_nResX != nResX || m_nResY != nResY || m_nSize != nDataSize)
        return;
    memcpy(m_pData, paData, nDataSize * sizeof(unsigned int));
}

// Marks the diamond of map cells within nFlag of the world point: the outer loop walks nX over
// [-nFlag, nFlag] and the inner one nY over [-(nFlag - |nX|), nFlag - |nX|]. Both bounds are
// inclusive, which is also the only reading that matches the name.
//
// The map step here is integer (708 / m_nResX, 354 / m_nResY), unlike the float form in
// SetMapPointExplored.
void CSWSAreaMap::SetWorldPointExplored(Vector vPos, int nFlag)
{
    if (!m_nSize)
        return;

    // Rotate by the north-axis quadrant: 0 identity, 1 = 180 degrees, 2 = (x,y) -> (y,-x),
    // 3 = (x,y) -> (-y,x). The step order matters: the chain assigns both outputs, so it
    // compiles to selects rather than branches.
    float fY1 = (m_nNorthAxis == 3) ?  vPos.x : vPos.y;
    float fX1 = (m_nNorthAxis == 3) ? -vPos.y : vPos.x;
    float fY2 = (m_nNorthAxis == 1) ? -fY1 : fY1;
    float fX2 = (m_nNorthAxis == 1) ? -fX1 : fX1;
    float fY  = (m_nNorthAxis == 2) ? -fX2 : fY2;
    float fX  = (m_nNorthAxis == 2) ? fY2 : fX2;

    int nMapX = static_cast<int>((fX - m_fOffsetX) / m_fScaleX + 0.5f);
#if KOTOR1
    if (static_cast<unsigned int>(nMapX) > 440)
#else
    if (static_cast<unsigned int>(nMapX) > 708)
#endif
        return;
    int nMapY = static_cast<int>((fY - m_fOffsetY) / m_fScaleY + 0.5f);
#if KOTOR1
    if (static_cast<unsigned int>(nMapY) > 256)
#else
    if (static_cast<unsigned int>(nMapY) > 354)
#endif
        return;

    for (int nX = -nFlag; nX <= nFlag; nX++)
    {
        int nRadius = nFlag - (nX < 0 ? -nX : nX);
        for (int nY = -nRadius; nY <= nRadius; nY++)
        {
#if KOTOR1
            int nPtX = nMapX + (440 / m_nResX) * nX;
            int nPtY = nMapY + (256 / m_nResY) * nY;
#else
            int nPtX = nMapX + (708 / m_nResX) * nX;
            int nPtY = nMapY + (354 / m_nResY) * nY;
#endif
            if (nPtX < 0)
                nPtX = 0;
            if (nPtY < 0)
                nPtY = 0;
#if KOTOR1
            if (nPtX > 440)
                nPtX = 440;
            if (nPtY > 256)
                nPtY = 256;
#else
            if (nPtX > 708)
                nPtX = 708;
            if (nPtY > 354)
                nPtY = 354;
#endif

            SetMapPointExplored(nPtX, nPtY);
        }
    }
}

int CSWSAreaMap::GetMapPixelFromWorldCoord(Vector vWorld, int& nMapX, int& nMapY)
{
    // Rotate by the north-axis quadrant: 0 identity, 1 = 180 degrees, 2 = (x,y) -> (y,-x),
    // 3 = (x,y) -> (-y,x). The step order matters: the chain assigns both outputs, so it
    // compiles to selects rather than branches.
    float fY1 = (m_nNorthAxis == 3) ?  vWorld.x : vWorld.y;
    float fX1 = (m_nNorthAxis == 3) ? -vWorld.y : vWorld.x;
    float fY2 = (m_nNorthAxis == 1) ? -fY1 : fY1;
    float fX2 = (m_nNorthAxis == 1) ? -fX1 : fX1;
    float fY  = (m_nNorthAxis == 2) ? -fX2 : fY2;
    float fX  = (m_nNorthAxis == 2) ? fY2 : fX2;

    nMapX = static_cast<int>((fX - m_fOffsetX) / m_fScaleX + 0.5f);
    nMapY = static_cast<int>((fY - m_fOffsetY) / m_fScaleY + 0.5f);
#if KOTOR1
    if (static_cast<unsigned int>(nMapX) > 440 || static_cast<unsigned int>(nMapY) >= 257)
#else
    if (static_cast<unsigned int>(nMapX) > 708 || static_cast<unsigned int>(nMapY) >= 355)
#endif
    {
        nMapX = -1;
        nMapY = -1;
        return false;
    }
    return true;
}

// One bit per map cell inside a dword, the dword index being
// ((m_nResX + 1) * nGridY + nGridX) / 32: the bitfield is indexed by the grid pixel, not the
// map pixel, and the guard is `>`, so nWord == m_nSize still writes.
void CSWSAreaMap::SetMapPointExplored(int nMapX, int nMapY)
{
#if KOTOR1
    if (static_cast<unsigned int>(nMapX) > 440 || static_cast<unsigned int>(nMapY) > 256)
#else
    if (static_cast<unsigned int>(nMapX) > 708 || static_cast<unsigned int>(nMapY) > 354)
#endif
        return;

#if KOTOR1
    float fScaleX = 440.0f / static_cast<float>(m_nResX);
    float fScaleY = 256.0f / static_cast<float>(m_nResY);
#else
    float fScaleX = 708.0f / static_cast<float>(m_nResX);
    float fScaleY = 354.0f / static_cast<float>(m_nResY);
#endif
    int nGridX = static_cast<int>(static_cast<float>(nMapX) / fScaleX + 0.5f);
    int nGridY = static_cast<int>(static_cast<float>(nMapY) / fScaleY + 0.5f);
    int nIndex = (m_nResX + 1) * nGridY + nGridX;
    int nWord = nIndex / 32;
    if (nWord > static_cast<int>(m_nSize))
        return;
    m_pData[nWord] |= 1 << nIndex;
}

// The second bound here is 355 while SetMapPointExplored uses 354, so the two guards are not
// the same source text.
bool CSWSAreaMap::GetGridPixelFromMapPixel(int nMapX, int nMapY, int& nGridX, int& nGridY)
{
#if KOTOR1
    if (static_cast<unsigned int>(nMapX) > 440 || static_cast<unsigned int>(nMapY) >= 257)
#else
    if (static_cast<unsigned int>(nMapX) > 708 || static_cast<unsigned int>(nMapY) >= 355)
#endif
    {
        nGridX = -1;
        nGridY = -1;
        return false;
    }

#if KOTOR1
    float fScaleX = 440.0f / static_cast<float>(m_nResX);
    float fScaleY = 256.0f / static_cast<float>(m_nResY);
#else
    float fScaleX = 708.0f / static_cast<float>(m_nResX);
    float fScaleY = 354.0f / static_cast<float>(m_nResY);
#endif
    nGridX = static_cast<int>(static_cast<float>(nMapX) / fScaleX + 0.5f);
    nGridY = static_cast<int>(static_cast<float>(nMapY) / fScaleY + 0.5f);
    return true;
}

void* CSWSAreaMap::GetMapData(unsigned int& nDataSizeDwords, int& nResX, int& nResY)
{
    if (!m_nSize)
        return 0;
    nDataSizeDwords = m_nSize;
    nResX = m_nResX;
    nResY = m_nResY;
    return m_pData;
}

    // Case 3 first.
float CSWSAreaMap::GetMapRotateCCWFromWorldOrientation(Vector vDir)
{
    float fYaw = Yaw(vDir);
    if (m_nNorthAxis == 3)
        fYaw += 270.0f;
    else if (m_nNorthAxis == 2)
        fYaw += 90.0f;
    else if (m_nNorthAxis == 1)
        fYaw += 180.0f;
    return fYaw;
}

void CSWSAreaMap::SetPartyMemberWorldLocation(int index, Vector vPos)
{
    if (index > 2)
        return;
    if (!m_nSize)
        return;

    // Rotate by the north-axis quadrant: 0 identity, 1 = 180 degrees, 2 = (x,y) -> (y,-x),
    // 3 = (x,y) -> (-y,x). The step order matters: the chain assigns both outputs, so it
    // compiles to selects rather than branches.
    float fY1 = (m_nNorthAxis == 3) ?  vPos.x : vPos.y;
    float fX1 = (m_nNorthAxis == 3) ? -vPos.y : vPos.x;
    float fY2 = (m_nNorthAxis == 1) ? -fY1 : fY1;
    float fX2 = (m_nNorthAxis == 1) ? -fX1 : fX1;
    float fY  = (m_nNorthAxis == 2) ? -fX2 : fY2;
    float fX  = (m_nNorthAxis == 2) ? fY2 : fX2;

    int nMapX = static_cast<int>((fX - m_fOffsetX) / m_fScaleX + 0.5f);
#if KOTOR1
    if (static_cast<unsigned int>(nMapX) > 440)
#else
    if (static_cast<unsigned int>(nMapX) > 708)
#endif
        return;
    int nMapY = static_cast<int>((fY - m_fOffsetY) / m_fScaleY + 0.5f);
#if KOTOR1
    if (static_cast<unsigned int>(nMapY) > 256)
#else
    if (static_cast<unsigned int>(nMapY) > 354)
#endif
        return;

    m_anPartyMapX[index] = nMapX;
    m_anPartyMapY[index] = nMapY;
}

void CSWSAreaMap::SetPartyMemberWorldOrientation(int index, Vector vDir)
{
    if (index > 2)
        return;
    if (!m_nSize)
        return;

    m_afPartyOrientation[index] = GetMapRotateCCWFromWorldOrientation(vDir);
}

int CSWSAreaMap::GetPartyMemberMapLocation(int nSlot, int& nMapX, int& nMapY)
{
    if (m_nSize == 0 || nSlot > 2)
        return false;
    if (m_anPartyMapX[nSlot] == -1)
        return false;
    nMapX = m_anPartyMapX[nSlot];
    nMapY = m_anPartyMapY[nSlot];
    return true;
}

float CSWSAreaMap::GetPartyMemberMapOrientation(int nSlot)
{
    if (nSlot > 2)
        return 0.0f;
    if (!m_nSize)
        return 0.0f;
    return m_afPartyOrientation[nSlot];
}

// The inverse of SetWorldPointExplored for a single point. The Y map coordinate is computed and
// range-checked before the X one here, unlike GetMapPixelFromWorldCoord.
int CSWSAreaMap::IsWorldPointExplored(Vector vPos)
{
    if (!m_nSize)
        return false;

    // Rotate by the north-axis quadrant: 0 identity, 1 = 180 degrees, 2 = (x,y) -> (y,-x),
    // 3 = (x,y) -> (-y,x). The step order matters: the chain assigns both outputs, so it
    // compiles to selects rather than branches.
    float fY1 = (m_nNorthAxis == 3) ?  vPos.x : vPos.y;
    float fX1 = (m_nNorthAxis == 3) ? -vPos.y : vPos.x;
    float fY2 = (m_nNorthAxis == 1) ? -fY1 : fY1;
    float fX2 = (m_nNorthAxis == 1) ? -fX1 : fX1;
    float fY  = (m_nNorthAxis == 2) ? -fX2 : fY2;
    float fX  = (m_nNorthAxis == 2) ? fY2 : fX2;

    int nMapY = static_cast<int>((fY - m_fOffsetY) / m_fScaleY + 0.5f);
#if KOTOR1
    if (static_cast<unsigned int>(nMapY) > 256)
#else
    if (static_cast<unsigned int>(nMapY) > 354)
#endif
        return false;
    int nMapX = static_cast<int>((fX - m_fOffsetX) / m_fScaleX + 0.5f);
#if KOTOR1
    if (static_cast<unsigned int>(nMapX) > 440)
#else
    if (static_cast<unsigned int>(nMapX) > 708)
#endif
        return false;

#if KOTOR1
    float fScaleX = 440.0f / static_cast<float>(m_nResX);
    float fScaleY = 256.0f / static_cast<float>(m_nResY);
#else
    float fScaleX = 708.0f / static_cast<float>(m_nResX);
    float fScaleY = 354.0f / static_cast<float>(m_nResY);
#endif
    int nGridX = static_cast<int>(static_cast<float>(nMapX) / fScaleX + 0.5f);
    int nGridY = static_cast<int>(static_cast<float>(nMapY) / fScaleY + 0.5f);
    int nIndex = (m_nResX + 1) * nGridY + nGridX;
    int nWord = nIndex / 32;
    if (nWord > static_cast<int>(m_nSize))
        return false;
    return (m_pData[nWord] & (1 << nIndex)) != 0;
}

void CSWSAreaMap::SetEntireMapExplored()
{
    if (!m_pData)
        return;
    memset(m_pData, 0xff, m_nSize * sizeof(unsigned int));
}
