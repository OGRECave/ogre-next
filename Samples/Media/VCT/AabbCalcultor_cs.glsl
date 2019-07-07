@insertpiece( SetCrossPlatformSettings )

#define __sharedOnlyBarrier memoryBarrierShared();barrier();

@insertpiece( PreBindingsHeaderCS )

layout(std430, binding = 0) readonly restrict buffer vertexBufferLayout
{
	Vertex vertexBuffer[];
};
layout(std430, binding = 1) readonly restrict buffer indexBufferLayout
{
	uint indexBuffer[];
};
layout(std430, binding = 2) writeonly restrict buffer outMeshAabbLayout
{
	MeshAabb outMeshAabb[];
};

layout( local_size_x = @value( threads_per_group_x ),
		local_size_y = @value( threads_per_group_y ),
		local_size_z = @value( threads_per_group_z ) ) in;

uniform usamplerBuffer inMeshBuffer;

shared float3 g_minAabb[@value( threads_per_group_x )];
shared float3 g_maxAabb[@value( threads_per_group_x )];

@insertpiece( HeaderCS )

uniform uint2 meshStart_meshEnd;

#define p_meshStart meshStart_meshEnd.x
#define p_meshEnd meshStart_meshEnd.y

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

void main()
{
	@insertpiece( BodyCS )
}
