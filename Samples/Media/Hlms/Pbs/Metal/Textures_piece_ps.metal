
@property( !hlms_render_depth_only && !hlms_shadowcaster && hlms_prepass )
	@undefpiece( DeclOutputType )
	@piece( DeclOutputType )
		struct PS_OUTPUT
		{
			float4 normals			[[ color(0) ]];
			float2 shadowRoughness	[[ color(1) ]];
		};
	@end
@end
