
//#include "SyntaxHighlightingMisc.h"

@insertpiece( SetCrossPlatformSettings )
@insertpiece( SetCompatibilityLayer )

struct VS_INPUT
{
@property( !iOS )
	ushort drawId	[[attribute(15)]];
@end
	@insertpiece( custom_vs_attributes )
};

struct PS_INPUT
{
	@insertpiece( Terra_VStoPS_block )
	float4 gl_Position [[position]];
	@foreach( hlms_pso_clip_distances, n )
		float gl_ClipDistance [[clip_distance]] [@value( hlms_pso_clip_distances )];
	@end
};

@insertpiece( DefaultTerraHeaderVS )

vertex PS_INPUT main_metal
(
	VS_INPUT input [[stage_in]]
	, uint inVs_vertexId	[[vertex_id]]
	@property( iOS )
		, ushort instanceId [[instance_id]]
		, constant ushort &baseInstance [[buffer(15)]]
	@end
	// START UNIFORM DECLARATION
	@insertpiece( PassDecl )
	@insertpiece( TerraInstanceDecl )
	@property( hlms_shadowcaster )
		@insertpiece( MaterialDecl )
	@end
	@insertpiece( AtmosphereNprSkyDecl )
	@property( !terra_use_uint )
		, texture2d<float, access::read> heightMap [[texture(@value(heightMap))]]
	@else
		, texture2d<uint, access::read> heightMap [[texture(@value(heightMap))]]
	@end
	@insertpiece( custom_vs_uniformDeclaration )
	// END UNIFORM DECLARATION
)
{
	PS_INPUT outVs;

	@insertpiece( custom_vs_preExecution )
	@insertpiece( DefaultTerraBodyVS )
	@insertpiece( custom_vs_posExecution )

	return outVs;
}
