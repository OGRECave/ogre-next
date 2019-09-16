struct VS_INPUT
{
	float2 vertex	: POSITION;
	uint vertexId	: SV_VertexID;
	#define gl_VertexID input.vertexId
};

struct PS_INPUT
{
	float4 gl_Position		: SV_Position;
	uint gl_ViewportIndex	: SV_ViewportArrayIndex;
};

PS_INPUT main
(
	VS_INPUT input,

	uniform float ogreBaseVertex,
	uniform float2 rsDepthRange,
	uniform matrix worldViewProj
)
{
	PS_INPUT outVs;

	outVs.gl_Position.xy= mul( worldViewProj, float4( input.vertex.xy, 0.0f, 1.0f ) ).xy;
	outVs.gl_Position.z = rsDepthRange.x;
	outVs.gl_Position.w = 1.0f;
	outVs.gl_ViewportIndex = gl_VertexID >= (3 * 4) ? 1 : 0;

	return outVs;
}
