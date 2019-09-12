#version 330

in vec2 vertex;
in vec3 normal;

uniform vec2 rsDepthRange;
uniform mat4 worldViewProj;

out gl_PerVertex
{
	vec4 gl_Position;
};

out block
{
	vec3 cameraDir;
} outVs;

void main()
{
	gl_Position.xy = (worldViewProj * vec4( vertex.xy, 0, 1.0f )).xy;
	gl_Position.z = rsDepthRange.y;
	gl_Position.w = 1.0f;
	outVs.cameraDir.xyz	= normal.xyz;
}
