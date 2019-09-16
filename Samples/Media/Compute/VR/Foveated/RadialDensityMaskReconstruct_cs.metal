@insertpiece( SetCrossPlatformSettings )

#if defined(__HAVE_SIMDGROUP_BALLOT__)
	#define anyInvocationARB( value ) simd_any( value )
#else
	#define anyInvocationARB( value ) (value)
#endif

#define OGRE_imageWrite2D4( outImage, iuv, value ) outImage.write( value, iuv )

@insertpiece( HeaderCS )

kernel void main_metal
(
	texture2d<float> srcTex		[[texture(0)]],
	sampler bilinearSampler		[[sampler(0)]],

	texture2d<@insertpiece(uav0_pf_type), access::write> dstTex [[texture(UAV_SLOT_START+0)]],

	constant RdmShaderParams &p	[[buffer(0)]],

	uint3 gl_GlobalInvocationID		: SV_DispatchThreadId
)
{
	@insertpiece( BodyCS )
}
