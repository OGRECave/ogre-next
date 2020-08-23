@insertpiece( SetCrossPlatformSettings )
@insertpiece( DeclUavCrossPlatform )

#define ushort2 uvec2

@insertpiece( PreBindingsHeaderCS )

vulkan_layout( ogre_T0 ) uniform samplerBuffer directionsBuffer;

vulkan( layout( ogre_s1 ) uniform sampler probeSampler );

vulkan_layout( ogre_t1 ) uniform texture3D vctProbe;

@property( vct_anisotropic )
	vulkan_layout( ogre_t2 ) uniform texture3D vctProbeX;
	vulkan_layout( ogre_t3 ) uniform texture3D vctProbeY;
	vulkan_layout( ogre_t4 ) uniform texture3D vctProbeZ;
@end

layout( vulkan( ogre_u0 ) vk_comma @insertpiece(uav0_pf_type) )
uniform restrict writeonly image2D irradianceField;

layout( vulkan( ogre_u1 ) vk_comma @insertpiece(uav1_pf_type) )
uniform restrict writeonly image2D irradianceFieldDepth;

shared float4 g_diffuseDepth[@value( threads_per_group_x ) * @value( threads_per_group_y ) * @value( threads_per_group_z )];

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
