@property( !hlms_shadowcaster && terra_enabled )

@piece( custom_VStoPS )
	midf terrainShadow;
@end

/// Extra per-pass global data we need for applying our
/// shadows to regular objects, passed to all PBS shaders.
@piece( custom_passBuffer )
    vec4 terraOrigin; //Normalized. i.e. -terrainOrigin / terrainDimensions
    //.xz = terrain 1.0 / XZ dimensions.
    //.y  = 1.0 / terrainHeight;
    vec4 invTerraBounds;
@end

/// Add the shadows' texture to the vertex shader
@piece( custom_vs_uniformDeclaration )
	vulkan_layout( ogre_t@value(terrainShadows) ) uniform texture2D terrainShadows;
	vulkan( layout( ogre_s@value(terrainShadows) ) uniform sampler terrainShadowSampler );
@end

/// Evaluate the shadow based on world XZ position & height in the vertex shader.
/// Doing it at the pixel shader level would be more accurate, but the difference
/// is barely noticeable, and slower
@piece( custom_vs_posExecution )
	@property( z_up )
		vec3 terraWorldPos = vec3( worldPos.x, -worldPos.z, worldPos.y );
	@else
		vec3 terraWorldPos = worldPos.xyz;
	@end
	vec3 terraShadowData = textureLod( vkSampler2D( terrainShadows, terrainShadowSampler ),
									   terraWorldPos.xz * passBuf.invTerraBounds.xz +
									   passBuf.terraOrigin.xz, 0 ).xyz;
	float terraHeightWeight = terraWorldPos.y * passBuf.invTerraBounds.y + passBuf.terraOrigin.y;
    terraHeightWeight = (terraHeightWeight - terraShadowData.y) * terraShadowData.z * 1023.0;
	outVs.terrainShadow = mix( midf_c( terraShadowData.x ), _h( 1.0 ), midf_c( saturate( terraHeightWeight ) ) );
@end

@property( hlms_lights_directional && hlms_num_shadow_map_lights )
    @piece( custom_ps_preLights )fShadow *= inPs.terrainShadow;@end
@else
    @piece( custom_ps_preLights )float fShadow = inPs.terrainShadow;@end
    @piece( DarkenWithShadowFirstLight )* fShadow@end
@end

@end
