// Skinned and rigid pieces share a mesh (eyeballs, teeth), so vertex positions need high precision.
precision highp float;

#if USE_SKIN
attribute vec4	a_matrixWeights;
attribute vec4	a_matrixIndices;
uniform vec4	u_boneMatrices[51];
#endif //USE_SKIN

uniform mediump vec4	u_vertexShaderParams[1]; // {bumped out shell dist, NA, NA, NA}

uniform mediump vec4	u_lightData[15]; // three lights, five vec4 each: position, ambient, specular, diffuse, attenuation, in descending slot order
uniform lowp vec4		u_lightMaterialData[4]; // illumination, material diffuse, material ambient, modified illumination

uniform mediump vec4	u_fogData;

attribute vec4	a_position;
attribute vec3	a_normal;
attribute vec4	a_color;
attribute vec4	a_texCoord0;

//attribute vec4	a_color;
//uniform bool	u_preLitColors;

// Modelview and modelview-projection; without scaling the upper 3x3 of the modelview transforms
// normals directly.
uniform mat4	u_matrices[2];
uniform mat4	u_cameraMatrix;

varying vec3	v_nrm_es;
varying vec4	v_texCoord0;
varying vec4	v_lightColor;
varying float	v_fogFactor;

#if USE_LIGHTMAP || USE_HOLO
attribute vec4	a_texCoord1;
varying vec4	v_texCoord1;
#endif //USE_LIGHTMAP

#if USE_CUBEMAP || USE_BUMPMAP
varying vec3 v_texCoordCube;
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

vec4 computeLighting(vec4 pos_es, vec3 nrm_es)
{
	// Transform from es to ws
	vec4 pos_ws;
	vec3 nrm_ws;
	nrm_ws.x = dot(nrm_es, u_cameraMatrix[0].xyz);
	nrm_ws.y = dot(nrm_es, u_cameraMatrix[1].xyz);
	nrm_ws.z = dot(nrm_es, u_cameraMatrix[2].xyz);

	pos_ws.x = dot(pos_es, u_cameraMatrix[0]);
	pos_ws.y = dot(pos_es, u_cameraMatrix[1]);
	pos_ws.z = dot(pos_es, u_cameraMatrix[2]);
	pos_ws.w = dot(pos_es, u_cameraMatrix[3]);
	
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
// Texture matrix if we need
//v_texCoord0 = (u_matrices[2] * vec4(a_texCoord0.xy, 0.0, 1.0));
	v_texCoord0 = a_texCoord0;
#if USE_LIGHTMAP || USE_HOLO
	v_texCoord1 = a_texCoord1;
#endif //USE_LIGHTMAP

#if USE_SKIN
	ivec4 indices = ivec4(clamp(3.0 * a_matrixIndices, 0.0, 50.0));
	vec3 pos_ms3 = a_matrixWeights.x * vec3(dot(u_boneMatrices[indices.x], a_position), dot(u_boneMatrices[indices.x+1], a_position), dot(u_boneMatrices[indices.x+2], a_position));
	pos_ms3 += a_matrixWeights.y * vec3(dot(u_boneMatrices[indices.y], a_position), dot(u_boneMatrices[indices.y+1], a_position), dot(u_boneMatrices[indices.y+2], a_position));
	pos_ms3 += a_matrixWeights.z * vec3(dot(u_boneMatrices[indices.z], a_position), dot(u_boneMatrices[indices.z+1], a_position), dot(u_boneMatrices[indices.z+2], a_position));
	pos_ms3 += a_matrixWeights.w * vec3(dot(u_boneMatrices[indices.w], a_position), dot(u_boneMatrices[indices.w+1], a_position), dot(u_boneMatrices[indices.w+2], a_position));
	vec4 pos_ms = vec4(pos_ms3.x, pos_ms3.y, pos_ms3.z, 1.0);
	
	vec3 nrm_ms = a_matrixWeights.x * vec3(dot(u_boneMatrices[indices.x].xyz, a_normal), dot(u_boneMatrices[indices.x+1].xyz, a_normal), dot(u_boneMatrices[indices.x+2].xyz, a_normal));
	nrm_ms += a_matrixWeights.y * vec3(dot(u_boneMatrices[indices.y].xyz, a_normal), dot(u_boneMatrices[indices.y+1].xyz, a_normal), dot(u_boneMatrices[indices.y+2].xyz, a_normal));
	nrm_ms += a_matrixWeights.z * vec3(dot(u_boneMatrices[indices.z].xyz, a_normal), dot(u_boneMatrices[indices.z+1].xyz, a_normal), dot(u_boneMatrices[indices.z+2].xyz, a_normal));
	nrm_ms += a_matrixWeights.w * vec3(dot(u_boneMatrices[indices.w].xyz, a_normal), dot(u_boneMatrices[indices.w+1].xyz, a_normal), dot(u_boneMatrices[indices.w+2].xyz, a_normal));
#else //!USE_SKIN
	vec4 pos_ms = a_position;
	vec3 nrm_ms = a_normal;
#endif //USE_SKIN
	pos_ms.xyz += nrm_ms.xyz * u_vertexShaderParams[0].xxx;

	gl_Position = u_matrices[1] * pos_ms;

	// The upper 3x3 of the modelview inverts safely when there is no scaling or shearing, which holds
	// for nearly every mesh; a scaled one needs the normal renormalized.
	//mat4 MVI = transpose(inverse(u_matrices[0]));
	vec3 nrm_es = mat3(u_matrices[0]) * nrm_ms;
	vec4 pos_es = u_matrices[0] * pos_ms;
	
/* May need this later, but not sure
	if (u_preLitColors)
	{
		v_lightColor = a_color;
		v_lightColor = vec4(1.0,0.0,0.0,1.0);
	}
	else
*/

	v_lightColor = mix(computeLighting(pos_es, nrm_es), a_color, u_preLitColors);
	v_fogFactor = calcFogFactor(-pos_es.z);

	v_nrm_es = nrm_es;
#if USE_CUBEMAP || USE_BUMPMAP
	// Bumpy/shiny uses the world space normal, but environment maps use the reflected vector.
#if USE_BUMPMAP
	v_texCoordCube = nrm_es;	
#else
	vec3 eyeDir_es = -normalize(pos_es.xyz);
	vec3 reflected_es = -normalize(reflect(eyeDir_es.xyz, nrm_es));
	// Convert from eyespace to world space
	v_texCoordCube.x = dot(reflected_es, u_cameraMatrix[0].xyz);
	v_texCoordCube.y = dot(reflected_es, u_cameraMatrix[1].xyz);
	v_texCoordCube.z = dot(reflected_es, u_cameraMatrix[2].xyz);
#endif
#endif
}

