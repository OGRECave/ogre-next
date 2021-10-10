// Scene voxelizer based on stitching 3D voxel meshes
// Matias N. Goldberg Copyright (c) 2021-present

@insertpiece( SetCrossPlatformSettings )

@insertpiece( DeclUavCrossPlatform )

@piece( CustomGlslExtensions )
	#extension GL_ARB_shader_group_vote: require
@end

@insertpiece( PreBindingsHeaderCS )

@property( syntax == glsl )
	#define ogre_U0 binding = 0
	#define ogre_U1 binding = 1
@end

vulkan_layout( ogre_t1 ) uniform texture3D meshTextures[3];

vulkan( layout( ogre_s1 ) uniform sampler trilinearSampler );

layout( vulkan( ogre_u2 ) vk_comma @insertpiece(uav2_pf_type) )
uniform restrict image3D voxelAlbedoTex;
layout( vulkan( ogre_u3 ) vk_comma @insertpiece(uav3_pf_type) )
uniform restrict image3D voxelNormalTex;
layout( vulkan( ogre_u4 ) vk_comma @insertpiece(uav4_pf_type) )
uniform restrict image3D voxelEmissiveTex;
layout( vulkan( ogre_u5 ) vk_comma @insertpiece(uav5_pf_type) )
uniform restrict uimage3D voxelAccumVal;

layout( local_size_x = @value( threads_per_group_x ),
		local_size_y = @value( threads_per_group_y ),
		local_size_z = @value( threads_per_group_z ) ) in;

@property( syntax == glsl )
	ReadOnlyBufferF( 6, InstanceBuffer, instanceBuffer );
@else
	ReadOnlyBufferF( 0, InstanceBuffer, instanceBuffer );
@end


@insertpiece( HeaderCS )

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform uint2 instanceStart_instanceEnd;
	uniform float3 voxelOrigin;
	uniform float3 voxelCellSize;
	uniform uint3 voxelPixelOrigin;
vulkan( }; )

#define p_instanceStart instanceStart_instanceEnd.x
#define p_instanceEnd instanceStart_instanceEnd.y
#define p_voxelOrigin voxelOrigin
#define p_voxelCellSize voxelCellSize
#define p_voxelPixelOrigin voxelPixelOrigin

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

void main()
{
	@insertpiece( BodyCS )
}
