#include <metal_stdlib>
using namespace metal;

struct PS_INPUT
{
	float2 uv0;
};

fragment float4 main_metal
(
	PS_INPUT inPs [[stage_in]],
	texture2d_array<float>	myTexture [[texture(0)]],
	sampler					mySampler [[sampler(0)]],
	constant float &sliceIdx [[buffer(PARAMETER_SLOT)]]
)
{
	return myTexture.sample( mySampler, inPs.uv0, (uint)sliceIdx ).xyzw;
}
