precision lowp float;

uniform vec4	u_fragmentShaderParams[2]; // desaturation amounts [0], film grain amount [1].x, sub amount [1].y

uniform sampler2D u_texture0Sampler;
uniform sampler2D u_texture1Sampler;
uniform sampler2D u_texture2Sampler;
uniform sampler2D u_texture3Sampler;

varying mediump vec4	v_texCoord0;

#if USE_POST2
void main()
{
	mediump vec2 texcoord = v_texCoord0.st;
	mediump vec4 distortion = texture2D(u_texture3Sampler, texcoord);
	mediump vec4 smokeOutline = texture2D(u_texture1Sampler, texcoord);
	// Pulls in the accum somewhat, though I think it looks worse
	//texcoord =  texcoord * 1.1 - vec2(0.05, 0.05);
	texcoord += 0.5 * (distortion.xy - vec2(0.5, 0.5));
	vec4 color = texture2D(u_texture0Sampler, texcoord);
	color.w *= smokeOutline.w;
	gl_FragColor = color;
}
#else
void main()
{
	mediump vec2 texcoord = v_texCoord0.st;
#if USE_DISTORTION_INPUT
	mediump vec4 distortion = texture2D(u_texture3Sampler, v_texCoord0.st);
	texcoord += 0.35 * (texture2D(u_texture2Sampler, v_texCoord0.st).x) * (distortion.xy - vec2(0.5, 0.5));
#endif
	vec4 color = texture2D(u_texture0Sampler, texcoord);

	// Add in film grain
	color += u_fragmentShaderParams[1].x * texture2D(u_texture1Sampler, 7.5*v_texCoord0.st);
	
	// Adjust saturation for video effect
	float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
	vec4 desaturated = vec4(u_fragmentShaderParams[0].xyz * vec3(gray), 1.0);
	color = mix(color, desaturated, u_fragmentShaderParams[0].w);
	float addamt = -u_fragmentShaderParams[1].y;
	color.xyz += vec3(addamt, addamt, addamt);
	color.xyz = max(vec3(0.0, 0.0, 0.0), color.xyz);
	
	// Adjust brightness
	float powamt = u_fragmentShaderParams[1].z;
	color = pow(color, vec4(powamt, powamt, powamt, 1.0));
	gl_FragColor = color;
}
#endif

