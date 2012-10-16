
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
		#if TEXTURE0_ALPHA_ONLY == 1
			color.a *= texture2D( texture0, v_uv0).a;
		#elif EMISSIVE == 1
			// Basically, the alpha color is used to
			// modulate the incoming fragment color, which
			// is the result of lighting.
			vec4 texColor = texture2D( texture0, v_uv0 );
			#if EMISSIVE_EXCLUSIVE == 0
				// In 'normal'emissive mode, the incoming color (lighting)
				// is mixed out as the emissiveness increases.
				color = mix( color, vec4(1,1,1,1), texColor.a ) * texColor;
			#elif EMISSIVE_EXCLUSIVE == 1
				// In 'exclusive' mode, everything is black, and mixed
				// to the emmissive color.
				color = mix( vec4(0,0,0,1), texColor, texColor.a );
			#endif
		#elif PROCEDURAL == 1
			vec4 c0 = texture2D( texture0, v_uv0 );
			vec4 c1 = texture2D( texture0, v_uv1 );
			vec4 c2 = texture2D( texture0, v_uv2 );
			vec4 c3 = texture2D( texture0, v_uv3 );
			color *= c0 + c1*c1.a + c2+c2.a + c3+c3.a;
		#else
			color *= texture2D( texture0, v_uv0 );
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

