#version ogre_glsl_ver_330

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

vulkan_layout( ogre_t0 ) uniform texture2D Blur0;
vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vec3 fromSRGB( vec3 x )
{
	return x * x;
}
vec3 toSRGB( vec3 x )
{
	return sqrt( x );
}

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform vec4 invTex0Size;
vulkan( }; )

#define NUM_SAMPLES 11

void main()
{
	vec2 uv = inPs.uv0.xy;

	uv.y -= invTex0Size.y * ((NUM_SAMPLES-1) * 0.5);
	vec3 sum = fromSRGB( texture( vkSampler2D( Blur0, samplerState ), uv ).xyz );

	for( int i=1; i<NUM_SAMPLES; ++i )
	{
		uv.y += invTex0Size.y;
		sum += fromSRGB( texture( vkSampler2D( Blur0, samplerState ), uv ).xyz );
	}

	fragColour = vec4( toSRGB( sum / NUM_SAMPLES ), 1.0 );
}
