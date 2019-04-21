#version 430

@property( hlms_amd_trinary_minmax )
	#extension GL_AMD_shader_trinary_minmax: require
@else
	#define min3( a, b, c ) min( a, min( b, c ) )
	#define max3( a, b, c ) max( a, max( b, c ) )
@end

#extension GL_ARB_shader_group_vote: require

#define float2 vec2
#define float3 vec3
#define float4 vec4

#define int3 ivec3

#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4

#define wshort3 int3

#define ogre_float4x3 mat3x4

#define mul( x, y ) ((x) * (y))

#define toFloat3x3( x ) mat3( x )

#define bufferFetch texelFetch

#define OGRE_SampleArray2D( tex, sampler, uv, arrayIdx ) texture( tex, vec3( uv, arrayIdx ) )

#define CONST_BUFFER( bufferName, bindingPoint ) layout( std140, binding = bindingPoint) uniform bufferName

#define OGRE_imageLoad3D( inImage, iuv ) imageLoad( inImage, int3( iuv ) )
#define OGRE_imageWrite3D1( outImage, iuv, value ) imageStore( outImage, int3( iuv ), value )
#define OGRE_imageWrite3D4( outImage, iuv, value ) imageStore( outImage, int3( iuv ), value )

@insertpiece( PreBindingsHeaderCS )

uniform sampler3D voxelAlbedoTex;
uniform sampler3D voxelNormalTex;
uniform sampler3D voxelEmissiveTex;

layout (@insertpiece(uav0_pf_type)) uniform restrict writeonly image3D lightVoxel;

layout( local_size_x = @value( threads_per_group_x ),
		local_size_y = @value( threads_per_group_y ),
		local_size_z = @value( threads_per_group_z ) ) in;

//layout( local_size_x = 4,
//		local_size_y = 4,
//		local_size_z = 4 ) in;


@insertpiece( HeaderCS )

uniform uint numLights;
uniform float3 rayMarchStepSize;
//uniform float3 voxelOrigin;
//uniform float3 voxelCellSize;

#define p_numLights numLights
#define p_rayMarchStepSize rayMarchStepSize
//#define p_voxelOrigin voxelOrigin
//#define p_voxelCellSize voxelCellSize

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

void main()
{
	@insertpiece( BodyCS )
}
