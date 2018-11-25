
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
