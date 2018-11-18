@insertpiece( SetCrossPlatformSettings )
@property( !GL430 )
@property( hlms_tex_gather )#extension GL_ARB_texture_gather: require@end
@end
@insertpiece( SetCompatibilityLayer )
@insertpiece( DeclareUvModifierMacros )

layout(std140) uniform;
#define FRAG_COLOR		0

@insertpiece( DefaultHeaderPS )

@property( !hlms_render_depth_only )
	@property( !hlms_shadowcaster )
		@property( !hlms_prepass )
			layout(location = FRAG_COLOR, index = 0) out vec4 outColour;
		@end @property( hlms_prepass )
			layout(location = 0) out vec4 outNormals;
			layout(location = 1) out vec2 outShadowRoughness;
		@end
	@end @property( hlms_shadowcaster )
	layout(location = FRAG_COLOR, index = 0) out float outColour;
	@end
@end

@property( hlms_use_prepass )
	@property( !hlms_use_prepass_msaa )
		uniform sampler2D gBuf_normals;
		uniform sampler2D gBuf_shadowRoughness;
	@end @property( hlms_use_prepass_msaa )
		uniform sampler2DMS gBuf_normals;
		uniform sampler2DMS gBuf_shadowRoughness;
		uniform sampler2DMS gBuf_depthTexture;
	@end

	@property( hlms_use_ssr )
		uniform sampler2D ssrTexture;
	@end
@end

@insertpiece( DeclPlanarReflTextures )
@insertpiece( DeclAreaApproxTextures )

@property( hlms_vpos )
in vec4 gl_FragCoord;
@end

// START UNIFORM DECLARATION
@property( !hlms_shadowcaster || alpha_test )
	@property( !hlms_shadowcaster )
		@insertpiece( PassStructDecl )
	@end
	@insertpiece( MaterialStructDecl )
	@insertpiece( InstanceDecl )
	@insertpiece( PccManualProbeDecl )
@end
@insertpiece( custom_ps_uniformDeclaration )
// END UNIFORM DECLARATION
@property( !hlms_shadowcaster || !hlms_shadow_uses_depth_texture || alpha_test || exponential_shadow_maps )
in block
{
@insertpiece( VStoPS_block )
} inPs;
@end

@property( !hlms_shadowcaster )

@property( hlms_forwardplus )
/*layout(binding = 1) */uniform usamplerBuffer f3dGrid;
/*layout(binding = 2) */uniform samplerBuffer f3dLightList;
@end
@property( irradiance_volumes )
	uniform sampler3D irradianceVolume;
@end

@property( !roughness_map && !hlms_decals_diffuse )#define ROUGHNESS material.kS.w@end
@foreach( num_textures, n )
	uniform sampler2DArray textureMaps@n;@end

@property( !hlms_enable_cubemaps_auto )
	@property( use_envprobe_map )uniform samplerCube		texEnvProbeMap;@end
@end
@property( hlms_enable_cubemaps_auto )
	@property( !hlms_cubemaps_use_dpm )
		@property( use_envprobe_map )uniform samplerCubeArray	texEnvProbeMap;@end
	@end
	@property( hlms_cubemaps_use_dpm )
		@property( use_envprobe_map )uniform sampler2DArray	texEnvProbeMap;@end
		@insertpiece( DeclDualParaboloidFunc )
	@end
@end

@property( (hlms_normal || hlms_qtangent) && !hlms_prepass && needs_view_dir )
	@insertpiece( DeclareBRDF )
	@insertpiece( DeclareBRDF_InstantRadiosity )
	@insertpiece( DeclareBRDF_AreaLightApprox )
@end

@property( use_parallax_correct_cubemaps )
	@insertpiece( DeclParallaxLocalCorrect )
@end

@insertpiece( DeclDecalsSamplers )

@insertpiece( DeclShadowMapMacros )
@insertpiece( DeclShadowSamplers )
@insertpiece( DeclShadowSamplingFuncs )

@insertpiece( DeclAreaLtcTextures )
@insertpiece( DeclAreaLtcLightFuncs )

