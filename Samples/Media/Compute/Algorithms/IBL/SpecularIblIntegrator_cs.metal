@insertpiece( SetCrossPlatformSettings )
@insertpiece( DeclUavCrossPlatform )

@property( !iOS )
	@property( uav0_texture_type == TextureTypes_TypeCube )
		#define UAV0_TEXTURE_WRITE texture2d_array<@insertpiece(uav0_pf_type), access::read_write>
	@else
		#define UAV0_TEXTURE_WRITE texture2d<@insertpiece(uav0_pf_type), access::read_write>
	@end
@else
	@property( uav0_texture_type == TextureTypes_TypeCube )
		#define UAV0_TEXTURE_WRITE texture2d_array<@insertpiece(uav0_pf_type), access::write>
	@else
		#define UAV0_TEXTURE_WRITE texture2d<@insertpiece(uav0_pf_type), access::write>
	@end
@end

#define PARAMS_ARG_DECL , constant Params &p , sampler EnvMapSampler, texturecube<float> convolutionSrc, UAV0_TEXTURE_WRITE lastResult
#define PARAMS_ARG , p, EnvMapSampler, convolutionSrc, lastResult

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

kernel void main_metal
(
	ushort3 gl_GlobalInvocationID		[[thread_position_in_grid]]

	, sampler EnvMapSampler					[[sampler(0)]]
	, texturecube<float> convolutionSrc		[[texture(0)]]
	, UAV0_TEXTURE_WRITE lastResult			[[texture(UAV_SLOT_START+0)]]

	, constant Params &p	[[buffer(PARAMETER_SLOT)]]
)
{
	@insertpiece( BodyCS )
}
