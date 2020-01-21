
//#include "SyntaxHighlightingMisc.h"

@insertpiece( SetCrossPlatformSettings )
@insertpiece( DeclareUvModifierMacros )

@insertpiece( DefaultTerraHeaderPS )

// START UNIFORM STRUCT DECLARATION
@insertpiece( PassStructDecl )
@insertpiece( TerraMaterialStructDecl )
@insertpiece( TerraInstanceStructDecl )
@insertpiece( custom_ps_uniformDeclaration )
// END UNIFORM STRUCT DECLARATION
struct PS_INPUT
{
	@insertpiece( Terra_VStoPS_block )
};

@padd( roughness_map0_sampler,	samplerStateStart )
@padd( roughness_map1_sampler,	samplerStateStart )
@padd( roughness_map2_sampler,	samplerStateStart )
@padd( roughness_map3_sampler,	samplerStateStart )

@padd( metalness_map0_sampler,	samplerStateStart )
@padd( metalness_map1_sampler,	samplerStateStart )
@padd( metalness_map2_sampler,	samplerStateStart )
@padd( metalness_map3_sampler,	samplerStateStart )

@property( !hlms_render_depth_only )
	@property( hlms_gen_normals_gbuffer )
		#define outPs_normals outPs.normals
	@end
	@property( hlms_prepass )
		#define outPs_shadowRoughness outPs.shadowRoughness
	@end
@end

@property( use_parallax_correct_cubemaps )
	@insertpiece( DeclParallaxLocalCorrect )
@end


@insertpiece( DeclShadowMapMacros )
@insertpiece( DeclShadowSamplingFuncs )

@insertpiece( DeclAreaLtcLightFuncs )

@property( hlms_enable_cubemaps_auto && hlms_cubemaps_use_dpm )
	@insertpiece( DeclDualParaboloidFunc )
@end

constexpr sampler shadowSampler = sampler( coord::normalized,
										   address::clamp_to_edge,
										   filter::linear,
										@property( hlms_no_reverse_depth )
										   compare_func::less_equal );
										@else
											compare_func::greater_equal );
										@end

@insertpiece( DeclOutputType )

@insertpiece( custom_ps_functions )

fragment @insertpiece( output_type ) main_metal
(
	PS_INPUT inPs [[stage_in]]
	@property( hlms_vpos )
		, float4 gl_FragCoord [[position]]
	@end
	@property( two_sided_lighting )
		, bool gl_FrontFacing [[front_facing]]
	@end
	@property( hlms_use_prepass_msaa && hlms_use_prepass )
		, uint gl_SampleMask [[sample_mask]]
	@end
	// START UNIFORM DECLARATION
	@property( !hlms_shadowcaster || alpha_test )
		@insertpiece( PassDecl )
		@insertpiece( TerraMaterialDecl )
		@insertpiece( PccManualProbeDecl )
	@end
	@insertpiece( custom_ps_uniformDeclaration )
	// END UNIFORM DECLARATION

	, texture2d<float> terrainNormals	[[texture(@value(terrainNormals))]]
	, texture2d<float> terrainShadows	[[texture(@value(terrainShadows))]]
	, sampler samplerStateTerra			[[sampler(@value(terrainNormals))]]

	@property( hlms_forwardplus )
		, device const ushort *f3dGrid [[buffer(TEX_SLOT_START+@value(f3dGrid))]]
		, device const float4 *f3dLightList [[buffer(TEX_SLOT_START+@value(f3dLightList))]]
	@end

	@property( hlms_use_prepass )
		@property( !hlms_use_prepass_msaa )
		, texture2d<float, access::read> gBuf_normals			[[texture(@value(gBuf_normals))]]
		, texture2d<float, access::read> gBuf_shadowRoughness	[[texture(@value(gBuf_shadowRoughness))]]
		@end @property( hlms_use_prepass_msaa )
		, texture2d_ms<float, access::read> gBuf_normals		[[texture(@value(gBuf_normals))]]
		, texture2d_ms<float, access::read> gBuf_shadowRoughness[[texture(@value(gBuf_shadowRoughness))]]
		@end

		@property( hlms_use_ssr )
		, texture2d<float, access::read> ssrTexture				[[texture(@value(ssrTexture))]]
		@end
	@end

	@insertpiece( DeclPlanarReflTextures )
	@insertpiece( DeclAreaApproxTextures )


	@property( irradiance_volumes )
		, texture3d<float>	irradianceVolume		[[texture(@value(irradianceVolume))]]
		, sampler			irradianceVolumeSampler	[[sampler(@value(irradianceVolume))]]
	@end

	@foreach( num_textures, n )
		, texture2d_array<float> textureMaps@n [[texture(@value(textureMaps@n))]]@end
	@property( use_envprobe_map )
		@property( !hlms_enable_cubemaps_auto )
			, texturecube<float>	texEnvProbeMap [[texture(@value(texEnvProbeMap))]]
		@end
		@property( hlms_enable_cubemaps_auto )
			@property( !hlms_cubemaps_use_dpm )
				, texturecube_array<float>	texEnvProbeMap [[texture(@value(texEnvProbeMap))]]
			@end
			@property( hlms_cubemaps_use_dpm )
				, texture2d_array<float>	texEnvProbeMap [[texture(@value(texEnvProbeMap))]]
			@end
		@end
		@property( envMapRegSampler < samplerStateStart )
			, sampler samplerState@value(envMapRegSampler) [[sampler(@value(envMapRegSampler))]]
		@end
	@end
	@foreach( num_samplers, n )
		, sampler samplerState@value(samplerStateStart) [[sampler(@counter(samplerStateStart))]]@end
	@insertpiece( DeclDecalsSamplers )
	@insertpiece( DeclShadowSamplers )
	@insertpiece( DeclAreaLtcTextures )
	@insertpiece( DeclVctTextures )
)
{
	PS_OUTPUT outPs;
	@insertpiece( custom_ps_preExecution )
	@insertpiece( DefaultTerraBodyPS )
	@insertpiece( custom_ps_posExecution )

@property( !hlms_render_depth_only )
	return outPs;
@end
}