@insertpiece( custom_ps_functions )

void main()
{
    @insertpiece( custom_ps_preExecution )
	@insertpiece( DefaultBodyPS )
	@insertpiece( custom_ps_posExecution )
}
@end
@property( hlms_shadowcaster )

@insertpiece( DeclShadowCasterMacros )

@property( alpha_test )
	Material material;
	float diffuseCol;
	@property( num_textures )uniform sampler2DArray textureMaps[@value( num_textures )];@end
	@property( diffuse_map )uint diffuseIdx;@end
	@property( detail_weight_map )uint weightMapIdx;@end
	@foreach( 4, n )
		@property( detail_map@n )uint detailMapIdx@n;@end @end
@end

@property( hlms_shadowcaster_point || exponential_shadow_maps )
	@insertpiece( PassStructDecl )
@end

void main()
{
	@insertpiece( custom_ps_preExecution )

@property( alpha_test )
	@property( !lower_gpu_overhead )
		uint materialId	= worldMaterialIdx[inPs.drawId].x & 0x1FFu;
		material = materialArray.m[materialId];
	@end @property( lower_gpu_overhead )
		material = materialArray.m[0];
	@end
@property( diffuse_map )	diffuseIdx			= material.indices0_3.x & 0x0000FFFFu;@end
@property( detail_weight_map )	weightMapIdx		= material.indices0_3.z & 0x0000FFFFu;@end
@property( detail_map0 )	detailMapIdx0		= material.indices0_3.z >> 16u;@end
@property( detail_map1 )	detailMapIdx1		= material.indices0_3.w & 0x0000FFFFu;@end
@property( detail_map2 )	detailMapIdx2		= material.indices0_3.w >> 16u;@end
@property( detail_map3 )	detailMapIdx3		= material.indices4_7.x & 0x0000FFFFu;@end

@property( detail_maps_diffuse || detail_maps_normal )
	//Prepare weight map for the detail maps.
	@property( detail_weight_map )
		vec4 detailWeights = @insertpiece( SamplerDetailWeightMap );
		@property( detail_weights )detailWeights *= material.cDetailWeights;@end
	@end @property( !detail_weight_map )
		@property( detail_weights )vec4 detailWeights = material.cDetailWeights;@end
		@property( !detail_weights )vec4 detailWeights = vec4( 1.0, 1.0, 1.0, 1.0 );@end
	@end
@end

	/// Sample detail maps and weight them against the weight map in the next foreach loop.
@foreach( detail_maps_diffuse, n )@property( detail_map@n )
	float detailCol@n	= texture( textureMaps[@value(detail_map@n_idx)],
									vec3( UV_DETAIL@n( inPs.uv@value(uv_detail@n).xy@insertpiece( offsetDetail@n ) ),
										  detailMapIdx@n ) ).w;
	detailCol@n = detailWeights.@insertpiece(detail_swizzle@n) * detailCol@n;@end
@end

@insertpiece( SampleDiffuseMap )

	/// 'insertpiece( SampleDiffuseMap )' must've written to diffuseCol. However if there are no
	/// diffuse maps, we must initialize it to some value. If there are no diffuse or detail maps,
	/// we must not access diffuseCol at all, but rather use material.kD directly (see piece( kD ) ).
	@property( !diffuse_map )diffuseCol = material.bgDiffuse.w;@end

	/// Blend the detail diffuse maps with the main diffuse.
@foreach( detail_maps_diffuse, n )
	@insertpiece( blend_mode_idx@n ) @add( t, 1 ) @end

	/// Apply the material's alpha over the textures
@property( TODO_REFACTOR_ACCOUNT_MATERIAL_ALPHA )	diffuseCol.xyz *= material.kD.xyz;@end

	if( material.kD.w @insertpiece( alpha_test_cmp_func ) diffuseCol )
		discard;
@end /// !alpha_test

	@insertpiece( DoShadowCastPS )

	@insertpiece( custom_ps_posExecution )
}
@end
