@insertpiece( SetCrossPlatformSettings )

#define OGRE_imageWrite3D4( outImage, iuv, value ) outImage[uint3( iuv )] = value

Texture3D inLightLowerMip	: register(t0);
Texture3D inVoxelNormalTex	: register(t1);
RWTexture3D<@insertpiece(uav0_pf_type)> outLightHigherMip[3] : register(u0);

@insertpiece( HeaderCS )

uniform int higherMipHalfWidth;

#define p_higherMipHalfWidth higherMipHalfWidth

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main( uint3 gl_GlobalInvocationID		: SV_DispatchThreadId )
{
	@insertpiece( BodyCS )
}
