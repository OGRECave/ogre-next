#include <metal_stdlib>
using namespace metal;

#define mul( x, y ) ((x) * (y))
#define INLINE inline
#define OGRE_Sample( tex, sampler, uv ) tex.sample( sampler, uv )

struct PS_INPUT
{
	float2 uv0;
	float3 cameraDir;
};

struct Params
{
	float2 projectionParams;
	float3 probeShapeHalfSize;
	float3 cameraPosLS;

	float3x3 viewSpaceToProbeLocalSpace;
};

#define p_projectionParams				p.projectionParams
#define p_probeShapeHalfSize			p.probeShapeHalfSize
#define p_cameraPosLS					p.cameraPosLS
#define p_viewSpaceToProbeLocalSpace	p.viewSpaceToProbeLocalSpace

#define HEADER
	#include "PccDepthCompressor_ps.any"
#undef HEADER

fragment float4 main_metal
(
	PS_INPUT inPs [[stage_in]],
	texture2d<float>	depthTexture	[[texture(0)]],
	sampler				pointSampler	[[sampler(0)]],

	constant Params &p					[[buffer(PARAMETER_SLOT)]]
)
{
	#include "PccDepthCompressor_ps.any"

	//RGB writes should be masked off
	return float4( 0, 0, 0, alpha );
}
