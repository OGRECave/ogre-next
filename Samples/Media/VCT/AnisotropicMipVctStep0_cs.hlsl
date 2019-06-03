@insertpiece( SetCrossPlatformSettings )

#define OGRE_imageWrite3D4( outImage, iuv, value ) outImage[uint3( iuv )] = value

Texture3D inLightLowerMip	: register(t0);
Texture3D inVoxelNormalTex	: register(t1);
RWTexture3D<@insertpiece(uav0_pf_type)> outLightHigherMip0 : register(u0);
RWTexture3D<@insertpiece(uav1_pf_type)> outLightHigherMip1 : register(u1);
RWTexture3D<@insertpiece(uav2_pf_type)> outLightHigherMip2 : register(u2);

@insertpiece( HeaderCS )

uniform int higherMipHalfWidth;

#define p_higherMipHalfWidth higherMipHalfWidth

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main( uint3 gl_GlobalInvocationID		: SV_DispatchThreadId )
{
	@insertpiece( BodyCS )
}
