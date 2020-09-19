@insertpiece( SetCrossPlatformSettings )
@insertpiece( DeclUavCrossPlatform )

#define ushort2 uvec2

@insertpiece( PreBindingsHeaderCS )

vulkan_layout( ogre_T0 ) uniform samplerBuffer integrationTapsBuffer;

layout( vulkan( ogre_u0 ) vk_comma @insertpiece(uav0_pf_type) )
uniform restrict image2D irradianceField;

@property( integrate_depth )
	shared float2 g_sharedValues[@value( threads_per_group_x ) * @value( threads_per_group_y )];
@else
	shared float4 g_sharedValues[@value( threads_per_group_x ) * @value( threads_per_group_y )];
@end

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
