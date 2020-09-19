#version ogre_glsl_ver_330

vulkan_layout( location = 0 )
in block
{
	flat vec4 voxelColour;
} inPs;

layout(location = 0, index = 0) out vec4 outColour;

void main()
{
	outColour.xyzw = inPs.voxelColour.xyzw;
}
