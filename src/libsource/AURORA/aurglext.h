#pragma once

// The GL extension and capability table, one query per feature the renderer probes at startup, most
// of them a bit test against the driver's accumulated extension word.

extern int existingextensions;

extern int renderlevel;

extern const int queryTranslationTable[2];

extern int useatifragmentshaders;
extern int usearbvertexprograms;
extern bool g_bDisableVertexBufferObjects;
extern int g_nUnlockATISoftShadowsForHotFix;

extern int ATI_FRAGMENT_SHADER_BIT;
extern int ARB_MULTITEXTURE_BIT;
extern int ARB_TEXTURE_COMPRESSION_BIT;
extern int ATIX_PN_TRIANGLES_BIT;
extern int EXT_COMPILED_VERTEX_ARRAY_BIT;
extern int EXT_SGIS_MIPMAP_BIT;
extern int EXT_SWAP_CONTROL_BIT;
extern int EXT_BGRA_BIT;
extern int EXT_TEXTURE_COMPRESSION_S3TC_BIT;
extern int EXT_TEXTURE_CUBE_MAP_BIT;
extern int EXT_TEXTURE_FILTER_ANISOTROPIC_BIT;
extern int GL_ARB_DEPTH_TEXTURE_BIT;
extern int GL_ARB_FRAGMENT_PROGRAM_BIT;
extern int GL_ARB_VERTEX_BUFFER_OBJECT_BIT;
extern int GL_ARB_VERTEX_PROGRAM_BIT;
extern int GL_STENCIL_TWO_SIDED_BIT;
extern int NV_FENCE_BIT;
extern int NV_PIXEL_SHADERS_BIT;
extern int NV_REGISTER_COMBINERS_BIT;
extern int NV_VERTEX_ARRAY_RANGE_BIT;
extern int NV_VERTEX_ARRAY_RANGE2_BIT;
extern int NV_VERTEX_PROGRAMS_BIT;
extern int WGL_ARB_BUFFER_REGION_BIT;
extern int WGL_ARB_RENDER_TO_TEXTURE_BIT;
extern int EXT_TEXTURE_ENV_COMBINE_BIT;
extern int NV_REGISTER_COMBINERS2_BIT;
extern int EXT_FOG_COORD_BIT;
extern int EXT_SECONDARY_COLOR_BIT;
extern int WGL_NV_RENDER_TO_TEXTURE_RECTANGLE_BIT;
extern int WGL_ARB_MULTISAMPLE_BIT;

int AurCompiledVertexArraysAvailable();
int AurPBufferAvailable();
int AurTruformAvailable();
int AurATIFragmentShaderAvailable();
int AurVertexProgramsARBAvailable();
bool AurSwapControlAvailable();
bool AurBumpMapCardValidate();
bool AurNonRadeon8500Validate();
int AurBumpMapAvailable();
bool AurNV1xLevelShadersAvailable();
bool AurNV2xLevelShadersAvailable();
int AurBumpyShinyAvailable();
bool AurATIValidate();
int AurOnePassEnvMapAvailable();
int AurMultiTextureAvailable();
int AurVertexProgramsNVAvailable();
int AurVertexBufferObjectARBAvailable();
int AurVertexProgramsAvailable();
int AurSoftShadowsAvailable();
int AurATISoftShadowsAvailable();
int AurFrameBufferModificationsAvailable();
int AurATIFrameBufferModificationsAvailable();
int AurCubeMapAvailable();
int AurAnisotropicFilterAvailable();
int AurRangeFenceAvailable();
int AurSGIMipmapAvailable();
int AurTextureCompressionAvailable();
char CheckExtension(char* pszName, const unsigned char* pszExtensions);
int AurGetAvailableStencilBits();
int AurGetStencilClearValue();
int AurTwoSidedStencilAvailable();
int AurBufferRegionAvailable();
bool AurCheckSoftShadowsAvailable();
bool AurCheckFrameBufferModificationsAvailable();
bool AuroraUseARBShaders();
