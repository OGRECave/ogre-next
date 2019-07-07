@insertpiece( SetCrossPlatformSettings )

#define OGRE_imageWrite3D1( outImage, iuv, value ) outImage[uint3( iuv )] = value.x
#define OGRE_imageWrite3D4( outImage, iuv, value ) outImage[uint3( iuv )] = value

@insertpiece( PreBindingsHeaderCS )

Texture3D voxelAlbedoTex	: register(t0);
Texture3D voxelNormalTex	: register(t1);
Texture3D voxelEmissiveTex	: register(t2);

SamplerState voxelAlbedoSampler	: register(s0);

RWTexture3D<@insertpiece(uav0_pf_type)> lightVoxel;

@property( vendor_shader_extension == Intel )
	#define anyInvocationARB( value ) IntelExt_WaveActiveAnyTrue( value )
@end
@property( vendor_shader_extension == NVIDIA )
	#define anyInvocationARB( value ) NvAny( value )
@end
@property( vendor_shader_extension == AMD )
	#define anyInvocationARB( value ) AmdDxExtShaderIntrinsics_BallotAny( value )
@end
@property( !vendor_shader_extension )
	#define anyInvocationARB( value ) (value)
@end

@insertpiece( HeaderCS )

uniform uint numLights;
uniform float3 rayMarchStepSize;
//uniform float3 voxelOrigin;
uniform float3 voxelCellSize;
uniform float4 dirCorrectionRatio_thinWallCounter;
uniform float3 invVoxelResolution;

#define p_numLights numLights
#define p_rayMarchStepSize rayMarchStepSize
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

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
	uint3 gl_GlobalInvocationID		: SV_DispatchThreadId
)
{
	@insertpiece( BodyCS )
}
