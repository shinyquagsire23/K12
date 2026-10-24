#pragma once

// visualeffects.2da rows the engine plays that nwscript.nss does not name. STEALTH_FIELD is the
// stealth shell, and DISTORTION the effect a stealthed party member wears instead when the
// frame-buffer effects are on.
enum {
    VFX_DUR_STUN             = 2002,
    VFX_BEAM_FORCE_STORM_ARC = 2061,
    VFX_BEAM_DROID_STUN      = 2065,
    VFX_BEAM_SHOCK           = 2066,
#if !KOTOR1  // K1's visualeffects.2da ends the beams at 2066
    VFX_BEAM_POISON_SPRAY    = 2068,
    VFX_BEAM_PLASMA_SPRAY    = 2069,
#endif
    VFX_REFLEX_SAVE          = 4000,
    VFX_COM_SNIPER_SHOT      = 4038,
    VFX_IMP_OVERKILL         = 6003,
#if !KOTOR1  // K1 has no row 7003
    VFX_DUR_BREATH           = 7003,
#endif
    VFX_DUR_DISTORTION       = 8000,
    VFX_DUR_CLOAK_REMOVAL    = 8001,
    VFX_DUR_STEALTH_FIELD    = 8002,
    VFX_IMP_DARK_SIDE        = 9014,
    VFX_IMP_LIGHT_SIDE       = 9015,
};
