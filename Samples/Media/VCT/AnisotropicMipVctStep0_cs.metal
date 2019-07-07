@insertpiece( SetCrossPlatformSettings )

#define OGRE_imageWrite3D4( outImage, iuv, value ) outImage.write( value, iuv )

@insertpiece( HeaderCS )

#define p_higherMipHalfWidth higherMipHalfWidth

kernel void main_metal
(
	texture3d<float> inLightLowerMip		[[texture(0)]],
	texture3d<float> inVoxelNormalTex		[[texture(1)]],
	texture3d<@insertpiece(uav0_pf_type), access::write> outLightHigherMip0	[[texture(UAV_SLOT_START+0)]],
	texture3d<@insertpiece(uav1_pf_type), access::write> outLightHigherMip1	[[texture(UAV_SLOT_START+1)]],
	texture3d<@insertpiece(uav2_pf_type), access::write> outLightHigherMip2	[[texture(UAV_SLOT_START+2)]],

	constant int &higherMipHalfWidth		[[buffer(PARAMETER_SLOT)]],

	ushort3 gl_GlobalInvocationID	[[thread_position_in_grid]]
)
{
	@insertpiece( BodyCS )
}
