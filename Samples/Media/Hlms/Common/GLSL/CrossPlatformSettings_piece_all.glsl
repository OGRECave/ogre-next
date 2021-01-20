@piece( SetCrossPlatformSettings )
@property( syntax == glslvk )
	#version 450 core
@else
	@property( GL3+ >= 430 )
		#version 430 core
	@else
		#version 330 core
	@end
@end

@insertpiece( CustomGlslExtensions )

@property( GL_ARB_shading_language_420pack )
    #extension GL_ARB_shading_language_420pack: require
    #define layout_constbuffer(x) layout( std140, x )
@else
	#define layout_constbuffer(x) layout( std140 )
@end

@property( hlms_instanced_stereo )
	#extension GL_ARB_shader_viewport_layer_array: require
@end

@property( GL_ARB_texture_buffer_range )
	#define bufferFetch texelFetch
	#define structuredBufferFetch texelFetch
@end

@property( hlms_amd_trinary_minmax )
	#extension GL_AMD_shader_trinary_minmax: require
@else
	#define min3( a, b, c ) min( a, min( b, c ) )
	#define max3( a, b, c ) max( a, max( b, c ) )
@end

#define float2 vec2
#define float3 vec3
#define float4 vec4

#define int2 ivec2
#define int3 ivec3
#define int4 ivec4

#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4

#define float2x2 mat2
#define float3x3 mat3
#define float4x4 mat4
#define ogre_float4x3 mat3x4

#define ushort uint
#define ushort3 uint3
#define ushort4 uint4

//Short used for read operations. It's an int in GLSL & HLSL. An ushort in Metal
#define rshort int
#define rshort2 int2
#define rint int
//Short used for write operations. It's an int in GLSL. An ushort in HLSL & Metal
#define wshort2 int2
#define wshort3 int3

#define toFloat3x3( x ) mat3( x )
#define buildFloat3x3( row0, row1, row2 ) mat3( row0, row1, row2 )

#define mul( x, y ) ((x) * (y))
#define saturate(x) clamp( (x), 0.0, 1.0 )
#define lerp mix
#define rsqrt inversesqrt
#define INLINE
#define NO_INTERPOLATION_PREFIX flat
#define NO_INTERPOLATION_SUFFIX

#define PARAMS_ARG_DECL
#define PARAMS_ARG

@property( syntax != glslvk )
	#define inVs_vertexId gl_VertexID
@else
	#define inVs_vertexId gl_VertexIndex
@end
#define inVs_vertex vertex
#define inVs_normal normal
#define inVs_tangent tangent
#define inVs_binormal binormal
#define inVs_blendWeights blendWeights
#define inVs_blendIndices blendIndices
#define inVs_qtangent qtangent
#define inVs_colour colour

@property( !hlms_instanced_stereo )
	#define inVs_drawId drawId
@else
	#define inVs_drawId (drawId >> 1u)
	#define inVs_stereoDrawId drawId
@end

#define finalDrawId inVs_drawId

@foreach( hlms_uv_count, n )
	#define inVs_uv@n uv@n@end

#define outVs_Position gl_Position
#define outVs_viewportIndex gl_ViewportIndex
#define outVs_clipDistance0 gl_ClipDistance[0]

#define gl_SampleMaskIn0 gl_SampleMaskIn[0]
#define reversebits bitfieldReverse

