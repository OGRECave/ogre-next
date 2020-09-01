@insertpiece( SetCrossPlatformSettings )
@insertpiece( DeclUavCrossPlatform )

#define ushort2 uvec2

@insertpiece( PreBindingsHeaderCS )

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform uint probeIdx;
	uniform float2 projectionParams;
	uniform float3 numProbes;
vulkan( }; )

#define p_probeIdx			probeIdx
#define p_projectionParams	projectionParams
#define p_numProbes			numProbes

vulkan( layout( ogre_s0 ) uniform sampler bilinearSampler );

vulkan_layout( ogre_t0 ) uniform textureCube colourCubemap;
vulkan_layout( ogre_t1 ) uniform textureCube depthCubemap;

layout( vulkan( ogre_u0 ) vk_comma @insertpiece(uav0_pf_type) )
uniform restrict writeonly image2D irradianceField;
layout( vulkan( ogre_u1 ) vk_comma @insertpiece(uav1_pf_type) )
uniform restrict writeonly image2D irradianceFieldDepth;

layout( local_size_x = @value( threads_per_group_x ),
		local_size_y = @value( threads_per_group_y ),
		local_size_z = @value( threads_per_group_z ) ) in;

@insertpiece( HeaderCS )

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

void main()
{
	@insertpiece( BodyCS )
}
