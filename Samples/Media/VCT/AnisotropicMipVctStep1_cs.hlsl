@insertpiece( SetCrossPlatformSettings )

#define OGRE_imageWrite3D4( outImage, iuv, value ) outImage[uint3( iuv )] = value

Texture3D inLightLowerMip[3]	: register(t0);
RWTexture3D<@insertpiece(uav0_pf_type)> outLightHigherMip[3] : register(u0);

uniform int4 higherMipHalfRes_lowerMipHalfWidth;

#define p_higherMipHalfRes higherMipHalfRes_lowerMipHalfWidth.xyz
#define p_lowerMipHalfWidth higherMipHalfRes_lowerMipHalfWidth.w

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main( uint3 gl_GlobalInvocationID		: SV_DispatchThreadId )
{
	@insertpiece( BodyCS )
}
