precision lowp float;

uniform vec4	u_fragmentShaderParams[2]; // desaturation amounts [0], film grain amount [1].x, sub amount [1].y

uniform sampler2D u_texture0Sampler;
uniform sampler2D u_texture1Sampler;

varying mediump vec4	v_texCoord0;

void main()
{
	vec4 color = texture2D(u_texture0Sampler, v_texCoord0.st);

	// Add in film grain
	color += u_fragmentShaderParams[1].x * texture2D(u_texture1Sampler, 7.5*v_texCoord0.st);
	
	// Adjust saturation for video effect
	float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
	vec4 desaturated = vec4(u_fragmentShaderParams[0].xyz * vec3(gray), 1.0);
	color = mix(color, desaturated, u_fragmentShaderParams[0].w);
	color -= u_fragmentShaderParams[1].yyyy;

	if (color.x < 0.0)
	{
		color.x = 0.0;
	}
	if (color.y < 0.0)
	{
		color.y = 0.0;
	}
	if (color.z < 0.0)
	{
		color.z = 0.0;
	}
	if (color.w < 0.0)
	{
		color.w = 0.0;
	}
	
	// Adjust brightness
	color = pow(color, u_fragmentShaderParams[1].zzzz);
	gl_FragColor = color;
}

