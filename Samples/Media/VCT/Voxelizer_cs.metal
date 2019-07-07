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

#define OGRE_imageLoad3D( inImage, iuv ) inImage.read( ushort3( iuv ) )
#define OGRE_imageWrite3D1( outImage, iuv, value ) outImage.write( value.x, iuv )
#define OGRE_imageWrite3D4( outImage, iuv, value ) outImage.write( value, iuv )

#define PARAMS_ARG_DECL , device Vertex *vertexBuffer, device uint *indexBuffer, device InstanceBuffer *instanceBuffer, constant Params &p
#define PARAMS_ARG , vertexBuffer, indexBuffer, instanceBuffer, p

struct Params
{
	uint2 instanceStart_instanceEnd;
	float3 voxelOrigin;
	float3 voxelCellSize;
};

#if defined(__HAVE_SIMDGROUP_BALLOT__)
	#define anyInvocationARB( value ) simd_any( value )
#else
	//At least on High Sierra AMD, when using a threadgroup uchar or bool,
	//it crashes the Metal compiler so we're using ushort instead
	inline bool emulatedAnyInvocationARB( bool value, ushort gl_LocalInvocationIndex,
										  threadgroup ushort g_emulatedGroupVote[64] )
	{
		g_emulatedGroupVote[gl_LocalInvocationIndex] = value ? 1u : 0u;

		for( ushort i=0u; i<6u; ++i )
		{
			threadgroup_barrier( mem_flags::mem_threadgroup );
			ushort nextIdx = gl_LocalInvocationIndex + (1u << i);
			ushort mask = (1u << (i+1u)) - 1u;
			if( !(gl_LocalInvocationIndex & mask) )
			{
				g_emulatedGroupVote[gl_LocalInvocationIndex] =
					g_emulatedGroupVote[gl_LocalInvocationIndex] | g_emulatedGroupVote[nextIdx];
			}
		}

		threadgroup_barrier( mem_flags::mem_threadgroup );
		return g_emulatedGroupVote[0] != 0u;
	}

	#define anyInvocationARB( value ) emulatedAnyInvocationARB( value, gl_LocalInvocationIndex, g_emulatedGroupVote )
	#define EMULATING_anyInvocationARB
#endif

/*inline float4 unpackUnormRGB10A2( uint v )
{
	return float4(	(v & 0x000003FF) / 1024.0f,
					((v >> 10u) & 0x000003FF) / 1024.0f,
					((v >> 20u) & 0x000003FF) / 1024.0f,
					(v >> 30u) / 2.0f );
}
inline uint packUnormRGB10A2( float4 v )
{
	uint retVal;
	retVal  = uint( round( saturate( v.x ) * 1024.0f ) );
	retVal |= uint( round( saturate( v.y ) * 1024.0f ) ) << 10u;
	retVal |= uint( round( saturate( v.z ) * 1024.0f ) ) << 20u;
	retVal |= uint( round( saturate( v.w ) * 2.0f ) ) << 30u;

	return retVal;
}*/

@insertpiece( PreBindingsHeaderCS )

@pset( samplerRegister, 1 )
@pset( texRegister, 1 )

@insertpiece( HeaderCS )

#define p_instanceStart p.instanceStart_instanceEnd.x
#define p_instanceEnd p.instanceStart_instanceEnd.y
#define p_numInstances p.numInstances
#define p_voxelOrigin p.voxelOrigin
#define p_voxelCellSize p.voxelCellSize

kernel void main_metal
(
	device Vertex *vertexBuffer				[[buffer(UAV_SLOT_START+0)]],
	device uint *indexBuffer				[[buffer(UAV_SLOT_START+1)]],

	device InstanceBuffer *instanceBuffer	[[buffer(TEX_SLOT_START+0)]],

	texture3d<@insertpiece(uav2_pf_type), access::read_write> voxelAlbedoTex	[[texture(UAV_SLOT_START+2)]],
	texture3d<@insertpiece(uav3_pf_type), access::read_write> voxelNormalTex	[[texture(UAV_SLOT_START+3)]],
	texture3d<@insertpiece(uav4_pf_type), access::read_write> voxelEmissiveTex	[[texture(UAV_SLOT_START+4)]],
	texture3d<@insertpiece(uav5_pf_type), access::read_write> voxelAccumVal		[[texture(UAV_SLOT_START+5)]],

@property( has_diffuse_tex || has_emissive_tex )
	sampler					poolSampler	[[sampler(@counter(samplerRegister))]],
	texture2d_array<float>	texturePool	[[texture(@counter(texRegister))]],
@end

	constant Material *materials	[[buffer(0)]],
	constant Params &p				[[buffer(PARAMETER_SLOT)]],

	ushort3 gl_GlobalInvocationID	[[thread_position_in_grid]],
	ushort3 gl_WorkGroupID			[[threadgroup_position_in_grid]],
	ushort gl_LocalInvocationIndex	[[thread_index_in_threadgroup]]
)
{
	#if !defined(__HAVE_SIMDGROUP_BALLOT__)
		threadgroup ushort g_emulatedGroupVote[64];
	#endif
	@insertpiece( BodyCS )
}
