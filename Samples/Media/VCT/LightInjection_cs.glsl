@insertpiece( SetCrossPlatformSettings )
#extension GL_ARB_shader_group_vote: require

#define OGRE_imageLoad3D( inImage, iuv ) imageLoad( inImage, int3( iuv ) )
#define OGRE_imageWrite3D1( outImage, iuv, value ) imageStore( outImage, int3( iuv ), value )
#define OGRE_imageWrite3D4( outImage, iuv, value ) imageStore( outImage, int3( iuv ), value )

@insertpiece( PreBindingsHeaderCS )

vulkan_layout( ogre_t0 ) uniform texture3D voxelAlbedoTex;
vulkan_layout( ogre_t1 ) uniform texture3D voxelNormalTex;
vulkan_layout( ogre_t2 ) uniform texture3D voxelEmissiveTex;

vulkan( layout( ogre_s0 ) uniform sampler voxelAlbedoSampler );

layout( vulkan( ogre_u0 ) vk_comma @insertpiece(uav0_pf_type) )
uniform restrict writeonly image3D lightVoxel;

layout( local_size_x = @value( threads_per_group_x ),
		local_size_y = @value( threads_per_group_y ),
		local_size_z = @value( threads_per_group_z ) ) in;

//layout( local_size_x = 4,
//		local_size_y = 4,
//		local_size_z = 4 ) in;


@insertpiece( HeaderCS )

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform uint numLights;
	uniform float4 rayMarchStepSize_bakingMultiplier;
	//uniform float3 voxelOrigin;
	uniform float3 voxelCellSize;
	uniform float4 dirCorrectionRatio_thinWallCounter;
	uniform float3 invVoxelResolution;
vulkan( }; )

#define p_numLights numLights
#define p_rayMarchStepSize rayMarchStepSize_bakingMultiplier.xyz
#define p_bakingMultiplier rayMarchStepSize_bakingMultiplier.w
//#define p_voxelOrigin voxelOrigin
#define p_voxelCellSize voxelCellSize
#define p_dirCorrectionRatio dirCorrectionRatio_thinWallCounter.xyz
#define p_thinWallCounter dirCorrectionRatio_thinWallCounter.w
#define p_invVoxelResolution invVoxelResolution

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

void main()
{
	@insertpiece( BodyCS )
}
