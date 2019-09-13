#include <metal_stdlib>
using namespace metal;

#define PI 3.14159265359f

struct PS_INPUT
{
	float3 cameraDir;
};

fragment float4 main_metal
(
	PS_INPUT inPs [[stage_in]],
	texture2d_array<float> skyEquirectangular	[[texture(0)]],
	sampler samplerState						[[sampler(0)]],

	constant float &sliceIdx [[buffer(PARAMETER_SLOT)]]
)
{
	float3 cameraDir = normalize( inPs.cameraDir );
	float2 longlat;
	longlat.x = atan2( cameraDir.x, -cameraDir.z ) + PI;
	longlat.y = acos( cameraDir.y );
	float2 uv = longlat / float2( 2.0f * PI, PI );

	return skyEquirectangular.sample( samplerState, uv.xy, uint( sliceIdx ) ).xyzw;
}
