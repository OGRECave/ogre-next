#version ogre_glsl_ver_330

//Short used for read operations. It's an int in GLSL & HLSL. An ushort in Metal
#define rshort2 ivec2
#define rshort3 ivec3
#define short2 ivec2

#define float4 vec4

vulkan_layout( ogre_t0 ) uniform texture2DArray srcTex;

vulkan_layout( location = 0 )
out float4 fragColour;

in vec4 gl_FragCoord;

void addSample( inout float4 accumVal, float4 newSample, inout float counter )
{
	if( newSample.a > 0 )
	{
		accumVal += newSample;
		counter += 1.0;
	}
}

void main()
{
	rshort2 iFragCoord = rshort2( gl_FragCoord.x * 2.0, gl_FragCoord.y * 2.0 );

	float4 accumVal = float4( 0, 0, 0, 0 );
	float counter = 0;
	float4 newSample;

	newSample = texelFetch( srcTex, rshort3( iFragCoord.xy, 0 ), 0 );
	addSample( accumVal, newSample, counter );

	newSample = texelFetch( srcTex, rshort3( iFragCoord.xy + rshort2( 1, 0 ), 0 ), 0 );
	addSample( accumVal, newSample, counter );

	newSample = texelFetch( srcTex, rshort3( iFragCoord.xy + rshort2( 0, 1 ), 0 ), 0 );
	addSample( accumVal, newSample, counter );

	newSample = texelFetch( srcTex, rshort3( iFragCoord.xy + rshort2( 1, 1 ), 0 ), 0 );
	addSample( accumVal, newSample, counter );

	if( counter > 0 )
		accumVal.xyzw /= counter;

	fragColour.xyzw = accumVal.xyzw;
}
