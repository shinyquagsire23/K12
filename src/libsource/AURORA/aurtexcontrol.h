// Aurora texture controllers: the shared base class and one subclass per procedural texture kind.
#pragma once

// ==== TextureController ====
// Base class for the procedural texture-image generators: Control() and Init() are the per-kind
// work, and ParseField() reads the ini keys that name the fields.
#ifndef TEXTURECONTROLLER_H
#define TEXTURECONTROLLER_H

#include "List.h"

class CAurTexture;

class TextureController {
public:

    virtual ~TextureController();
    virtual void Control() = 0;
    virtual void Init() = 0;
    virtual void ParseField(char*);
    virtual bool IsLinear() { return false; }
    virtual unsigned char* Distort(unsigned char* pImage);
    virtual void SetImage();

    TextureController(CAurTexture* pTexture);

    // Returns the combined image: pDst when it was non-null, otherwise a freshly allocated buffer
    // that the receiver must free.
    unsigned char* Height2Channel(unsigned char* pSrc, unsigned char* pDst);

    CAurTexture*   m_pTexture;
    int            m_nLastFrame;
    int            m_nDistort;
    int            m_nDistortAngle;
    float          m_fDistortionAmplitude;
    float          m_fSpeed;
    List<float>    m_lstChannelScale;
    List<float>    m_lstChannelTranslate;
    unsigned char* m_pSourceImage;

    // Shared by every texture controller; defined out of line.
    static unsigned char newimage[256 * 256 * 4];
};

#endif // TEXTURECONTROLLER_H

// ==== ArturoTextureController ====
// The "arturo" plasma texture.
#ifndef ARTUROTEXTURECONTROLLER_H
#define ARTUROTEXTURECONTROLLER_H


class ArturoTextureController : public TextureController {
public:

    virtual void Control();
    virtual void Init();
    virtual void ParseField(char* pszLine);

    ArturoTextureController(CAurTexture* pTexture);

    ~ArturoTextureController()
    {
        delete[] m_pfHeights;
        delete[] m_pfPrevHeights;
        delete[] m_pfNextHeights;
        delete[] m_pImage;
    }

    int    m_nArturoWidth;
    int    m_nArturoHeight;
    float* m_pfNextHeights;
    float* m_pfHeights;
    float* m_pfPrevHeights;
    float  m_fBlend;
    unsigned char* m_pImage;

    virtual void BuildArturoImage();
};

#endif // ARTUROTEXTURECONTROLLER_H

// ==== CycleTIDTextureController ====
// A texture-id animation cycler.
#ifndef CYCLETIDTEXTURECONTROLLER_H
#define CYCLETIDTEXTURECONTROLLER_H


class CycleTIDTextureController : public TextureController {
public:

    virtual void Control();
    virtual void Init() {}
    virtual void ParseField(char* pszLine);
    virtual bool IsLinear() { return false; }

    CycleTIDTextureController(CAurTexture* pTexture);

    float m_fFPS;
    int   m_nTIDIndex;
    float m_fFrameTime;
    int   m_nReserved1;
};

#endif // CYCLETIDTEXTURECONTROLLER_H

// ==== DirtyTextureController ====
// The "dirty"/"dirty2"/"dirty3" glass-dirt texture.
#ifndef DIRTYTEXTURECONTROLLER_H
#define DIRTYTEXTURECONTROLLER_H


class DirtyTextureController : public TextureController {
public:

    virtual void Control();
    virtual void Init();  // Defined in CSWSArea.cpp, not here.
    virtual bool IsLinear() { return false; }

    DirtyTextureController(CAurTexture* pTexture, int nMode) : TextureController(pTexture)
    {
        m_pImage = 0;
        m_nReserved1 = 0;
        m_nDirtyIndex = nMode;
        m_nFormula = 0;
    }

    ~DirtyTextureController()
    {
        delete[] m_pImage;
    }

    unsigned char* m_pImage;
    int  m_nDirtyIndex;
    char m_nFormula;
    int  m_nReserved1;

    virtual void Init(int nColor, int nTextureSize, int nFormula, int nFunction);

    // Builds one of the three perlin channel images Init(int,int,int,int) allocates.
    void MakePerlinTexture(unsigned char* pImage, int nMaxChannel);
};

#endif // DIRTYTEXTURECONTROLLER_H

// ==== HeatDistortTextureController ====
// The heat-haze distortion texture, never instantiated.
#ifndef HEATDISTORTTEXTURECONTROLLER_H
#define HEATDISTORTTEXTURECONTROLLER_H


class HeatDistortTextureController : public TextureController {
public:

    virtual void Control();
    virtual void Init();
    virtual bool IsLinear() { return true; }

    ~HeatDistortTextureController()
    {
        delete[] m_pImage;
    }

    unsigned char* m_pImage;
};

#endif // HEATDISTORTTEXTURECONTROLLER_H

// ==== LifeTextureController ====
// The "life" (cellular-automaton) texture. The constructor seeds nothing, so m_pCells stays
// uninitialised and the destructor deletes an uninitialised pointer when Init() never ran. That is
// an engine bug, reproduced rather than fixed.
#ifndef LIFETEXTURECONTROLLER_H
#define LIFETEXTURECONTROLLER_H


class LifeTextureController : public TextureController {
public:

    virtual void Control();
    virtual void Init();

    LifeTextureController(CAurTexture* pTexture)
        : TextureController(pTexture)
    {
    }

    ~LifeTextureController()
    {
        delete[] m_pCells;
    }

    unsigned char* m_pCells;
};

#endif // LIFETEXTURECONTROLLER_H

