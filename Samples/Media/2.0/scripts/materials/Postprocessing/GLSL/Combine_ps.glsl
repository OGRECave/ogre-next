#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan_layout( ogre_t1 ) uniform texture2D Sum;
vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform float blur;
vulkan( }; )

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

void main()
{
	vec4 render = texture( vkSampler2D( RT, samplerState ), inPs.uv0 );
	vec4 sum = texture( vkSampler2D( Sum, samplerState ), inPs.uv0 );

	fragColour = mix( render, sum, blur );
}
