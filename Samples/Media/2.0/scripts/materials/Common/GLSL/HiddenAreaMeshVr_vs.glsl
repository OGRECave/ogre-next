#version 330

#extension GL_ARB_shader_viewport_layer_array : require

#define float2 vec2
#define float3 vec3
#define float4 vec4

#define float4x4 mat4
#define mul( x, y ) ((x) * (y))

uniform float4x4 projectionMatrix;

in vec4 vertex;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	gl_Position.xy = mul( projectionMatrix, float4( vertex.xy, 0.0f, 1.0f ) ).xy;
	gl_Position.z = 0.0f;
	gl_Position.w = vertex.w;
	gl_ViewportIndex = int( vertex.z );
}
