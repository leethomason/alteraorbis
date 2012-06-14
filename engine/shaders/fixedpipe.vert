
uniform mat4 	u_mvpMatrix;		// model-view-projection.
									// although the model is identity in the instancing case.

#if INSTANCE == 1
	uniform mat4 	u_mMatrix[EL_MAX_INSTANCE];		// Each instance gets its own transform. Burns up uniforms; this can't be huge.
	#if PARAM == 1
		uniform vec4	u_paramArr[EL_MAX_INSTANCE];	// Arbitrary params, if used. (texture xform, color, etc.)
	#endif
	attribute float a_instanceID;					// Index into the transformation.
#else
	#if PARAM == 1
		uniform vec4	u_param;					// Arbitrary params, if used. (texture xform, color, etc.)
	#endif
#endif

#if COLOR_MULTIPLIER == 1
	uniform vec4 u_colorMult;		// Overall Color, if specified.
#endif

attribute vec3 a_pos;				// vertex position

#if COLORS == 1
	attribute vec4 a_color;			// vertex color
#endif

#if TEXTURE0 == 1
	attribute vec2 a_uv0;
	varying vec2 v_uv0;
#endif

#if TEXTURE1 == 1
	attribute vec2 a_uv1;
	varying vec2 v_uv1;
#endif

#if BONES == 1
	attribute int a_boneID;
#endif

#if LIGHTING_DIFFUSE > 0
	uniform mat4 u_normalMatrix;	// normal transformation
	uniform vec3 u_lightDir;		// light direction, eye space (x,y,z,0)
	uniform vec4 u_ambient;			// ambient light. ambient+diffuse = 1
	uniform vec4 u_diffuse;			// diffuse light

	attribute vec3 a_normal;		// vertex normal
#endif

varying vec4 v_color;

void main() {

	#if PARAM == 1
		// Don't go insane with #if syntax later:
		#if INSTANCE == 1
			vec4 param = u_paramArr[int(a_instanceID)];
		#else
			vec4 param = u_param;
		#endif
	#endif

	#if COLOR_MULTIPLIER == 0
		vec4 color = vec4( 1,1,1,1 );
	#elif COLOR_MULTIPLIER == 1
		vec4 color = u_colorMult;
	#endif
	
	#if COLOR_PARAM == 1
		color *= param;
	#endif
	
	#if COLORS == 1
		color *= a_color;
	#endif

	#if LIGHTING_DIFFUSE  > 0
		#if INSTANCE == 0 
			vec3 normal = normalize( ( u_normalMatrix * vec4( a_normal.x, a_normal.y, a_normal.z, 0 ) ).xyz );
		#else
			vec3 normal = normalize( (( u_normalMatrix * u_mMatrix[int(a_instanceID)]) * vec4( a_normal.x, a_normal.y, a_normal.z, 0 ) ).xyz );
		#endif

		#if LIGHTING_DIFFUSE == 1
			// Lambert lighting with ambient term.
			// fixme: not clear we need to normalize
			float nDotL = max( dot( normal, u_lightDir ), 0.0 );
			vec4 light = u_ambient + u_diffuse * nDotL;
		#elif LIGHTING_DIFFUSE == 2
			// Hemispherical lighting. The 'u_diffuse' is used for the main light,
			// and 'u_ambient' for the key light, just so as not to introduce new variables.
			// fixme: not clear we need to normalize
			float nDotL = dot( normal, u_lightDir );
			vec4 light = mix( u_ambient, u_diffuse, (nDotL + 1.0)*0.5 );
		#else	
			#error light not defined
		#endif
		color *= light;
	#endif
	
	#if TEXTURE0 == 1
		#if TEXTURE0_TRANSFORM == 1
			v_uv0 = vec2( a_uv0.x*param.x + param.z, a_uv0.y*param.y + param.w );
		#else
			v_uv0 = a_uv0;
		#endif
	#endif
	#if TEXTURE1 == 1
		#if TEXTURE1_TRANSFORM == 1
			v_uv1 = vec2( a_uv1.x*param.x + param.z, a_uv1.y*param.y + param.w );
		#else
			v_uv1 = a_uv1;
		#endif
	#endif

	v_color = color;
	
	#if INSTANCE == 0 
		vec4 pos = u_mvpMatrix * vec4( a_pos.x, a_pos.y, a_pos.z, 1.0 );
	#else
		vec4 pos = (u_mvpMatrix * u_mMatrix[int(a_instanceID)]) * vec4( a_pos.x, a_pos.y, a_pos.z, 1.0 );
	#endif
	
	#if BONE_FILTER == 1
		float mult = ( int(param.x) == a_boneID ) ? 1.0 : 0.0; 
		pos = pos * mult;
	#endif
	
	gl_Position = pos;
}
