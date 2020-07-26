
#define INLINE

#define inVs_vertexId input.vertexId
#define outVs_Position outVs.gl_Position

uniform float4x4 worldViewProjMatrix;
uniform uint vertexBase;
uniform uint3 bandMaskPower;
uniform float2 sectionsBandArc;

uniform uint3 numProbes;
uniform float3 aspectRatioFixer;

#define p_worldViewProjMatrix worldViewProjMatrix
#define p_vertexBase vertexBase

#define p_bandMask			bandMaskPower.x
#define p_bandPower			bandMaskPower.y
#define p_totalPoints		bandMaskPower.z
#define p_sectionsInBand	sectionsBandArc.x
#define p_sectionArc		sectionsBandArc.y

#define p_numProbes			numProbes
#define p_aspectRatioFixer	aspectRatioFixer

#define PARAMS_ARG_DECL
#define PARAMS_ARG

struct VS_INPUT
{
	uint vertexId	: SV_VertexID;
};

struct PS_INPUT
{
	nointerpolation uint probeIdx	: TEXCOORD0;
	float3 normal                   : TEXCOORD1;

	float4 gl_Position	: SV_Position;
};

#define HEADER
	#include "IfdVisualizer_vs.any"
#undef HEADER

PS_INPUT main( VS_INPUT input )
{
	PS_INPUT outVs;
	#include "IfdVisualizer_vs.any"

	return outVs;
}
