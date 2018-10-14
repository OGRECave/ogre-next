#include <metal_stdlib>
using namespace metal;

struct PS_INPUT
{
	float2 uv0;
};

fragment half4 main_metal
(
	PS_INPUT inPs [[stage_in]],

	texturecube<half> cubeTexture	[[texture(0)]],
	sampler samplerState			[[sampler(0)]],

	constant float &lodLevel		[[buffer(PARAMETER_SLOT)]]
)
{
	float3 cubeDir;

	cubeDir.x = fmod( inPs.uv0.x, 0.5 ) * 4.0 - 1.0;
	cubeDir.y = inPs.uv0.y * 2.0 - 1.0;
	cubeDir.z = 0.5 - 0.5 * (cubeDir.x * cubeDir.x + cubeDir.y * cubeDir.y);

	cubeDir.z = inPs.uv0.x < 0.5 ? cubeDir.z : -cubeDir.z;

	return cubeTexture.sample( samplerState, cubeDir.xyz, level(lodLevel) ).xyzw;
}
