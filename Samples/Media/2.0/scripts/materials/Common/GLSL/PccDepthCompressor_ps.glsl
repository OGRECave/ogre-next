#version 330

#define float2 vec2
#define float3 vec3
#define float4 vec4

#define float3x3 mat3

#define mul( x, y ) ((x) * (y))
#define INLINE

#define OGRE_Sample( tex, sampler, uv ) texture( tex, uv )

uniform sampler2D depthTexture;

uniform float2 projectionParams;
uniform float3 probeShapeHalfSize;
uniform float3 cameraPosLS;

uniform float3x3 viewSpaceToProbeLocalSpace;

#define p_projectionParams				projectionParams
#define p_probeShapeHalfSize			probeShapeHalfSize
#define p_cameraPosLS					cameraPosLS
#define p_viewSpaceToProbeLocalSpace	viewSpaceToProbeLocalSpace

in block
{
	vec2 uv0;
	vec3 cameraDir;
} inPs;

#define HEADER
	#include "PccDepthCompressor_ps.any"
#undef HEADER

out float4 fragColour;

void main()
{
	#include "PccDepthCompressor_ps.any"
	//RGB writes should be masked off
	fragColour = float4( 0, 0, 0, alpha );
}
