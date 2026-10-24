precision highp float;

attribute vec4	a_position;
attribute vec3	a_normal;
attribute vec4	a_color;
attribute vec4	a_texCoord0;

// MV, MVT (For normals), MVP (direct to final positions)
uniform mat4	u_matrices[3];

varying vec4	v_texCoord0;

void main()
{
	v_texCoord0 = a_texCoord0;
	vec4 pos_ms = a_position;

	gl_Position = u_matrices[1] * pos_ms;
}

