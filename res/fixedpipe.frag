//
// CAUTION: a header is inserted *before* this text. #version and layout information goes there.

#if TEXTURE0 == 1
	uniform sampler2D texture0;
	varying vec2 v_uv0;
	#if TEXTURE0_CLIP
		varying vec4 v_texture0Clip;
	#endif
	#if TEXTURE0_COLORMAP == 1
		varying mat4 v_colorMap;
	#endif
#endif
#if TEXTURE1 == 1
	uniform sampler2D texture1;
	varying vec2 v_uv1;
#endif

varying vec4 v_color;

#if SATURATION
	varying float v_saturation;
#endif

void main() 
{
	vec4 color = v_color;
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

		#if TEXTURE0_ALPHA_ONLY == 1
			color.a *= sample.a;
		#elif EMISSIVE == 1
			// Basically, the alpha color is used to
			// modulate the incoming fragment color, which
			// is the result of lighting.
			#if EMISSIVE_EXCLUSIVE == 0
				// In 'normal'emissive mode, the incoming color (lighting)
				// is mixed out as the emissiveness increases.
				color = mix( color, vec4(1,1,1,1), sample.a ) * sample;
			#elif EMISSIVE_EXCLUSIVE == 1
				// In 'exclusive' mode, everything is black, and mixed
				// to the emmissive color. Note that the incoming color.a
				// doesn't do anything unless it's used in mix()
				color = mix( vec4(0,0,0,1), sample, sample.a*color.a ) * color;
				//color = vec4( 1,0,0,1 );
			#endif
		#else
			color *= sample;
		#endif
	#endif
	#if TEXTURE1 == 1
		#if TEXTURE1_ALPHA_ONLY == 1
			color.a *= texture( texture1, v_uv1).a;
		#else
			color *= texture( texture1, v_uv1 );
		#endif
	#endif
	#if PREMULT == 1
		color = vec4( color.r*color.a, color.g*color.a, color.b*color.a, color.a );
	#endif
	#if SATURATION == 1
		float g = (color.r + color.g + color.b) * 0.33;
		vec4 gray = vec4( g, g, g, color.a );
		color = mix( gray, color, v_saturation );
	#endif
	gl_FragColor = color;
}

