@insertpiece( SetCrossPlatformSettings )

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
	float2 startBias_invStartBias;
};

#define p_voxelCellSize p.voxelCellSize
#define p_invVoxelResolution p.invVoxelResolution
#define p_iterationDampening p.iterationDampening
#define p_vctStartBias p.startBias_invStartBias.x
#define p_vctInvStartBias p.startBias_invStartBias.y

@insertpiece( HeaderCS )

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

kernel void main_metal
(
	texture3d<float> voxelAlbedoTex		[[texture(0)]],
	texture3d<float> voxelNormalTex		[[texture(1)]],
	texture3d<float> vctProbe			[[texture(2)]],

	texture3d<float> vctProbeX			[[texture(3)]],
	texture3d<float> vctProbeY			[[texture(4)]],
	texture3d<float> vctProbeZ			[[texture(5)]],

	sampler vctProbeSampler				[[sampler(2)]],

	texture3d<@insertpiece(uav0_pf_type), access::write> lightVoxel [[texture(UAV_SLOT_START+0)]],

	constant Params &p				[[buffer(PARAMETER_SLOT)]],
	ushort3 gl_GlobalInvocationID	[[thread_position_in_grid]]
)
{
	@insertpiece( BodyCS )
}
