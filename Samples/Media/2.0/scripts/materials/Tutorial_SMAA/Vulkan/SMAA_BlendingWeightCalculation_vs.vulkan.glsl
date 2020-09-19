#version 450 core

#define VERTEX_SHADER 1
#include "SMAA_Vulkan.glsl"
#define SMAA_INCLUDE_VS 1
#define SMAA_INCLUDE_PS 0

#include "SMAA.hlsl"

layout( OGRE_POSITION ) in vec3 vertex;
layout( OGRE_TEXCOORD0 ) in vec2 uv0;

out gl_PerVertex
{
	vec4 gl_Position;
};

layout( location = 0 )
out block
{
	vec2 uv0;
	vec2 pixcoord0;
	vec4 offset[3];
} outVs;

void main()
{
	gl_Position = worldViewProj * vec4( vertex, 1.0 );
	outVs.uv0 = uv0.xy;
	SMAABlendingWeightCalculationVS( uv0.xy, outVs.pixcoord0, outVs.offset );
}
