struct VS_INPUT
{
	float4 position	: POSITION;
};

struct PS_INPUT
{
	float4 gl_Position		: SV_Position;
	uint gl_ViewportIndex	: SV_ViewportArrayIndex;
};

PS_INPUT main
(
	VS_INPUT input,
	uniform float4x4 projectionMatrix,
	uniform float2 rsDepthRange
)
{
	PS_INPUT outVs;

	outVs.gl_Position.xy	= mul( projectionMatrix, float4( input.position.xy, 0.0f, 1.0f ) ).xy;
	outVs.gl_Position.z		= rsDepthRange.x;
	outVs.gl_Position.w		= 1.0f;
	outVs.gl_ViewportIndex	= int( input.position.z );

	return outVs;
}
