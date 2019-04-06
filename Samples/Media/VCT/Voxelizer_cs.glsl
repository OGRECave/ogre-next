// Heavily modified version based on voxelization shader written by mattatz
//	https://github.com/mattatz/unity-voxel
// Original work Copyright (c) 2018 mattatz under MIT license.
//
// mattatz's work is a full voxelization algorithm.
// We only need to voxelize the shell (aka the contour),
// not to fill the entire voxel.
//
// Adapted for Ogre and for use for Voxel Cone Tracing by
// Matias N. Goldberg Copyright (c) 2019

#version 430

@property( hlms_amd_trinary_minmax )
	#extension GL_AMD_shader_trinary_minmax: require
@else
	#define min3( a, b, c ) min( a, min( b, c ) )
	#define max3( a, b, c ) max( a, max( b, c ) )
@end

#define float2 vec2
#define float3 vec3
#define float4 vec4

#define int3 ivec3

#define uint3 uvec3

#define ogre_float4x3 mat3x4

#define mul( x, y ) ((x) * (y))

#define toFloat3x3( x ) mat3( x )

#define bufferFetch texelFetch

#define OGRE_SampleArray2D( tex, sampler, uv, arrayIdx ) texture( tex, vec3( uv, arrayIdx ) )

#define CONST_BUFFER( bufferName, bindingPoint ) layout( std140, binding = bindingPoint) uniform bufferName

@insertpiece( PreBindingsHeaderCS )

layout(std430, binding = 0) buffer vertexBufferLayout
{
	Vertex vertexBuffer[];
};
layout(std430, binding = 1) buffer indexBufferLayout
{
	uint indexBuffer[];
};

layout (@insertpiece(uav2_pf_type)) uniform restrict image3D voxelAlbedoTex;
layout (@insertpiece(uav3_pf_type)) uniform restrict image3D voxelNormalTex;
layout (@insertpiece(uav4_pf_type)) uniform restrict image3D voxelEmissiveTex;
layout (@insertpiece(uav5_pf_type)) uniform restrict uimage3D voxelAccumVal;

layout( local_size_x = @value( threads_per_group_x ),
		local_size_y = @value( threads_per_group_y ),
		local_size_z = @value( threads_per_group_z ) ) in;

//layout (rgba8) uniform restrict writeonly image3D voxelAlbedoTex;
//layout (rgb10_a2) uniform restrict writeonly image3D voxelNormalTex;
//layout (rgba8) uniform restrict writeonly image3D voxelEmissiveTex;

//layout( local_size_x = 4,
//		local_size_y = 4,
//		local_size_z = 4 ) in;


uniform samplerBuffer instanceBuffer;

@property( has_diffuse_tex || emissive_is_diffuse_tex )
	uniform sampler2DArray diffuseTex;
@end
@property( has_emissive_tex )
	uniform sampler2DArray emissiveTex;
@end

@insertpiece( HeaderCS )

uniform uint2 instanceStart_instanceEnd;
uniform float3 voxelOrigin;
uniform float3 voxelCellSize;

#define p_instanceStart instanceStart_instanceEnd.x
#define p_instanceEnd instanceStart_instanceEnd.y
#define p_numInstances numInstances
#define p_voxelOrigin voxelOrigin
#define p_voxelCellSize voxelCellSize

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

void main()
{
	@insertpiece( BodyCS )
}
