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
    #define inVs_vertexId gl_VertexIndex
#else
    #define inVs_vertexId gl_VertexID
#endif
#define outVs_Position gl_Position

#define PARAMS_ARG_DECL
#define PARAMS_ARG

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform float4x4 worldViewProjMatrix;
    uniform uint vertexBase;
    uniform uint3 bandMaskPower;
    uniform float2 sectionsBandArc;

    uniform uint3 numProbes;
    uniform float3 aspectRatioFixer;
vulkan( }; )

#define p_worldViewProjMatrix worldViewProjMatrix
#define p_vertexBase vertexBase

#define p_bandMask			bandMaskPower.x
#define p_bandPower			bandMaskPower.y
#define p_totalPoints		bandMaskPower.z
#define p_sectionsInBand	sectionsBandArc.x
#define p_sectionArc		sectionsBandArc.y

#define p_numProbes			numProbes
#define p_aspectRatioFixer	aspectRatioFixer

out gl_PerVertex
{
	vec4 gl_Position;
};

vulkan_layout( location = 0 )
out block
{
	flat uint probeIdx;
	float3 normal;
} outVs;

#define HEADER
	#include "IfdVisualizer_vs.any"
#undef HEADER

void main()
{
	#include "IfdVisualizer_vs.any"
}
