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

inline half3x3 toMatHalf3x3( half4x4 m )
{
	return half3x3( m[0].xyz, m[1].xyz, m[2].xyz );
}
inline half3x3 toMatHalf3x3( half3x4 m )
{
	return half3x3( m[0].xyz, m[1].xyz, m[2].xyz );
}
inline half3x3 toMatHalf3x3( float4x4 m )
{
	return half3x3( half3( m[0].xyz ), half3(  m[1].xyz ), half3(  m[2].xyz ) );
}
inline half3x3 toMatHalf3x3( float3x4 m )
{
	return half3x3( half3( m[0].xyz ), half3(  m[1].xyz ), half3(  m[2].xyz ) );
}

#define ogre_float4x3 float3x4

//Short used for read operations. It's an int in GLSL & HLSL. An ushort in Metal
#define rshort ushort
#define rshort2 ushort2
#define rint uint
//Short used for write operations. It's an int in GLSL. An ushort in HLSL & Metal
#define wshort2 ushort2
#define wshort3 ushort3

#define toFloat3x3( x ) toMat3x3( x )
#define buildFloat3x3( row0, row1, row2 ) float3x3( float3( row0 ), float3( row1 ), float3( row2 ) )

#define buildFloat4x4( row0, row1, row2, row3 ) float4x4( float4( row0 ), float4( row1 ), float4( row2 ), float4( row3 ) )

#define getMatrixRow( mat, idx ) mat[idx]

// See CrossPlatformSettings_piece_all.glsl for an explanation
@property( precision_mode == full32 )
	// In Metal 'half' is an actual datatype. It should be OK to override it
	// as long as we do it before including metal_stdlib
	#define _h(x) (x)

	#define midf float
	#define midf2 float2
	#define midf3 float3
	#define midf4 float4
	#define midf2x2 float2x2
	#define midf3x3 float3x3
	#define midf4x4 float4x4

	#define midf_c float
	#define midf2_c float2
	#define midf3_c float3
	#define midf4_c float4
	#define midf2x2_c float2x2
	#define midf3x3_c float3x3
	#define midf4x4_c float4x4

	#define toMidf3x3( x ) toMat3x3( x )
	#define buildMidf3x3( row0, row1, row2 ) float3x3( row0, row1, row2 )

	#define ensureValidRangeF16(x)
@end
@property( precision_mode == midf16 )
	// In Metal 'half' is an actual datatype. It should be OK to override it
	// as long as we do it before including metal_stdlib
	#define _h(x) half(x)

	#define midf half
	#define midf2 half2
	#define midf3 half3
	#define midf4 half4
	#define midf2x2 half2x2
	#define midf3x3 half3x3
	#define midf4x4 half4x4

	#define midf_c half
	#define midf2_c half2
	#define midf3_c half3
	#define midf4_c half4
	#define midf2x2_c half2x2
	#define midf3x3_c half3x3
	#define midf4x4_c half4x4

	#define toMidf3x3( x ) toMatHalf3x3( x )
	#define buildMidf3x3( row0, row1, row2 ) half3x3( half3( row0 ), half3( row1 ), half3( row2 ) )

	#define ensureValidRangeF16(x) x = min(x, 65504.0h)
@end

#define min3( a, b, c ) min( a, min( b, c ) )
#define max3( a, b, c ) max( a, max( b, c ) )

#define mul( x, y ) ((x) * (y))
#define lerp mix
#define INLINE inline
#define NO_INTERPOLATION_PREFIX
#define NO_INTERPOLATION_SUFFIX [[flat]]

#define floatBitsToUint(x) as_type<uint>(x)
inline float uintBitsToFloat( uint x )
{
	return as_type<float>( x );
}
inline float2 uintBitsToFloat( uint2 x )
{
	return as_type<float2>( x );
}
#define floatBitsToInt(x) as_type<int>(x)
#define lessThan( a, b ) (a < b)
#define discard discard_fragment()

