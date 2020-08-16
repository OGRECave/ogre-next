#version ogre_glsl_ver_330

#extension GL_ARB_shader_viewport_layer_array : require

#define float2 vec2
#define float3 vec3
#define float4 vec4

#define float4x4 mat4
#define mul( x, y ) ((x) * (y))

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform float4x4 projectionMatrix;
	uniform float2 rsDepthRange;
vulkan( }; )

vulkan_layout( OGRE_POSITION ) in vec4 vertex;

vulkan_layout( location = 0 )
out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	gl_Position.xy = mul( projectionMatrix, float4( vertex.xy, 0.0f, 1.0f ) ).xy;
	gl_Position.z = rsDepthRange.x;
	gl_Position.w = 1.0f;
	gl_ViewportIndex = int( vertex.z );
}
