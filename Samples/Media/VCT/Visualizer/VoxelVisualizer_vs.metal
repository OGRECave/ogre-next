
#include <metal_stdlib>
using namespace metal;

#define mul( x, y ) ((x) * (y))
#define INLINE inline

#define OGRE_Load3D( tex, iuv, lod ) tex.read( ushort3( iuv ), lod )

#define outVs_Position outVs.gl_Position

struct Params
{
	float4x4 worldViewProjMatrix;
	uint vertexBase;
	uint3 voxelResolution;
};

#define p_worldViewProjMatrix p.worldViewProjMatrix
#define p_vertexBase p.vertexBase
#define p_voxelResolution p.voxelResolution

struct PS_INPUT
{
	float4 voxelColour [[flat]];

	float4 gl_Position [[position]];
};

#define HEADER
	#include "VoxelVisualizer_vs.any"
#undef HEADER

vertex PS_INPUT main_metal
(
	uint inVs_vertexId	[[vertex_id]]

	, constant Params &p [[buffer(PARAMETER_SLOT)]]
	, texture3d<float, access::read> voxelTex	[[texture(0)]]
	#ifdef SEPARATE_OPACITY
	,	texture3d<float, access::read> otherTex	[[texture(1)]]
	#endif
)
{
	PS_INPUT outVs;
	#include "VoxelVisualizer_vs.any"

	return outVs;
}
