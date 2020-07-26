@insertpiece( SetCrossPlatformSettings )
@insertpiece( SetCompatibilityLayer )

out gl_PerVertex
{
	vec4 gl_Position;
@property( hlms_pso_clip_distances )
	float gl_ClipDistance[@value(hlms_pso_clip_distances)];
@end
};

layout(std140) uniform;

@insertpiece( DefaultHeaderVS )
@insertpiece( custom_vs_uniformDeclaration )

in vec4 vertex;
@property( hlms_colour )in vec4 colour;@end

@foreach( hlms_uv_count, n )
	in vec@value( hlms_uv_count@n ) uv@n;
@end

@property( GL_ARB_base_instance )
	in uint drawId;
@end

@insertpiece( custom_vs_attributes )

@property( !hlms_shadowcaster || !hlms_shadow_uses_depth_texture || exponential_shadow_maps )
	out block
	{
		@insertpiece( VStoPS_block )
	} outVs;
@end

// START UNIFORM GL DECLARATION
/*layout(binding = 0) */uniform samplerBuffer worldMatBuf;
@property( texture_matrix )/*layout(binding = 1) */uniform samplerBuffer animationMatrixBuf;@end
@property( !GL_ARB_base_instance )uniform uint baseInstance;@end
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
