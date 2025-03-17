@piece( SetCrossPlatformSettings )
#define ushort uint
#define ushort3 uint3
#define ushort4 uint4
#define ogre_float4x3 float4x3

//Short used for read operations. It's an int in GLSL & HLSL. An ushort in Metal
#define rshort int
#define rshort2 int2
#define rint int
//Short used for write operations. It's an int in GLSL. An ushort in HLSL & Metal
#define wshort2 uint2
#define wshort3 uint3

#define toFloat3x3( x ) ((float3x3)(x))
#define buildFloat3x3( row0, row1, row2 ) transpose( float3x3( row0, row1, row2 ) )

#define buildFloat4x4( row0, row1, row2, row3 ) transpose( float4x4( row0, row1, row2, row3 ) )

#define getMatrixRow( mat, idx ) transpose( mat )[idx]

// See CrossPlatformSettings_piece_all.glsl for an explanation
@property( precision_mode == full32 )
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

	#define toMidf3x3( x ) ((float3x3)( x ))
	#define buildMidf3x3( row0, row1, row2 ) transpose( float3x3( row0, row1, row2 ) )

	#define ensureValidRangeF16(x)
@end
@property( precision_mode == relaxed )
	#define _h(x) min16float((x))

	#define midf min16float
	#define midf2 min16float2
	#define midf3 min16float3
	#define midf4 min16float4
	#define midf2x2 min16float2x2
	#define midf3x3 min16float3x3
	#define midf4x4 min16float4x4

	// For casting to midf
	#define midf_c min16float
	#define midf2_c min16float2
	#define midf3_c min16float3
	#define midf4_c min16float4
	#define midf2x2_c min16float2x2
	#define midf3x3_c min16float3x3
	#define midf4x4_c min16float4x4

	#define toMidf3x3( x ) ((min16float3x3)( x ))
	#define buildMidf3x3( row0, row1, row2 ) transpose( min16float3x3( row0, row1, row2 ) )

	#define ensureValidRangeF16(x) x = min(x, min16float(65504.0))
@end

#define min3( a, b, c ) min( a, min( b, c ) )
#define max3( a, b, c ) max( a, max( b, c ) )

#define INLINE
#define NO_INTERPOLATION_PREFIX nointerpolation
#define NO_INTERPOLATION_SUFFIX

#define PARAMS_ARG_DECL
#define PARAMS_ARG

#define floatBitsToUint(x) asuint(x)
#define uintBitsToFloat(x) asfloat(x)
#define floatBitsToInt(x) asint(x)
#define fract frac
#define lessThan( a, b ) (a < b)

#define inVs_vertexId input.vertexId
#define inVs_vertex input.vertex
#define inVs_normal input.normal
#define inVs_tangent input.tangent
#define inVs_binormal input.binormal
#define inVs_blendWeights input.blendWeights
#define inVs_blendIndices input.blendIndices
#define inVs_qtangent input.qtangent
#define inVs_colour input.colour
@property( !hlms_instanced_stereo )
	#define inVs_drawId input.drawId
@else
	#define inVs_drawId (input.drawId >> 1u)
	#define inVs_stereoDrawId input.drawId
@end

#define finalDrawId inVs_drawId

@foreach( hlms_uv_count, n )
	#define inVs_uv@n input.uv@n@end

#define outVs_Position outVs.gl_Position
#define outVs_viewportIndex outVs.gl_ViewportIndex
#define outVs_clipDistance0 outVs.gl_ClipDistance0.x

#define gl_SampleMaskIn0 gl_SampleMask
#define interpolateAtSample( interp, subsample ) EvaluateAttributeAtSample( interp, subsample )
#define findLSB firstbitlow
#define findMSB firstbithigh
#define mod( a, b ) ( (a) - (b) * floor( (a) / (b) ) )

#define outPs_colour0 outPs.colour0
#define OGRE_Sample( tex, sampler, uv ) tex.Sample( sampler, uv )
#define OGRE_SampleLevel( tex, sampler, uv, lod ) tex.SampleLevel( sampler, uv, lod )
#define OGRE_SampleArray2D( tex, sampler, uv, arrayIdx ) tex.Sample( sampler, float3( uv, arrayIdx ) )
#define OGRE_SampleArray2DLevel( tex, sampler, uv, arrayIdx, lod ) tex.SampleLevel( sampler, float3( uv, arrayIdx ), lod )
#define OGRE_SampleArrayCubeLevel( tex, sampler, uv, arrayIdx, lod ) tex.SampleLevel( sampler, float4( uv, arrayIdx ), lod )
#define OGRE_SampleGrad( tex, sampler, uv, ddx, ddy ) tex.SampleGrad( sampler, uv, ddx, ddy )
#define OGRE_SampleArray2DGrad( tex, sampler, uv, arrayIdx, ddx, ddy ) tex.SampleGrad( sampler, float3( uv, arrayIdx ), ddx, ddy )
#define OGRE_ddx( val ) ddx( val )
#define OGRE_ddy( val ) ddy( val )
#define OGRE_Load2D( tex, iuv, lod ) tex.Load( int3( iuv, lod ) )
#define OGRE_LoadArray2D( tex, iuv, arrayIdx, lod ) tex.Load( int4( iuv, arrayIdx, lod ) )
#define OGRE_Load2DMS( tex, iuv, subsample ) tex.Load( iuv, subsample )

