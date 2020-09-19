#version ogre_glsl_ver_330

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

vulkan_layout( ogre_t0 ) uniform texture2D rt0;
vulkan_layout( ogre_t1 ) uniform texture2D lumRt;

vulkan( layout( ogre_s0 ) uniform sampler samplerBilinear );
vulkan( layout( ogre_s1 ) uniform sampler samplerPoint );

vec3 toSRGB( vec3 x )
{
	return sqrt( x );
}

vulkan( layout( ogre_P0 ) uniform Params { )
	//.x = 'low' threshold. All colour below this value is 0.
	//.y = 1 / (low - high). All colour above 'high' is at full brightness.
	//Colour between low and high is faded.
	uniform vec2 brightThreshold;
vulkan( }; )

void main()
{
	//Perform a high-pass filter on the image
	float fInvLumAvg = texture( vkSampler2D( lumRt, samplerPoint ), vec2( 0.0, 0.0 ) ).x;

	vec3 vSample = texture( vkSampler2D( rt0, samplerBilinear ), inPs.uv0 ).xyz;
	vSample.xyz *= fInvLumAvg;
	
	vec3 w = clamp( (vSample.xyz - brightThreshold.xxx) * brightThreshold.yyy, 0, 1 );
	vSample.xyz *= w * w * (3.0 - 2.0 * w);

	//We use RGB10 format which doesn't natively support sRGB, so we use a cheap approximation (gamma = 2)
	//This preserves a lot of quality. Don't forget to convert it back when sampling from the texture.
	fragColour = vec4( toSRGB( vSample * 0.0625 ), 1.0 );
}
