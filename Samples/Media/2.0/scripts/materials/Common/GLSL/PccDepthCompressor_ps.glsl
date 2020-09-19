#version ogre_glsl_ver_330

#define float2 vec2
#define float3 vec3
#define float4 vec4

#define float3x3 mat3

#define mul( x, y ) ((x) * (y))
#define INLINE

#ifdef VULKAN
	#define OGRE_Sample( tex, sampler, uv ) texture( sampler2D( tex, sampler ), uv )
#else
	#define OGRE_Sample( tex, sampler, uv ) texture( tex, uv )
#endif

vulkan_layout( ogre_t0 ) uniform texture2D depthTexture;
vulkan( layout( ogre_s0 ) uniform sampler pointSampler );

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform float2 projectionParams;
	uniform float3 probeShapeHalfSize;
	uniform float3 cameraPosLS;

	uniform float3x3 viewSpaceToProbeLocalSpace;
vulkan( }; )

#define p_projectionParams				projectionParams
#define p_probeShapeHalfSize			probeShapeHalfSize
#define p_cameraPosLS					cameraPosLS
#define p_viewSpaceToProbeLocalSpace	viewSpaceToProbeLocalSpace

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
	vec3 cameraDir;
} inPs;

#define HEADER
	#include "PccDepthCompressor_ps.any"
#undef HEADER

layout( location = 0 ) out float4 fragColour;

void main()
{
	#include "PccDepthCompressor_ps.any"
	//RGB writes should be masked off
	fragColour = float4( 0, 0, 0, alpha );
}
