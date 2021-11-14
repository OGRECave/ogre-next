#include <metal_stdlib>
using namespace metal;

struct PS_INPUT
{
	float2 uv0;
};

fragment float4 main_metal
(
	PS_INPUT inPs [[stage_in]],
	array<texture2d<float>, 2> myTexA	[[texture(0)]],
	array<texture2d<float>, 2> myTexB	[[texture(2)]],
	texture2d<float> myTexC				[[texture(4)]],
	sampler samplerState				[[sampler(0)]]
)
{
	float4 a = myTexA[0].sample( samplerState, inPs.uv0 );
	float4 b = myTexA[1].sample( samplerState, inPs.uv0 );
	float4 c = myTexB[0].sample( samplerState, inPs.uv0 );
	float4 d = myTexB[1].sample( samplerState, inPs.uv0 );
	float4 e = myTexC.sample( samplerState, inPs.uv0 );

	float4 fragColour = a + b + c + d + e;

	return fragColour;
}
