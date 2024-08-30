@insertpiece( SetCrossPlatformSettings )
@insertpiece( DeclareUvModifierMacros )

@insertpiece( DefaultTerraHeaderPS )


// START UNIFORM DECLARATION
@insertpiece( custom_ps_uniformDeclaration )
// END UNIFORM DECLARATION

@insertpiece( PccManualProbeDecl )

struct PS_INPUT
{
	@insertpiece( Terra_VStoPS_block )
};

@pset( currSampler, samplerStateStart )

@property( !hlms_shadowcaster )

@property( !hlms_render_depth_only )
	@property( hlms_gen_normals_gbuffer )
		#define outPs_normals outPs.normals
	@end
	@property( hlms_prepass )
		#define outPs_shadowRoughness outPs.shadowRoughness
	@end
@end

@property( hlms_use_prepass )
	@property( !hlms_use_prepass_msaa )
		Texture2D<unorm float4> gBuf_normals			: register(t@value(gBuf_normals));
		Texture2D<unorm float2> gBuf_shadowRoughness	: register(t@value(gBuf_shadowRoughness));
	@else
		Texture2DMS<unorm float4> gBuf_normals			: register(t@value(gBuf_normals));
		Texture2DMS<unorm float2> gBuf_shadowRoughness	: register(t@value(gBuf_shadowRoughness));
		Texture2DMS<float> gBuf_depthTexture			: register(t@value(gBuf_depthTexture));
	@end

	@property( hlms_use_ssr )
		Texture2D<float4> ssrTexture : register(t@value(ssrTexture));
	@end
@end

@insertpiece( DeclPlanarReflTextures )
@insertpiece( DeclAreaApproxTextures )
@insertpiece( DeclBlueNoiseTexture )

Texture2D<float3> terrainNormals	: register(t@value(terrainNormals));
Texture2D<float4> terrainShadows	: register(t@value(terrainShadows));
SamplerState samplerStateTerra		: register(s@value(terrainNormals));

@property( hlms_forwardplus )
	Buffer<uint> f3dGrid : register(t@value(f3dGrid));
	ReadOnlyBuffer( @value(f3dLightList), float4, f3dLightList );
@end

@property( irradiance_volumes )
	Texture3D<float4>	irradianceVolume		: register(t@value(irradianceVolume));
	SamplerState		irradianceVolumeSampler	: register(s@value(irradianceVolume));
@end

@foreach( num_textures, n )
	Texture2DArray textureMaps@n : register(t@value(textureMaps@n));@end

@property( use_envprobe_map )
	@property( !hlms_enable_cubemaps_auto )
		TextureCube	texEnvProbeMap : register(t@value(texEnvProbeMap));
	@else
		@property( !hlms_cubemaps_use_dpm )
			TextureCubeArray	texEnvProbeMap : register(t@value(texEnvProbeMap));
		@else
			Texture2DArray	texEnvProbeMap : register(t@value(texEnvProbeMap));
			@insertpiece( DeclDualParaboloidFunc )
		@end
	@end
	@property( envMapRegSampler < samplerStateStart )
		SamplerState samplerState@value(envMapRegSampler) : register(s@value(envMapRegSampler));
	@end
@end

@foreach( num_samplers, n )
	SamplerState samplerState@value(currSampler) : register(s@counter(currSampler));@end

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

@insertpiece( DeclOutputType )

@insertpiece( custom_ps_functions )

@insertpiece( output_type ) main
(
	PS_INPUT inPs
	@property( hlms_vpos ), float4 gl_FragCoord : SV_Position@end
	@property( two_sided_lighting ), bool gl_FrontFacing : SV_IsFrontFace@end
	@property( hlms_use_prepass_msaa && hlms_use_prepass ), uint gl_SampleMask : SV_Coverage@end
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
@else ///!hlms_shadowcaster

@insertpiece( DeclOutputType )

@insertpiece( output_type ) main( PS_INPUT inPs )
{
@property( !hlms_render_depth_only || exponential_shadow_maps || hlms_shadowcaster_point )
	PS_OUTPUT outPs;
@end

	@insertpiece( custom_ps_preExecution )
	@insertpiece( DefaultBodyPS )
	@insertpiece( custom_ps_posExecution )

@property( !hlms_render_depth_only || exponential_shadow_maps || hlms_shadowcaster_point )
	return outPs;
@end
}
@end
