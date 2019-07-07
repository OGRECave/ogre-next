@insertpiece( SetCrossPlatformSettings )

#define OGRE_imageWrite3D1( outImage, iuv, value ) outImage.write( value.x, iuv )
#define OGRE_imageWrite3D4( outImage, iuv, value ) outImage.write( value, iuv )

@insertpiece( PreBindingsHeaderCS )

#if defined(__HAVE_SIMDGROUP_BALLOT__)
	#define anyInvocationARB( value ) simd_any( value )
#else
	#define anyInvocationARB( value ) (value)
#endif

@insertpiece( HeaderCS )

struct Params
{
	uint numLights;
	float3 rayMarchStepSize;
	//float3 voxelOrigin;
	float3 voxelCellSize;
	float4 dirCorrectionRatio_thinWallCounter;
	float3 invVoxelResolution;
};

#define p_numLights p.numLights
#define p_rayMarchStepSize p.rayMarchStepSize
//#define p_voxelOrigin p.voxelOrigin
#define p_voxelCellSize p.voxelCellSize
#define p_dirCorrectionRatio p.dirCorrectionRatio_thinWallCounter.xyz
#define p_thinWallCounter p.dirCorrectionRatio_thinWallCounter.w
#define p_invVoxelResolution p.invVoxelResolution

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

kernel void main_metal
(
	texture3d<float> voxelAlbedoTex		[[texture(0)]],
	texture3d<float> voxelNormalTex		[[texture(1)]],
	texture3d<float> voxelEmissiveTex	[[texture(2)]],

	sampler voxelAlbedoSampler			[[sampler(0)]],

	texture3d<@insertpiece(uav0_pf_type), access::write> lightVoxel [[texture(UAV_SLOT_START+0)]],

	constant Light *lights			[[buffer(0)]],
	constant Params &p				[[buffer(PARAMETER_SLOT)]],
	ushort3 gl_GlobalInvocationID	[[thread_position_in_grid]]
)
{
	@insertpiece( BodyCS )
}
