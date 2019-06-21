@insertpiece( SetCrossPlatformSettings )

#define OGRE_imageWrite3D1( outImage, iuv, value ) outImage[uint3( iuv )] = value.x
#define OGRE_imageWrite3D4( outImage, iuv, value ) outImage[uint3( iuv )] = value

@insertpiece( PreBindingsHeaderCS )

Texture3D voxelAlbedoTex	: register(t0);
Texture3D voxelNormalTex	: register(t1);
Texture3D vctProbe			: register(t2);

@property( vct_anisotropic )
	Texture3D vctProbeX	: register(t3);
	Texture3D vctProbeY	: register(t4);
	Texture3D vctProbeZ	: register(t5);
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
uniform float2 startBias_invStartBias;

#define p_voxelCellSize voxelCellSize
#define p_invVoxelResolution invVoxelResolution
#define p_iterationDampening iterationDampening
#define p_vctStartBias startBias_invStartBias.x
#define p_vctInvStartBias startBias_invStartBias.y

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