#define outPs_colour0 outColour
@property( syntax != glslvk )
	#define OGRE_Sample( tex, sampler, uv ) texture( tex, uv )
	#define OGRE_SampleLevel( tex, sampler, uv, lod ) textureLod( tex, uv, lod )
	#define OGRE_SampleArray2D( tex, sampler, uv, arrayIdx ) texture( tex, vec3( uv, arrayIdx ) )
	#define OGRE_SampleArray2DLevel( tex, sampler, uv, arrayIdx, lod ) textureLod( tex, vec3( uv, arrayIdx ), lod )
	#define OGRE_SampleArrayCubeLevel( tex, sampler, uv, arrayIdx, lod ) textureLod( tex, vec4( uv, arrayIdx ), lod )
	#define OGRE_SampleGrad( tex, sampler, uv, ddx, ddy ) textureGrad( tex, uv, ddx, ddy )
	#define OGRE_SampleArray2DGrad( tex, sampler, uv, arrayIdx, ddx, ddy ) textureGrad( tex, vec3( uv, arrayIdx ), ddx, ddy )

	#define texture2D sampler2D
	#define texture2DArray sampler2DArray
	#define texture3D sampler3D
	#define textureCube samplerCube
	#define textureCubeArray samplerCubeArray
@else
	#define OGRE_SampleArray2D( tex, sampler, uv, arrayIdx ) texture( sampler2DArray( tex, sampler ), vec3( uv, arrayIdx ) )
	#define OGRE_SampleArray2DLevel( tex, sampler, uv, arrayIdx, lod ) textureLod( sampler2DArray( tex, sampler ), vec3( uv, arrayIdx ), lod )
	#define OGRE_SampleArrayCubeLevel( tex, sampler, uv, arrayIdx, lod ) textureLod( samplerCubeArray( tex, sampler ), vec4( uv, arrayIdx ), lod )
	#define OGRE_SampleArray2DGrad( tex, sampler, uv, arrayIdx, ddx, ddy ) textureGrad( sampler2DArray( tex, sampler ), vec3( uv, arrayIdx ), ddx, ddy )

	float4 OGRE_Sample( texture2D t, sampler s, float2 uv ) { return texture( sampler2D( t, s ), uv ); }
	float4 OGRE_Sample( texture3D t, sampler s, float3 uv ) { return texture( sampler3D( t, s ), uv ); }
	float4 OGRE_Sample( textureCube t, sampler s, float3 uv ) { return texture( samplerCube( t, s ), uv ); }

	float4 OGRE_SampleLevel( texture2D t, sampler s, float2 uv, float lod ) { return textureLod( sampler2D( t, s ), uv, lod ); }
	float4 OGRE_SampleLevel( texture3D t, sampler s, float3 uv, float lod ) { return textureLod( sampler3D( t, s ), uv, lod ); }
	float4 OGRE_SampleLevel( textureCube t, sampler s, float3 uv, float lod ) { return textureLod( samplerCube( t, s ), uv, lod ); }

	float4 OGRE_SampleGrad( texture2D t, sampler s, float2 uv, float2 myDdx, float2 myDdy ) { return textureGrad( sampler2D( t, s ), uv, myDdx, myDdy ); }
	float4 OGRE_SampleGrad( texture3D t, sampler s, float3 uv, float3 myDdx, float3 myDdy ) { return textureGrad( sampler3D( t, s ), uv, myDdx, myDdy ); }
	float4 OGRE_SampleGrad( textureCube t, sampler s, float3 uv, float3 myDdx, float3 myDdy ) { return textureGrad( samplerCube( t, s ), uv, myDdx, myDdy ); }
@end
#define OGRE_ddx( val ) dFdx( val )
#define OGRE_ddy( val ) dFdy( val )
#define OGRE_Load2D( tex, iuv, lod ) texelFetch( tex, iuv, lod )
#define OGRE_LoadArray2D( tex, iuv, arrayIdx, lod ) texelFetch( tex, ivec3( iuv, arrayIdx ), lod )
#define OGRE_Load2DMS( tex, iuv, subsample ) texelFetch( tex, iuv, subsample )

#define OGRE_Load3D( tex, iuv, lod ) texelFetch( tex, ivec3( iuv ), lod )

#define bufferFetch1( buffer, idx ) texelFetch( buffer, idx ).x

