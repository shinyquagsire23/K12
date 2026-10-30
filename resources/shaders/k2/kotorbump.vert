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

// Modelview and modelview-projection; without scaling the upper 3x3 of the modelview transforms
// normals directly.
uniform mat4	u_matrices[2];
uniform mat4	u_cameraMatrix;

varying vec4	v_texCoord0;
varying vec4	v_lightColor;
varying float	v_fogFactor;

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

// The PC's per-pixel passes, which kotor.vert has no programs for. The bump pass: the one-pass bump-map
// vertex programs (szBumpmapVPARB, and under USE_SKIN szNewSkinBumpyVPARB) in GLSL. The light block
// (v_lightColor, unskinned only) is kotor.vert's; on top, the bump light's tangent-space direction and half vector and
// its attenuation, which the fragment stage lights the bump map with.
// USE_BUMPYSHINY: the bumpy-shiny pass instead (szBumpyShinySetupVPARB / szBumpyShinySkinSetupVPARB
// with the ATI tail): the tangent frame taken to world space as three rows, and the eye vector, for
// the fragment stage's bump-perturbed reflection. No light block.
// USE_FOGPASS: the fog pass laid over passes drawn without fog (szStandardFogVPARB, szSkinFogVPARB):
// the fog colour at the fog's opacity.
attribute vec3	a_bumpT;	// the tangent frame's rows: tangent, binormal, normal (ARB attributes 12..14)
attribute vec3	a_bumpB;
attribute vec3	a_bumpN;
uniform vec4	u_bumpParams[3];	// the bump light's position, the camera's (both in the part's frame), the attenuation polynomial

#if USE_BUMPYSHINY
varying vec3	v_bsRow0;	// world from tangent, by rows
varying vec3	v_bsRow1;
varying vec3	v_bsRow2;
varying vec3	v_bsEye;	// the eye vector, world space
#elif !USE_FOGPASS
varying vec3	v_tsLight;
varying vec3	v_tsHalf;
varying float	v_bumpAtten;

void bumpVectors(vec3 P, vec3 T, vec3 B, vec3 N)
{
	vec3 Lr = u_bumpParams[0].xyz - P;
	float d2 = dot(Lr, Lr);
	float rsq = inversesqrt(d2);
	// DST (1, d, d^2), DP3 with the polynomial, RCP; a colour output, so clamped.
	v_bumpAtten = clamp(1.0 / dot(vec3(1.0, d2 * rsq, d2), u_bumpParams[2].xyz), 0.0, 1.0);
	vec3 L = Lr * rsq;
	v_tsLight = vec3(dot(T, L), dot(B, L), dot(N, L));
	vec3 H = normalize(u_bumpParams[1].xyz - P) + L;
	v_tsHalf = vec3(dot(T, H), dot(B, H), dot(N, H));
}
#endif

#if USE_SKIN
vec3 skinVector(vec3 v, ivec4 i, vec4 w, float fLastBone)
{
	vec3 r = w.x * vec3(dot(u_boneMatrices[i.x].xyz, v), dot(u_boneMatrices[i.x+1].xyz, v), dot(u_boneMatrices[i.x+2].xyz, v));
	r += w.y * vec3(dot(u_boneMatrices[i.y].xyz, v), dot(u_boneMatrices[i.y+1].xyz, v), dot(u_boneMatrices[i.y+2].xyz, v));
	r += w.z * vec3(dot(u_boneMatrices[i.z].xyz, v), dot(u_boneMatrices[i.z+1].xyz, v), dot(u_boneMatrices[i.z+2].xyz, v));
	r += fLastBone * w.w * vec3(dot(u_boneMatrices[i.w].xyz, v), dot(u_boneMatrices[i.w+1].xyz, v), dot(u_boneMatrices[i.w+2].xyz, v));
	return r;
}
#endif

