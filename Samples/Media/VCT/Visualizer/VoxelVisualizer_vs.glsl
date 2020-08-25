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

#define OGRE_Load3D( tex, iuv, lod ) texelFetch( tex, ivec3( iuv ), lod )

#ifdef VULKAN
    #define inVs_vertexId gl_VertexIndex
#else
    #define inVs_vertexId gl_VertexID
#endif
#define outVs_Position gl_Position

vulkan_layout( ogre_t0 ) uniform texture3D voxelTex;
#ifdef SEPARATE_OPACITY
    vulkan_layout( ogre_t1 ) uniform texture3D otherTex;
#endif

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform float4x4 worldViewProjMatrix;
    uniform uint vertexBase;
    uniform uint3 voxelResolution;
vulkan( }; )

#define p_worldViewProjMatrix worldViewProjMatrix
#define p_vertexBase vertexBase
#define p_voxelResolution voxelResolution

out gl_PerVertex
{
	vec4 gl_Position;
};

vulkan_layout( location = 0 )
out block
{
	flat float4 voxelColour;
} outVs;

#define HEADER
	#include "VoxelVisualizer_vs.any"
#undef HEADER

void main()
{
	#include "VoxelVisualizer_vs.any"
}
