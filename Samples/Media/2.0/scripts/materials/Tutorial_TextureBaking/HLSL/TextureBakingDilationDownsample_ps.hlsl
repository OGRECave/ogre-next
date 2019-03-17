//Short used for read operations. It's an int in GLSL & HLSL. An ushort in Metal
#define rshort2 int2
#define rshort3 int3
#define rshort4 int4
#define short2 int2

Texture2DArray srcTex;

struct PS_INPUT
{
	float2 uv0			: TEXCOORD0;
};

void addSample( inout float4 accumVal, float4 newSample, inout float counter )
{
	if( newSample.a > 0 )
	{
		accumVal += newSample;
		counter += 1.0;
	}
}

float4 main( PS_INPUT inPs, float4 gl_FragCoord : SV_Position ) : SV_Target
{
	rshort2 iFragCoord = rshort2( gl_FragCoord.x * 2.0, gl_FragCoord.y * 2.0 );

	float4 accumVal = float4( 0, 0, 0, 0 );
	float counter = 0;
	float4 newSample;

	newSample = srcTex.Load( rshort4( iFragCoord.xy, 0, 0 ) );
	addSample( accumVal, newSample, counter );

	newSample = srcTex.Load( rshort4( iFragCoord.xy + rshort2( 1, 0 ), 0, 0 ) );
	addSample( accumVal, newSample, counter );

	newSample = srcTex.Load( rshort4( iFragCoord.xy + rshort2( 0, 1 ), 0, 0 ) );
	addSample( accumVal, newSample, counter );

	newSample = srcTex.Load( rshort4( iFragCoord.xy + rshort2( 1, 1 ), 0, 0 ) );
	addSample( accumVal, newSample, counter );

	if( counter > 0 )
		accumVal.xyzw /= counter;

	return accumVal.xyzw;
}
