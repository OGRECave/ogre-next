#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2D myTexA[2];
vulkan_layout( ogre_t2 ) uniform texture2D myTexB[2];
vulkan_layout( ogre_t4 ) uniform texture2D myTexC;
vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

void main()
{
	vec4 a = texture( vkSampler2D( myTexA[0], samplerState ), inPs.uv0 );
	vec4 b = texture( vkSampler2D( myTexA[1], samplerState ), inPs.uv0 );
	vec4 c = texture( vkSampler2D( myTexB[0], samplerState ), inPs.uv0 );
	vec4 d = texture( vkSampler2D( myTexB[1], samplerState ), inPs.uv0 );
	vec4 e = texture( vkSampler2D( myTexC, samplerState ), inPs.uv0 );

	fragColour = a + b + c + d + e;
}
