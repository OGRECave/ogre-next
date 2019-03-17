#version 330

//Short used for read operations. It's an int in GLSL & HLSL. An ushort in Metal
#define rshort2 ivec2
#define rshort3 ivec3
#define short2 ivec2

#define float4 vec4

uniform sampler2DArray srcTex;

out float4 fragColour;
in vec4 gl_FragCoord;

void main()
{
	rshort2 iFragCoord = rshort2( gl_FragCoord.xy );

	float4 c = texelFetch( srcTex, rshort3( iFragCoord.xy, 0 ), 0 );

	if( c.a == 0 )
	{
		c = c.a > 0 ? c : texelFetch( srcTex, rshort3( iFragCoord.xy + short2(  1,  0 ), 0 ), 0 );
		c = c.a > 0 ? c : texelFetch( srcTex, rshort3( iFragCoord.xy + short2(  0,  1 ), 0 ), 0 );
		c = c.a > 0 ? c : texelFetch( srcTex, rshort3( iFragCoord.xy + short2( -1,  0 ), 0 ), 0 );
		c = c.a > 0 ? c : texelFetch( srcTex, rshort3( iFragCoord.xy + short2(  0, -1 ), 0 ), 0 );

		c = c.a > 0 ? c : texelFetch( srcTex, rshort3( iFragCoord.xy + short2(  1,  1 ), 0 ), 0 );
		c = c.a > 0 ? c : texelFetch( srcTex, rshort3( iFragCoord.xy + short2( -1,  1 ), 0 ), 0 );
		c = c.a > 0 ? c : texelFetch( srcTex, rshort3( iFragCoord.xy + short2(  1, -1 ), 0 ), 0 );
		c = c.a > 0 ? c : texelFetch( srcTex, rshort3( iFragCoord.xy + short2(  1, -1 ), 0 ), 0 );
	}

	fragColour.xyzw = c.xyzw;
}
