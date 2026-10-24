#pragma once

// The ASL GL shim's own source file.
//
// Only SetBumpedOutShellIos lives here: it is a free function with no namespace, so it needs a
// translation unit of its own rather than a place among the ASLgl::gl* bodies.
//
// Its only caller is PartSkin::DrawBumpedOutShellVP, which reads the bump amplitude off the Gob. Its
// sibling reader is ASLgl::GlesKotorSetupShader, which uploads this same block as the vertex
// shader's u_vertexShaderParams uniform.

// The shader's parameter block, four floats uploaded as one vec4. Slot 0 is the bump-outline shell
// amplitude. The uniform location comes from the literal "u_vertexShaderParams".
extern float s_vertexShaderParams[4];

// Sets the per-Gob bump amplitude, a pure store into slot 0 of the block above.
void SetBumpedOutShellIos(float f);
