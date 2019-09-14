#version 330

#extension GL_ARB_shader_viewport_layer_array : require

in vec2 vertex;

uniform float ogreBaseVertex;
uniform vec2 rsDepthRange;
uniform mat4 worldViewProj;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	gl_Position.xy = (worldViewProj * vec4( vertex.xy, 0, 1.0f )).xy;
	gl_Position.z = rsDepthRange.x;
	gl_Position.w = 1.0f;
	gl_ViewportIndex = (gl_VertexID - ogreBaseVertex) >= (3 * 4) ? 1 : 0;
}
