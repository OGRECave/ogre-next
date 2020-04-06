@insertpiece( SetCrossPlatformSettings )
@insertpiece( DeclUavCrossPlatform )

#define PARAMS_ARG_DECL , constant IrradianceFieldGenParams &p
#define PARAMS_ARG , p

@insertpiece( PreBindingsHeaderCS )

@insertpiece( HeaderCS )

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

kernel void main_metal
(
	uint3 gl_WorkGroupID			[[threadgroup_position_in_grid]],
	ushort3 gl_LocalInvocationID	[[thread_position_in_threadgroup]],
	ushort gl_LocalInvocationIndex	[[thread_index_in_threadgroup]],

	constant IrradianceFieldGenParams &p		[[buffer(CONST_SLOT_START+0)]],

	device const float2 *integrationTapsBuffer	[[buffer(TEX_SLOT_START+0)]],

	texture2d<@insertpiece(uav0_pf_type), access::read_write> irradianceField [[texture(UAV_SLOT_START+0)]]
)
{
	@property( integrate_depth )
		threadgroup float2 g_sharedValues[@value( threads_per_group_x ) * @value( threads_per_group_y )];
	@else
		threadgroup float4 g_sharedValues[@value( threads_per_group_x ) * @value( threads_per_group_y )];
	@end

	@insertpiece( BodyCS )
}
