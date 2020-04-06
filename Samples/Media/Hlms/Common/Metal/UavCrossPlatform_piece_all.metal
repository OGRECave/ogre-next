
@piece( DeclUavCrossPlatform )

#define OGRE_imageLoad2D( inImage, iuv ) inImage.read( ushort2( iuv ) )
#define OGRE_imageLoad2DArray( inImage, iuvw ) inImage.read( ushort2( iuvw.xy ), ushort( iuvw.z ) )

#define OGRE_imageWrite2D1( outImage, iuv, value ) outImage.write( float4( value, 0, 0, 0 ), iuv )
#define OGRE_imageWrite2D2( outImage, iuv, value ) outImage.write( (value).xyxy, iuv )
#define OGRE_imageWrite2D4( outImage, iuv, value ) outImage.write( value, iuv )

#define OGRE_imageLoad3D( inImage, iuv ) inImage.read( ushort3( iuv ) )

#define OGRE_imageWrite3D1( outImage, iuv, value ) outImage.write( value.x, iuv )
#define OGRE_imageWrite3D4( outImage, iuv, value ) outImage.write( value, iuv )

#define OGRE_imageWrite2DArray1( outImage, iuvw, value ) outImage.write( value.x, ushort2( iuvw.xy ), ushort( iuvw.z ) )
#define OGRE_imageWrite2DArray4( outImage, iuvw, value ) outImage.write( value, ushort2( iuvw.xy ), ushort( iuvw.z ) )

#define __sharedOnlyBarrier threadgroup_barrier( mem_flags::mem_threadgroup )

@end
