@insertpiece( SetCrossPlatformSettings )
@insertpiece( DeclUavCrossPlatform )

@insertpiece( PreBindingsHeaderCS )

struct Params
{
	uint probeIdx;
	float2 projectionParams;
	float3 numProbes;
};

#define p_probeIdx			p.probeIdx
#define p_projectionParams	p.projectionParams
#define p_numProbes			p.numProbes

@insertpiece( HeaderCS )

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

kernel void main_metal
(
	ushort3 gl_LocalInvocationID		[[thread_position_in_threadgroup]],

	constant Params &p							[[buffer(0)]],

	texturecube<float> colourCubemap			[[texture(0)]],
	texturecube<float> depthCubemap				[[texture(0)]],

	sampler bilinearSampler						[[sampler(0)]],

	texture2d<@insertpiece(uav0_pf_type), access::write> irradianceField		[[texture(UAV_SLOT_START+0)]],
	texture2d<@insertpiece(uav1_pf_type), access::write> irradianceFieldDepth	[[texture(UAV_SLOT_START+1)]]
)
{
	@insertpiece( BodyCS )
}
