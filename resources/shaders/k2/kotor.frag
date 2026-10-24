precision lowp float;

uniform vec4	u_fragmentShaderParams[3]; // ambient color [0], bump light color or force sight [1], alpha test [2].w

uniform sampler2D u_texture0Sampler;
uniform vec3	u_fogColor;

varying mediump vec3	v_nrm_es;

varying mediump vec4	v_texCoord0;
varying vec4	v_lightColor;
varying float	v_fogFactor;

#if USE_LIGHTMAP || USE_HOLO
varying vec4 v_texCoord1;
#endif
#if USE_LIGHTMAP || USE_BUMPMAP || USE_HOLO
uniform sampler2D u_texture1Sampler;
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
	vec4 textureColor = texture2D(u_texture0Sampler, v_texCoord0.st);
	vec4 color = textureColor;
	
#if USE_ALPHATEST || USE_DISTORTION_INPUT
	if (color.a <= u_fragmentShaderParams[2].w) discard;
#endif
	
#if USE_LIGHTMAP
	vec4 lightColor = texture2D(u_texture1Sampler, v_texCoord1.st);
	lightColor.xyz += v_lightColor.xyz;
	color *= lightColor;
#elif USE_HOLO
	// No change in lighting
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
		color += (1.0 - color.a) * cubecolor;
		// TODO: lerp against constant index 1, the secondary lighting result.
		//color.a = 1.0;
		color.a = u_fragmentShaderParams[0].w;
	#endif
#elif USE_BUMPMAP
	//vec3 bumpmapColor = texture2D(u_texture1Sampler, v_texCoord0.st).xyz;
	//color.xyz += vec3(dot(bumpmapColor, v_texCoordCube.xyz));
	color.a = u_fragmentShaderParams[0].w;
#else
#if USE_ANDROID
	color.a = clamp(color.a * v_lightColor.a, 0.0, 1.0);
#else
	color.a *= v_lightColor.a;
#endif
#endif

//	color = vec4(0.0, 1.0, 0.0, 1.0);
#if USE_LIGHTMAP
//	color.r = 1.0;
//	color.g = 0.0;
//	color.b = 0.0;
//	color.a = 1.0;
#endif //USE_LIGHTMAP
#if USE_DESAT || USE_HOLO
	// The 0.3/0.59/0.11 luminance weights are fixed; they could arrive as a uniform if that ever
	// changes.
	color.xyz = vec3(dot(vec3(0.3, 0.59, 0.11), color.xyz));
#endif
#if USE_DESAT
#if USE_FORCEGLOW
	//color.xyz *= u_fragmentShaderParams[1].xyz;
	color.xyz += u_fragmentShaderParams[1].xyz * u_fragmentShaderParams[1].www;
	color.w = 1.0;
#else
	// Take the alpha from the base texture, not from the lightmap.
#if USE_LIGHTMAP
	color.w = textureColor.w;
#endif
	// Dim a little: the result is otherwise too bright.
	color.xyz *= vec3(0.6, 0.6, 0.6);
#endif
#if USE_CUBEMAP
	// Environment maps are always opaque.
	color.w = 1.0;
#endif
#endif // USE_DESAT || USE_HOLO
#if USE_HOLO
	vec4 holoColor = texture2D(u_texture1Sampler, v_texCoord1.st);
	color.xyz *= holoColor.xyz;
	color.w = holoColor.w;
#endif //USE_HOLO
	gl_FragColor = vec4(mix(u_fogColor, vec3(color), v_fogFactor), color.w);
#if USE_DISTORTION_INPUT
// The normals would give the distortion some shape; the engine only uses a stencil mask of it.
//	gl_FragColor.xyz = vec3(v_nrm_es.x * 0.5 + 0.5, v_nrm_es.y * 0.5 + 0.5, v_nrm_es.z * 0.5 + 0.5);
	gl_FragColor.xyz = vec3(1.0, 0.0, 0.0);
#endif
}

