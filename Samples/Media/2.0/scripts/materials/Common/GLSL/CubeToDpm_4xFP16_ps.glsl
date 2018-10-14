#version 330

uniform float lodLevel;
uniform samplerCube cubeTexture;

in block
{
	vec2 uv0;
} inPs;

out vec4 fragColour;

void main()
{
	vec3 cubeDir;
	cubeDir.x = mod( inPs.uv0.x, 0.5 ) * 4.0 - 1.0;
	cubeDir.y = inPs.uv0.y * 2.0 - 1.0;
	cubeDir.z = 0.5 - 0.5 * (cubeDir.x * cubeDir.x + cubeDir.y * cubeDir.y);
	cubeDir.z = inPs.uv0.x < 0.5 ? cubeDir.z : -cubeDir.z;

	fragColour.xyzw = textureLod( cubeTexture, cubeDir.xyz, lodLevel ).xyzw;
}
