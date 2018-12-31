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

@insertpiece( DefaultTerraHeaderVS )
@insertpiece( custom_vs_uniformDeclaration )

@property( GL_ARB_base_instance )
	in uint drawId;
@end

@insertpiece( custom_vs_attributes )

out block
{
	@insertpiece( Terra_VStoPS_block )
} outVs;

// START UNIFORM GL DECLARATION
/*layout(binding = 0) */uniform sampler2D heightMap;
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
