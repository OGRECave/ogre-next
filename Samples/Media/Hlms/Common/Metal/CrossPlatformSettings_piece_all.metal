@piece( SetCrossPlatformSettings )
#include <metal_stdlib>
using namespace metal;

struct float1
{
	float x;
	float1() {}
	float1( float _x ) : x( _x ) {}
};

#define ogre_float4x3 float3x4

#define toFloat3x3( x ) toMat3x3( x )
#define buildFloat3x3( row0, row1, row2 ) float3x3( row0, row1, row2 )

#define min3( a, b, c ) min( a, min( b, c ) )
#define max3( a, b, c ) max( a, max( b, c ) )

#define mul( x, y ) ((x) * (y))
#define lerp mix
#define INLINE inline
#define NO_INTERPOLATION_PREFIX
#define NO_INTERPOLATION_SUFFIX [[flat]]

#define finalDrawId drawId

#define floatBitsToUint(x) as_type<uint>(x)
#define uintBitsToFloat(x) as_type<float>(x)
#define floatBitsToInt(x) as_type<int>(x)
#define lessThan( a, b ) (a < b)

#define outVs_Position outVs.gl_Position

#define gl_SampleMaskIn0 gl_SampleMask
//#define interpolateAtSample( interp, subsample ) interpolateAtSample( interp, subsample )
#define findLSB ctz

#define OGRE_Sample( tex, sampler, uv ) tex.sample( sampler, uv )
#define OGRE_SampleLevel( tex, sampler, uv, lod ) tex.sample( sampler, uv, level( lod ) )
#define OGRE_SampleArray2D( tex, sampler, uv, arrayIdx ) tex.sample( sampler, float2( uv ), arrayIdx )
#define OGRE_SampleArray2DLevel( tex, sampler, uv, arrayIdx, lod ) tex.sample( sampler, float2( uv ), ushort( arrayIdx ), level( lod ) )
#define OGRE_SampleArrayCubeLevel( tex, sampler, uv, arrayIdx, lod ) tex.sample( sampler, float3( uv ), ushort( arrayIdx ), level( lod ) )
#define OGRE_SampleGrad( tex, sampler, uv, ddx, ddy ) tex.sample( sampler, uv, gradient2d( ddx, ddy ) )
#define OGRE_SampleArray2DGrad( tex, sampler, uv, arrayIdx, ddx, ddy ) tex.sample( sampler, uv, ushort( arrayIdx ), gradient2d( ddx, ddy ) )
#define OGRE_ddx( val ) dfdx( val )
#define OGRE_ddy( val ) dfdy( val )

#define bufferFetch( buffer, idx ) buffer[idx]
#define bufferFetch1( buffer, idx ) buffer[idx]

#define CONST_BUFFER_STRUCT_BEGIN( structName, bindingPoint ) struct structName
#define CONST_BUFFER_STRUCT_END( variableName )

#define FLAT_INTERPOLANT( decl, bindingPoint ) decl [[flat]]
#define INTERPOLANT( decl, bindingPoint ) decl
@end
