@piece( SetCrossPlatformSettings )
#include <metal_stdlib>
using namespace metal;

struct float1
{
	float x;
	float1() {}
	float1( float _x ) : x( _x ) {}
};

inline float3x3 toMat3x3( float4x4 m )
{
    return float3x3( m[0].xyz, m[1].xyz, m[2].xyz );
}
inline float3x3 toMat3x3( float3x4 m )
{
	return float3x3( m[0].xyz, m[1].xyz, m[2].xyz );
}

#define ogre_float4x3 float3x4

//Short used for read operations. It's an int in GLSL & HLSL. An ushort in Metal
#define rshort2 ushort2
#define rint uint
//Short used for write operations. It's an int in GLSL. An ushort in HLSL & Metal
#define wshort2 ushort2
#define wshort3 ushort3

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
#define discard discard_fragment()

#define inVs_vertex input.position
#define inVs_blendWeights input.blendWeights
#define inVs_blendIndices input.blendIndices
#define inVs_qtangent input.qtangent
@property( iOS )
    #define inVs_drawId (baseInstance + instanceId)
@else
    #define inVs_drawId input.drawId
@end
@foreach( hlms_uv_count, n )
    #define inVs_uv@n input.uv@n@end

#define outVs_Position outVs.gl_Position
#define outVs_clipDistance0 outVs.gl_ClipDistance0

#define gl_SampleMaskIn0 gl_SampleMask
//#define interpolateAtSample( interp, subsample ) interpolateAtSample( interp, subsample )
#define findLSB ctz

#define outPs_colour0 outPs.colour0
#define OGRE_Sample( tex, sampler, uv ) tex.sample( sampler, uv )
#define OGRE_SampleLevel( tex, sampler, uv, lod ) tex.sample( sampler, uv, level( lod ) )
#define OGRE_SampleArray2D( tex, sampler, uv, arrayIdx ) tex.sample( sampler, float2( uv ), arrayIdx )
#define OGRE_SampleArray2DLevel( tex, sampler, uv, arrayIdx, lod ) tex.sample( sampler, float2( uv ), ushort( arrayIdx ), level( lod ) )
#define OGRE_SampleArrayCubeLevel( tex, sampler, uv, arrayIdx, lod ) tex.sample( sampler, float3( uv ), ushort( arrayIdx ), level( lod ) )
#define OGRE_SampleGrad( tex, sampler, uv, ddx, ddy ) tex.sample( sampler, uv, gradient2d( ddx, ddy ) )
#define OGRE_SampleArray2DGrad( tex, sampler, uv, arrayIdx, ddx, ddy ) tex.sample( sampler, uv, ushort( arrayIdx ), gradient2d( ddx, ddy ) )
#define OGRE_ddx( val ) dfdx( val )
#define OGRE_ddy( val ) dfdy( val )
#define OGRE_Load2D( tex, iuv, lod ) tex.read( iuv, lod )
#define OGRE_Load2DMS( tex, iuv, subsample ) tex.read( iuv, subsample )

#define OGRE_Load3D( tex, iuv, lod ) tex.read( ushort3( iuv ), lod )

#define bufferFetch( buffer, idx ) buffer[idx]
#define bufferFetch1( buffer, idx ) buffer[idx]

#define structuredBufferFetch( buffer, idx ) buffer[idx]

#define OGRE_Texture3D_float4 texture3d<float>

#define OGRE_SAMPLER_ARG_DECL( samplerName ) , sampler samplerName
#define OGRE_SAMPLER_ARG( samplerName ) , samplerName

#define CONST_BUFFER_STRUCT_BEGIN( structName, bindingPoint ) struct structName
#define CONST_BUFFER_STRUCT_END( variableName )

#define FLAT_INTERPOLANT( decl, bindingPoint ) decl [[flat]]
#define INTERPOLANT( decl, bindingPoint ) decl

#define OGRE_OUT_REF( declType, variableName ) thread declType &variableName
#define OGRE_INOUT_REF( declType, variableName ) thread declType &variableName

#define OGRE_ARRAY_START( type ) {
#define OGRE_ARRAY_END }
@end
