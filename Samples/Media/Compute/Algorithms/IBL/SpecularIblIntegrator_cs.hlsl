@insertpiece( SetCrossPlatformSettings )
@insertpiece( DeclUavCrossPlatform )

TextureCube		convolutionSrc	: register(t0);
SamplerState	EnvMapSampler	: register(s0);
@property( uav0_texture_type == TextureTypes_TypeCube )
	RWTexture2DArray<@insertpiece(uav0_pf_type)> lastResult : register(u0);
@else
	RWTexture2D<@insertpiece(uav0_pf_type)> lastResult : register(u0);
@end

uniform float4 params0;
uniform float4 params1;
uniform float4 params2;

uniform float4 iblCorrection;

#define p_convolutionSamplesOffset params0.x
#define p_convolutionSampleCount params0.y
#define p_convolutionMaxSamples params0.z
#define p_convolutionRoughness params0.w

#define p_convolutionMip params1.x
#define p_environmentScale params1.y

#define p_inputResolution params2.xy
#define p_outputResolution params2.zw

#define p_iblCorrection iblCorrection

@insertpiece( HeaderCS )

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
	uint3 gl_GlobalInvocationID		: SV_DispatchThreadId
)
{
	@insertpiece( BodyCS )
}
