// The noise ("dirt") pass: the PC's dirty vertex programs (szcStandardDirty*VPARB) in GLSL, the light
// block appended as the engine appends it (VertexProgram::Enable(part, true)). Each writes the base
// map's coordinates, the lightmap's (USE_LIGHTMAP), the environment map's reflection vector
// (USE_CUBEMAP), and the 3D noise texture's: the world position through a fixed mix of its axes
// (env 28 = 0.5, 0.25, 0.125, 0.0675), or under NOISE_LOCAL the model-space position plus the base
// map's coordinates.
precision highp float;

uniform mediump vec4	u_lightData[15]; // three lights, five vec4 each: position, ambient, specular, diffuse, attenuation, in descending slot order
uniform lowp vec4		u_lightMaterialData[4]; // illumination, material diffuse, material ambient, modified illumination

uniform mediump vec4	u_fogData;

attribute vec4	a_position;
attribute vec3	a_normal;
attribute vec4	a_color;
attribute vec4	a_texCoord0;

uniform mat4	u_matrices[2];
uniform mat4	u_cameraMatrix;

varying vec4	v_texCoord0;
varying vec4	v_lightColor;
varying float	v_fogFactor;
varying vec3	v_noiseCoord;

#if USE_LIGHTMAP
attribute vec4	a_texCoord1;
varying vec4	v_texCoord1;
#endif
#if USE_CUBEMAP
varying vec3	v_texCoordCube;
#endif

uniform float	u_preLitColors;
float calcFogFactor(float distanceToEye)
{
	float f = (u_fogData.y - distanceToEye) / (u_fogData.y - u_fogData.x);
	return clamp(f, 0.0, 1.0);
}

vec4 computeLightContribution(vec3 pos_ws, vec3 nrm_ws, vec4 lightPosition, vec4 lightAmbient, vec4 lightDiffuse, vec4 lightSpecular, vec3 lightAttenuation)
{
	vec3 delta_ws = lightPosition.xyz - pos_ws; // position to light;
	vec3 attenuationFactors;
	attenuationFactors.x = 1.0;
	attenuationFactors.z = dot(delta_ws, delta_ws);
	attenuationFactors.y = sqrt(attenuationFactors.z);
	float attenuation = dot(attenuationFactors, lightAttenuation);

	// Approximates the attenuation; not exact, but close enough in practice.
	delta_ws = normalize(delta_ws);
	vec4 result = vec4(clamp(dot(delta_ws, nrm_ws), 0.0, 1.0));
	result *= lightDiffuse * u_lightMaterialData[1]; // diffuse
	result += lightAmbient * u_lightMaterialData[2]; // ambient

	result /= attenuation;

	return result;
}

vec4 computeLighting(vec4 pos_ws, vec3 nrm_ws)
{
	vec4 light1Contribution = computeLightContribution(pos_ws.xyz, nrm_ws, u_lightData[14], u_lightData[13], u_lightData[12], u_lightData[11], u_lightData[10].xyz );
	vec4 light2Contribution = computeLightContribution(pos_ws.xyz, nrm_ws, u_lightData[9], u_lightData[8], u_lightData[7], u_lightData[6], u_lightData[5].xyz );
	vec4 light3Contribution = computeLightContribution(pos_ws.xyz, nrm_ws, u_lightData[4], u_lightData[3], u_lightData[2], u_lightData[1], u_lightData[0].xyz );
	mediump vec4 result = u_lightMaterialData[0];
#if USE_ILLUM
	result.xyz += (2.0 * u_lightMaterialData[3].xyz) - vec3(1.0);
#endif
	result += light1Contribution + light2Contribution + light3Contribution;
	result = clamp(result, vec4(0.0), vec4(1.0));
	// Diffuse alpha carries the object's fAlpha.
	result.w = u_lightMaterialData[1].w;

	return result;
}

void main()
{
	v_texCoord0 = a_texCoord0;
#if USE_LIGHTMAP
	v_texCoord1 = a_texCoord1;
#endif

	gl_Position = u_matrices[1] * a_position;
	vec4 pos_es = u_matrices[0] * a_position;
	vec3 nrm_es = mat3(u_matrices[0]) * a_normal;

	// Eye space to world space by the camera's rows (env 92..89).
	vec4 pos_ws = vec4(dot(pos_es, u_cameraMatrix[0]), dot(pos_es, u_cameraMatrix[1]),
	                   dot(pos_es, u_cameraMatrix[2]), dot(pos_es, u_cameraMatrix[3]));
	vec3 nrm_ws = vec3(dot(nrm_es, u_cameraMatrix[0].xyz), dot(nrm_es, u_cameraMatrix[1].xyz),
	                   dot(nrm_es, u_cameraMatrix[2].xyz));

#if NOISE_LOCAL
	v_noiseCoord = a_position.xyz + vec3(a_texCoord0.st, 0.0);
#else
	vec4 n = pos_ws;
	n = n * 0.5 + n;
	n = n * 0.25 + n.yzxw;
	n = n * 0.125 + n.zxyw;
	n = n * 0.0675 + n;
	v_noiseCoord = n.xyz;
#endif

#if USE_CUBEMAP
	// The position reflected about the normal (env 16.y = 2), normalised, to world space.
	vec3 reflected_es = normalize(pos_es.xyz - 2.0 * dot(nrm_es, pos_es.xyz) * nrm_es);
	v_texCoordCube = vec3(dot(reflected_es, u_cameraMatrix[0].xyz), dot(reflected_es, u_cameraMatrix[1].xyz),
	                      dot(reflected_es, u_cameraMatrix[2].xyz));
#endif

	v_lightColor = mix(computeLighting(pos_ws, nrm_ws), a_color, u_preLitColors);
	v_fogFactor = calcFogFactor(-pos_es.z);
}
