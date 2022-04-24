#include <metal_stdlib>
using namespace metal;

#define lerp mix

struct PS_INPUT
{
	float3 cameraDir;
};

struct Params
{
	float4 packedParams0;
	float3 skyLightAbsorption;
	float4 sunAbsorption;
	float4 cameraDisplacement;
	float4 packedParams1;
	float4 packedParams2;
	float4 packedParams3;
};

#define p_densityCoeff		p.packedParams0.x
#define p_lightDensity		p.packedParams0.y
#define p_sunHeight			p.packedParams0.z
#define p_sunHeightWeight	p.packedParams0.w
#define p_skyLightAbsorption	p.skyLightAbsorption
#define p_sunAbsorption		p.sunAbsorption.xyz
#define p_sunPower			p.sunAbsorption.w
#define p_cameraDisplacement	p.cameraDisplacement
#define p_mieAbsorption		p.packedParams1.xyz
#define p_finalMultiplier	p.packedParams1.w
#define p_sunDir			p.packedParams2.xyz
#define p_borderLimit		p.packedParams2.w
#define p_skyColour			p.packedParams3.xyz
#define p_densityDiffusion	p.packedParams3.w

#define HEADER
#include "AtmosphereNprSky_ps.any"
#undef HEADER

fragment float4 main_metal
(
	PS_INPUT inPs [[stage_in]],

	constant Params &p [[buffer(PARAMETER_SLOT)]]
)
{
	float4 fragColour;

	#include "AtmosphereNprSky_ps.any"

	fragColour.xyz = atmoColour;
	fragColour.w = 1.0f;

	return fragColour;
}
