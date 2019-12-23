@insertpiece( SetCrossPlatformSettings )
@insertpiece( DeclUavCrossPlatform )

TextureCube		convolutionSrc	: register(t0);
SamplerState		: register(s0);
RWTexture2DArray<@insertpiece(uav0_pf_type)> lastResult : register(u0);

#define PARAMS_ARG_DECL , constant Params &p
#define PARAMS_ARG , p

struct Params
{
	float4 params0;
	float4 params1;
	float4 params2;

	float4 iblCorrection;
};

#define p_convolutionSamplesOffset p.params0.x
#define p_convolutionSampleCount p.params0.y
#define p_convolutionMaxSamples p.params0.z
#define p_convolutionRoughness p.params0.w

#define p_convolutionMip p.params1.x
#define p_environmentScale p.params1.y

#define p_inputResolution p.params2.xy
#define p_outputResolution p.params2.zw

#define p_iblCorrection p.iblCorrection

@insertpiece( HeaderCS )

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
	ushort3 gl_GlobalInvocationID		[[thread_position_in_grid]],

	, sampler EnvMapSampler															[[sampler(0)]]
	, texturecube convolutionSrc													[[texture(0)]]
	, texture2d_array<@insertpiece(uav0_pf_type), access::read_write> lastResult	[[texture(UAV_SLOT_START+0)]]

	, constant Params &p	[[buffer(PARAMETER_SLOT)]]
)
{
	@insertpiece( BodyCS )
}
