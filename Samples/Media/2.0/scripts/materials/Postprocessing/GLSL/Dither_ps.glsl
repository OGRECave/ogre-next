#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan_layout( ogre_t1 ) uniform texture2D noise;

vulkan( layout( ogre_s0 ) uniform sampler samplerState0 );
vulkan( layout( ogre_s1 ) uniform sampler samplerState1 );

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

void main()
{
	float c = dot( texture( vkSampler2D( RT, samplerState0 ), inPs.uv0 ).xyz, vec3(0.30, 0.11, 0.59) );
	float n = texture( vkSampler2D( noise, samplerState1 ), inPs.uv0 ).x * 2.0 - 1.0;
	c += n;

	c = step(c, 0.5);

	fragColour = vec4( c, c, c, 1.0 );
}
