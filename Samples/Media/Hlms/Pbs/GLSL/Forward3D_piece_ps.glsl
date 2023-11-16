@property( hlms_forwardplus )

@property( hlms_enable_decals )
	@piece( DeclDecalsSamplers )
		@property( syntax == glslvk )
			layout( ogre_s@value(decalsSampler) ) uniform sampler decalsSampler;
		@end
		@property( hlms_decals_diffuse )vulkan_layout( ogre_t@value(decalsDiffuseTex) ) midf_tex uniform texture2DArray decalsDiffuseTex;@end
		@property( hlms_decals_normals )vulkan_layout( ogre_t@value(decalsNormalsTex) ) midf_tex uniform texture2DArray decalsNormalsTex;@end
		@property( hlms_decals_diffuse == hlms_decals_emissive )
			#define decalsEmissiveTex decalsDiffuseTex
		@end
		@property( hlms_decals_emissive && hlms_decals_diffuse != hlms_decals_emissive )
			vulkan_layout( ogre_t@value(decalsEmissiveTex) ) midf_tex uniform texture2DArray decalsEmissiveTex;
		@end
	@end
@end

@end
