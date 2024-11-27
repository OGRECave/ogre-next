@insertpiece( SetCrossPlatformSettings )
@insertpiece( SetCompatibilityLayer )

layout(std140) uniform;
#define FRAG_COLOR		0

@property( hlms_vpos )
in vec4 gl_FragCoord;
@end

@property( !hlms_shadowcaster )
layout(location = FRAG_COLOR, index = 0) out midf4 outColour;
@end @property( hlms_shadowcaster )
layout(location = FRAG_COLOR, index = 0) out midf outColour;
@end

// START UNIFORM DECLARATION
@insertpiece( custom_ps_uniformDeclaration )
// END UNIFORM DECLARATION
@property( !hlms_shadowcaster || !hlms_shadow_uses_depth_texture || ( ( alpha_test || hlms_alpha_hash ) && hlms_uv_count ) || exponential_shadow_maps || hlms_shadowcaster_point )
	vulkan_layout( location = 0 ) in block
	{
		@insertpiece( VStoPS_block )
	} inPs;
@end

@property( !hlms_shadowcaster || alpha_test || hlms_alpha_hash )
	@property( syntax != glslvk )
		@foreach( num_textures, n )
			@property( is_texture@n_array )
				midf_tex uniform sampler2DArray textureMapsArray@n;
			@else
				midf_tex uniform sampler2D textureMaps@n;
			@end
		@end
	@else
		@foreach( num_textures, n )
			@property( is_texture@n_array )
				layout( ogre_t@value(textureMapsArray@n) ) midf_tex uniform texture2DArray textureMapsArray@n;
			@else
				layout( ogre_t@value(textureMaps@n) ) midf_tex uniform texture2D textureMaps@n;
			@end
		@end
	@end
@end

@pset( currSampler, samplerStateStart )

@property( syntax == glslvk )
	@foreach( num_samplers, n )
		layout( ogre_s@counter(currSampler) ) uniform sampler samplerState@n;@end
@end

@insertpiece( DeclBlueNoiseTexture )

@insertpiece( DefaultHeaderPS )

@property( hlms_shadowcaster )
	@insertpiece( DeclShadowCasterMacros )
@end

void main()
{
	@insertpiece( custom_ps_preExecution )
	@property( !hlms_shadowcaster || alpha_test || hlms_alpha_hash )
		@insertpiece( DefaultBodyPS )
	@end
	@property( hlms_shadowcaster )
		@insertpiece( DoShadowCastPS )
	@end
	@insertpiece( custom_ps_posExecution )
}
