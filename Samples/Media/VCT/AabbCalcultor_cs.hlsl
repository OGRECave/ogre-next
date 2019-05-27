@insertpiece( SetCrossPlatformSettings )

#define __sharedOnlyBarrier GroupMemoryBarrierWithGroupSync()

@insertpiece( PreBindingsHeaderCS )

RWStructuredBuffer<Vertex> vertexBuffer		: register(u0);
RWStructuredBuffer<uint> indexBuffer		: register(u1);
RWStructuredBuffer<MeshAabb> outMeshAabb	: register(u2);

Buffer<uint4> inMeshBuffer : register(t0);

groupshared float3 g_minAabb[@value( threads_per_group_x )];
groupshared float3 g_maxAabb[@value( threads_per_group_x )];

@insertpiece( HeaderCS )

uniform uint2 meshStart_meshEnd;

#define p_meshStart meshStart_meshEnd.x
#define p_meshEnd meshStart_meshEnd.y

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main( uint gl_LocalInvocationIndex	: SV_GroupIndex )
{
	@insertpiece( BodyCS )
}
