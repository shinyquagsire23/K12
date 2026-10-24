// Client-side animation object for a projectile. No extra fields yet.
#ifndef CSWCANIMBASEPROJECTILE_H
#define CSWCANIMBASEPROJECTILE_H

#include "nwcanimbase.h"

class CResRef;
struct Vector;

class CSWCAnimBaseProjectile : public CSWCAnimBase {
public:
    // The animations SetAnimation plays: "default", "travel01", "impact01", "throwout" and
    // "throwback".
    enum {
        PROJECTILE_ANIMATION_DEFAULT    = 0,
        PROJECTILE_ANIMATION_TRAVEL     = 1,
        PROJECTILE_ANIMATION_IMPACT     = 2,
        PROJECTILE_ANIMATION_THROW_OUT  = 3,
        PROJECTILE_ANIMATION_THROW_BACK = 4,
    };

    CSWCAnimBaseProjectile();

    virtual ~CSWCAnimBaseProjectile();
    virtual int LoadModel(CResRef const&, unsigned char);
    virtual int SetAnimation(unsigned short, float, int, float);
    // int, not bool: an override may not change the return type.
    virtual int SetOrientation(Vector const&);
};

#endif // CSWCANIMBASEPROJECTILE_H
