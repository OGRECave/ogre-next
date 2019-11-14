
//#include "SyntaxHighlightingMisc.h"

@insertpiece( SetCrossPlatformSettings )

@insertpiece( DefaultHeaderVS )

struct VS_INPUT
{
	float4 position [[attribute(VES_POSITION)]];
@property( hlms_normal )	float3 normal [[attribute(VES_NORMAL)]];@end
@property( hlms_qtangent )	float4 qtangent [[attribute(VES_NORMAL)]];@end

@property( normal_map && !hlms_qtangent )
	float3 tangent	[[attribute(VES_TANGENT)]];
	@property( hlms_binormal )float3 binormal	[[attribute(VES_BINORMAL)]];@end
@end

@property( hlms_skeleton )
	uint4 blendIndices	[[attribute(VES_BLEND_INDICES)]];
	float4 blendWeights [[attribute(VES_BLEND_WEIGHTS)]];@end

@foreach( hlms_uv_count, n )
	float@value( hlms_uv_count@n ) uv@n [[attribute(VES_TEXTURE_COORDINATES@n)]];@end
@property( !iOS )
	ushort drawId [[attribute(15)]];
@end
	@insertpiece( custom_vs_attributes )
};

struct PS_INPUT
{
@insertpiece( VStoPS_block )
	float4 gl_Position [[position]];
	@foreach( hlms_pso_clip_distances, n )
		float gl_ClipDistance@n [[clip_distance]];
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
	, device const float4 *worldMatBuf [[buffer(TEX_SLOT_START+0)]]
	@property( hlms_pose )
		@property( !hlms_pose_half )
			, device const float4 *poseBuf	[[buffer(TEX_SLOT_START+4)]]
		@else
			, device const half4 *poseBuf	[[buffer(TEX_SLOT_START+4)]]
		@end
	@end
	@property( hlms_vertex_id )
		, uint vertexId [[vertex_id]]
		, uint baseVertex [[base_vertex]]
	@end
	@insertpiece( custom_vs_uniformDeclaration )
	// END UNIFORM DECLARATION
)
{
	PS_INPUT outVs;
@property( !hlms_qtangent && hlms_normal )
	float3 normal	= input.normal;
	@property( normal_map )float3 tangent	= input.tangent;@end
	@property( hlms_binormal )float3 binormal	= input.binormal;@end
@end

	@insertpiece( custom_vs_preExecution )
	@insertpiece( DefaultBodyVS )
	@insertpiece( custom_vs_posExecution )

	return outVs;
}