#define inVs_vertex input.position
#define inVs_normal input.normal
#define inVs_tangent input.tangent
#define inVs_binormal input.binormal
#define inVs_blendWeights input.blendWeights
#define inVs_blendIndices input.blendIndices
#define inVs_qtangent input.qtangent
#define inVs_colour input.colour
@property( iOS )
	@property( !hlms_instanced_stereo )
		#define inVs_drawId (baseInstance + instanceId)
	@else
		#define inVs_drawId ((baseInstance + instanceId) >> 1u)
		#define inVs_stereoDrawId (baseInstance + instanceId)
	@end
@else
	@property( !hlms_instanced_stereo )
		#define inVs_drawId input.drawId
	@else
		#define inVs_drawId (input.drawId >> 1u)
		#define inVs_stereoDrawId input.drawId
	@end
@end
@foreach( hlms_uv_count, n )
    #define inVs_uv@n input.uv@n@end

#define finalDrawId inVs_drawId

#define outVs_Position outVs.gl_Position
#define outVs_viewportIndex outVs.gl_ViewportIndex
#define outVs_clipDistance0 outVs.gl_ClipDistance[0]

#define gl_SampleMaskIn0 gl_SampleMask
//#define interpolateAtSample( interp, subsample ) interpolateAtSample( interp, subsample )
#define findLSB clz
#define findMSB ctz
#define reversebits reverse_bits
#define mod( a, b ) ( (a) - (b) * floor( (a) / (b) ) )

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
#define OGRE_LoadArray2D( tex, iuv, arrayIdx, lod ) tex.read( ushort2( iuv ), arrayIdx, lod )
#define OGRE_Load2DMS( tex, iuv, subsample ) tex.read( iuv, subsample )

#define OGRE_Load3D( tex, iuv, lod ) tex.read( ushort3( iuv ), lod )

#define OGRE_Load2DF16( tex, iuv, lod ) tex.read( iuv, lod )
#define OGRE_Load2DMSF16( tex, iuv, subsample ) tex.read( iuv, subsample )
#define OGRE_SampleF16( tex, sampler, uv ) tex.sample( sampler, uv )
#define OGRE_SampleLevelF16( tex, sampler, uv, lod ) tex.sample( sampler, uv, level( lod ) )
#define OGRE_SampleArray2DF16( tex, sampler, uv, arrayIdx ) tex.sample( sampler, float2( uv ), arrayIdx )
#define OGRE_SampleArray2DLevelF16( tex, sampler, uv, arrayIdx, lod ) tex.sample( sampler, float2( uv ), ushort( arrayIdx ), level( lod ) )
#define OGRE_SampleArrayCubeLevelF16( tex, sampler, uv, arrayIdx, lod ) tex.sample( sampler, float3( uv ), ushort( arrayIdx ), level( lod ) )
#define OGRE_SampleGradF16( tex, sampler, uv, ddx, ddy ) tex.sample( sampler, uv, gradient2d( ddx, ddy ) )
#define OGRE_SampleArray2DGradF16( tex, sampler, uv, arrayIdx, ddx, ddy ) tex.sample( sampler, uv, ushort( arrayIdx ), gradient2d( ddx, ddy ) )

#define bufferFetch( buffer, idx ) buffer[idx]
#define bufferFetch1( buffer, idx ) buffer[idx]
#define readOnlyFetch( bufferVar, idx ) bufferVar[idx]
#define readOnlyFetch1( bufferVar, idx ) bufferVar[idx]

#define structuredBufferFetch( buffer, idx ) buffer[idx]

#define OGRE_Texture3D_float4 texture3d<float>

#define OGRE_ArrayTex( declType, varName, arrayCount ) array<declType, arrayCount> varName

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

#define unpackSnorm4x8 unpack_snorm4x8_to_float
#define unpackSnorm2x16 unpack_snorm2x16_to_float

@end
