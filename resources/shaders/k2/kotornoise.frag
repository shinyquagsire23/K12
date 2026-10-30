// The noise ("dirt") pass: the PC's noise pixel-shader modes (kPSNoise*, swkotor2.exe's combiner and
// ATI programs; docs/prep/noise_k2pc_v2263.md) in GLSL. The base map is roughened by the area's 3D
// noise texture -- NOISE_FORMULA 0 subtracts it, 1 adds it, 2 multiplies by it -- and lit by the
// vertex light, plus the lightmap under USE_LIGHTMAP. USE_CUBEMAP adds the environment map where the
// base map's alpha is below 1: after the noise and unlit alone, before both with the lightmap.
// USE_DESAT: the force-sight variants (kPSDesatNoise*).
precision mediump float;

uniform vec4	u_fragmentShaderParams[3]; // force sight [1], alpha test [2].w
uniform vec3	u_fogColor;

uniform sampler2D u_texture0Sampler;	// the base map
varying mediump vec4	v_texCoord0;
varying vec4	v_lightColor;
varying float	v_fogFactor;
varying vec3	v_noiseCoord;

#if USE_LIGHTMAP
uniform sampler2D u_texture1Sampler;	// the lightmap
varying vec4	v_texCoord1;
#endif
#if USE_CUBEMAP
varying vec3	v_texCoordCube;
#if USE_LIGHTMAP
uniform samplerCube u_texture2Sampler;	// the environment map
#define ENV_SAMPLER u_texture2Sampler
#else
uniform samplerCube u_texture1Sampler;
#define ENV_SAMPLER u_texture1Sampler
#endif
#endif
#if USE_LIGHTMAP && USE_CUBEMAP
uniform sampler3D u_texture3Sampler;	// the noise
#define NOISE_SAMPLER u_texture3Sampler
#elif USE_LIGHTMAP || USE_CUBEMAP
uniform sampler3D u_texture2Sampler;
#define NOISE_SAMPLER u_texture2Sampler
#else
uniform sampler3D u_texture1Sampler;
#define NOISE_SAMPLER u_texture1Sampler
#endif

void main()
{
	vec4 base = texture2D(u_texture0Sampler, v_texCoord0.st);
#if USE_ALPHATEST
	if (base.a <= u_fragmentShaderParams[2].w) discard;
#endif
	vec3 noise = texture3D(NOISE_SAMPLER, v_noiseCoord).rgb;
	vec3 color = base.rgb;
	float alpha = base.a;
#if USE_CUBEMAP
	vec3 env = textureCube(ENV_SAMPLER, v_texCoordCube).rgb * (1.0 - base.a);
	alpha = 1.0;
#if USE_LIGHTMAP
	color += env;
#else
	color *= v_lightColor.rgb;
#endif
#endif
#if NOISE_FORMULA == 0
	color -= noise;
#elif NOISE_FORMULA == 1
	color += noise;
#else
	color *= noise;
#endif
	color = clamp(color, 0.0, 1.0);
#if USE_LIGHTMAP
	color = clamp(color * (texture2D(u_texture1Sampler, v_texCoord1.st).rgb + v_lightColor.rgb), 0.0, 1.0);
#elif USE_CUBEMAP
	color = clamp(color + env, 0.0, 1.0);
#else
	color *= v_lightColor.rgb;
#endif
#if USE_DESAT
	color = vec3(dot(vec3(0.3, 0.59, 0.11), color) * 0.65);
	alpha *= u_fragmentShaderParams[1].a;
#endif
	gl_FragColor = vec4(mix(u_fogColor, color, v_fogFactor), alpha);
}
