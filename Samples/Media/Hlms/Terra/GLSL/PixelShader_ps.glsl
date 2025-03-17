@insertpiece( SetCrossPlatformSettings )
@property( GL3+ < 430 )
	@property( hlms_tex_gather )#extension GL_ARB_texture_gather: require@end
@end
@insertpiece( SetCompatibilityLayer )

@insertpiece( DeclareUvModifierMacros )

layout(std140) uniform;
#define FRAG_COLOR		0

@insertpiece( DefaultTerraHeaderPS )

// START UNIFORM DECLARATION
@insertpiece( custom_ps_uniformDeclaration )
// END UNIFORM DECLARATION

@insertpiece( PccManualProbeDecl )

vulkan_layout( location = 0 )
in block
{
	@insertpiece( Terra_VStoPS_block )
} inPs;

@pset( currSampler, samplerStateStart )

@property( !hlms_render_depth_only )
	@property( !hlms_shadowcaster )
		@property( !hlms_prepass )
			layout(location = @counter(rtv_target), index = 0) out midf4 outColour;
		@end
		@property( hlms_gen_normals_gbuffer )
			#define outPs_normals outNormals
			layout(location = @counter(rtv_target)) out vec4 outNormals;
		@end
		@property( hlms_prepass )
			#define outPs_shadowRoughness outShadowRoughness
			layout(location = @counter(rtv_target)) out vec2 outShadowRoughness;
		@end
	@else
		layout(location = @counter(rtv_target), index = 0) out float outColour;
	@end
@end

@insertpiece( custom_ps_output_types )

@property( !hlms_shadowcaster )

@property( hlms_use_prepass )
	@property( !hlms_use_prepass_msaa )
		uniform sampler2D gBuf_normals;
		uniform sampler2D gBuf_shadowRoughness;
	@else
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
@insertpiece( DeclBlueNoiseTexture )

@property( hlms_vpos )
in vec4 gl_FragCoord;
@end

vulkan_layout( ogre_t@value(terrainNormals) )	midf_tex uniform texture2D terrainNormals;
vulkan_layout( ogre_t@value(terrainShadows) )	midf_tex uniform texture2D terrainShadows;
vulkan( layout( ogre_s@value(terrainNormals) )	uniform sampler samplerStateTerra );

@property( hlms_forwardplus )
	vulkan_layout( ogre_T@value(f3dGrid) ) uniform usamplerBuffer f3dGrid;
	ReadOnlyBufferF( @value(f3dLightList), float4, f3dLightList );
@end
@property( irradiance_volumes )
	vulkan_layout( ogre_t@value(irradianceVolume) )	midf_tex uniform texture3D	irradianceVolume;
	vulkan( layout( ogre_s@value(irradianceVolume) )uniform sampler				irradianceVolumeSampler );
@end

@foreach( num_textures, n )
	vulkan_layout( ogre_t@value(textureMaps@n) ) midf_tex uniform texture2DArray textureMaps@n;@end

@property( use_envprobe_map )
	@property( !hlms_enable_cubemaps_auto )
		vulkan_layout( ogre_t@value(texEnvProbeMap) ) midf_tex uniform textureCube texEnvProbeMap;
	@else
		@property( !hlms_cubemaps_use_dpm )
			vulkan_layout( ogre_t@value(texEnvProbeMap) ) midf_tex uniform textureCubeArray texEnvProbeMap;
		@else
			vulkan_layout( ogre_t@value(texEnvProbeMap) ) midf_tex uniform texture2DArray texEnvProbeMap;
			@insertpiece( DeclDualParaboloidFunc )
		@end
	@end
	@property( envMapRegSampler < samplerStateStart && syntax == glslvk )
		layout( ogre_s@value(envMapRegSampler) ) uniform sampler samplerState@value(envMapRegSampler);
	@end
@end

@property( syntax == glslvk )
	@foreach( num_samplers, n )
		layout( ogre_s@value(currSampler) ) uniform sampler samplerState@counter(currSampler);@end
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

@insertpiece( DeclVctTextures )
@insertpiece( DeclIrradianceFieldTextures )

@insertpiece( custom_ps_functions )

void main()
{
	@insertpiece( custom_ps_preExecution )
	@insertpiece( DefaultTerraBodyPS )
	@insertpiece( custom_ps_posExecution )
}
@else /// !hlms_shadowcaster

void main()
{
	@insertpiece( custom_ps_preExecution )
	@insertpiece( DefaultBodyPS )
	@insertpiece( custom_ps_posExecution )
}
@end
