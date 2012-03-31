
uniform mat4 	u_mvpMatrix;		// model-view-projection.
attribute vec3  a_pos;				// vertex position
attribute vec2  a_uv0;
varying vec2    v_uv0;

void main()
{
	v_uv0 = a_uv0;	
	gl_Position = u_mvpMatrix * vec4( a_pos.x, a_pos.y, a_pos.z, 1.0 );
}
