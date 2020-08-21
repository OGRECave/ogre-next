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
    float nColors = 8.0;
    float gamma = 0.6;

	vec4 texCol = texture( vkSampler2D( RT, samplerState ), inPs.uv0 );
    vec3 tc = texCol.xyz;
    tc = pow(tc, vec3(gamma));
    tc = tc * nColors;
    tc = floor(tc);
    tc = tc / nColors;
    tc = pow(tc, vec3(1.0 / gamma));
    fragColour = vec4(tc, texCol.w);
}
