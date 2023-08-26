
//#include "SyntaxHighlightingMisc.h"

@insertpiece( SetCrossPlatformSettings )
@insertpiece( DeclareUvModifierMacros )

@insertpiece( DeclLightProfilesTexture )
@insertpiece( DeclBlueNoiseTexture )

@insertpiece( DefaultHeaderPS )
@insertpiece( custom_ps_uniformDeclaration )

@insertpiece( PccManualProbeDecl )

struct PS_INPUT
{
@insertpiece( VStoPS_block )
};

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

@property( hlms_ss_refractions_available )
	@property( !hlms_use_prepass || !hlms_use_prepass_msaa )
		@property( !hlms_use_prepass_msaa )
			Texture2D<float> gBuf_depthTexture			: register(t@value(gBuf_depthTexture));
			#define depthTextureNoMsaa gBuf_depthTexture
		@else
			Texture2D<float> depthTextureNoMsaa			: register(t@value(depthTextureNoMsaa));
		@end
	@end
	Texture2D		refractionMap			: register(t@value(refractionMap));
	SamplerState	refractionMapSampler	: register(s@value(refractionMap));
@end

@insertpiece( DeclPlanarReflTextures )
@insertpiece( DeclAreaApproxTextures )

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
			@property( use_envprobe_map )Texture2DArray	texEnvProbeMap : register(t@value(texEnvProbeMap));@end
			@insertpiece( DeclDualParaboloidFunc )
		@end
	@end
	@property( envMapRegSampler < samplerStateStart )
		SamplerState samplerState@value(envMapRegSampler) : register(s@value(envMapRegSampler));
	@end
@end

@foreach( num_samplers, n )
	SamplerState samplerState@value(samplerStateStart) : register(s@counter(samplerStateStart));@end

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
	@insertpiece( DefaultBodyPS )
	@insertpiece( custom_ps_posExecution )

	@property( hlms_use_prepass_msaa && false )
		//Useful debug stuff for debugging precision issues.
		/*float testD = gBuf_depthTexture.Load( iFragCoord.xy, 0 );
		outPs.colour0.xyz = testD * testD * testD * testD * testD * testD * testD * testD;*/
		/*float3 col3 = lerp( outPs.colour0.xyz, float3( 1, 1, 1 ), 0.85 );
		outPs.colour0.xyz = 0;
		if( gl_SampleMaskIn & 0x1 )
			outPs.colour0.x = col3.x;
		if( gl_SampleMaskIn & 0x2 )
			outPs.colour0.y = col3.y;
		if( gl_SampleMaskIn & 0x4 )
			outPs.colour0.z = col3.z;
		if( gl_SampleMaskIn & 0x8 )
			outPs.colour0.w = 1.0;*/
		/*outPs.colour0.x = pixelDepth;
		outPs.colour0.y = msaaDepth;
		outPs.colour0.z = 0;
		outPs.colour0.w = 1;*/
	@end

@property( !hlms_render_depth_only )
	return outPs;
@end
}
@else ///!hlms_shadowcaster

@insertpiece( DeclShadowCasterMacros )

@foreach( num_textures, n )
	Texture2DArray textureMaps@n : register(t@value(textureMaps@n));@end
@foreach( num_samplers, n )
	SamplerState samplerState@value(samplerStateStart) : register(s@counter(samplerStateStart));@end

@property( hlms_shadowcaster_point || exponential_shadow_maps )
	@insertpiece( PassStructDecl )
@end

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
