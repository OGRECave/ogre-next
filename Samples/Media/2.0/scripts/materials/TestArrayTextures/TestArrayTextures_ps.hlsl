Texture2D myTexA[2]				: register(t0);
Texture2D myTexB[2]				: register(t2);
Texture2D myTexC				: register(t4);
SamplerState samplerState		: register(s0);

float4 main( float2 uv0 : TEXCOORD0 ) : SV_Target
{
	float4 a = myTexA[0].Sample( samplerState, uv0 );
	float4 b = myTexA[1].Sample( samplerState, uv0 );
	float4 c = myTexB[0].Sample( samplerState, uv0 );
	float4 d = myTexB[1].Sample( samplerState, uv0 );
	float4 e = myTexC.Sample( samplerState, uv0 );

	float4 fragColour = a + b + c + d + e;

	return fragColour;
}
