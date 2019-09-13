#version 330

#define float2 vec2
#define float3 vec3

#define PI 3.14159265359f

uniform sampler2DArray skyEquirectangular;
uniform float sliceIdx;

in block
{
    vec3 cameraDir;
} inPs;

out vec4 fragColour;

float atan2(in float y, in float x)
{
	return x == 0.0 ? sign(y)*PI/2 : atan(y, x);
}

void main()
{
	float3 cameraDir = normalize( inPs.cameraDir );
	float2 longlat;
	longlat.x = atan2( cameraDir.x, -cameraDir.z ) + PI;
	longlat.y = acos( cameraDir.y );
	float2 uv = longlat / float2( 2.0f * PI, PI );
	fragColour = texture( skyEquirectangular, vec3( uv.xy, 0 ) );
}
