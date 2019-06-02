@insertpiece( SetCrossPlatformSettings )

#define __sharedOnlyBarrier threadgroup_barrier( mem_flags::mem_threadgroup )

@insertpiece( PreBindingsHeaderCS )

@insertpiece( HeaderCS )

struct Params
{
	uint2 meshStart_meshEnd;
};

#define p_meshStart p.meshStart_meshEnd.x
#define p_meshEnd p.meshStart_meshEnd.y

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

void main_metal
(
	device Vertex *vertexBuffer		[[buffer(UAV_SLOT_START+0)]],
	device uint *indexBuffer		[[buffer(UAV_SLOT_START+1)]],
	device MeshAabb *indexBuffer	[[buffer(UAV_SLOT_START+2)]],

	device uint4 inMeshBuffer		[[buffer(TEX_SLOT_START+0)]],

	constant Params &p				[[buffer(PARAMETER_SLOT)]],

	ushort gl_LocalInvocationIndex	[[thread_index_in_threadgroup]]
)
{
	threadgroup float3 g_minAabb[@value( threads_per_group_x )];
	threadgroup float3 g_maxAabb[@value( threads_per_group_x )];

	@insertpiece( BodyCS )
}
