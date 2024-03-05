@insertpiece( SetCrossPlatformSettings )
@insertpiece( SetCompatibilityLayer )

out gl_PerVertex
{
	vec4 gl_Position;
@property( hlms_pso_clip_distances && !hlms_emulate_clip_distances )
	float gl_ClipDistance[@value(hlms_pso_clip_distances)];
@end
};

layout(std140) uniform;

@insertpiece( DefaultTerraHeaderVS )
@insertpiece( custom_vs_uniformDeclaration )

@property( GL_ARB_base_instance )
	vulkan_layout( OGRE_DRAWID ) in uint drawId;
@end

@insertpiece( custom_vs_attributes )

vulkan_layout( location = 0 )
out block
{
	@insertpiece( Terra_VStoPS_block )
} outVs;

// START UNIFORM GL DECLARATION
@property( !terra_use_uint )
	vulkan_layout( ogre_t@value(heightMap) ) uniform texture2D heightMap;
@else
	vulkan_layout( ogre_t@value(heightMap) ) uniform utexture2D heightMap;
@end
@property( !GL_ARB_base_instance )uniform uint baseInstance;@end
// END UNIFORM GL DECLARATION

void main()
{
@property( !GL_ARB_base_instance )
	uint drawId = baseInstance + uint( gl_InstanceID );
@end
	@insertpiece( custom_vs_preExecution )
	@insertpiece( DefaultTerraBodyVS )
	@insertpiece( custom_vs_posExecution )
}
