#version ogre_glsl_ver_330

#define float2 vec2
#define float3 vec3
#define float4 vec4

#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4

#define float4x4 mat4

#define mul( x, y ) ((x) * (y))
#define INLINE

#ifdef VULKAN
    #define OGRE_SampleLevel( tex, sampler, uv, lod ) textureLod( sampler2D( tex, sampler ), uv, lod )
#else
    #define OGRE_SampleLevel( tex, sampler, uv, lod ) textureLod( tex, uv, lod )
#endif

vulkan_layout( location = 0 )
in block
{
	flat uint probeIdx;
	vec3 normal;
} inPs;

layout(location = 0, index = 0) out vec4 outColour;

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform float4 allParams;
    uniform float2 rangeMult;
vulkan( }; )

#define p_borderedRes		allParams.x
#define p_fullWidth			allParams.y
#define p_invFullResolution	allParams.zw
#define p_rangeMult			rangeMult

vulkan_layout( ogre_t0 ) uniform texture2D ifdTex;
vulkan( layout( ogre_s0 ) uniform sampler ifdSampler );

#define HEADER
#include "IfdVisualizer_ps.any"
#undef HEADER

void main()
{
	#include "IfdVisualizer_ps.any"
}
