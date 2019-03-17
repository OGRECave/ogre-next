#include <metal_stdlib>
using namespace metal;

//Short used for read operations. It's an int in GLSL & HLSL. An ushort in Metal
#define rshort2 ushort2

struct PS_INPUT
{
	float2 uv0;
};

fragment float4 main_metal
(
	PS_INPUT inPs [[stage_in]],
	float4				gl_FragCoord	[[position]],
	texture2d_array<float>	srcTex			[[texture(0)]]
)
{
	short2 iFragCoord = short2( gl_FragCoord.xy );

	float4 c = srcTex.read( rshort2( iFragCoord.xy ), 0u, 0u );

	if( c.a == 0 )
	{
		c = c.a > 0 ? c : srcTex.read( rshort2( iFragCoord.xy + short2(  1,  0 ) ), 0u, 0u );
		c = c.a > 0 ? c : srcTex.read( rshort2( iFragCoord.xy + short2(  0,  1 ) ), 0u, 0u );
		c = c.a > 0 ? c : srcTex.read( rshort2( iFragCoord.xy + short2( -1,  0 ) ), 0u, 0u );
		c = c.a > 0 ? c : srcTex.read( rshort2( iFragCoord.xy + short2(  0, -1 ) ), 0u, 0u );

		c = c.a > 0 ? c : srcTex.read( rshort2( iFragCoord.xy + short2(  1,  1 ) ), 0u, 0u );
		c = c.a > 0 ? c : srcTex.read( rshort2( iFragCoord.xy + short2( -1,  1 ) ), 0u, 0u );
		c = c.a > 0 ? c : srcTex.read( rshort2( iFragCoord.xy + short2(  1, -1 ) ), 0u, 0u );
		c = c.a > 0 ? c : srcTex.read( rshort2( iFragCoord.xy + short2(  1, -1 ) ), 0u, 0u );
	}

	return c.xyzw;
}
