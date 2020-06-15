
//#include "SyntaxHighlightingMisc.h"

@insertpiece( SetCrossPlatformSettings )

@insertpiece( DefaultHeaderVS )

struct VS_INPUT
{
	float4 position [[attribute(VES_POSITION)]];
@property( hlms_colour )	float4 colour [[attribute(VES_DIFFUSE)]];@end
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

	@property( hlms_pso_clip_distances )
		float gl_ClipDistance [[clip_distance]] [@value( hlms_pso_clip_distances )];
	@end
};

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
	@property( texture_matrix ), device const float4 *animationMatrixBuf [[buffer(TEX_SLOT_START+1)]]@end
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
