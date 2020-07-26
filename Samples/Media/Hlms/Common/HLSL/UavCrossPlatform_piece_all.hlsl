
@piece( DeclUavCrossPlatform )

#define OGRE_imageLoad2D( inImage, iuv ) inImage[uint2( iuv )]
#define OGRE_imageLoad2DArray( inImage, iuvw ) inImage[uint3( iuvw )]

#define OGRE_imageWrite2D1( outImage, iuv, value ) outImage[uint2( iuv )] = value
#define OGRE_imageWrite2D2( outImage, iuv, value ) outImage[uint2( iuv )] = (value).xy
#define OGRE_imageWrite2D4( outImage, iuv, value ) outImage[uint2( iuv )] = value

#define OGRE_imageLoad3D( inImage, iuv ) inImage[uint3( iuv )]

#define OGRE_imageWrite3D1( outImage, iuv, value ) outImage[uint3( iuv )] = value.x
#define OGRE_imageWrite3D4( outImage, iuv, value ) outImage[uint3( iuv )] = value

#define OGRE_imageWrite2DArray1( outImage, iuvw, value ) outImage[uint3( iuvw )] = value.x
#define OGRE_imageWrite2DArray4( outImage, iuvw, value ) outImage[uint3( iuvw )] = value

#define __sharedOnlyBarrier GroupMemoryBarrierWithGroupSync()

@end
