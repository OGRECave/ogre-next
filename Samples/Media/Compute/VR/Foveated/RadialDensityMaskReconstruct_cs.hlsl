@insertpiece( SetCrossPlatformSettings )

#define short2 int2
#define ushort2 uint2
#define OGRE_imageWrite3D4( outImage, iuv, value ) outImage[uint2( iuv )] = value

RWTexture2D<@insertpiece(uav0_pf_type)> dstTex		: register(u0);

Texture2D srcTex : register(t0);

@insertpiece( HeaderCS )

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
	uint3 gl_GlobalInvocationID		: SV_DispatchThreadId
)
{
	@insertpiece( BodyCS )
}
