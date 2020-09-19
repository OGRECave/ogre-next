#version ogre_glsl_ver_330

#define float2 vec2
#define float3 vec3

#define PI 3.14159265359f

vulkan_layout( ogre_t0 ) uniform texture2DArray skyEquirectangular;
vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform float sliceIdx;
vulkan( }; )

vulkan_layout( location = 0 )
in block
{
    vec3 cameraDir;
} inPs;

vulkan_layout( location = 0 )
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
	fragColour = texture( vkSampler2DArray( skyEquirectangular, samplerState ), vec3( uv.xy, 0 ) );
}
