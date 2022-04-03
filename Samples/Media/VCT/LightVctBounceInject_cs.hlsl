@insertpiece( SetCrossPlatformSettings )

#define OGRE_imageWrite3D1( outImage, iuv, value ) outImage[uint3( iuv )] = value.x
#define OGRE_imageWrite3D4( outImage, iuv, value ) outImage[uint3( iuv )] = value

@insertpiece( PreBindingsHeaderCS )

@pset( vctTexUnit, 2 )

Texture3D voxelAlbedoTex	: register(t0);
Texture3D voxelNormalTex	: register(t1);
Texture3D vctProbes[@value( hlms_num_vct_cascades )]		: register(t@value(vctTexUnit));
@add( vctTexUnit, hlms_num_vct_cascades )

@property( vct_anisotropic )
	Texture3D vctProbeX[@value( hlms_num_vct_cascades )]	: register(t@value(vctTexUnit));
	@add( vctTexUnit, hlms_num_vct_cascades )

	Texture3D vctProbeY[@value( hlms_num_vct_cascades )]	: register(t@value(vctTexUnit));
	@add( vctTexUnit, hlms_num_vct_cascades )

	Texture3D vctProbeZ[@value( hlms_num_vct_cascades )]	: register(t@value(vctTexUnit));
	@add( vctTexUnit, hlms_num_vct_cascades )
@end

SamplerState vctProbeSampler	: register(s2);

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

uniform float3 voxelCellSize;
uniform float3 invVoxelResolution;
uniform float iterationDampening;

uniform float4 startBias_invStartBias_cascadeMaxLod[@value( hlms_num_vct_cascades )];

@property( hlms_num_vct_cascades > 1 )
	uniform float4 fromPreviousProbeToNext[@value( hlms_num_vct_cascades ) - 1][2];
@else
	// Unused, but declare them to shut up warnings of setting non-existant params
	uniform float4 fromPreviousProbeToNext[1][2];
@end

#define p_voxelCellSize voxelCellSize
#define p_invVoxelResolution invVoxelResolution
#define p_iterationDampening iterationDampening
#define p_vctStartBias_invStartBias_cascadeMaxLod startBias_invStartBias_cascadeMaxLod
#define p_vctFromPreviousProbeToNext fromPreviousProbeToNext

@insertpiece( HeaderCS )

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
