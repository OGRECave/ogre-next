#version ogre_glsl_ver_330

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform float lodLevel;
vulkan( }; )

vulkan_layout( ogre_t0 ) uniform textureCube cubeTexture;
vulkan( layout( ogre_s0 ) uniform sampler cubeSampler );

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

vulkan_layout( location = 0 )
out vec4 fragColour;

void main()
{
	vec3 cubeDir;
	cubeDir.x = mod( inPs.uv0.x, 0.5 ) * 4.0 - 1.0;
	cubeDir.y = inPs.uv0.y * 2.0 - 1.0;
	cubeDir.z = 0.5 - 0.5 * (cubeDir.x * cubeDir.x + cubeDir.y * cubeDir.y);
	cubeDir.z = inPs.uv0.x < 0.5 ? cubeDir.z : -cubeDir.z;

	fragColour.xyzw = textureLod( vkSamplerCube( cubeTexture, cubeSampler ), cubeDir.xyz, lodLevel ).xyzw;
}
