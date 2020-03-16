
#include <metal_stdlib>
using namespace metal;

#define mul( x, y ) ((x) * (y))
#define INLINE inline

#define OGRE_SampleLevel( tex, sampler, uv, lod ) tex.sample( sampler, uv, level( lod ) )

#define p_borderedRes		p.allParams.x
#define p_fullWidth			p.allParams.y
#define p_invFullResolution	p.allParams.zw
#define p_rangeMult			p.rangeMult

struct PS_INPUT
{
	uint probeIdx [[flat]];
	float3 normal;
};

struct Params
{
	float4 allParams;
	float2 rangeMult;
};

#define HEADER
	#include "IfdVisualizer_ps.any"
#undef HEADER

fragment float4 main_metal
(
	PS_INPUT inPs [[stage_in]]

	, constant Params &p [[buffer(PARAMETER_SLOT)]]

	, texture2d<float> ifdTex		[[texture(0)]]
	, sampler ifdSampler			[[sampler(0)]]
)
{
	float4 outColour;
	#include "IfdVisualizer_ps.any"

	return outColour;
}
