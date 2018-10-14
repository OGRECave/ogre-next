
TextureCube<float4>	cubeTexture : register(t0);
SamplerState		samplerState : register(s0);

struct PS_INPUT
{
	float2 uv0			: TEXCOORD0;
};

float4 main
(
	PS_INPUT inPs,
	uniform float lodLevel
) : SV_Target0
{
	float3 cubeDir;

	cubeDir.x = fmod( inPs.uv0.x, 0.5 ) * 4.0 - 1.0;
	cubeDir.y = inPs.uv0.y * 2.0 - 1.0;
	cubeDir.z = 0.5 - 0.5 * (cubeDir.x * cubeDir.x + cubeDir.y * cubeDir.y);

	cubeDir.z = inPs.uv0.x < 0.5 ? cubeDir.z : -cubeDir.z;

	return cubeTexture.SampleLevel( samplerState, cubeDir.xyz, lodLevel ).xyzw;
}