#define OGRE_Load3D( tex, iuv, lod ) tex.Load( int4( iuv, lod ) )

#define OGRE_Load2DF16( tex, iuv, lod ) tex.Load( int3( iuv, lod ) )
#define OGRE_Load2DMSF16( tex, iuv, subsample ) tex.Load( iuv, subsample )
#define OGRE_SampleF16( tex, sampler, uv ) tex.Sample( sampler, uv )
#define OGRE_SampleLevelF16( tex, sampler, uv, lod ) tex.SampleLevel( sampler, uv, lod )
#define OGRE_SampleArray2DF16( tex, sampler, uv, arrayIdx ) tex.Sample( sampler, float3( uv, arrayIdx ) )
#define OGRE_SampleArray2DLevelF16( tex, sampler, uv, arrayIdx, lod ) tex.SampleLevel( sampler, float3( uv, arrayIdx ), lod )
#define OGRE_SampleArrayCubeLevelF16( tex, sampler, uv, arrayIdx, lod ) tex.SampleLevel( sampler, float4( uv, arrayIdx ), lod )
#define OGRE_SampleGradF16( tex, sampler, uv, ddx, ddy ) tex.SampleGrad( sampler, uv, ddx, ddy )
#define OGRE_SampleArray2DGradF16( tex, sampler, uv, arrayIdx, ddx, ddy ) tex.SampleGrad( sampler, float3( uv, arrayIdx ), ddx, ddy )

#define bufferFetch( buffer, idx ) buffer.Load( idx )
#define bufferFetch1( buffer, idx ) buffer.Load( idx ).x

#define structuredBufferFetch( buffer, idx ) buffer[idx]

@property( hlms_readonly_is_tex )
	#define ReadOnlyBuffer( slot, varType, varName ) Buffer<varType> varName : register(t##slot)
	#define readOnlyFetch( buffer, idx ) buffer.Load( idx )
	#define readOnlyFetch1( buffer, idx ) buffer.Load( idx ).x
@else
	#define ReadOnlyBuffer( slot, varType, varName ) StructuredBuffer<varType> varName : register(t##slot)
	#define readOnlyFetch( bufferVar, idx ) bufferVar[idx]
	#define readOnlyFetch1( bufferVar, idx ) bufferVar[idx].x
@end

#define OGRE_Texture3D_float4 Texture3D

#define OGRE_ArrayTex( declType, varName, arrayCount ) declType varName[arrayCount]

#define OGRE_SAMPLER_ARG_DECL( samplerName ) , SamplerState samplerName
#define OGRE_SAMPLER_ARG( samplerName ) , samplerName

#define CONST_BUFFER( bufferName, bindingPoint ) cbuffer bufferName : register(b##bindingPoint)
#define CONST_BUFFER_STRUCT_BEGIN( structName, bindingPoint ) cbuffer structName : register(b##bindingPoint) { struct _##structName
#define CONST_BUFFER_STRUCT_END( variableName ) variableName; }

#define FLAT_INTERPOLANT( decl, bindingPoint ) nointerpolation decl : TEXCOORD##bindingPoint
#define INTERPOLANT( decl, bindingPoint ) decl : TEXCOORD##bindingPoint

#define OGRE_OUT_REF( declType, variableName ) out declType variableName
#define OGRE_INOUT_REF( declType, variableName ) inout declType variableName

#define OGRE_ARRAY_START( type ) {
#define OGRE_ARRAY_END }

float4 unpackSnorm4x8( uint value )
{
	int signedValue = int( value );
	int4 packed = int4( signedValue << 24, signedValue << 16, signedValue << 8, signedValue ) >> 24;
	return clamp( float4( packed ) / 127.0, -1.0, 1.0 );
}

float2 unpackSnorm2x16( uint value )
{
	int signedValue = int( value );
	int2 packed = int2( signedValue << 16, signedValue ) >> 16;
	return clamp( float2( packed ) / 32767.0, -1.0, 1.0 );
}

@end
