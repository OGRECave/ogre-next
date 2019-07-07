@insertpiece( SetCrossPlatformSettings )

#define PARAMS_ARG_DECL , device InstanceBuffer *instanceBuffer, device AabbBuffer *inMeshAabb
#define PARAMS_ARG , instanceBuffer, inMeshAabb

@insertpiece( PreBindingsHeaderCS )

@insertpiece( HeaderCS )

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

kernel void main_metal
(
	device InstanceBuffer *instanceBuffer	[[buffer(UAV_SLOT_START+0)]],

	device AabbBuffer *inMeshAabb			[[buffer(TEX_SLOT_START+0)]],

	ushort3 gl_GlobalInvocationID			[[thread_position_in_grid]]
)
{
	@insertpiece( BodyCS )
}
