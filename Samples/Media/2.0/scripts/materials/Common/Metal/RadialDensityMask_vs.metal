#include <metal_stdlib>
using namespace metal;

struct VS_INPUT
{
	float4 position [[attribute(VES_POSITION)]];
};

struct PS_INPUT
{
	float4 gl_Position		[[position]];
	uint gl_ViewportIndex	[[viewport_array_index]];
};

struct Params
{
	float ogreBaseVertex;
	float2 rsDepthRange;
	float4x4 worldViewProj;
};

vertex PS_INPUT main_metal
(
	VS_INPUT input [[stage_in]],
	constant Params &p [[buffer(PARAMETER_SLOT)]],

	uint gl_VertexID	[[vertex_id]]
)
{
	PS_INPUT outVs;

	outVs.gl_Position.xy= ( p.worldViewProj * float4( input.position.xy, 0.0f, 1.0f ) ).xy;
	outVs.gl_Position.z = p.rsDepthRange.x;
	outVs.gl_Position.w = 1.0f;
	outVs.gl_ViewportIndex = gl_VertexID >= (3 * 4) ? 1 : 0;

	return outVs;
}
