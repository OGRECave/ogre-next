#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform textureCube depthTexture;
vulkan( layout( ogre_s0 ) uniform sampler cubeSampler );

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

in vec4 gl_FragCoord;
//out float gl_FragDepth;

#ifdef OUTPUT_TO_COLOUR
	vulkan_layout( location = 0 )
	out float fragColour;
#endif

void main()
{
	vec3 cubeDir;

	cubeDir.x = mod( inPs.uv0.x, 0.5 ) * 4.0 - 1.0;
	cubeDir.y = inPs.uv0.y * 2.0 - 1.0;
	cubeDir.z = 0.5 - 0.5 * (cubeDir.x * cubeDir.x + cubeDir.y * cubeDir.y);

	cubeDir.z = inPs.uv0.x < 0.5 ? cubeDir.z : -cubeDir.z;

	float depthValue = textureLod( vkSamplerCube( depthTexture, cubeSampler ), cubeDir.xyz, 0 ).x;

#ifdef OUTPUT_TO_COLOUR
	fragColour = depthValue;
#else
	gl_FragDepth = depthValue;
#endif
}
