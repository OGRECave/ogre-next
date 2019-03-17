#include <metal_stdlib>
using namespace metal;

//Short used for read operations. It's an int in GLSL & HLSL. An ushort in Metal
#define rshort2 ushort2

struct PS_INPUT
{
	float2 uv0;
};

void addSample( thread float4 &accumVal, float4 newSample, thread float &counter )
{
	if( newSample.a > 0 )
	{
		accumVal += newSample;
		counter += 1.0;
	}
}

fragment float4 main_metal
(
	PS_INPUT inPs [[stage_in]],
	float4              	gl_FragCoord	[[position]],
	texture2d_array<float>	srcTex			[[texture(0)]]
)
{
	rshort2 iFragCoord = rshort2( gl_FragCoord.x * 2.0, gl_FragCoord.y * 2.0 );

	float4 accumVal = float4( 0, 0, 0, 0 );
	float counter = 0;
	float4 newSample;

	newSample = srcTex.read( iFragCoord.xy, 0u, 0u );
	addSample( accumVal, newSample, counter );

	newSample = srcTex.read( iFragCoord.xy + rshort2( 1, 0 ), 0u, 0u );
	addSample( accumVal, newSample, counter );

	newSample = srcTex.read( iFragCoord.xy + rshort2( 0, 1 ), 0u, 0u );
	addSample( accumVal, newSample, counter );

	newSample = srcTex.read( iFragCoord.xy + rshort2( 1, 1 ), 0u, 0u );
	addSample( accumVal, newSample, counter );

	if( counter > 0 )
		accumVal.xyzw /= counter;

	return accumVal.xyzw;
}
