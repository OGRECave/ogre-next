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

vertex PS_INPUT main_metal
(
	VS_INPUT input [[stage_in]],
	constant float4x4 &projectionMatrix	[[buffer(PARAMETER_SLOT)]]
)
{
	PS_INPUT outVs;

	outVs.gl_Position.xy	= mul( projectionMatrix, float4( input.position.xy, 0.0f, 1.0f ) ).xy;
	outVs.gl_Position.z		= 0.0f;
	outVs.gl_Position.w		= input.position.w;
	outVs.gl_ViewportIndex	= uint( input.position.z );

	return outVs;
}
