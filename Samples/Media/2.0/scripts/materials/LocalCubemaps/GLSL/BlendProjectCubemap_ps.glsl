#version ogre_glsl_ver_330

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
	vec3 posLS;
} inPs;

vulkan_layout( ogre_t0 ) uniform textureCube cubeTexture;
vulkan( layout( ogre_s0 ) uniform sampler cubeSampler );

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform float weight;
	uniform float lodLevel;
vulkan( }; )

void main()
{
	fragColour = textureLod( vkSamplerCube( cubeTexture, cubeSampler ),
							 inPs.posLS, lodLevel ).xyzw * weight;
}
