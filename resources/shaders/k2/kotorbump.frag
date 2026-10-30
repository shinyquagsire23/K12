// The PC's per-pixel passes, which kotor.frag has no programs for (swkotor2.exe's NV register-combiner
// programs, docs/prep/bump_k2pc_v2263.md). The bump pass: kPSBumpmap; under USE_BUMPYSHINY the
// bumpy-shiny one (kPSBumpyShinyTextures + kPSBumpyShiny); under USE_FOGPASS the fog pass (kPSFog).
// The normalization cube maps the programs renormalised their interpolated vectors with are
// normalize() here. USE_DESAT / USE_FORCEGLOW: the force-sight bump modes (kPSDesatBumpmap /
// kPSFGlowBumpmap: the desaturated diffuse alone) and bumpy-shiny modes (kotor.frag's tail).
precision mediump float;

uniform vec4	u_fragmentShaderParams[3]; // bump: the light's specular colour [0], its diffuse colour [1]; bumpy-shiny: [0].a the alpha; force sight: its colour [1]
uniform vec3	u_fogColor;

varying mediump vec4	v_texCoord0;
varying vec4	v_lightColor;
varying float	v_fogFactor;

vec3 forceSight(vec3 color, inout float alpha)
{
#if USE_DESAT
	color = vec3(dot(vec3(0.3, 0.59, 0.11), color));
#if USE_FORCEGLOW
	color += u_fragmentShaderParams[1].xyz * u_fragmentShaderParams[1].www;
	alpha = 1.0;
#else
	color *= vec3(0.6, 0.6, 0.6);
#endif
#endif
	return color;
}

#if USE_FOGPASS
void main()
{
	gl_FragColor = vec4(u_fogColor, 1.0 - v_fogFactor);
}
#elif USE_BUMPYSHINY
uniform sampler2D u_texture0Sampler;	// the bump map
uniform samplerCube u_texture2Sampler;	// the environment cube
varying vec3	v_bsRow0;
varying vec3	v_bsRow1;
varying vec3	v_bsRow2;
varying vec3	v_bsEye;

void main()
{
	vec3 N = texture2D(u_texture0Sampler, v_texCoord0.st).rgb * 2.0 - 1.0;
	vec3 Nw = vec3(dot(normalize(v_bsRow0), N), dot(normalize(v_bsRow1), N), dot(normalize(v_bsRow2), N));
	vec3 E = normalize(v_bsEye);
	// The reflection of E about the perturbed normal, unnormalised as the program leaves it.
	vec3 R = 2.0 * dot(Nw, E) * Nw - E * dot(Nw, Nw);
	vec4 env = textureCube(u_texture2Sampler, R);
	float alpha = u_fragmentShaderParams[0].a;
	vec3 color = forceSight(env.rgb, alpha);
	// The fog pass, or the diffuse pass laid over this one, fogs it; the reflection fogs to black.
	gl_FragColor = vec4(color * v_fogFactor, alpha);
}
#else
uniform sampler2D u_texture0Sampler;	// the bump map: a tangent-space normal, "up" in blue
uniform sampler2D u_texture3Sampler;	// the diffuse map
varying vec3	v_tsLight;
varying vec3	v_tsHalf;
varying float	v_bumpAtten;

void main()
{
	vec4 base = texture2D(u_texture3Sampler, v_texCoord0.st);
#if USE_DESAT
	// Force sight: no bump and no light, the diffuse desaturated -- times the sight's colour plus
	// its glow, or dimmed.
	float luma = dot(vec3(0.3, 0.59, 0.11), base.rgb);
#if USE_FORCEGLOW
	vec3 color = luma * u_fragmentShaderParams[1].rgb + u_fragmentShaderParams[1].rgb * u_fragmentShaderParams[1].a;
	float alpha = 1.0;
#else
	vec3 color = vec3(luma * 0.8);
	float alpha = base.a;
#endif
	gl_FragColor = vec4(mix(u_fogColor, clamp(color, 0.0, 1.0), v_fogFactor), alpha);
#else
	// The light's diffuse colour (env1) times N.L and the attenuation on top of the vertex light,
	// times the diffuse map; plus the specular colour (env0) times N.H to the 16th where the half
	// vector faces out of the surface, masked by the diffuse map's alpha.
	vec3 N = texture2D(u_texture0Sampler, v_texCoord0.st).rgb * 2.0 - 1.0;
	vec3 L = normalize(v_tsLight);
	vec3 H = normalize(v_tsHalf);
	float NdotL = max(dot(N, L), 0.0);
	float s = max(dot(N, H), 0.0);
	s *= s;
	s *= s;
	s *= s;
	s *= s;
	if (H.z < 0.0)
		s = 0.0;
	vec3 color = base.rgb * min(v_lightColor.rgb + NdotL * u_fragmentShaderParams[1].rgb * v_bumpAtten, 1.0);
	color = min(color + s * v_bumpAtten * u_fragmentShaderParams[0].rgb * (1.0 - base.a), 1.0);
	gl_FragColor = vec4(mix(u_fogColor, color, v_fogFactor), v_lightColor.a);
#endif
}
#endif
