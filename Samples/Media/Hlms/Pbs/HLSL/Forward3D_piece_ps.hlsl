@property( hlms_forwardplus )

@property( hlms_enable_decals )
	@piece( DeclDecalsSamplers )
		SamplerState decalsSampler : register(s@value(decalsSampler));
		@property( hlms_decals_diffuse )Texture2DArray decalsDiffuseTex : register(t@value(decalsDiffuseTex));@end
		@property( hlms_decals_normals )Texture2DArray<float2> decalsNormalsTex : register(t@value(decalsNormalsTex));@end
		@property( hlms_decals_diffuse == hlms_decals_emissive )
			#define decalsEmissiveTex decalsDiffuseTex
		@end
		@property( hlms_decals_emissive && hlms_decals_diffuse != hlms_decals_emissive )
			Texture2DArray decalsEmissiveTex : register(t@value(decalsEmissiveTex));
		@end
	@end
@end

@end
