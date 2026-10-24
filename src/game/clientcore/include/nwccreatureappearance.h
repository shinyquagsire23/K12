#pragma once

#include "nwsobject.h"

class CSWCCreature;
struct CCAppearanceInfo;
class CExoString;
class CResRef;

// Client-side helper owning a creature's animation and model appearance: part
// variations, body/head/tail creation, and the camera-follow geometry.
class CSWCCreatureAppearance {
public:
    CSWCCreatureAppearance(CSWCCreature* pCreature);
    ~CSWCCreatureAppearance();

    unsigned char  m_cachedBTypeVariant;
    unsigned char  m_cachedBTypeNumber;
    unsigned short m_aCachedBodyParts[6];
    unsigned char  _pad0e[0x10-0x0e];
    unsigned long long m_oidHeadItem;
    unsigned int   m_bArmorEquipped;
    unsigned short m_nAppearanceId;
    unsigned char  m_nHeadNumber;
    unsigned char  m_nGender;
    unsigned char  m_nPhenotype;
    unsigned char  m_nSkinColor;
    unsigned char  m_nHairColor;
    unsigned char  m_nTattooColor1;
    unsigned char  m_nTattooColor2;
    unsigned char  m_cachedTailVariant;
    unsigned char  m_cachedWingVariant;
    unsigned char  _pad27;
    float          m_fPersonalSpace;
    float          m_fHeight;
    unsigned short m_cachedAlignment;
    unsigned char  _pad32[0x34-0x32];
    unsigned int   m_bUseBackupHead;
    unsigned char  m_race;
    unsigned char  _pad39[0x3c-0x39];
    unsigned int   m_bGroundTilt;
    CSWCCreature*  m_pCreature;
    int            m_bRebuildHead;
    int            m_bRebuildTail;
    unsigned long long m_bRebuildWings;
    unsigned long long m_bAlignmentDirty;
    float          m_driveAccel;
    float          m_fDriveMaxSpeed;
    float          m_walkSpeed;
    float          m_runSpeed;
    unsigned int   m_bCreatedWhilePaused;

    void GetPartVariations(unsigned char* paOut, unsigned char n);
    int  GetArmourClass();
    int  RequestTailAppearance(CCAppearanceInfo* pInfo);
    int  CreateTail(unsigned char a, CCAppearanceInfo* pInfo);
    int  CreateWing(unsigned char a, CCAppearanceInfo* pInfo);
    int  RequestWingAppearance(CCAppearanceInfo* pInfo);

    void* GetCameraFollowPart();
    int   GetFollowingBody();
    float GetHeadHeightOffset();
    float GetCameraOffset();
    void  SetAppearanceAlignment(short a);
    float GetRunLooksAwfulRate();
    float GetHeight();
    char  GetGenderChar();
    int   RequestBType(struct CCAppearanceInfo* pInfo);
    int   CreateHead(unsigned char a, struct CCAppearanceInfo* pInfo);
    int   HeadExists(struct CCAppearanceInfo* pInfo);
    int  RequestBodyAppearance(struct CCAppearanceInfo* pInfo);
    int  RequestHeadAppearance(struct CCAppearanceInfo* pInfo);
    int  RequestSimple(struct CCAppearanceInfo* pInfo);
    void GetBTypeModelAndTextureResRef(struct CCAppearanceInfo* pInfo, CResRef* pModelOut,
                                        CResRef* pTextureOut);
    int  CreateBodyAppearance(struct CCAppearanceInfo* pInfo);
    int  CreateSimple(unsigned char a, struct CCAppearanceInfo* pInfo);
    int  CreateBTypeBody(unsigned char a, struct CCAppearanceInfo* pInfo);
    int  CreateHeadAppearance(struct CCAppearanceInfo* pInfo);
    int  CreateTailAppearance(struct CCAppearanceInfo* pInfo);
    int  CreateWingAppearance(struct CCAppearanceInfo* pInfo);
    void ForceAnimate();

    int GetAnimType(const CExoString& s);
};
