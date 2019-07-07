#version 330

#define float2 vec2
#define float3 vec3
#define float4 vec4

#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4

#define float4x4 mat4

#define mul( x, y ) ((x) * (y))
#define INLINE

#define OGRE_Load3D( tex, iuv, lod ) texelFetch( tex, ivec3( iuv ), lod )

#define inVs_vertexId gl_VertexID
#define outVs_Position gl_Position

uniform sampler3D voxelTex;
#ifdef SEPARATE_OPACITY
	uniform sampler3D otherTex;
#endif

uniform float4x4 worldViewProjMatrix;
uniform uint vertexBase;
uniform uint3 voxelResolution;

#define p_worldViewProjMatrix worldViewProjMatrix
#define p_vertexBase vertexBase
#define p_voxelResolution voxelResolution

out gl_PerVertex
{
	vec4 gl_Position;
};

out block
{
	flat float4 voxelColour;
} outVs;

#define HEADER
	#include "VoxelVisualizer_vs.any"
#undef HEADER

void main()
{
	#include "VoxelVisualizer_vs.any"
}
