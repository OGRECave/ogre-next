// Translates a 3D textures by an UVW offset
// Matias N. Goldberg Copyright (c) 2021-present

@insertpiece( SetCrossPlatformSettings )

@insertpiece( DeclUavCrossPlatform )

@insertpiece( PreBindingsHeaderCS )

layout( vulkan( ogre_u0 ) vk_comma @insertpiece(uav0_pf_type) )
uniform restrict image3D dstTex0;
layout( vulkan( ogre_u1 ) vk_comma @insertpiece(uav1_pf_type) )
uniform restrict image3D dstTex1;
layout( vulkan( ogre_u2 ) vk_comma @insertpiece(uav2_pf_type) )
uniform restrict image3D dstTex2;

layout( local_size_x = @value( threads_per_group_x ),
		local_size_y = @value( threads_per_group_y ),
		local_size_z = @value( threads_per_group_z ) ) in;

@insertpiece( HeaderCS )

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform uint3 startOffset;
	uniform uint3 pixelsToClear;
vulkan( }; )

#define p_startOffset startOffset
#define p_pixelsToClear pixelsToClear

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

void main()
{
	@insertpiece( BodyCS )
}
