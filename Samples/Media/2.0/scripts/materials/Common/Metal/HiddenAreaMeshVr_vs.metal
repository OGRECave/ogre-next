#include <metal_stdlib>
using namespace metal;

struct VS_INPUT
{
	float4 position	[[attribute(VES_POSITION)]];
};

struct PS_INPUT
{
	float4 gl_Position		[[position]];
	uint gl_ViewportIndex	[[viewport_array_index]];
};

struct Params
{
	float4x4 projectionMatrix;
	float2 rsDepthRange;
};

vertex PS_INPUT main_metal
(
	VS_INPUT input [[stage_in]],
	constant Params &p	[[buffer(PARAMETER_SLOT)]]
)
{
	PS_INPUT outVs;

	outVs.gl_Position.xy	= ( p.projectionMatrix * float4( input.position.xy, 0.0f, 1.0f ) ).xy;
	outVs.gl_Position.z		= p.rsDepthRange.x;
	outVs.gl_Position.w		= 1.0f;
	outVs.gl_ViewportIndex	= uint( input.position.z );

	return outVs;
}
