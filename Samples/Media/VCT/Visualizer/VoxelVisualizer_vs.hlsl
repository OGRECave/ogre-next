
#define INLINE

#define OGRE_Load3D( tex, iuv, lod ) tex.Load( int4( iuv, lod ) )

#define inVs_vertexId input.vertexId
#define outVs_Position outVs.gl_Position

Texture3D voxelTex : register(t0);
#ifdef SEPARATE_OPACITY
	uniform Texture3D otherTex : register(t1);
#endif

uniform float4x4 worldViewProjMatrix;
uniform uint vertexBase;
uniform uint3 voxelResolution;

#define p_worldViewProjMatrix worldViewProjMatrix
#define p_vertexBase vertexBase
#define p_voxelResolution voxelResolution

struct VS_INPUT
{
	uint vertexId	: SV_VertexID;
};

struct PS_INPUT
{
	nointerpolation float4 voxelColour		: TEXCOORD0;

	float4 gl_Position	: SV_Position;
};

#define HEADER
	#include "VoxelVisualizer_vs.any"
#undef HEADER

PS_INPUT main( VS_INPUT input )
{
	PS_INPUT outVs;
	#include "VoxelVisualizer_vs.any"

	return outVs;
}
