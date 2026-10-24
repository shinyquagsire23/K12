// The ARB fragment-program installers' shader-id table.
//
// The shader-id objects are plain .bss ints, externals whose definitions live outside this tree.
// The ones the installers below need are listed in declaration order. IllumNoiseMultiplyShaderID
// does not carry the "Fragment" infix of its siblings.

#ifndef GLRENDERFRAGMENTPROGRAMS_H
#define GLRENDERFRAGMENTPROGRAMS_H

extern unsigned int lightmapFragmentShaderID;
extern unsigned int envmapFragmentShaderID;
extern unsigned int envmapFogFragmentShaderID;
extern unsigned int bumpmapFragmentShaderID;
extern unsigned int normalmapFragmentShaderID;
extern unsigned int envmapLightmapFragmentShaderID;
extern unsigned int bumpyshinyFragmentShaderID;
extern unsigned int fogFragmentShaderID;
extern unsigned int standardFogFragmentShaderID;
extern unsigned int envmapCubeFragmentShaderID;
extern unsigned int envmapFogCubeFragmentShaderID;
extern unsigned int envmapLightmapCubeFragmentShaderID;
extern unsigned int NoiseSubtractFragmentShaderID;
extern unsigned int NoiseLMSubtractFragmentShaderID;
extern unsigned int NoiseLMAddFragmentShaderID;
extern unsigned int NoiseEMLMSubtractFragmentShaderID;
extern unsigned int NoiseEMLMAddFragmentShaderID;
extern unsigned int FGlowFragmentShaderID;
extern unsigned int FGlowLMFragmentShaderID;
extern unsigned int FGlowEMFragmentShaderID;
extern unsigned int FGlowEMLMFragmentShaderID;
extern unsigned int FGlowBumpyShinyFragmentShaderID;
extern unsigned int FGlowBumpmapFragmentShaderID;
extern unsigned int DesatFragmentShaderID;
extern unsigned int DesatLMFragmentShaderID;
extern unsigned int DesatEMFragmentShaderID;
extern unsigned int DesatEMLMFragmentShaderID;
extern unsigned int DesatBumpyShinyFragmentShaderID;
extern unsigned int DesatBumpmapFragmentShaderID;
extern unsigned int DesatNoiseSubFragmentShaderID;
extern unsigned int DesatNoiseSubEMFragmentShaderID;
extern unsigned int DesatNoiseSubLMFragmentShaderID;
extern unsigned int DesatNoiseSubEMLMFragmentShaderID;
extern unsigned int DesatNoiseAddFragmentShaderID;
extern unsigned int DesatNoiseAddEMFragmentShaderID;
extern unsigned int DesatNoiseAddLMFragmentShaderID;
extern unsigned int DesatNoiseAddEMLMFragmentShaderID;
extern unsigned int DesatNoiseMultFragmentShaderID;
extern unsigned int DesatNoiseMultEMFragmentShaderID;
extern unsigned int DesatNoiseMultLMFragmentShaderID;
extern unsigned int DesatNoiseMultEMLMFragmentShaderID;
extern unsigned int HoloFragmentShaderID;
extern unsigned int HoloDesatFragmentShaderID;
extern unsigned int IllumFragmentShaderID;
extern unsigned int IllumEnvFragmentShaderID;
extern unsigned int IllumNoiseSubtractFragmentShaderID;
extern unsigned int IllumNoiseAddFragmentShaderID;
extern unsigned int IllumNoiseMultiplyShaderID;
extern unsigned int IllumNoiseEMSubtractFragmentShaderID;
extern unsigned int IllumNoiseEMAddFragmentShaderID;
extern unsigned int IllumNoiseEMMultiplyFragmentShaderID;
extern unsigned int IllumDesatFragmentShaderID;
extern unsigned int IllumDesatEMFragmentShaderID;
extern unsigned int IllumDesatBumpyShinyFragmentShaderID;
extern unsigned int IllumDesatBumpmapFragmentShaderID;
extern unsigned int IllumDesatNoiseSubFragmentShaderID;
extern unsigned int IllumDesatNoiseSubEMFragmentShaderID;
extern unsigned int IllumDesatNoiseAddFragmentShaderID;
extern unsigned int IllumDesatNoiseAddEMFragmentShaderID;
extern unsigned int IllumDesatNoiseMultFragmentShaderID;
extern unsigned int IllumDesatNoiseMultEMFragmentShaderID;

// Installs the 31 base ARB fragment programs, once, from the ARB init path.
void sInitializeARBFragmentShaders();

// The other half of this unit, GLRender::EnableFragmentShadersARB: a fixed prologue that enables the
// fragment-program target, binds the program and clears g_illumShader, then one switch over the
// shader id whose arms run the same per-unit texture batteries the fixed-function
// EnableFragmentShaders uses, plus the program's colour constants and a closing bind. The
// *ShaderID globals above are that switch's program-id table.

#endif // GLRENDERFRAGMENTPROGRAMS_H
