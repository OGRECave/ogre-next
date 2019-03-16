
Texture2DArray	myTexture : register(t0);
SamplerState	mySampler : register(s0);

float4 main( float2 uv : TEXCOORD0, uniform float sliceIdx ) : SV_Target
{
    return myTexture.Sample( mySampler, float3( uv, sliceIdx ) ).xyzw;
}
