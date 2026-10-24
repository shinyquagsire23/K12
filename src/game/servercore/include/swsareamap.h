#pragma once

// The module's saved-data map; the module holds one by pointer and CSWSArea::LoadMaps feeds it the
// "AreaMapData" blob from the area's GIT file.
//
// The constructor and destructor are declared here and defined out of line, which also keeps the
// type non-trivially constructible and destructible.

#include "Vector.h"

class CSWSAreaMap
{
public:
    CSWSAreaMap();
    ~CSWSAreaMap();

    // Consumes the per-area map blob and its two resolutions.
    void LoadSavedAreaMapData(unsigned int* paData, unsigned int nDataSize, int nResX, int nResY);
    // Converts the world anchors to map anchors.
    void Initialize(int nMode, int nResX, unsigned int nNorthAxis, float fW1X, float fW1Y,
                    float fW2X, float fW2Y, int nMapPt1X, int nMapPt1Y, int nMapPt2X,
                    int nMapPt2Y, int nZoom);
    // Both helpers return an int, and every call site tests it against zero.
    int GetMapPixelFromWorldCoord(Vector vWorld, int& nMapX, int& nMapY);
    bool GetGridPixelFromMapPixel(int nMapX, int nMapY, int& nGridX, int& nGridY);
    // Hands out the serialised map blob and its dimensions.
    void* GetMapData(unsigned int& nDataSizeDwords, int& nResX, int& nResY);
    int   GetPartyMemberMapLocation(int nSlot, int& nMapX, int& nMapY);
    float GetMapRotateCCWFromWorldOrientation(Vector vDir);
    float GetPartyMemberMapOrientation(int nSlot);
    // Exploration tracking and party-blip placement, driven by the server's main loop.
    void SetWorldPointExplored(Vector pos, int flag);
    void SetEntireMapExplored();
    void SetMapPointExplored(int nMapX, int nMapY);
    int  IsWorldPointExplored(Vector pos);
    void SetPartyMemberWorldLocation(int index, Vector pos);
    void SetPartyMemberWorldOrientation(int index, Vector orient);

    unsigned int* m_pData;      // the saved map blob, one bitfield word per 32 cells
    unsigned int  m_nSize;      // dword count of m_pData; the HUD's "is there a map" gate
    int           m_nResX;
    int           m_nResY;
    unsigned int  m_nNorthAxis; // a quadrant select, 0 through 3
    // Map units per pixel: Initialize sets it to 1.0f / nZoom, and the HUD multiplies its map
    // picture's stored width and height by it before SetExtent.
    float         m_fMapScale;
    float         m_fScaleX;    // the world-to-map X divisor
    float         m_fScaleY;
    float         m_fOffsetX;
    float         m_fOffsetY;
    int           m_anPartyMapX[3];         // -1 means "no blip"
    int           m_anPartyMapY[3];
    float         m_afPartyOrientation[3];  // the HUD's party-blip arrow angle
};
