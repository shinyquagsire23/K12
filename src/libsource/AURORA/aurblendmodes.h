#pragma once

// The engine's blend factors. Each value indexes blendTranslationTable (aurglcore.cpp), which holds
// the GL blend factor it stands for; a Material keeps one per blending mode.
namespace Aurora {
enum AuroraBlendModes {
    kBlendSrcAlpha = 0,         // GL_SRC_ALPHA
    kBlendInvSrcAlpha = 1,      // GL_ONE_MINUS_SRC_ALPHA
    kBlendOne = 2,              // GL_ONE
    kBlendZero = 3,             // GL_ZERO
    kBlendSrcColor = 4,         // GL_SRC_COLOR
    kBlendDstColor = 5,         // GL_DST_COLOR
    kBlendInvDstAlpha = 6,      // GL_ONE_MINUS_DST_ALPHA
    kBlendDstAlpha = 7,         // GL_DST_ALPHA
};
}

extern const unsigned int blendTranslationTable[];
