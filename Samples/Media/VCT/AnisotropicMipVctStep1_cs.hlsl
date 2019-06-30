@insertpiece( SetCrossPlatformSettings )

#define OGRE_imageWrite3D4( outImage, iuv, value ) outImage[uint3( iuv )] = value

Texture3D inLightLowerMip0	: register(t0);
Texture3D inLightLowerMip1	: register(t1);
Texture3D inLightLowerMip2	: register(t2);

RWTexture3D<@insertpiece(uav0_pf_type)> outLightHigherMip0 : register(u0);
RWTexture3D<@insertpiece(uav1_pf_type)> outLightHigherMip1 : register(u1);
RWTexture3D<@insertpiece(uav2_pf_type)> outLightHigherMip2 : register(u2);

uniform int4 higherMipHalfRes_lowerMipHalfWidth;

#define p_higherMipHalfRes higherMipHalfRes_lowerMipHalfWidth.xyz
#define p_lowerMipHalfWidth higherMipHalfRes_lowerMipHalfWidth.w

@insertpiece( HeaderCS )

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main( uint3 gl_GlobalInvocationID		: SV_DispatchThreadId )
{
	@insertpiece( BodyCS )
}
