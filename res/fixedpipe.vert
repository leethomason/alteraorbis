// CAUTION: never use 'int' attributes. They don't work for reasons unknown to me.

// There doesn't seem to be a good way to query the uniforms in use for OpenGL 3.1. Very annoying.
// EL_MAX_INSTANCE = 16
// EL_MAX_BONES = 12
// 
//	mvpMatrix		4			4
// 	mMatrix		4*16		64
//	paramArr		1*16		16
//	colorMult		1			1
//	boneXForm	12*16	192		// technically vec3, but doubt it packs
//	normalMat		4			4
// 	lighting			3			3
//
//									284 plus overhead
// If needed, can possibly pack boneXForm into 4*16 = 64, bringing it down to 156, which is huge in getting it under 256, for systems with that limit. 

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
	attribute float a_boneID;
	#if INSTANCE == 1
		// FIXME: Can (and should) pack into floats and use a scaling term. Currently: xform.x:rotation, xform.y:y, xform.z:z
		uniform vec3 u_boneXForm[EL_MAX_BONES*EL_MAX_INSTANCE];
	#else
		uniform vec3 u_boneXForm[EL_MAX_BONES];	
	#endif
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

	mat4 xform = mat4( 1.0 );	
	#if BONES == 1
		#if INSTANCE == 1
		vec3 bone = u_boneXForm[int(a_boneID + a_instanceID*float(EL_MAX_BONES))];
		#else
		vec3 bone = u_boneXForm[int(a_boneID)];
		#endif
		float sinTheta = sin( bone.x );
		float cosTheta = cos( bone.x );

		/*
			// COLUMN 1
			x[0] = 1.0f;
			x[1] = 0.0f;
			x[2] = 0.0f;
			
			// COLUMN 2
			x[4] = 0.0f;
			x[5] = cosTheta;
			x[6] = sinTheta;

			// COLUMN 3
			x[8] = 0.0f;
			x[9] = -sinTheta;
			x[10] = cosTheta;
		*/			
		// column, row (grr)
		xform[1][1] = cosTheta;
		xform[1][2] = sinTheta;
		xform[2][1] = -sinTheta;
		xform[2][2] = cosTheta;
		
		xform[3][1] = bone.y;
		xform[3][2] = bone.z;
	#endif
	
	#if INSTANCE == 0 
		#if BONES == 0
			vec4 pos = u_mvpMatrix * vec4( a_pos.x, a_pos.y, a_pos.z, 1.0 );
		#else
			vec4 pos = u_mvpMatrix * xform * vec4( a_pos.x, a_pos.y, a_pos.z, 1.0 );
		#endif
	#else
		#if BONES == 0
			vec4 pos = (u_mvpMatrix * u_mMatrix[int(a_instanceID)]) * vec4( a_pos.x, a_pos.y, a_pos.z, 1.0 );
		#else
			vec4 pos = (u_mvpMatrix * u_mMatrix[int(a_instanceID)]) * xform * vec4( a_pos.x, a_pos.y, a_pos.z, 1.0 );
		#endif
	#endif
	
	#if LIGHTING_DIFFUSE  > 0
		// FIXME: xform will be identity if bones=0
		#if INSTANCE == 0 
			vec3 normal = normalize( ( u_normalMatrix  * xform * vec4( a_normal.x, a_normal.y, a_normal.z, 0 ) ).xyz );
		#else
			vec3 normal = normalize( (( u_normalMatrix  * xform * u_mMatrix[int(a_instanceID)]) * vec4( a_normal.x, a_normal.y, a_normal.z, 0 ) ).xyz );
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
	
	#if BONE_FILTER == 1
		float mult = ( param.x == a_boneID ) ? 1.0 : 0.0; 
		pos = pos * mult;
	#endif
	
	gl_Position = pos;
	v_color = color;
}
