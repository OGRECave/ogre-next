@insertpiece( SetCrossPlatformSettings )

#define OGRE_imageWrite3D4( outImage, iuv, value ) outImage.write( value, iuv )

#define p_higherMipHalfRes higherMipHalfRes_lowerMipHalfWidth.xyz
#define p_lowerMipHalfWidth higherMipHalfRes_lowerMipHalfWidth.w

void main
(
	texture3d<float> inLightLowerMip[3]											[[texture(0)]],
	texture3d<@insertpiece(uav0_pf_type), access::write> outLightHigherMip[3]	[[texture(UAV_SLOT_START+0)]],

	constant int4 &higherMipHalfRes_lowerMipHalfWidth		[[buffer(PARAMETER_SLOT)]],

	ushort3 gl_GlobalInvocationID	[[thread_position_in_grid]]
)
{
	@insertpiece( BodyCS )
}
