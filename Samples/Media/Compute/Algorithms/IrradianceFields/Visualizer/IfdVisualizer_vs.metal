
#include <metal_stdlib>
using namespace metal;

#define mul( x, y ) ((x) * (y))
#define INLINE inline

#define outVs_Position outVs.gl_Position

#define PARAMS_ARG_DECL , constant Params &p
#define PARAMS_ARG , p

struct Params
{
	float4x4 worldViewProjMatrix;
	uint vertexBase;
	uint3 bandMaskPower;
	float2 sectionsBandArc;

	uint3 numProbes;
	float3 aspectRatioFixer;
};

#define p_worldViewProjMatrix p.worldViewProjMatrix
#define p_vertexBase p.vertexBase

#define p_bandMask			p.bandMaskPower.x
#define p_bandPower			p.bandMaskPower.y
#define p_totalPoints		p.bandMaskPower.z
#define p_sectionsInBand	p.sectionsBandArc.x
#define p_sectionArc		p.sectionsBandArc.y

#define p_numProbes			p.numProbes
#define p_aspectRatioFixer	p.aspectRatioFixer

struct PS_INPUT
{
	uint probeIdx [[flat]];
	float3 normal;

	float4 gl_Position [[position]];
};

#define HEADER
	#include "IfdVisualizer_vs.any"
#undef HEADER

vertex PS_INPUT main_metal
(
	uint inVs_vertexId	[[vertex_id]]

	, constant Params &p [[buffer(PARAMETER_SLOT)]]
)
{
	PS_INPUT outVs;
	#include "IfdVisualizer_vs.any"

	return outVs;
}
