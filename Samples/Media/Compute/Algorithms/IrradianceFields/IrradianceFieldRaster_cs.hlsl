@insertpiece( SetCrossPlatformSettings )
@insertpiece( DeclUavCrossPlatform )

#define ushort2 uint2

@insertpiece( PreBindingsHeaderCS )

uniform uint probeIdx;
uniform float2 projectionParams;
uniform float3 numProbes;

#define p_probeIdx			probeIdx
#define p_projectionParams	projectionParams
#define p_numProbes			numProbes

SamplerState bilinearSampler	: register(s0);

TextureCube colourCubemap		: register(t0);
TextureCube depthCubemap		: register(t1);

RWTexture2D<@insertpiece(uav0_pf_type)> irradianceField			: register(u0);
RWTexture2D<@insertpiece(uav1_pf_type)> irradianceFieldDepth	: register(u1);

@insertpiece( HeaderCS )

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
	uint3 gl_LocalInvocationID		: SV_GroupThreadID
)
{
	@insertpiece( BodyCS )
}
