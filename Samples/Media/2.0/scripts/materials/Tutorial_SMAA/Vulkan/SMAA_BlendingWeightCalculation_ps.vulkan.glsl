#version 450 core

#include "SMAA_Vulkan.glsl"
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1

layout( ogre_s0 ) uniform sampler LinearSampler;

// !!!WARNING!!! These are NOT true, they have to be compiled because otherwise it won't compile
layout( ogre_s1 ) uniform sampler PointSampler;

#include "SMAA.hlsl"

layout( location = 0 )
in block
{
	vec2 uv0;
	vec2 pixcoord0;
	vec4 offset[3];
} inPs;

layout( ogre_t0 ) uniform texture2D edgeTex;
layout( ogre_t1 ) uniform texture2D areaTex;
layout( ogre_t2 ) uniform texture2D searchTex;

layout( location = 0 )
out vec4 fragColour;

void main()
{
	fragColour = SMAABlendingWeightCalculationPS( inPs.uv0, inPs.pixcoord0, inPs.offset,
												  edgeTex, areaTex, searchTex,
												  vec4( 0, 0, 0, 0 ) );
}
