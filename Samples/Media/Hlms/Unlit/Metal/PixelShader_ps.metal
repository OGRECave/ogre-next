@insertpiece( SetCrossPlatformSettings )

struct PS_INPUT
{
	@insertpiece( VStoPS_block )
};

@insertpiece( DeclOutputType )

@insertpiece( DefaultHeaderPS )

@property( hlms_shadowcaster )
	@insertpiece( DeclShadowCasterMacros )
@end

fragment @insertpiece( output_type ) main_metal
(
	PS_INPUT inPs [[stage_in]]
	// START UNIFORM DECLARATION
	@property( has_planar_reflections || hlms_shadowcaster_point )
		@insertpiece( PassDecl )
	@end
	@property( !hlms_shadowcaster || alpha_test )
		@insertpiece( MaterialDecl )
	@end
	@insertpiece( custom_ps_uniformDeclaration )
	// END UNIFORM DECLARATION
	@property( hlms_vpos ), float4 gl_FragCoord [[position]]@end

	@property( !hlms_shadowcaster || alpha_test )
		@foreach( num_textures, n )
			@property( is_texture@n_array )
				, texture2d_array<float> textureMapsArray@n [[texture(@value(textureMapsArray@n))]]
			@else
				, texture2d<float> textureMaps@n [[texture(@value(textureMaps@n))]]
			@end
		@end
	@end
	@foreach( num_samplers, n )
		, sampler samplerState@n [[sampler(@counter(samplerStateStart))]]@end
)
{
	PS_OUTPUT outPs;

	@insertpiece( custom_ps_preExecution )
	@property( !hlms_shadowcaster || alpha_test )
		@insertpiece( DefaultBodyPS )
	@end
	@property( hlms_shadowcaster )
		@insertpiece( DoShadowCastPS )
	@end
	@insertpiece( custom_ps_posExecution )

@property( !hlms_render_depth_only )
	return outPs;
@end
}
