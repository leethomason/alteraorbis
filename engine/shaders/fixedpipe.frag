
#if TEXTURE0 == 1
	uniform sampler2D texture0;
	varying vec2 v_uv0;
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
			color = mix( color, vec4(1,1,1,1), texColor.a ) * texColor;
			#if EMISSIVE_EXCLUSIVE == 1
				color *= sign( texColor.a-0.1 );
			#endif
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

