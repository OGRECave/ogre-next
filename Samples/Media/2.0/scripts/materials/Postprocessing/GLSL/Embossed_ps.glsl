#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2D RT;
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
    vec4 Color;
    Color.a = 1.0;
    Color.rgb = vec3(0.5);
	Color -= texture( vkSampler2D( RT, samplerState ), inPs.uv0 - 0.001 ) * 2.0;
	Color += texture( vkSampler2D( RT, samplerState ), inPs.uv0 + 0.001 ) * 2.0;
    Color.rgb = vec3((Color.r+Color.g+Color.b)/3.0);
    fragColour = Color;
}
