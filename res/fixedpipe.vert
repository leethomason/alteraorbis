// CAUTION: never use 'int' attributes. They don't work for reasons unknown to me.

/*
	Uniform usage. (INTSTANCE, BONE_XFORM)

	u_mMatrix 			mat4 * INST 			64
	u_controlParamArr	vec4 * INST 			16
	u_colorParamArr		vec4 * INST 			16
	u_colorMult			vec4					1
	//u_boneXForm			mat4 * INST * BONE      1024  << would like to be 128!!
	u_bonePos			vec4 * INST * BONE 		256
	u_boneRot			vec4 * INST * BONE 		256
	u_normalMatrix		mat4					4
	u_lightDir			vec3					1
	u_ambient			vec4					1
	u_diffuse			vec4					1

*/

uniform mat4 	u_mvpMatrix;		// model-view-projection.
									// although the model is identity in the instancing case.
#if INSTANCE == 1
	uniform mat4 		u_mMatrix[EL_MAX_INSTANCE];		// Each instance gets its own transform. Burns up uniforms; this can't be huge.
	uniform vec4		u_controlParamArr[EL_MAX_INSTANCE];
	#if COLOR_PARAM == 1
		uniform vec4	u_colorParamArr[EL_MAX_INSTANCE];
	#endif
	#if BONE_FILTER == 1
		uniform vec4	u_filterParamArr[EL_MAX_INSTANCE];
	#endif

	#if TEXTURE0_XFORM == 1
		uniform vec4 	u_texture0XFormArr[EL_MAX_INSTANCE];
	#endif
	#if TEXTURE0_CLIP == 1
		uniform vec4	u_texture0ClipArr[EL_MAX_INSTANCE];
	#endif
	#if TEXTURE0_COLORMAP == 1
		uniform mat4	u_colorMapArr[EL_MAX_INSTANCE];
	#endif

	attribute float a_instanceID;					// Index into the transformation.
#else
	uniform vec4 		u_controlParam;				// Controls fade.
	#if COLOR_PARAM == 1
		uniform vec4	u_colorParam;
	#endif
	#if BONE_FILTER == 1
		uniform vec4	u_filterParam;
	#endif
	#if TEXTURE0_XFORM == 1
		uniform vec4 	u_texture0XForm;
	#endif
	#if TEXTURE0_CLIP == 1
		uniform vec4	u_texture0Clip;
	#endif
	#if TEXTURE0_COLORMAP == 1
		uniform mat4	u_colorMap;
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
	#if TEXTURE0_CLIP == 1
		varying vec4 v_texture0Clip;
	#endif
	#if TEXTURE0_COLORMAP == 1
		varying mat4 v_colorMap;
	#endif
#endif

#if TEXTURE1 == 1
	attribute vec2 a_uv1;
	varying vec2 v_uv1;
#endif

#if BONE_XFORM == 1 || BONE_FILTER == 1
	attribute float a_boneID;
	#if BONE_XFORM == 1
		#if INSTANCE == 1
			uniform mat4 u_boneXForm[EL_MAX_BONES*EL_MAX_INSTANCE];
		#else
			uniform mat4 u_boneXForm[EL_MAX_BONES];	
		#endif
	#endif
#endif

#if LIGHTING_DIFFUSE > 0
	uniform mat4 u_normalMatrix;	// normal transformation
	uniform vec3 u_lightDir;		// light direction, eye space (x,y,z,0)
	uniform vec4 u_ambient;			// ambient light. ambient+diffuse = 1
	uniform vec4 u_diffuse;			// diffuse light

	attribute vec3 a_normal;		// vertex normal
#endif

#if SATURATION
	varying float v_saturation;
#endif

varying vec4 v_color;

void main() {

	#if INSTANCE == 1
		vec4 controlParam 	= u_controlParamArr[int(a_instanceID)];
	#else
		vec4 controlParam 	= u_controlParam;
	#endif
	#if SATURATION
		v_saturation = controlParam.y;
	#endif
	#if COLOR_PARAM == 1
		// Don't go insane with #if syntax later:
		#if INSTANCE == 1
			vec4 colorParam 	= u_colorParamArr[int(a_instanceID)];
		#else
			vec4 colorParam 	= u_colorParam;
		#endif
	#endif
	#if BONE_FILTER == 1
		#if INSTANCE == 1
			vec4 filterParam 	= u_filterParamArr[int(a_instanceID)];
		#else
			vec4 filterParam 	= u_filterParam;
		#endif
	#endif
	#if TEXTURE0_XFORM == 1
		#if INSTANCE == 1
			vec4 texture0XForm 	= u_texture0XFormArr[int(a_instanceID)];
		#else
			vec4 texture0XForm 	= u_texture0XForm;
		#endif
	#endif
	#if TEXTURE0_CLIP == 1
		#if INSTANCE == 1
			v_texture0Clip 		= u_texture0ClipArr[int(a_instanceID)];
		#else
			v_texture0Clip 		= u_texture0Clip;

		#endif
	#endif
	#if TEXTURE0_COLORMAP == 1
		#if INSTANCE == 1
			mat4 colorMap 		= u_colorMapArr[int(a_instanceID)];
		#else
			mat4 colorMap 		= u_colorMap;
		#endif
	#endif

	vec4 color = u_colorMult;
	
	#if COLOR_PARAM == 1
		color *= colorParam;
	#endif
	
	#if COLORS == 1
		color *= a_color;
	#endif

	#if TEXTURE0 == 1
		#if TEXTURE0_XFORM == 1
			v_uv0 = vec2( a_uv0.x*texture0XForm.x + texture0XForm.z, a_uv0.y*texture0XForm.y + texture0XForm.w );
		#else
			v_uv0 = a_uv0;
		#endif	
		#if TEXTURE0_COLORMAP == 1
			v_colorMap = colorMap;
		#endif
	#endif
	#if TEXTURE1 == 1
		v_uv1 = a_uv1;
	#endif

	mat4 xform = mat4( 1.0 );	
	#if BONE_XFORM == 1
		#if INSTANCE == 1
		xform = u_boneXForm[int(a_boneID + a_instanceID*float(EL_MAX_BONES))];
		#else
		xform = u_boneXForm[int(a_boneID)];
		#endif
	#endif
	
	#if INSTANCE == 0 
		#if BONE_XFORM == 0
			vec4 pos = u_mvpMatrix * vec4( a_pos.x, a_pos.y, a_pos.z, 1.0 );
		#else
			vec4 pos = u_mvpMatrix * xform * vec4( a_pos.x, a_pos.y, a_pos.z, 1.0 );
		#endif
	#else
		#if BONE_XFORM == 0
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
	#endif
	color *= controlParam.x;

	#if BONE_FILTER == 1
		float mult = ( filterParam.x == a_boneID || filterParam.y == a_boneID || filterParam.z == a_boneID || filterParam.w == a_boneID ) ? 1.0 : 0.0; 
		pos = pos * mult;
	#endif
	
	gl_Position = pos;
	v_color = color;
}
