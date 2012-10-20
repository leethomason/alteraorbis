
#if TEXTURE0 == 1
	uniform sampler2D texture0;
	varying vec2 v_uv0;
	#if PROCEDURAL == 1
		varying vec2 v_uv1;
		varying vec2 v_uv2;
		varying vec2 v_uv3;

		// A single texture that is procedural pretty much eats up all the varying data
		varying vec4 v_color0;
		varying vec4 v_color1;
		varying vec4 v_color2;
		varying vec4 v_color3;
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
			//vec4 color0 = vec4( 1.0, 0.6, 0.4, 1 );	// (r) skin
			//vec4 color1 = vec4( 1, 1, 1, 1);		// (b0) highlight
			//vec4 color2 = vec4( 0, 0.6, 1.0, 1 );	// (b1) glasses / tattoo
			//vec4 color3 = vec4( 0.5, 0.2, 0, 1 );	// (g) hair

			vec4 s0 = texture2D( texture0, v_uv0 );
			vec4 s1 = texture2D( texture0, v_uv1 );
			vec4 s2 = texture2D( texture0, v_uv2 );
			vec4 s3 = texture2D( texture0, v_uv3 );
			
			vec4  a = vec4( s0.a, s1.a, s2.a, s3.a );

			// red->C0
			// green->C3
			// on layer 0 and 1, blue->C1
			// on layer 2 and 3, blue->C2 (alpha channel used)
			s0.a = 0.0;
			s1.a = 0.0;
			s2.a = s2.b;
			s2.b = 0.0;
			s3.a = s3.b;
			s3.b = 0.0;
			
			vec4 t = mix( mix( mix(  s0, s1, a[1] ), s2, a[2] ), s3, a[3] );
			vec4 sample = t.r*v_color0 + t.g*v_color3 + t.b*v_color1 + t.a*v_color2;
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

