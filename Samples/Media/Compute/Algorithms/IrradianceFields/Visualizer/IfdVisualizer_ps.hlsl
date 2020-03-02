
#define INLINE

#define OGRE_SampleLevel( tex, sampler, uv, lod ) tex.SampleLevel( sampler, uv, lod )

uniform float4 allParams;
uniform float2 rangeMult;

#define p_resolution		allParams.x
#define p_fullWidth			allParams.y
#define p_invFullResolution	allParams.zw
#define p_rangeMult			rangeMult

Texture2D ifdTex		: register(t0);
SamplerState ifdSampler	: register(s0);

struct PS_INPUT
{
	nointerpolation uint probeIdx	: TEXCOORD0;
	float3 normal                   : TEXCOORD1;
};

#define HEADER
	#include "IfdVisualizer_ps.any"
#undef HEADER

float4 main( PS_INPUT inPs ) : SV_Target
{
	float4 outColour;
	#include "IfdVisualizer_ps.any"

	return outColour;
}
