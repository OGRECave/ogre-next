
//Set the sampler starts. Note that 'padd' get calculated before _any_ 'add'
@set( texUnit, 1 )

@property( hlms_forwardplus )
	@add( texUnit, 2 )
@end

@property( hlms_use_prepass )
	@set( gBuf_normals, texUnit )
	@add( gBuf_shadowRoughness, texUnit, 1 )
	@add( texUnit, 2 )

	@property( hlms_use_prepass_msaa )
		@set( gBuf_depthTexture, texUnit )
		@add( texUnit, 1 )
	@end

	@property( hlms_use_ssr )
		@set( ssrTexture, texUnit )
		@add( texUnit, 1 )
	@end
@end

@property( irradiance_volumes )
	@set( irradianceVolumeTexUnit, texUnit )
	@add( texUnit, 1 )
@end

@property( hlms_lights_area_tex_mask )
	@set( areaLightsApproxTexUnit, texUnit )
	@add( texUnit, 1 )
@end

@property( hlms_lights_area_ltc )
	@set( ltcMatrixTexUnit, texUnit )
	@add( texUnit, 1 )
@end

@property( hlms_enable_decals )
	@set( decalsTexUnit, texUnit )
	@add( texUnit, hlms_enable_decals )
@end

@set( textureRegShadowMapStart, texUnit )
@add( texUnit, hlms_num_shadow_map_textures )

@property( parallax_correct_cubemaps )
	@set( globaPccTexUnit, texUnit )
	@add( texUnit, 1 )
@end

@property( has_planar_reflections )
	@set( planarReflectionTexUnit, texUnit )
	@add( texUnit, 1 )
@end

@set( textureRegStart, texUnit )
@set( samplerStateStart, texUnit )

@add( diffuse_map_sampler,		samplerStateStart )
@add( normal_map_tex_sampler,	samplerStateStart )
@add( specular_map_sampler,		samplerStateStart )
@add( roughness_map_sampler,	samplerStateStart )
@add( envprobe_map_sampler,		samplerStateStart )
@add( detail_weight_map_sampler,samplerStateStart )
@add( detail_map0_sampler,		samplerStateStart )
@add( detail_map0_nm_sampler,	samplerStateStart )
@add( detail_map1_sampler,		samplerStateStart )
@add( detail_map1_nm_sampler,	samplerStateStart )
@add( detail_map2_sampler,		samplerStateStart )
@add( detail_map2_nm_sampler,	samplerStateStart )
@add( detail_map3_sampler,		samplerStateStart )
@add( detail_map3_nm_sampler,	samplerStateStart )

@set( envMapRegSampler, envprobe_map_sampler )
@add( texUnit, num_textures )

@set( envMapReg, texUnit )

@property( use_envprobe_map )
	@property( !envprobe_map || envprobe_map == target_envprobe_map )
		/// Auto cubemap textures are set at the beginning. Manual cubemaps are the end.
		@set( envMapReg, globaPccTexUnit )
		@set( envMapRegSampler, globaPccTexUnit )
	@end
@end

@property( !hlms_render_depth_only && !hlms_shadowcaster && hlms_prepass )
	@undefpiece( DeclOutputType )
	@piece( DeclOutputType )
		struct PS_OUTPUT
		{
			float4 normals			: SV_Target0;
			float2 shadowRoughness	: SV_Target1;
		};
	@end
@end
