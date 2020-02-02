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
	uint3 gl_GlobalInvocationID		[[thread_position_in_grid]],
	ushort gl_LocalInvocationIndex	[[thread_index_in_threadgroup]],

	constant IrradianceFieldGenParams &p		[[buffer(0)]],

	device const float4 *directionsBuffer	[[buffer(TEX_SLOT_START+0)]],

	texture3d<float> vctProbe				[[texture(1)]],

	@property( vct_anisotropic )
		texture3d<float> vctProbeX			[[texture(2)]],
		texture3d<float> vctProbeY			[[texture(3)]],
		texture3d<float> vctProbeZ			[[texture(4)]],
	@end

	sampler probeSampler					[[sampler(1)]],

	texture2d<@insertpiece(uav0_pf_type), access::write> irradianceField		[[texture(UAV_SLOT_START+0)]],
	texture2d<@insertpiece(uav1_pf_type), access::write> irradianceFieldDepth	[[texture(UAV_SLOT_START+1)]]
)
{
	threadgroup float4 g_diffuseDepth[@value( threads_per_group_x ) * @value( threads_per_group_y ) * @value( threads_per_group_z )];

	@insertpiece( BodyCS )
}
