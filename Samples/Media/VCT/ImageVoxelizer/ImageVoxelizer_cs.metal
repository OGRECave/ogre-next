// Scene voxelizer based on stitching 3D voxel meshes
// Matias N. Goldberg Copyright (c) 2021-present

@insertpiece( SetCrossPlatformSettings )

@insertpiece( DeclUavCrossPlatform )

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

@insertpiece( PreBindingsHeaderCS )

#define PARAMS_ARG_DECL , device InstanceBuffer *instanceBuffer, constant Params &p
#define PARAMS_ARG , instanceBuffer, p

struct Params
{
	uint2 instanceStart_instanceEnd;
	float3 voxelOrigin;
	float3 voxelCellSize;
	uint3 voxelPixelOrigin;
	@property( check_out_of_bounds )
		uint3 pixelsToWrite;
	@end
};

#define p_instanceStart p.instanceStart_instanceEnd.x
#define p_instanceEnd p.instanceStart_instanceEnd.y
#define p_voxelOrigin p.voxelOrigin
#define p_voxelCellSize p.voxelCellSize
#define p_voxelPixelOrigin p.voxelPixelOrigin
#define p_pixelsToWrite p.pixelsToWrite

@insertpiece( HeaderCS )

kernel void main_metal
(
	device InstanceBuffer *instanceBuffer	[[buffer(TEX_SLOT_START+0)]],

	texture3d<@insertpiece(uav0_pf_type), access::read_write> voxelAlbedoTex	[[texture(UAV_SLOT_START+0)]],
	texture3d<@insertpiece(uav1_pf_type), access::read_write> voxelNormalTex	[[texture(UAV_SLOT_START+1)]],
	texture3d<@insertpiece(uav2_pf_type), access::read_write> voxelEmissiveTex	[[texture(UAV_SLOT_START+2)]],
	texture3d<@insertpiece(uav3_pf_type), access::read_write> voxelAccumVal		[[texture(UAV_SLOT_START+3)]],

	array< texture3d<float>, @value( tex_meshes_per_batch ) > meshTextures	[[texture(1)]],
	sampler trilinearSampler												[[sampler(1)]],

	constant Params &p	[[buffer(PARAMETER_SLOT)]],

	ushort3 gl_GlobalInvocationID	[[thread_position_in_grid]],
	ushort3 gl_WorkGroupID			[[threadgroup_position_in_grid]],
	ushort gl_LocalInvocationIndex	[[thread_index_in_threadgroup]]
)
{
	uint3 gl_WorkGroupSize = uint3( @value( threads_per_group_x )u, @value( threads_per_group_y )u, @value( threads_per_group_z )u );
	threadgroup ushort g_emulatedGroupVote[64];

	@insertpiece( BodyCS )
}
