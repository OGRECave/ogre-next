// Heavily modified version based on voxelization shader written by mattatz
//	https://github.com/mattatz/unity-voxel
// Original work Copyright (c) 2018 mattatz under MIT license.
//
// mattatz's work is a full voxelization algorithm.
// We only need to voxelize the shell (aka the contour),
// not to fill the entire voxel.
//
// Adapted for Ogre and for use for Voxel Cone Tracing by
// Matias N. Goldberg Copyright (c) 2019

@insertpiece( SetCrossPlatformSettings )

@insertpiece( DeclUavCrossPlatform )

@piece( CustomGlslExtensions )
	#extension GL_ARB_shader_group_vote: require
@end

@property( !vendor_shader_extension )
	// We are emulating anyInvocationARB because for it to work
	// correctly we need the following guarantees:
	//	1. gl_SubGroupSizeARB is known beforehand and all lanes [0; gl_SubGroupSizeARB) are always active
	//	2. VK_EXT_subgroup_size_control to enforce 1
	//	3. REQUIRE_FULL_SUBGROUPS_BIT_EXT to ensure all lanes are occupied the way we need
	//     (perfect lockstep)
	//
	// See https://github.com/KhronosGroup/GLSL/issues/19#issuecomment-881931582
	// See https://forums.ogre3d.org/viewtopic.php?p=551000#p551000
	shared bool g_emulatedGroupVote[64];

	bool emulatedAnyInvocationARB( bool value )
	{
		g_emulatedGroupVote[gl_LocalInvocationIndex] = value;

		for( uint i=0u; i<6u; ++i )
		{
			__sharedOnlyBarrier;
			uint nextIdx = gl_LocalInvocationIndex + (1u << i);
			uint mask = (1u << (i+1u)) - 1u;
			if( ( gl_LocalInvocationIndex & mask ) == 0u )
			{
				g_emulatedGroupVote[gl_LocalInvocationIndex] =
					g_emulatedGroupVote[gl_LocalInvocationIndex] || g_emulatedGroupVote[nextIdx];
			}
		}

		__sharedOnlyBarrier;
		return g_emulatedGroupVote[0];
	}

	#define anyInvocationARB( value ) emulatedAnyInvocationARB( value )
@end

@insertpiece( PreBindingsHeaderCS )

@property( syntax == glsl )
	#define ogre_U0 binding = 0
	#define ogre_U1 binding = 1
@end

layout( std430, ogre_U0 ) readonly restrict buffer vertexBufferLayout
{
	Vertex vertexBuffer[];
};
layout( std430, ogre_U1 ) readonly restrict buffer indexBufferLayout
{
	uint indexBuffer[];
};

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

//layout (rgba8) uniform restrict writeonly image3D voxelAlbedoTex;
//layout (rgb10_a2) uniform restrict writeonly image3D voxelNormalTex;
//layout (rgba8) uniform restrict writeonly image3D voxelEmissiveTex;

//layout( local_size_x = 4,
//		local_size_y = 4,
//		local_size_z = 4 ) in;

@property( syntax == glsl )
	ReadOnlyBufferF( 6, InstanceBuffer, instanceBuffer );
@else
	ReadOnlyBufferF( 0, InstanceBuffer, instanceBuffer );
@end

@property( has_diffuse_tex || has_emissive_tex )
	vulkan_layout( ogre_t1 ) uniform texture2DArray texturePool;
	vulkan( layout( ogre_s1 ) uniform sampler poolSampler );
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
