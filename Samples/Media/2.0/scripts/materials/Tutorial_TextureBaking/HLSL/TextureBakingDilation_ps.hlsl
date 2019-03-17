//Short used for read operations. It's an int in GLSL & HLSL. An ushort in Metal
#define rshort2 int2
#define rshort4 int4
#define short2 int2

Texture2DArray srcTex;

struct PS_INPUT
{
	float2 uv0			: TEXCOORD0;
};

float4 main( PS_INPUT inPs, float4 gl_FragCoord : SV_Position ) : SV_Target
{
	rshort2 iFragCoord = rshort2( gl_FragCoord.xy );

	float4 c = srcTex.Load( rshort4( iFragCoord.xy, 0, 0 ) );

	if( c.a == 0 )
	{
		c = c.a > 0 ? c : srcTex.Load( rshort4( iFragCoord.xy + short2(  1,  0 ), 0, 0 ) );
		c = c.a > 0 ? c : srcTex.Load( rshort4( iFragCoord.xy + short2(  0,  1 ), 0, 0 ) );
		c = c.a > 0 ? c : srcTex.Load( rshort4( iFragCoord.xy + short2( -1,  0 ), 0, 0 ) );
		c = c.a > 0 ? c : srcTex.Load( rshort4( iFragCoord.xy + short2(  0, -1 ), 0, 0 ) );

		c = c.a > 0 ? c : srcTex.Load( rshort4( iFragCoord.xy + short2(  1,  1 ), 0, 0 ) );
		c = c.a > 0 ? c : srcTex.Load( rshort4( iFragCoord.xy + short2( -1,  1 ), 0, 0 ) );
		c = c.a > 0 ? c : srcTex.Load( rshort4( iFragCoord.xy + short2(  1, -1 ), 0, 0 ) );
		c = c.a > 0 ? c : srcTex.Load( rshort4( iFragCoord.xy + short2(  1, -1 ), 0, 0 ) );
	}

	return c.xyzw;
}
