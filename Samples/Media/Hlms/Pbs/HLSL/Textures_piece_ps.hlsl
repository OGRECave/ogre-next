
@property( !hlms_render_depth_only && !hlms_shadowcaster )
	@piece( ExtraOutputTypes )
		@property( hlms_gen_normals_gbuffer )
			float4 normals			: SV_Target@counter(rtv_target);
		@end
		@property( hlms_prepass )
			float2 shadowRoughness	: SV_Target@counter(rtv_target);
		@end
	@end
@end
