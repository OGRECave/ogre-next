
struct PS_INPUT
{
	float3 cameraDir	: TEXCOORD0;
};

#define p_densityCoeff		packedParams0.x
#define p_lightDensity		packedParams0.y
#define p_sunHeight			packedParams0.z
#define p_sunHeightWeight	packedParams0.w
#define p_skyLightAbsorption	skyLightAbsorption
#define p_sunAbsorption		sunAbsorption.xyz
#define p_sunPower			sunAbsorption.w
#define p_cameraDisplacement	cameraDisplacement
#define p_mieAbsorption		packedParams1.xyz
#define p_finalMultiplier	packedParams1.w
#define p_sunDir			packedParams2.xyz
#define p_borderLimit		packedParams2.w
#define p_skyColour			packedParams3.xyz
#define p_densityDiffusion	packedParams3.w

#define HEADER
#include "AtmosphereNprSky_ps.any"
#undef HEADER

float4 main
(
	PS_INPUT inPs,

	uniform float4 packedParams0,
	uniform float3 skyLightAbsorption,
	uniform float4 sunAbsorption,
	uniform float4 cameraDisplacement,
	uniform float4 packedParams1,
	uniform float4 packedParams2,
	uniform float4 packedParams3
) : SV_Target0
{
	float4 fragColour;

	#include "AtmosphereNprSky_ps.any"

	fragColour.xyz = atmoColour;
	fragColour.w = 1.0f;

	return fragColour;
}
