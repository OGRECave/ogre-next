#version 330

#define float2 vec2
#define float3 vec3
#define float4 vec4

#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4

#define float4x4 mat4

#define mul( x, y ) ((x) * (y))
#define INLINE

#define OGRE_SampleLevel( tex, sampler, uv, lod ) textureLod( tex, uv, lod )

in block
{
	flat uint probeIdx;
	vec3 normal;
} inPs;

layout(location = 0, index = 0) out vec4 outColour;

uniform float4 allParams;
uniform float2 rangeMult;

#define p_resolution		allParams.x
#define p_fullWidth			allParams.y
#define p_invFullResolution	allParams.zw
#define p_rangeMult			rangeMult

uniform sampler2D ifdTex;

#define HEADER
#include "IfdVisualizer_ps.any"
#undef HEADER

void main()
{
	#include "IfdVisualizer_ps.any"
}
