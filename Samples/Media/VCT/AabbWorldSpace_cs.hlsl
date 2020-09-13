@insertpiece( SetCrossPlatformSettings )

@insertpiece( PreBindingsHeaderCS )

RWStructuredBuffer<InstanceBuffer> instanceBuffer	: register(u0);

StructuredBuffer<AabbBuffer> inMeshAabb : register(t0);

@insertpiece( HeaderCS )

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
	uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	@insertpiece( BodyCS )
}
