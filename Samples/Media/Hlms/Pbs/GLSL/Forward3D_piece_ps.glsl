@property( hlms_forwardplus )

@property( hlms_enable_decals )
	@piece( DeclDecalsSamplers )
		@property( hlms_decals_diffuse )uniform sampler2DArray decalsDiffuseTex;@end
		@property( hlms_decals_normals )uniform sampler2DArray decalsNormalsTex;@end
		@property( hlms_decals_diffuse == hlms_decals_emissive )
			#define decalsEmissiveTex decalsDiffuseTex
		@end
		@property( hlms_decals_emissive && hlms_decals_diffuse != hlms_decals_emissive )
			uniform sampler2DArray decalsEmissiveTex;
		@end
	@end
@end

@end
