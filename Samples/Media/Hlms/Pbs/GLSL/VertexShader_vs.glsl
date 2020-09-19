@insertpiece( SetCrossPlatformSettings )
@insertpiece( SetCompatibilityLayer )

out gl_PerVertex
{
	vec4 gl_Position;
@property( hlms_global_clip_planes )
	float gl_ClipDistance[@value(hlms_global_clip_planes)];
@end
};

layout(std140) uniform;

@insertpiece( DefaultHeaderVS )
@insertpiece( custom_vs_uniformDeclaration )

vulkan_layout( OGRE_POSITION ) in vec4 vertex;

@property( hlms_normal )vulkan_layout( OGRE_NORMAL ) in vec3 normal;@end
@property( hlms_qtangent )vulkan_layout( OGRE_NORMAL ) in vec4 qtangent;@end

@property( normal_map && !hlms_qtangent )
	vulkan_layout( OGRE_TANGENT ) in vec3 tangent;
	@property( hlms_binormal )vulkan_layout( OGRE_BIRNORMAL ) in vec3 binormal;@end
@end

@property( hlms_skeleton )
	vulkan_layout( OGRE_BLENDINDICES )in uvec4 blendIndices;
	vulkan_layout( OGRE_BLENDWEIGHT )in vec4 blendWeights;
@end

@foreach( hlms_uv_count, n )
	vulkan_layout( OGRE_TEXCOORD@n ) in vec@value( hlms_uv_count@n ) uv@n;@end

@property( GL_ARB_base_instance )
	vulkan_layout( OGRE_DRAWID ) in uint drawId;
@end

@insertpiece( custom_vs_attributes )

@property( !hlms_shadowcaster || !hlms_shadow_uses_depth_texture || alpha_test || exponential_shadow_maps )
	vulkan_layout( location = 0 ) out block
	{
		@insertpiece( VStoPS_block )
	} outVs;
@end

// START UNIFORM GL DECLARATION
ReadOnlyBufferF( 0, float4, worldMatBuf );

@property( !GL_ARB_base_instance )uniform uint baseInstance;@end
@property( hlms_pose )
	vulkan_layout( ogre_T@value(poseBuf) ) uniform samplerBuffer poseBuf;
@end
// END UNIFORM GL DECLARATION

void main()
{
@property( !GL_ARB_base_instance )
    uint drawId = baseInstance + uint( gl_InstanceID );
@end
    @insertpiece( custom_vs_preExecution )
	@insertpiece( DefaultBodyVS )
	@insertpiece( custom_vs_posExecution )
}
