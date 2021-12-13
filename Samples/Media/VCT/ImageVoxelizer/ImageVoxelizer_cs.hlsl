// Scene voxelizer based on stitching 3D voxel meshes
// Matias N. Goldberg Copyright (c) 2021-present

@insertpiece( SetCrossPlatformSettings )

@insertpiece( DeclUavCrossPlatform )

@property( vendor_shader_extension == Intel )
	#define anyInvocationARB( value ) IntelExt_WaveActiveAnyTrue( value )
@end
@property( vendor_shader_extension == NVIDIA )
	#define anyInvocationARB( value ) NvAny( value )
@end
@property( vendor_shader_extension == AMD )
	#define anyInvocationARB( value ) AmdDxExtShaderIntrinsics_BallotAny( value )
@end

@property( !typed_uav_load )
	groupshared uint g_voxelAccumValue[32];
@end
@property( !vendor_shader_extension )
	groupshared bool g_emulatedGroupVote[64];

	bool emulatedAnyInvocationARB( bool value, uint gl_LocalInvocationIndex )
	{
		g_emulatedGroupVote[gl_LocalInvocationIndex] = value;

		for( uint i=0u; i<6u; ++i )
		{
			GroupMemoryBarrierWithGroupSync();
			uint nextIdx = gl_LocalInvocationIndex + (1u << i);
			uint mask = (1u << (i+1u)) - 1u;
			if( !(gl_LocalInvocationIndex & mask) )
			{
				g_emulatedGroupVote[gl_LocalInvocationIndex] =
					g_emulatedGroupVote[gl_LocalInvocationIndex] || g_emulatedGroupVote[nextIdx];
			}
		}

		GroupMemoryBarrierWithGroupSync();
		return g_emulatedGroupVote[0];
	}

	#define anyInvocationARB( value ) emulatedAnyInvocationARB( value, gl_LocalInvocationIndex )
	#define EMULATING_anyInvocationARB
@end

float4 unpackUnorm4x8( uint v )
{
	return float4(	(v & 0xFF) / 255.0f,
					((v >> 8u) & 0xFF) / 255.0f,
					((v >> 16u) & 0xFF) / 255.0f,
					(v >> 24u) / 255.0f );
}
uint packUnorm4x8( float4 v )
{
	uint retVal;
	retVal  = uint( round( saturate( v.x ) * 255.0f ) );
	retVal |= uint( round( saturate( v.y ) * 255.0f ) ) << 8u;
	retVal |= uint( round( saturate( v.z ) * 255.0f ) ) << 16u;
	retVal |= uint( round( saturate( v.w ) * 255.0f ) ) << 24u;

	return retVal;
}

float4 unpackUnormRGB10A2( uint v )
{
	return float4(	(v & 0x000003FF) / 1024.0f,
					((v >> 10u) & 0x000003FF) / 1024.0f,
					((v >> 20u) & 0x000003FF) / 1024.0f,
					(v >> 30u) / 2.0f );
}
uint packUnormRGB10A2( float4 v )
{
	uint retVal;
	retVal  = uint( round( saturate( v.x ) * 1024.0f ) );
	retVal |= uint( round( saturate( v.y ) * 1024.0f ) ) << 10u;
	retVal |= uint( round( saturate( v.z ) * 1024.0f ) ) << 20u;
	retVal |= uint( round( saturate( v.w ) * 2.0f ) ) << 30u;

	return retVal;
}

@insertpiece( PreBindingsHeaderCS )

RWTexture3D<@insertpiece(uav0_pf_type)> voxelAlbedoTex		: register(u0);
RWTexture3D<@insertpiece(uav1_pf_type)> voxelNormalTex		: register(u1);
RWTexture3D<@insertpiece(uav2_pf_type)> voxelEmissiveTex	: register(u2);
RWTexture3D<@insertpiece(uav3_pf_type)> voxelAccumVal		: register(u3);

StructuredBuffer<InstanceBuffer> instanceBuffer : register(t0);

Texture3D<float4> meshTextures[@value( tex_meshes_per_batch )]	: register( t1 );
SamplerState trilinearSampler									: register( s1 );

@insertpiece( HeaderCS )

uniform uint2 instanceStart_instanceEnd;
uniform float3 voxelOrigin;
uniform float3 voxelCellSize;
uniform uint3 voxelPixelOrigin;
@property( check_out_of_bounds )
	uniform uint3 pixelsToWrite;
@end

#define p_instanceStart instanceStart_instanceEnd.x
#define p_instanceEnd instanceStart_instanceEnd.y
#define p_voxelOrigin voxelOrigin
#define p_voxelCellSize voxelCellSize
#define p_voxelPixelOrigin voxelPixelOrigin
#define p_pixelsToWrite pixelsToWrite

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
	uint3 gl_GlobalInvocationID		: SV_DispatchThreadId,
	uint3 gl_WorkGroupID			: SV_GroupID,
	uint gl_LocalInvocationIndex	: SV_GroupIndex
)
{
	uint3 gl_WorkGroupSize = uint3( @value( threads_per_group_x )u, @value( threads_per_group_y )u, @value( threads_per_group_z )u );
	@insertpiece( BodyCS )
}
