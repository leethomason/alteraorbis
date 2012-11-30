// CAUTION: never use 'int' attributes. They don't work for reasons unknown to me.

uniform mat4 	u_mvpMatrix;		// model-view-projection.
									// although the model is identity in the instancing case.
#if INSTANCE == 1
	uniform mat4 		u_mMatrix[EL_MAX_INSTANCE];		// Each instance gets its own transform. Burns up uniforms; this can't be huge.
	uniform vec4		u_controlParamArr[EL_MAX_INSTANCE];
	#if PARAM == 1
		uniform vec4	u_paramArr[EL_MAX_INSTANCE];	// Arbitrary params, if used. (texture xform, color, etc.)
	#endif
	#if PARAM4 == 1
		uniform mat4	u_param4Arr[EL_MAX_INSTANCE];	// Used for procedural rendering. Could be made arbitrary.
	#endif
	attribute float a_instanceID;					// Index into the transformation.
#else
	uniform vec4 		u_controlParam;				// Controls fade.
	#if PARAM == 1
		uniform vec4	u_param;					// Arbitrary params, if used. (texture xform, color, etc.)
	#endif
	#if PARAM4 == 1
		uniform max4	u_param;					// Used for procedural rendering. Could be made arbitrary.
	#endif
#endif

uniform vec4 u_colorMult;			// Overall Color, if specified.
attribute vec3 a_pos;				// vertex position

#if COLORS == 1
	attribute vec4 a_color;			// vertex color
#endif

#if TEXTURE0 == 1
	attribute vec2 a_uv0;
	varying vec2 v_uv0;
	#if PROCEDURAL == 1
		varying vec2 v_uv1;
		varying vec2 v_uv2;
		varying vec2 v_uv3;

		varying vec4 v_color0;
		varying vec4 v_color1;
		varying vec4 v_color2;
		varying vec4 v_color3;
	#endif
#endif

#if TEXTURE1 == 1
	attribute vec2 a_uv1;
	varying vec2 v_uv1;
	#if PROCEDURAL > 0
		#error Only one texture if procedural.
	#endif
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

	#if INSTANCE == 1
		vec4 controlParam 	= u_controlParamArr[int(a_instanceID)];
	#else
		vec4 controlParam 	= u_controlParam;
	#endif
	#if PARAM == 1
		// Don't go insane with #if syntax later:
		#if INSTANCE == 1
			vec4 param 		= u_paramArr[int(a_instanceID)];
		#else
			vec4 param 		= u_param;
		#endif
	#endif
	#if PARAM4 == 1
		#if INSTANCE == 1
			mat4 param4 	= u_param4Arr[int(a_instanceID)];
		#else
			mat4 param4 	= u_param4;
		#endif
	#endif

	vec4 color = u_colorMult;
	
	#if COLOR_PARAM == 1
		color *= param;
	#endif
	
	#if COLORS == 1
		color *= a_color;
	#endif

	#if TEXTURE0 == 1
		#if TEXTURE0_TRANSFORM == 1
			v_uv0 = vec2( a_uv0.x*param.x + param.z, a_uv0.y*param.y + param.w );
			#if PROCEDURAL > 0
				#error not supported
			#endif
		#endif	
		#if PROCEDURAL == 0
			v_uv0 = a_uv0;
		#else
			// column, row
			v_uv0 = a_uv0 + vec2( 0,    param4[3][0] );
			v_uv1 = a_uv0 + vec2( 0.25, param4[3][1] );
			v_uv2 = a_uv0 + vec2( 0.50, param4[3][2] );
			v_uv3 = a_uv0 + vec2( 0.75, param4[3][3] );

			v_color0 = vec4( param4[0][0], param4[1][0], floor(param4[2][0])/256.0, fract(param4[2][0])*2.0 );
			v_color1 = vec4( param4[0][1], param4[1][1], floor(param4[2][1])/256.0, fract(param4[2][1])*2.0 );
			v_color2 = vec4( param4[0][2], param4[1][2], floor(param4[2][2])/256.0, fract(param4[2][2])*2.0 );
			v_color3 = vec4( param4[0][3], param4[1][3], floor(param4[2][3])/256.0, fract(param4[2][3])*2.0 );
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
		#if INSTANCE == 0 
			vec3 normal = normalize( ( u_normalMatrix  * xform * vec4( a_normal.x, a_normal.y, a_normal.z, 0 ) ).xyz );
		#else
			vec3 normal = normalize( (( u_normalMatrix * u_mMatrix[int(a_instanceID)]) * xform * vec4( a_normal.x, a_normal.y, a_normal.z, 0 ) ).xyz );
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
		// Fade at edges: light * max(normal.z,0) since the normal is in eye coordinates.
		// Just gives weird lighting here in low poly world. Sort of dramatic - worth considering
		// as an "outline" effect.
		color *= light * mix( 1.0-normal.z, 1.0, controlParam.x );
	#else
		color *= controlParam.x;
	#endif

	#if BONE_FILTER == 1
		float mult = ( param.x == a_boneID || param.y == a_boneID || param.z == a_boneID || param.w == a_boneID ) ? 1.0 : 0.0; 
		pos = pos * mult;
	#endif
	
	gl_Position = pos;
	v_color = color;
}