// ==== PerlinTextureController ====
// The Perlin-noise texture. It adds one virtual, BuildPerlinImage, and like the Life controller its
// constructor seeds none of its buffers, which the destructor then deletes.
#ifndef PERLINTEXTURECONTROLLER_H
#define PERLINTEXTURECONTROLLER_H


class PerlinTextureController : public TextureController {
public:

    virtual void Control();
    virtual void Init();
    virtual void ParseField(char* pszLine);

    PerlinTextureController(CAurTexture* pTexture)
        : TextureController(pTexture)
    {
    }

    ~PerlinTextureController()
    {
        delete[] m_pPrevNoise;
        delete[] m_pNextNoise;
    }

    unsigned char* m_pPrevNoise;
    unsigned char* m_pNextNoise;
    float          m_fBlend;

    virtual void BuildPerlinImage(unsigned char* pDst, int nSize);
};

#endif // PERLINTEXTURECONTROLLER_H

// ==== RandomTextureController ====
// The random-noise texture.
#ifndef RANDOMTEXTURECONTROLLER_H
#define RANDOMTEXTURECONTROLLER_H


class RandomTextureController : public TextureController {
public:

    virtual void Control();
    virtual void Init();

    RandomTextureController(CAurTexture* pTexture)
        : TextureController(pTexture)
    {
        m_pNoise = 0;
        m_pImage = 0;
    }

    ~RandomTextureController()
    {
        delete[] m_pNoise;
        delete[] m_pImage;
    }

    unsigned char* m_pNoise;
    unsigned char* m_pImage;
};

#endif // RANDOMTEXTURECONTROLLER_H

// ==== RingDistortTextureController ====
// The "ringtexdistort" refraction texture.
#ifndef RINGDISTORTTEXTURECONTROLLER_H
#define RINGDISTORTTEXTURECONTROLLER_H


class RingDistortTextureController : public TextureController {
public:

    virtual void Control();
    virtual void Init();
    virtual bool IsLinear() { return true; }

    RingDistortTextureController(CAurTexture* pTexture)
        : TextureController(pTexture)
    {
        m_pImage = 0;
        m_fPhase = 0.0f;
    }

    ~RingDistortTextureController()
    {
        delete[] m_pImage;
    }

    unsigned char* m_pImage;
    float m_fPhase;
};

#endif // RINGDISTORTTEXTURECONTROLLER_H

// ==== WaterTextureController ====
// Procedural water.
#ifndef WATERTEXTURECONTROLLER_H
#define WATERTEXTURECONTROLLER_H


class WaterTextureController : public TextureController {
public:

    virtual void Control();
    virtual void Init();
    virtual void ParseField(char* pszLine);

    WaterTextureController(CAurTexture* pTexture);

    ~WaterTextureController()
    {
        delete[] m_pfHeights;
        delete[] m_pfVelocities;
        delete[] m_pfNextHeights;
        delete[] m_pImage;
        delete[] m_pBlurImage;
    }

    int   m_nWaterWidth;
    int   m_nWaterHeight;
    float* m_pfHeights;
    float* m_pfNextHeights;
    float* m_pfVelocities;
    float m_fForcePhase;
    float m_fAnglePhase;
    float m_fForceCycleSpeed;
    float m_fAngleCycleSpeed;
    unsigned char* m_pImage;
    unsigned char* m_pBlurImage;
    int   m_nFrameCounter;
    int   m_nCycle;  // redraw phase, cycled 0..2 by Control()
};

#endif // WATERTEXTURECONTROLLER_H

// ==== WaveTextureController ====
// Procedural wave.
#ifndef WAVETEXTURECONTROLLER_H
#define WAVETEXTURECONTROLLER_H


class WaveTextureController : public TextureController {
public:

    virtual void Control();
    virtual void Init();
    virtual void ParseField(char* pszLine);

    WaveTextureController(CAurTexture* pTexture);

    ~WaveTextureController()
    {
        delete[] m_pfHeights;
        delete[] m_pfPrevHeights;
    }

    float* m_pfHeights;
    float* m_pfPrevHeights;
};

#endif // WAVETEXTURECONTROLLER_H

// ==== aurtexcontrol ====
// The texture controller's free-function entry points, plus two ordering helpers from the adjacent
// texture-controller TU.
#ifndef AURTEXCONTROL_H
#define AURTEXCONTROL_H


class CAurTexture;

void AurTextureResetAll();

void AurTextureRefreshAll();

// The global new-texture mode, snapshotted into CAurTextureBasic::m_bModeIsDynamic.
void AurTextureSetModeStatic();
void AurTextureSetModeDynamic();

// Stub: always returns 0.
int AurTextureSetDim(int nWidth, int nHeight);

int AurTextureGetMaxTexID();

// Empty body kept for the symbol.
void AurTextureClearAll();

int AurTextureGetMem();

void AurTextureMakeCurrent();

void AurTextureForceControllerUpdate();

int AuroraQueryTextureMemoryInUse();

// Cache lookup by (name, filter); a miss returns null rather than constructing.
CAurTexture* AurTextureFind(char* pszName, char* pszFilter, int* pnOffset, int nType);

// When pTexture is non-null the name/filter pair is ignored and a fresh CAurTexture wrapping it is
// returned; otherwise the pair is looked up and, on a miss, the texture is constructed.
CAurTexture* AurTextureGet(char* pszName, CAurTexture* pTexture, char* pszFilter, int* pnOffset,
                           int nType);

int AurTextureSetMem(int nMem);

// Rebuilds the ordering array from the valid static textures and returns its address.
List<int>* AurTextureGetOrdering();

#endif // AURTEXCONTROL_H
