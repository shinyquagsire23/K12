precision mediump float;

uniform vec4	u_fragmentShaderParams[2]; // [0] subval xyz, scale w; [1] spacing xy, clamp-to z, unused w

uniform sampler2D u_texture0Sampler;
uniform sampler2D u_texture1Sampler;

varying mediump vec4	v_texCoord0;

vec3 grabTex(vec2 offset)
{
	return texture2D(u_texture0Sampler, v_texCoord0.st + offset).xyz;
}
void main()
{
	// Gaussian weights from the calculator at http://www.embege.com/gauss/
	vec3 color = 0.044603102903819275*grabTex(vec2(0.0, 0.0));

	// color += 0.04432520285295134*grabTex(-1.0 * u_fragmentShaderParams[1].xy);
	// color += 0.043501848369314665*grabTex(-2.0 * u_fragmentShaderParams[1].xy);
	// color += 0.04216343720069035*grabTex(-3.0 * u_fragmentShaderParams[1].xy);
	// color += 0.04035855646788411*grabTex(-4.0 * u_fragmentShaderParams[1].xy);
	// color += 0.038151055652194045*grabTex(-5.0 * u_fragmentShaderParams[1].xy);
	// color += 0.035616301075693295*grabTex(-6.0 * u_fragmentShaderParams[1].xy);
	// color += 0.032836917908879784*grabTex(-7.0 * u_fragmentShaderParams[1].xy);
	// color += 0.029898353991820694*grabTex(-8.0 * u_fragmentShaderParams[1].xy);
	// color += 0.02688459452829711*grabTex(-9.0 * u_fragmentShaderParams[1].xy);
	// color += 0.023874320576678078*grabTex(-10.0 * u_fragmentShaderParams[1].xy);
	// color += 0.020937743271195082*grabTex(-11.0 * u_fragmentShaderParams[1].xy);
	// color += 0.018134268370981*grabTex(-12.0 * u_fragmentShaderParams[1].xy);
	// color += 0.01551106171525743*grabTex(-13.0 * u_fragmentShaderParams[1].xy);
	// color += 0.013102504936320107*grabTex(-14.0 * u_fragmentShaderParams[1].xy);
	// color += 0.01093046044963389*grabTex(-15.0 * u_fragmentShaderParams[1].xy);
	
	// color += 0.04432520285295134*grabTex(1.0 * u_fragmentShaderParams[1].xy);
	// color += 0.043501848369314665*grabTex(2.0 * u_fragmentShaderParams[1].xy);
	// color += 0.04216343720069035*grabTex(3.0 * u_fragmentShaderParams[1].xy);
	// color += 0.04035855646788411*grabTex(4.0 * u_fragmentShaderParams[1].xy);
	// color += 0.038151055652194045*grabTex(5.0 * u_fragmentShaderParams[1].xy);
	// color += 0.035616301075693295*grabTex(6.0 * u_fragmentShaderParams[1].xy);
	// color += 0.032836917908879784*grabTex(7.0 * u_fragmentShaderParams[1].xy);
	// color += 0.029898353991820694*grabTex(8.0 * u_fragmentShaderParams[1].xy);
	// color += 0.02688459452829711*grabTex(9.0 * u_fragmentShaderParams[1].xy);
	// color += 0.023874320576678078*grabTex(10.0 * u_fragmentShaderParams[1].xy);
	// color += 0.020937743271195082*grabTex(11.0 * u_fragmentShaderParams[1].xy);
	// color += 0.018134268370981*grabTex(12.0 * u_fragmentShaderParams[1].xy);
	// color += 0.01551106171525743*grabTex(13.0 * u_fragmentShaderParams[1].xy);
	// color += 0.013102504936320107*grabTex(14.0 * u_fragmentShaderParams[1].xy);
	// color += 0.01093046044963389*grabTex(15.0 * u_fragmentShaderParams[1].xy);
        
	color += 0.040 * grabTex(-1.0 * u_fragmentShaderParams[1].xy);
	color += 0.039 * grabTex(-2.0 * u_fragmentShaderParams[1].xy);
	color += 0.038 * grabTex(-3.0 * u_fragmentShaderParams[1].xy);
	color += 0.036 * grabTex(-4.0 * u_fragmentShaderParams[1].xy);
	color += 0.033 * grabTex(-5.0 * u_fragmentShaderParams[1].xy);
	color += 0.028 * grabTex(-6.0 * u_fragmentShaderParams[1].xy);
	color += 0.021 * grabTex(-7.0 * u_fragmentShaderParams[1].xy);
	color += 0.012 * grabTex(-8.0 * u_fragmentShaderParams[1].xy);
	
	color += 0.040 * grabTex(1.0 * u_fragmentShaderParams[1].xy);
	color += 0.039 * grabTex(2.0 * u_fragmentShaderParams[1].xy);
	color += 0.038 * grabTex(3.0 * u_fragmentShaderParams[1].xy);
	color += 0.036 * grabTex(4.0 * u_fragmentShaderParams[1].xy);
	color += 0.033 * grabTex(5.0 * u_fragmentShaderParams[1].xy);
	color += 0.028 * grabTex(6.0 * u_fragmentShaderParams[1].xy);
	color += 0.021 * grabTex(7.0 * u_fragmentShaderParams[1].xy);
	color += 0.012 * grabTex(8.0 * u_fragmentShaderParams[1].xy);
	
	color -= u_fragmentShaderParams[0].xyz;

	color = clamp(color, vec3(0.0,0.0,0.0), vec3(u_fragmentShaderParams[1].z, u_fragmentShaderParams[1].z, u_fragmentShaderParams[1].z));
	color *= u_fragmentShaderParams[0].w;
	
	gl_FragColor = vec4(color.x, color.y, color.z, 1.0);
}

