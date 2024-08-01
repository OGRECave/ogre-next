
//#include "SyntaxHighlightingMisc.h"

@insertpiece( SetCrossPlatformSettings )

@insertpiece( DefaultHeaderVS )

struct VS_INPUT
{
@property( !hlms_particle_system )
	float4 position [[attribute(VES_POSITION)]];
	@property( hlms_normal )	float3 normal [[attribute(VES_NORMAL)]];@end
	@property( hlms_qtangent )	midf4 qtangent [[attribute(VES_NORMAL)]];@end

	@property( normal_map && !hlms_qtangent )
		@property( hlms_tangent4 )float4 tangent	[[attribute(VES_TANGENT)]];@end
		@property( !hlms_tangent4 )float3 tangent	[[attribute(VES_TANGENT)]];@end
		@property( hlms_binormal )float3 binormal	[[attribute(VES_BINORMAL)]];@end
	@end

	@foreach( hlms_uv_count, n )
		float@value( hlms_uv_count@n ) uv@n [[attribute(VES_TEXTURE_COORDINATES@n)]];@end
@end /// hlms_particle_system

@property( hlms_skeleton )
	uint4 blendIndices	[[attribute(VES_BLEND_INDICES)]];
	float4 blendWeights [[attribute(VES_BLEND_WEIGHTS)]];@end

@property( !iOS )
	ushort drawId [[attribute(15)]];
@end
	@insertpiece( custom_vs_attributes )
};

struct PS_INPUT
{
@insertpiece( VStoPS_block )
	float4 gl_Position [[position]];

	@property( hlms_pso_clip_distances )
		float gl_ClipDistance [[clip_distance]] [@value( hlms_pso_clip_distances )];
	@end
};

// START UNIFORM METAL STRUCT DECLARATION
// END UNIFORM METAL  STRUCT DECLARATION


vertex PS_INPUT main_metal
(
	VS_INPUT input [[stage_in]]
	@property( iOS )
		, ushort instanceId [[instance_id]]
		, constant ushort &baseInstance [[buffer(15)]]
	@end
	// START UNIFORM DECLARATION
	@insertpiece( PassDecl )
	@insertpiece( InstanceDecl )
	@insertpiece( AtmosphereNprSkyDecl )
	@property( !hlms_particle_system )
		, device const float4 *worldMatBuf [[buffer(TEX_SLOT_START+0)]]
	@end
	@property( hlms_pose )
		@property( !hlms_pose_half )
			, device const float4 *poseBuf	[[buffer(TEX_SLOT_START+@value(poseBuf))]]
		@else
			, device const half4 *poseBuf	[[buffer(TEX_SLOT_START+@value(poseBuf))]]
		@end
	@end

	@insertpiece( ParticleSystemDeclVS )

	@property( hlms_vertex_id )
		, uint inVs_vertexId [[vertex_id]]
		, uint baseVertexID [[base_vertex]]
	@end
	@insertpiece( custom_vs_uniformDeclaration )
	// END UNIFORM DECLARATION
)
{
	PS_INPUT outVs;

	@insertpiece( custom_vs_preExecution )
	@insertpiece( DefaultBodyVS )
	@insertpiece( custom_vs_posExecution )

	return outVs;
}
