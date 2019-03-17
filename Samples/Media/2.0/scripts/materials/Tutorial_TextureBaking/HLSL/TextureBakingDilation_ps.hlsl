//Short used for read operations. It's an int in GLSL & HLSL. An ushort in Metal
#define rshort2 int2
#define rshort3 int3
#define short2 int2

Texture2D srcTex;

struct PS_INPUT
{
	float2 uv0			: TEXCOORD0;
};

float4 main( PS_INPUT inPs, float4 gl_FragCoord : SV_Position ) : SV_Target
{
	rshort2 iFragCoord = rshort2( gl_FragCoord.x * 2.0, gl_FragCoord.y * 2.0 );

	float4 c = srcTex.Load( rshort3( iFragCoord.xy, 0 ) );

	if( c.a == 0 )
	{
		c = c.a > 0 ? c : srcTex.Load( rshort3( iFragCoord.xy + short2(  1,  0 ), 0 ) );
		c = c.a > 0 ? c : srcTex.Load( rshort3( iFragCoord.xy + short2(  0,  1 ), 0 ) );
		c = c.a > 0 ? c : srcTex.Load( rshort3( iFragCoord.xy + short2( -1,  0 ), 0 ) );
		c = c.a > 0 ? c : srcTex.Load( rshort3( iFragCoord.xy + short2(  0, -1 ), 0 ) );

		c = c.a > 0 ? c : srcTex.Load( rshort3( iFragCoord.xy + short2(  1,  1 ), 0 ) );
		c = c.a > 0 ? c : srcTex.Load( rshort3( iFragCoord.xy + short2( -1,  1 ), 0 ) );
		c = c.a > 0 ? c : srcTex.Load( rshort3( iFragCoord.xy + short2(  1, -1 ), 0 ) );
		c = c.a > 0 ? c : srcTex.Load( rshort3( iFragCoord.xy + short2(  1, -1 ), 0 ) );
	}

	return c.xyzw;
}
