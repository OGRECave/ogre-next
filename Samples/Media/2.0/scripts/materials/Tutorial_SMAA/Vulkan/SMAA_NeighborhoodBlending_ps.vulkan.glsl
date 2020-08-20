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
	vec4 offset;
} inPs;

layout( ogre_t0 ) uniform texture2D rt_input; //Can be sRGB
layout( ogre_t1 ) uniform texture2D blendTex;
#if SMAA_REPROJECTION
	layout( ogre_t2 ) uniform texture2D velocityTex;
#endif

layout( location = 0 )
out vec4 fragColour;

void main()
{
#if SMAA_REPROJECTION
	fragColour = SMAANeighborhoodBlendingPS( inPs.uv0, inPs.offset, rt_input, blendTex, velocityTex );
#else
	fragColour = SMAANeighborhoodBlendingPS( inPs.uv0, inPs.offset, rt_input, blendTex );
#endif
}