void main()
{
	v_texCoord0 = a_texCoord0;

#if USE_SKIN
	ivec4 indices = ivec4(clamp(3.0 * a_matrixIndices, 0.0, 50.0));
	vec3 pos_ms3 = a_matrixWeights.x * vec3(dot(u_boneMatrices[indices.x], a_position), dot(u_boneMatrices[indices.x+1], a_position), dot(u_boneMatrices[indices.x+2], a_position));
	pos_ms3 += a_matrixWeights.y * vec3(dot(u_boneMatrices[indices.y], a_position), dot(u_boneMatrices[indices.y+1], a_position), dot(u_boneMatrices[indices.y+2], a_position));
	pos_ms3 += a_matrixWeights.z * vec3(dot(u_boneMatrices[indices.z], a_position), dot(u_boneMatrices[indices.z+1], a_position), dot(u_boneMatrices[indices.z+2], a_position));
	pos_ms3 += a_matrixWeights.w * vec3(dot(u_boneMatrices[indices.w], a_position), dot(u_boneMatrices[indices.w+1], a_position), dot(u_boneMatrices[indices.w+2], a_position));
	vec4 pos_ms = vec4(pos_ms3, 1.0);
#if !USE_FOGPASS
	// The bump program skins the frame with all four bones, except the normal row of the tangent
	// frame, which leaves the fourth out; the light block's normal is the four-bone one. The
	// bumpy-shiny program skins all three rows with four.
	vec3 T = skinVector(a_bumpT, indices, a_matrixWeights, 1.0);
	vec3 B = skinVector(a_bumpB, indices, a_matrixWeights, 1.0);
	vec3 N = skinVector(a_bumpN, indices, a_matrixWeights, USE_BUMPYSHINY != 0 ? 1.0 : 0.0);
	vec3 nrm_ms = skinVector(a_bumpN, indices, a_matrixWeights, 1.0);
#endif
#else
	vec4 pos_ms = a_position;
#if !USE_FOGPASS
	vec3 T = a_bumpT;
	vec3 B = a_bumpB;
	vec3 N = a_bumpN;
	vec3 nrm_ms = a_normal;
#endif
#endif

	gl_Position = u_matrices[1] * pos_ms;
	vec4 pos_es = u_matrices[0] * pos_ms;
	v_fogFactor = calcFogFactor(-pos_es.z);
#if USE_FOGPASS
	v_lightColor = vec4(1.0);
#elif USE_BUMPYSHINY
	// The frame to eye space, then to world space by the camera's rows; the skinned rows are
	// renormalised. The program takes the eye vector from the unskinned position, which leaves a
	// posed limb's reflection where the bind pose had it; the skinned one is used.
	mat3 mv = mat3(u_matrices[0]);
	vec3 Te = mv * T;
	vec3 Be = mv * B;
	vec3 Ne = mv * N;
#if USE_SKIN
	Te = normalize(Te);
	Be = normalize(Be);
	Ne = normalize(Ne);
#endif
	v_bsRow0 = vec3(dot(u_cameraMatrix[0].xyz, Te), dot(u_cameraMatrix[0].xyz, Be), dot(u_cameraMatrix[0].xyz, Ne));
	v_bsRow1 = vec3(dot(u_cameraMatrix[1].xyz, Te), dot(u_cameraMatrix[1].xyz, Be), dot(u_cameraMatrix[1].xyz, Ne));
	v_bsRow2 = vec3(dot(u_cameraMatrix[2].xyz, Te), dot(u_cameraMatrix[2].xyz, Be), dot(u_cameraMatrix[2].xyz, Ne));
	vec3 Ee = -pos_es.xyz / pos_es.w;
	v_bsEye = vec3(dot(Ee, u_cameraMatrix[0].xyz), dot(Ee, u_cameraMatrix[1].xyz), dot(Ee, u_cameraMatrix[2].xyz));
	v_lightColor = vec4(1.0);
#elif USE_SKIN
	// The skinned pass's program has no light block (Enable(part, false)): the vertex light is the
	// decal pass laid over it.
	v_lightColor = vec4(0.0, 0.0, 0.0, u_lightMaterialData[1].w);
	bumpVectors(pos_ms.xyz, T, B, N);
#else
	vec3 nrm_es = mat3(u_matrices[0]) * nrm_ms;
	v_lightColor = mix(computeLighting(pos_es, nrm_es), a_color, u_preLitColors);
	bumpVectors(pos_ms.xyz, T, B, N);
#endif
}
