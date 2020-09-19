#version 450 core

#include "SMAA_Vulkan.glsl"
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1

layout( ogre_s0 ) uniform sampler PointSampler;
layout( ogre_s1 ) uniform sampler LinearSampler;

#include "SMAA.hlsl"

layout( location = 0 )
in block
{
	vec2 uv0;
	vec4 offset[3];
} inPs;

layout( ogre_t0 ) uniform texture2D rt_input;  //Must not be sRGB
#if SMAA_PREDICATION
	layout( ogre_t1 ) uniform texture2D depthTex;
#endif

layout( location = 0 )
out vec2 fragColour;

void main()
{
#if !SMAA_EDGE_DETECTION_MODE || SMAA_EDGE_DETECTION_MODE == 2
	#if SMAA_PREDICATION
		fragColour = SMAAColorEdgeDetectionPS( inPs.uv0, inPs.offset, rt_input, depthTex );
	#else
		fragColour = SMAAColorEdgeDetectionPS( inPs.uv0, inPs.offset, rt_input );
	#endif
#else
	#if SMAA_PREDICATION
		fragColour = SMAALumaEdgeDetectionPS( inPs.uv0, inPs.offset, rt_input, depthTex );
	#else
		fragColour = SMAALumaEdgeDetectionPS( inPs.uv0, inPs.offset, rt_input );
	#endif
#endif
}
