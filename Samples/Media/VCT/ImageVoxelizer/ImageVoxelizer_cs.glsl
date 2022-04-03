// Scene voxelizer based on stitching 3D voxel meshes
// Matias N. Goldberg Copyright (c) 2021-present

@insertpiece( SetCrossPlatformSettings )

@insertpiece( DeclUavCrossPlatform )


@insertpiece( PreBindingsHeaderCS )

@pset( uses_array_bindings, 1 )

ReadOnlyBufferF( @value( texture0_slot ), InstanceBuffer, instanceBuffer );
vulkan_layout( ogre_t1 ) uniform texture3D meshTextures[@value( tex_meshes_per_batch )];

vulkan( layout( ogre_s1 ) uniform sampler trilinearSampler );

layout( vulkan( ogre_u0 ) vk_comma @insertpiece(uav0_pf_type) )
uniform restrict image3D voxelAlbedoTex;
layout( vulkan( ogre_u1 ) vk_comma @insertpiece(uav1_pf_type) )
uniform restrict image3D voxelNormalTex;
layout( vulkan( ogre_u2 ) vk_comma @insertpiece(uav2_pf_type) )
uniform restrict image3D voxelEmissiveTex;
layout( vulkan( ogre_u3 ) vk_comma @insertpiece(uav3_pf_type) )
uniform restrict uimage3D voxelAccumVal;

layout( local_size_x = @value( threads_per_group_x ),
		local_size_y = @value( threads_per_group_y ),
		local_size_z = @value( threads_per_group_z ) ) in;

@insertpiece( HeaderCS )

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform uint2 instanceStart_instanceEnd;
	uniform float3 voxelOrigin;
	uniform float3 voxelCellSize;
	uniform uint3 voxelPixelOrigin;
	@property( check_out_of_bounds )
		uniform uint3 pixelsToWrite;
	@end
vulkan( }; )

#define p_instanceStart instanceStart_instanceEnd.x
#define p_instanceEnd instanceStart_instanceEnd.y
#define p_voxelOrigin voxelOrigin
#define p_voxelCellSize voxelCellSize
#define p_voxelPixelOrigin voxelPixelOrigin
#define p_pixelsToWrite pixelsToWrite

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

void main()
{
	@insertpiece( BodyCS )
}
