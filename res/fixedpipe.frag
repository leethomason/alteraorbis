
#if TEXTURE0 == 1
	uniform sampler2D texture0;
	varying vec2 v_uv0;
	#if PROCEDURAL == 1
		varying vec2 v_uv1;
		varying vec2 v_uv2;
		varying vec2 v_uv3;
	#endif
#endif
#if TEXTURE1 == 1
	uniform sampler2D texture1;
	varying vec2 v_uv1;
#endif

varying vec4 v_color;

void main() 
{
	vec4 color = v_color;
	#if TEXTURE0 == 1
		#if  PROCEDURAL == 0
			vec4 sample = texture2D( texture0, v_uv0 );
		#elif PROCEDURAL == 1
			float r = 1.0;

			vec4 sample = vec4( 0, 0, 0, 0);

			vec4 color0 = vec4( 1.0, 0.6, 0.4, 1 );	// (r) skin
			vec4 color1 = vec4( 1, 1, 1, 1);		// (b0) highlight
			vec4 color2 = vec4( 0, 0.6, 1.0, 1 );	// (b1) glasses / tattoo
			vec4 color3 = vec4( 0.5, 0.2, 0, 1 );	// (g) hair

			vec4 s0 = texture2D( texture0, v_uv0 );
			vec4 s1 = texture2D( texture0, v_uv1 );
			vec4 s2 = texture2D( texture0, v_uv2 );
			vec4 s3 = texture2D( texture0, v_uv3 );

			vec4 t = mix( mix( mix(  s0, s1, s1.a ), s2, s2.a ), s3, s3.a );
			sample = t.r*color0 + t.g*color3 + t.b*color2;

			/*
			// Additive
			vec4 t = vec4( 0, 0, 0, 0);
			vec4 s = texture2D( texture0, v_uv3 );
			t += s * s.a * r;
			r -= r*s.a;

			s = texture2D( texture0, v_uv2 );
			t += s * s.a * r;
			r -= r*s.a;

			s = texture2D( texture0, v_uv1 );
			t += s * s.a * r;
			r -= r*s.a;

			s = texture2D( texture0, v_uv0 );
			t += s * s.a * r;
			sample = t.r*color0 + t.g*color3 + t.b*color2;
			*/
			
			/*
			s = texture2D( texture0, v_uv2 );
			vec4 c2 = s.r*color0 + s.g*color3 + s.b*color2;
			c2.a = s.a;

			s = texture2D( texture0, v_uv3 );
			vec4 c3 = s.r*color0 + s.g*color3 + s.b*color2;
			c3.a = s.a;
			*/
			
			/*
			vec4 s = texture2D( texture0, v_uv0 );
			vec4 c0 = s.r*color0 + s.g*color3 + s.b*color1;
			
			s = texture2D( texture0, v_uv1 );
			vec4 c1 = s.r*color0 + s.g*color3 + s.b*color1;

			sample = mix( c0, c1, c1.a );
			*/
			//vec4 sample = mix( mix( mix( c0, c1, c1.a ), c2, c2.a ), c3, c3.a );
		#endif

		#if TEXTURE0_ALPHA_ONLY == 1
			color.a *= sample.a;
		#elif EMISSIVE == 1
			// Basically, the alpha color is used to
			// modulate the incoming fragment color, which
			// is the result of lighting.
			vec4 texColor = sample;
			#if EMISSIVE_EXCLUSIVE == 0
				// In 'normal'emissive mode, the incoming color (lighting)
				// is mixed out as the emissiveness increases.
				color = mix( color, vec4(1,1,1,1), texColor.a ) * texColor;
			#elif EMISSIVE_EXCLUSIVE == 1
				// In 'exclusive' mode, everything is black, and mixed
				// to the emmissive color.
				color = mix( vec4(0,0,0,1), texColor, texColor.a );
			#endif
		#else
			color *= sample;
		#endif
	#endif
	#if TEXTURE1 == 1
		#if TEXTURE1_ALPHA_ONLY == 1
			color.a *= texture2D( texture1, v_uv1).a;
		#else
			color *= texture2D( texture1, v_uv1 );
		#endif
	#endif
	#if PREMULT == 1
		color = vec4( color.r*color.a, color.g*color.a, color.b*color.a, color.a );
	#endif
	gl_FragColor = color;
}

