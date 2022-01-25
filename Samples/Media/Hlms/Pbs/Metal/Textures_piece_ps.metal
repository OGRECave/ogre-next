
@property( !hlms_render_depth_only && !hlms_shadowcaster )
	@piece( ExtraOutputTypes )
		@property( hlms_gen_normals_gbuffer )
			midf4 normals			[[ color(@counter(rtv_target)) ]];
		@end
		@property( hlms_prepass )
			midf2 shadowRoughness	[[ color(@counter(rtv_target)) ]];
		@end
	@end
@end
