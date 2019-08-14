
#define INLINE

#define OGRE_Sample( tex, sampler, uv ) tex.Sample( sampler, uv )

struct PS_INPUT
{
	float2 uv0			: TEXCOORD0;
	float3 cameraDir	: TEXCOORD1;
};

Texture2D<float> depthTexture	: register(t0);
SamplerState pointSampler		: register(s0);

uniform float2 projectionParams;
uniform float3 probeShapeHalfSize;
uniform float3 cameraPosLS;

uniform float3x3 viewSpaceToProbeLocalSpace;

#define p_projectionParams				projectionParams
#define p_probeShapeHalfSize			probeShapeHalfSize
#define p_cameraPosLS					cameraPosLS
#define p_viewSpaceToProbeLocalSpace	viewSpaceToProbeLocalSpace

#define HEADER
	#include "PccDepthCompressor_ps.any"
#undef HEADER

float4 main
(
	PS_INPUT inPs
) : SV_Target0
{
	#include "PccDepthCompressor_ps.any"

	//RGB writes should be masked off
	return float4( 0, 0, 0, alpha );
}
