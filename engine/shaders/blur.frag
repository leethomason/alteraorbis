uniform sampler2D texture0;
varying vec2 v_uv0;

#define NUM_SAMPLES 9
float offset[NUM_SAMPLES] = float[]( -0.008, -0.006, -0.004, -0.002, 0, 0.002, 0.004, 0.006, 0.008 ); 
float weight[NUM_SAMPLES] = float[]( 0.1,  0.1,  0.1,  0.1, 0.2, 0.1, 0.1, 0.1, 0.1 );

void main()
{
	vec4 color = vec4( 0, 0, 0, 0 );
	for( int i=0; i<NUM_SAMPLES; ++i ) {
		#if BLUR_Y == 0
			color += texture2D( texture0, vec2( v_uv0.x+offset[i], v_uv0.y )) * weight[i];
		#else
			color += texture2D( texture0, vec2( v_uv0.x, v_uv0.y+offset[i] )) * weight[i];
		#endif
	}
	gl_FragColor = color;	
}
