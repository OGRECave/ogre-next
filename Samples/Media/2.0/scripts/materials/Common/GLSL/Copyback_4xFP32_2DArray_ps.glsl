#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2DArray tex;
vulkan( layout( ogre_s0 ) uniform sampler texSampler );

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform float sliceIdx;
vulkan( }; )

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

vulkan_layout( location = 0 )
out vec4 fragColour;

void main()
{
	fragColour = texture( vkSampler2DArray( tex, texSampler ), vec3( inPs.uv0, sliceIdx ) );
}
