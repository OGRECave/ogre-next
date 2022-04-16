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
	float3 skyColour;
	float3 sunDir;
	float3 skyLightAbsorption;
	float3 sunAbsorption;
	float4 packedParams1;
};

#define p_densityCoeff		p.packedParams0.x
#define p_lightDensity		p.packedParams0.y
#define p_sunHeight			p.packedParams0.z
#define p_sunHeightWeight	p.packedParams0.w
#define p_skyColour			p.skyColour
#define p_sunDir			p.sunDir
#define p_skyLightAbsorption	p.skyLightAbsorption
#define p_sunAbsorption		p.sunAbsorption
#define p_mieAbsorption		p.packedParams1.xyz
#define p_finalMultiplier	p.packedParams1.w

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

	fragColour.xyz = finalResult;
	fragColour.w = 1.0f;

	return fragColour;
}
