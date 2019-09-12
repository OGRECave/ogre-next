
struct VS_INPUT
{
	float2 vertex	: POSITION;
	float3 normal	: NORMAL;
};

struct PS_INPUT
{
	float3 cameraDir	: TEXCOORD0;

	float4 gl_Position	: SV_Position;
};

PS_INPUT main
(
	VS_INPUT input,

	uniform float2 rsDepthRange,
	uniform matrix worldViewProj
)
{
	PS_INPUT outVs;

	outVs.gl_Position.xy= mul( worldViewProj, float4( input.vertex.xy, 0.0f, 1.0f ) ).xy;
	outVs.gl_Position.z = rsDepthRange.y;
	outVs.gl_Position.w = 1.0f;
	outVs.cameraDir		= input.normal;

	return outVs;
}
