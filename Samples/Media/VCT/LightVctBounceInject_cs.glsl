@insertpiece( SetCrossPlatformSettings )
#extension GL_ARB_shader_group_vote: require

@insertpiece( DeclUavCrossPlatform )

@insertpiece( PreBindingsHeaderCS )

vulkan_layout( ogre_t0 ) uniform texture3D voxelAlbedoTex;
vulkan_layout( ogre_t1 ) uniform texture3D voxelNormalTex;
vulkan_layout( ogre_t2 ) uniform texture3D vctProbes[@value( hlms_num_vct_cascades )];

@property( vct_anisotropic )
	vulkan_layout( ogre_t3 ) uniform texture3D vctProbeX[@value( hlms_num_vct_cascades )];
	vulkan_layout( ogre_t4 ) uniform texture3D vctProbeY[@value( hlms_num_vct_cascades )];
	vulkan_layout( ogre_t5 ) uniform texture3D vctProbeZ[@value( hlms_num_vct_cascades )];
@end

vulkan( layout( ogre_s2 ) uniform sampler vctProbeSampler );

layout( vulkan( ogre_u0 ) vk_comma @insertpiece(uav0_pf_type) )
uniform restrict writeonly image3D lightVoxel;

layout( local_size_x = @value( threads_per_group_x ),
        local_size_y = @value( threads_per_group_y ),
        local_size_z = @value( threads_per_group_z ) ) in;

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform float3 voxelCellSize;
	uniform float3 invVoxelResolution;
	uniform float iterationDampening;
	uniform float startBias[@value( hlms_num_vct_cascades )];
	uniform float invStartBias[@value( hlms_num_vct_cascades )];
	uniform float cascadeMaxLod[@value( hlms_num_vct_cascades )];
	@property( hlms_num_vct_cascades > 1 )
		uniform float fromPrevLodToNext[@value( hlms_num_vct_cascades ) - 1];
		uniform float4 fromPreviousProbeToNext[@value( hlms_num_vct_cascades ) - 1][3];
	@end
vulkan( }; )

#define p_voxelCellSize voxelCellSize
#define p_invVoxelResolution invVoxelResolution
#define p_iterationDampening iterationDampening
#define p_vctStartBias startBias
#define p_vctInvStartBias invStartBias
#define p_vctCascadeMaxLod cascadeMaxLod
#define p_vctFromPrevLodToNext fromPrevLodToNext
#define p_vctFromPreviousProbeToNext fromPreviousProbeToNext

@insertpiece( HeaderCS )

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

void main()
{
    @insertpiece( BodyCS )
}
