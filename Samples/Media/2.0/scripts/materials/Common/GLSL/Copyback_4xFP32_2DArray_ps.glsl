#version 330

uniform sampler2DArray tex;
uniform float sliceIdx;

in block
{
	vec2 uv0;
} inPs;

out vec4 fragColour;

void main()
{
	fragColour = texture( tex, vec3( inPs.uv0, sliceIdx ) );
}
