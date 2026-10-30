// The bump pass: KOTOR 1's one-pass bump-map fragment program (kPSBumpmap, cBumpmapFragmentShader)
// in GLSL; under USE_BUMPYSHINY the bumpy-shiny one (kPSBumpyShiny, cBumpyshinyFragmentShader), under
// USE_LMBUMP the PC's lightmapped bump combiners (kPSLightmapBump*). The
// normalization cube maps the programs renormalised their interpolated vectors with are normalize()
// here.
precision mediump float;

uniform vec4	u_fragmentShaderParams[2]; // bump: the light's specular colour [0], its diffuse colour [1]; bumpy-shiny: [0].a the alpha
uniform vec3	u_fogColor;

varying mediump vec4	v_texCoord0;
varying vec4	v_lightColor;
varying float	v_fogFactor;

#if USE_BUMPYSHINY
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
	// The diffuse pass is laid over this one, fogged itself, so the reflection fogs to black.
	gl_FragColor = vec4(env.rgb * v_fogFactor, env.a * u_fragmentShaderParams[0].a);
}
#elif USE_LMBUMP
// The PC's lightmapped bump passes, its NV register-combiner modes (swkotor.exe 0x0042e600): mode 8
// (diffuse) is ((max(N.L, 0) * c1 * atten + lightmap) * diffuse; modes 9..11 (specular)
// lightmap * diffuse + c0 * atten * max(N.H, 0)^p, where the alpha combiners square N.H and the last
// stage multiplies the square back in: p = 1, 3, 5 for exponents 1, 2, 4 (8 has no mode on NVIDIA;
// the pattern gives 7). Opaque (the final alpha is invert(zero)); the normalization cube on unit 1
// is normalize() here.
uniform sampler2D u_texture0Sampler;	// the bump map
uniform sampler2D u_texture2Sampler;	// the lightmap
uniform sampler2D u_texture3Sampler;	// the diffuse map
varying vec4	v_texCoord1;
varying vec3	v_tsLight;
varying vec3	v_tsHalf;
varying float	v_bumpAtten;

void main()
{
	vec3 N = texture2D(u_texture0Sampler, v_texCoord0.st).rgb * 2.0 - 1.0;
	vec3 lightmap = texture2D(u_texture2Sampler, v_texCoord1.st).rgb;
	vec3 base = texture2D(u_texture3Sampler, v_texCoord0.st).rgb;
#if USE_LMBUMP == 1
	float NdotL = clamp(dot(N, normalize(v_tsLight)), 0.0, 1.0);
	vec3 color = clamp(NdotL * u_fragmentShaderParams[1].rgb * v_bumpAtten + lightmap, 0.0, 1.0) * base;
#else
	float NdotH = clamp(dot(N, normalize(v_tsHalf)), 0.0, 1.0);
	float p = float(LMBUMP_EXPONENT == 1 ? 1 : LMBUMP_EXPONENT == 2 ? 3 : LMBUMP_EXPONENT == 4 ? 5 : 7);
	vec3 color = clamp(lightmap * base + u_fragmentShaderParams[0].rgb * v_bumpAtten * pow(NdotH, p), 0.0, 1.0);
#endif
	gl_FragColor = vec4(mix(u_fogColor, color, v_fogFactor), 1.0);
}
#else
uniform sampler2D u_texture0Sampler;	// the diffuse map
uniform sampler2D u_texture1Sampler;	// the bump map: a tangent-space normal, "up" in blue
varying vec3	v_tsLight;
varying vec3	v_tsHalf;
varying float	v_bumpAtten;

// The program as written multiplies N.L by the light's diffuse colour (env1) and then overwrites
// the product with N.L times the attenuation alone, so every light bumps white. The engine computes
// and uploads that colour for every bump light (and its debug switches recolour it), which the PC's
// combiners read; 1 takes it, 0 is the program's text.
#ifndef BUMP_LIGHT_COLOUR
#define BUMP_LIGHT_COLOUR 1
#endif

void main()
{
	vec4 base = texture2D(u_texture0Sampler, v_texCoord0.st);
	vec3 N = texture2D(u_texture1Sampler, v_texCoord0.st).rgb * 2.0 - 1.0;
	vec3 L = normalize(v_tsLight);
	vec3 H = normalize(v_tsHalf);
	float NdotL = dot(N, L);
	float NdotH = dot(N, H);
	float s = NdotH * NdotH;
	s *= s;
	s *= s;
#if BUMP_LIGHT_COLOUR
	vec3 diffuse = NdotL * v_bumpAtten * u_fragmentShaderParams[1].rgb;
#else
	vec3 diffuse = vec3(NdotL * v_bumpAtten);
#endif
	vec3 color = clamp(v_lightColor.rgb + diffuse, 0.0, 1.0) * base.rgb;
	vec3 spec = s * v_bumpAtten * u_fragmentShaderParams[0].rgb * (1.0 - base.a);
	color = clamp(color + spec, 0.0, 1.0);
	gl_FragColor = vec4(mix(u_fogColor, color, v_fogFactor), v_lightColor.a);
}
#endif
