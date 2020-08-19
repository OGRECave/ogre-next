#version ogre_glsl_ver_330

vulkan_layout( location = 0 )
out float fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

const vec2 c_offsets[4] = vec2[4]
(
	vec2( -1.0, -1.0 ), vec2( 1.0, -1.0 ),
	vec2( -1.0,  1.0 ), vec2( 1.0,  1.0 )
);

vulkan_layout( ogre_t0 ) uniform texture2D lumRt;
vulkan( layout( ogre_s0 ) uniform sampler samplerBilinear );

vulkan_layout( ogre_t1 ) uniform texture2D oldLumRt;
vulkan( layout( ogre_s1 ) uniform sampler samplerPoint );

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform vec3 exposure;
	uniform float timeSinceLast;
	uniform vec4 tex0Size;
vulkan( }; )

void main()
{
	float fLumAvg = texture( vkSampler2D( lumRt, samplerBilinear ),
							 inPs.uv0 + c_offsets[0] * tex0Size.zw ).x;

	for( int i=1; i<4; ++i )
	{
		fLumAvg += texture( vkSampler2D( lumRt, samplerBilinear ),
							inPs.uv0 + c_offsets[i] * tex0Size.zw ).x;
	}

	fLumAvg *= 0.25; // /= 4.0;

	float newLum = exposure.x / exp( clamp( fLumAvg, exposure.y, exposure.z ) );
	float oldLum = texture( vkSampler2D( oldLumRt, samplerPoint ), vec2( 0.0, 0.0 ) ).x;

	//Adapt luminicense based 75% per second.
	fragColour = mix( newLum, oldLum, pow( 0.25, timeSinceLast ) );
}
