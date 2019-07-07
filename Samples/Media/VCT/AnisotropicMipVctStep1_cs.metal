@insertpiece( SetCrossPlatformSettings )

#define OGRE_imageWrite3D4( outImage, iuv, value ) outImage.write( value, iuv )

#define p_higherMipHalfRes higherMipHalfRes_lowerMipHalfWidth.xyz
#define p_lowerMipHalfWidth higherMipHalfRes_lowerMipHalfWidth.w

@insertpiece( HeaderCS )

kernel void main_metal
(
	texture3d<float> inLightLowerMip0											[[texture(0)]],
	texture3d<float> inLightLowerMip1											[[texture(1)]],
	texture3d<float> inLightLowerMip2											[[texture(2)]],
	texture3d<@insertpiece(uav0_pf_type), access::write> outLightHigherMip0	[[texture(UAV_SLOT_START+0)]],
	texture3d<@insertpiece(uav1_pf_type), access::write> outLightHigherMip1	[[texture(UAV_SLOT_START+1)]],
	texture3d<@insertpiece(uav2_pf_type), access::write> outLightHigherMip2	[[texture(UAV_SLOT_START+2)]],

	constant int4 &higherMipHalfRes_lowerMipHalfWidth		[[buffer(PARAMETER_SLOT)]],

	ushort3 gl_GlobalInvocationID	[[thread_position_in_grid]]
)
{
	@insertpiece( BodyCS )
}
