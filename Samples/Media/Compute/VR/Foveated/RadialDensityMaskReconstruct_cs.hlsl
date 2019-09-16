@insertpiece( SetCrossPlatformSettings )

@property( vendor_shader_extension == Intel )
	#define anyInvocationARB( value ) IntelExt_WaveActiveAnyTrue( value )
@end
@property( vendor_shader_extension == NVIDIA )
	#define anyInvocationARB( value ) NvAny( value )
@end
@property( vendor_shader_extension == AMD )
	#define anyInvocationARB( value ) AmdDxExtShaderIntrinsics_BallotAny( value )
@end
@property( !vendor_shader_extension )
	#define anyInvocationARB( value ) (value)
@end

#define short2 int2
#define ushort2 uint2
#define OGRE_imageWrite2D4( outImage, iuv, value ) outImage[uint2( iuv )] = value

RWTexture2D<@insertpiece(uav0_pf_type)> dstTex		: register(u0);

Texture2D srcTex				: register(t0);
SamplerState bilinearSampler	: register(s0);

@insertpiece( HeaderCS )

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
	uint3 gl_GlobalInvocationID		: SV_DispatchThreadId
)
{
	@insertpiece( BodyCS )
}
