@insertpiece( SetCrossPlatformSettings )
@insertpiece( DeclUavCrossPlatform )

#define ushort2 uint2

@insertpiece( PreBindingsHeaderCS )

@insertpiece( HeaderCS )

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

kernel void main_metal
(
	uint3 gl_GlobalInvocationID		[[thread_position_in_grid]]

	, texture2d<@insertpiece(uav0_pf_type), access::read_write> ifdTex	[[texture(UAV_SLOT_START+0)]]

	, constant IfdBorderMirrorParams &p [[buffer(CONST_SLOT_START+0)]]
)
{
	@insertpiece( BodyCS )
}
