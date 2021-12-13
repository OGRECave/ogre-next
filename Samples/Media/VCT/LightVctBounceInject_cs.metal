@insertpiece( SetCrossPlatformSettings )
@insertpiece( DeclUavCrossPlatform )

#define OGRE_imageWrite3D1( outImage, iuv, value ) outImage.write( value.x, iuv )
#define OGRE_imageWrite3D4( outImage, iuv, value ) outImage.write( value, iuv )

#define PARAMS_ARG_DECL , constant Params &p
#define PARAMS_ARG , p

@insertpiece( PreBindingsHeaderCS )

#if defined(__HAVE_SIMDGROUP_BALLOT__)
	#define anyInvocationARB( value ) simd_any( value )
#else
	#define anyInvocationARB( value ) (value)
#endif

struct Params
{
	float3 voxelCellSize;
	float3 invVoxelResolution;
	float iterationDampening;

	float4 startBias_invStartBias_cascadeMaxLod[@value( hlms_num_vct_cascades )];

	@property( hlms_num_vct_cascades > 1 )
		float4 fromPreviousProbeToNext[@value( hlms_num_vct_cascades ) - 1][2];
	@else
		// Unused, but declare them to shut up warnings of setting non-existant params
		float4 fromPreviousProbeToNext[1][2];
	@end
};

#define p_voxelCellSize p.voxelCellSize
#define p_invVoxelResolution p.invVoxelResolution
#define p_iterationDampening p.iterationDampening
#define p_vctStartBias_invStartBias_cascadeMaxLod p.startBias_invStartBias_cascadeMaxLod
#define p_vctFromPreviousProbeToNext p.fromPreviousProbeToNext

@insertpiece( HeaderCS )

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

@pset( vctTexUnit, 2 )

kernel void main_metal
(
	texture3d<float> voxelAlbedoTex		[[texture(0)]],
	texture3d<float> voxelNormalTex		[[texture(1)]],
	array< texture3d<float>, @value( hlms_num_vct_cascades ) > vctProbes	[[texture(@value(vctTexUnit))]],
	@add( vctTexUnit, hlms_num_vct_cascades )

	@property( vct_anisotropic )
		array< texture3d<float>, @value( hlms_num_vct_cascades ) > vctProbeX	[[texture(@value(vctTexUnit))]],
		@add( vctTexUnit, hlms_num_vct_cascades )

		array< texture3d<float>, @value( hlms_num_vct_cascades ) > vctProbeY	[[texture(@value(vctTexUnit))]],
		@add( vctTexUnit, hlms_num_vct_cascades )

		array< texture3d<float>, @value( hlms_num_vct_cascades ) > vctProbeZ	[[texture(@value(vctTexUnit))]],
		@add( vctTexUnit, hlms_num_vct_cascades )
	@end

	sampler vctProbeSampler				[[sampler(2)]],

	texture3d<@insertpiece(uav0_pf_type), access::write> lightVoxel [[texture(UAV_SLOT_START+0)]],

	constant Params &p				[[buffer(PARAMETER_SLOT)]],
	ushort3 gl_GlobalInvocationID	[[thread_position_in_grid]]
)
{
	@insertpiece( BodyCS )
}