@property( syntax != glslvk )
	#define OGRE_SAMPLER_ARG_DECL( samplerName )
	#define OGRE_SAMPLER_ARG( samplerName )

	#define CONST_BUFFER( bufferName, bindingPoint ) layout_constbuffer(binding = bindingPoint) uniform bufferName
	#define CONST_BUFFER_STRUCT_BEGIN( structName, bindingPoint ) layout_constbuffer(binding = bindingPoint) uniform structName
	#define CONST_BUFFER_STRUCT_END( variableName ) variableName

	@property( hlms_readonly_is_tex )
		#define ReadOnlyBufferF( slot, varType, varName ) uniform samplerBuffer varName
		#define ReadOnlyBufferU( slot, varType, varName ) uniform usamplerBuffer varName
		#define ReadOnlyBufferVarF( varType ) samplerBuffer
		#define readOnlyFetch( buffer, idx ) texelFetch( buffer, idx )
		#define readOnlyFetch1( buffer, idx ) texelFetch( buffer, idx ).x
	@else
		#define ReadOnlyBufferF( slot, varType, varName ) layout(std430, binding = slot) readonly restrict buffer _##varName { varType varName[]; }
		#define ReadOnlyBufferU( slot, varType, varName ) layout(std430, binding = slot) readonly restrict buffer _##varName { varType varName[]; }
		#define ReadOnlyBufferVarF( varType ) varType
		#define readOnlyFetch( bufferVar, idx ) bufferVar[idx]
		#define readOnlyFetch1( bufferVar, idx ) bufferVar[idx]
	@end
@else
	#define OGRE_SAMPLER_ARG_DECL( samplerName ) , sampler samplerName
	#define OGRE_SAMPLER_ARG( samplerName ) , samplerName

	#define CONST_BUFFER( bufferName, bindingPoint ) layout_constbuffer(ogre_B##bindingPoint) uniform bufferName
	#define CONST_BUFFER_STRUCT_BEGIN( structName, bindingPoint ) layout_constbuffer(ogre_B##bindingPoint) uniform structName
	#define CONST_BUFFER_STRUCT_END( variableName ) variableName

	#define ReadOnlyBufferF( slot, varType, varName ) layout(std430, ogre_R##slot) readonly restrict buffer _##varName { varType varName[]; }
	#define ReadOnlyBufferU ReadOnlyBufferF
	#define readOnlyFetch( bufferVar, idx ) bufferVar[idx]
	#define readOnlyFetch1( bufferVar, idx ) bufferVar[idx]
@end


#define OGRE_Texture3D_float4 texture3D

#define FLAT_INTERPOLANT( decl, bindingPoint ) flat decl
#define INTERPOLANT( decl, bindingPoint ) decl

#define OGRE_OUT_REF( declType, variableName ) out declType variableName
#define OGRE_INOUT_REF( declType, variableName ) inout declType variableName

#define OGRE_ARRAY_START( type ) type[](
#define OGRE_ARRAY_END )
@end

@property( !GL_ARB_texture_buffer_range || !GL_ARB_shading_language_420pack )
@piece( SetCompatibilityLayer )
	@property( !GL_ARB_texture_buffer_range )
        #define samplerBuffer sampler2D
        #define isamplerBuffer isampler2D
        #define usamplerBuffer usampler2D
        vec4 bufferFetch( in sampler2D sampl, in int pixelIdx )
        {
            ivec2 pos = ivec2( mod( pixelIdx, 2048 ), int( uint(pixelIdx) >> 11u ) );
            return texelFetch( sampl, pos, 0 );
        }
        ivec4 bufferFetch(in isampler2D sampl, in int pixelIdx)
        {
            ivec2 pos = ivec2( mod( pixelIdx, 2048 ), int( uint(pixelIdx) >> 11u ) );
            return texelFetch( sampl, pos, 0 );
        }
        uvec4 bufferFetch( in usampler2D sampl, in int pixelIdx )
        {
            ivec2 pos = ivec2( mod( pixelIdx, 2048 ), int( uint(pixelIdx) >> 11u ) );
            return texelFetch( sampl, pos, 0 );
        }
	@end
@end
@end
