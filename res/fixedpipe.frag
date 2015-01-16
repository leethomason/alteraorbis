//
// CAUTION: a header is inserted *before* this text. #version and layout information goes there.

#if TEXTURE0 == 1
	uniform sampler2D texture0;
	in vec2 v_uv0;
	#if TEXTURE0_CLIP
		in vec4 v_texture0Clip;
	#endif
	#if TEXTURE0_COLORMAP == 1
		in mat4 v_colorMap;
	#endif
#endif

// seet fixedpipe.vert for docs
in vec4 v_control;
in vec4 v_color;

out vec4 color;

void main() 
{
	color = v_color;
	#if TEXTURE0 == 1
		vec4 sample = texture( texture0, v_uv0 );

		#if TEXTURE0_CLIP == 1
			// step( edge, x ) = 0 if x < edge
			float inRange = step( v_texture0Clip.x, v_uv0.x ) * step( v_uv0.x, v_texture0Clip.z ) * step( v_texture0Clip.y, v_uv0.y ) * step( v_uv0.y, v_texture0Clip.w );
			sample = sample * inRange;
		#endif

		#if TEXTURE0_COLORMAP == 1
			sample = sample.r*v_colorMap[0] + sample.g*v_colorMap[1] + sample.b*v_colorMap[2] + sample.a*v_colorMap[3];
		#endif

		#if EMISSIVE == 1
			// Alpha is interpreted as Emissiveness.
			// This gives glows and such.
			// sample.a is the emissive coeff - if the texture supports alpha at all!
			// Alpha support (1 or 0) is in the control.z slot.
			float emCoeff = v_control.z * sample.a;

			#if EMISSIVE_EXCLUSIVE == 0
				// In 'normal'emissive mode, the incoming color (lighting)
				// is mixed out as the emissiveness increases.
				color = mix( color, vec4(1,1,1,1), emCoeff ) * sample;
			#elif EMISSIVE_EXCLUSIVE == 1
				// In 'exclusive' mode, everything is black, and mixed
				// to the emmissive color. Note that the incoming color.a
				// doesn't do anything unless it's used in mix()
				color = mix( vec4(0,0,0,1), sample, emCoeff*color.a ) * color;
			#endif
		#else
			color *= sample;
		#endif
	#endif
	#if PREMULT == 1
		color = vec4( color.r*color.a, color.g*color.a, color.b*color.a, color.a );
	#endif
	#if SATURATION == 1
		float g = (color.r + color.g + color.b) * 0.33;
		vec4 gray = vec4( g, g, g, color.a );
		color = mix( gray, color, v_control.y );
	#endif
}

