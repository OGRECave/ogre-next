@insertpiece( SetCrossPlatformSettings )
@property( GL3+ < 430 )
	@property( hlms_tex_gather )#extension GL_ARB_texture_gather: require@end
@end
@insertpiece( SetCompatibilityLayer )
@insertpiece( DeclareUvModifierMacros )

layout(std140) uniform;
#define FRAG_COLOR		0

@property( !hlms_render_depth_only )
	@property( !hlms_shadowcaster )
		@property( !hlms_prepass )
			layout(location = FRAG_COLOR, index = 0) out vec4 outColour;
		@end @property( hlms_prepass )
			#define outPs_normals outNormals
			#define outPs_shadowRoughness outShadowRoughness
			layout(location = 0) out vec4 outNormals;
			layout(location = 1) out vec2 outShadowRoughness;
		@end
	@end @property( hlms_shadowcaster )
	layout(location = FRAG_COLOR, index = 0) out float outColour;
	@end
@end

@property( hlms_use_prepass )
	@property( !hlms_use_prepass_msaa )
		uniform sampler2D gBuf_normals;
		uniform sampler2D gBuf_shadowRoughness;
	@else
		uniform sampler2DMS gBuf_normals;
		uniform sampler2DMS gBuf_shadowRoughness;
		uniform sampler2DMS gBuf_depthTexture;
	@end

	@property( hlms_use_ssr )
		uniform sampler2D ssrTexture;
	@end
@end

@property( hlms_ss_refractions_available )
	@property( !hlms_use_prepass || !hlms_use_prepass_msaa )
		@property( !hlms_use_prepass_msaa )
			uniform sampler2D gBuf_depthTexture;
			#define depthTextureNoMsaa gBuf_depthTexture
		@else
			uniform sampler2D depthTextureNoMsaa;
		@end
	@end
	uniform sampler2D refractionMap;
@end

@insertpiece( DeclPlanarReflTextures )
@insertpiece( DeclAreaApproxTextures )

@property( hlms_vpos )
in vec4 gl_FragCoord;
@end

// START UNIFORM DECLARATION
@property( !hlms_shadowcaster || alpha_test )
	@property( !hlms_shadowcaster )
		@insertpiece( PassStructDecl )
	@end
	@insertpiece( MaterialStructDecl )
	@insertpiece( InstanceStructDecl )
	@insertpiece( PccManualProbeDecl )
@end
@insertpiece( custom_ps_uniformDeclaration )
// END UNIFORM DECLARATION

@insertpiece( DefaultHeaderPS )

@property( !hlms_shadowcaster || !hlms_shadow_uses_depth_texture || alpha_test || exponential_shadow_maps )
in block
{
@insertpiece( VStoPS_block )
} inPs;
@end

@property( !hlms_shadowcaster )

@property( hlms_forwardplus )
/*layout(binding = 1) */uniform usamplerBuffer f3dGrid;
/*layout(binding = 2) */uniform samplerBuffer f3dLightList;
@end
@property( irradiance_volumes )
	uniform sampler3D irradianceVolume;
@end

@foreach( num_textures, n )
	uniform sampler2DArray textureMaps@n;@end

@property( !hlms_enable_cubemaps_auto )
	@property( use_envprobe_map )uniform samplerCube		texEnvProbeMap;@end
@else
	@property( !hlms_cubemaps_use_dpm )
		@property( use_envprobe_map )uniform samplerCubeArray	texEnvProbeMap;@end
	@else
		@property( use_envprobe_map )uniform sampler2DArray	texEnvProbeMap;@end
		@insertpiece( DeclDualParaboloidFunc )
	@end
@end

@property( use_parallax_correct_cubemaps )
	@insertpiece( DeclParallaxLocalCorrect )
@end

@insertpiece( DeclDecalsSamplers )

@insertpiece( DeclShadowMapMacros )
@insertpiece( DeclShadowSamplers )
@insertpiece( DeclShadowSamplingFuncs )

@insertpiece( DeclAreaLtcTextures )
@insertpiece( DeclAreaLtcLightFuncs )

@insertpiece( DeclVctTextures )
@insertpiece( DeclIrradianceFieldTextures )

@insertpiece( custom_ps_functions )

void main()
{
    @insertpiece( custom_ps_preExecution )
	@insertpiece( DefaultBodyPS )
	@insertpiece( custom_ps_posExecution )
}
@else ///!hlms_shadowcaster

@insertpiece( DeclShadowCasterMacros )

@property( alpha_test )
	@foreach( num_textures, n )
		uniform sampler2DArray textureMaps@n;@end
@end

@property( hlms_shadowcaster_point || exponential_shadow_maps )
	@insertpiece( PassStructDecl )
@end

void main()
{
	@insertpiece( custom_ps_preExecution )
	@insertpiece( DefaultBodyPS )
	@insertpiece( custom_ps_posExecution )
}
@end ///hlms_shadowcaster
