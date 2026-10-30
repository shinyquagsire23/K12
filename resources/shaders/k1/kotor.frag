precision lowp float;

uniform vec4	u_fragmentShaderParams[2]; // ambient color [0], bump light color [1]

uniform sampler2D u_texture0Sampler;
uniform vec3	u_fogColor;

varying mediump vec4	v_texCoord0;
varying vec4	v_lightColor;
varying float	v_fogFactor;

#if USE_LIGHTMAP
varying vec4 v_texCoord1;
#endif
#if USE_LIGHTMAP || USE_BUMPMAP
uniform sampler2D u_texture1Sampler;
#endif

// The one-pass reflection as an NVIDIA card drew it (the Mac's cEnvmapCubeFragmentShader): the
// reflection added over the lit texture, weighted by one minus its alpha. 0 mixes the two by the alpha,
// as ATI's program (cEnvmapFogFragmentShader) did.
#ifndef USE_ENVMAP_ADD
#define USE_ENVMAP_ADD 0
#endif

#if USE_CUBEMAP || USE_BUMPMAP
#if USE_FLAT_CUBEMAP
uniform sampler2D u_texture2Sampler;
#else
uniform samplerCube u_texture2Sampler;
#endif
varying vec3 v_texCoordCube;
#endif

void main()
{
	vec4 color = texture2D(u_texture0Sampler, v_texCoord0.st);
	
#if USE_ALPHATEST
	if (color.a < 0.5) discard;
#endif
	
#if USE_LIGHTMAP
	vec4 lightColor = texture2D(u_texture1Sampler, v_texCoord1.st);
	lightColor.xyz += v_lightColor.xyz;
	color *= lightColor;
#else
	color.xyz *= v_lightColor.xyz;
#endif //USE_LIGHTMAP

#if USE_CUBEMAP
	#if USE_FLAT_CUBEMAP
	vec4 cubecolor = texture2D(u_texture2Sampler, v_texCoordCube.st);
	#else
	vec4 cubecolor = textureCube(u_texture2Sampler, v_texCoordCube);
	#endif
	
	#if USE_BUMPMAP && !USE_SKIN
		color = cubecolor;
		color.a *= u_fragmentShaderParams[0].w;
	#else
		#if USE_LIGHTMAP
			color += (1.0 - color.a) * cubecolor;
		#elif USE_ENVMAP_ADD
			color.rgb += (1.0 - color.a) * cubecolor.rgb;
		#else
			color = mix(cubecolor, color, color.a);
		#endif
		// TODO: mix the result with constant index 1 as well when secondary lighting is present
		color.a = u_fragmentShaderParams[0].w;
	#endif
#elif USE_BUMPMAP
	color.a = u_fragmentShaderParams[0].w;
#else
	color.a *= v_lightColor.a;
#endif

//	color = vec4(0.0, 1.0, 0.0, 1.0);
#if USE_LIGHTMAP
//	color.r = 1.0;
//	color.g = 0.0;
//	color.b = 0.0;
//	color.a = 1.0;
#endif //USE_LIGHTMAP
	gl_FragColor = vec4(mix(u_fogColor, vec3(color), v_fogFactor), color.w);
}

