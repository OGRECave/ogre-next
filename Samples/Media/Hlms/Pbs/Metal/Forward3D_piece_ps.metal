@property( hlms_forwardplus )

@property( hlms_enable_decals )
	@piece( DeclDecalsSamplers )
		, sampler decalsSampler	[[sampler(@value(decalsSampler))]]
		@property( hlms_decals_diffuse ), texture2d_array<midf> decalsDiffuseTex	[[texture(@value(decalsDiffuseTex))]]@end
		@property( hlms_decals_normals ), texture2d_array<midf> decalsNormalsTex	[[texture(@value(decalsNormalsTex))]]@end
		@property( hlms_decals_diffuse == hlms_decals_emissive )
			#define decalsEmissiveTex decalsDiffuseTex
		@end
		@property( hlms_decals_emissive && hlms_decals_diffuse != hlms_decals_emissive )
			, texture2d_array<midf> decalsEmissiveTex	[[texture(@value(decalsEmissiveTex))]]
		@end
	@end
@end

@end
