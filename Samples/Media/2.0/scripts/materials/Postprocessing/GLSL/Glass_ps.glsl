#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan_layout( ogre_t1 ) uniform texture2D NormalMap;

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
	vec2 normal = 2.0 * (texture( vkSampler2D( NormalMap, samplerState1 ), inPs.uv0 * 2.5 ).xy - 0.5);

	fragColour = texture( vkSampler2D( RT, samplerState0 ), inPs.uv0 + normal.xy * 0.05 );
}
