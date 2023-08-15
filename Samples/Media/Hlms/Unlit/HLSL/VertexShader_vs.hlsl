
//#include "SyntaxHighlightingMisc.h"

@insertpiece( SetCrossPlatformSettings )

// START UNIFORM D3D PRE DECLARATION
@insertpiece( ParticleSystemDeclVS )
// END UNIFORM D3D PRE DECLARATION

@insertpiece( DefaultHeaderVS )
@insertpiece( custom_vs_uniformDeclaration )

// START UNIFORM D3D DECLARATION
@property( !hlms_particle_system )
	ReadOnlyBuffer( 0, float4, worldMatBuf );
@end
@property( texture_matrix )
	ReadOnlyBuffer( @value( texture_matrix ), float4, animationMatrixBuf );
@end
// END UNIFORM D3D DECLARATION

struct VS_INPUT
{
@property( !hlms_particle_system )
	float4 vertex : POSITION;
@end
@property( hlms_colour && !hlms_particle_system )
	float4 colour : COLOR0;
@end
@foreach( hlms_uv_count, n )
	float@value( hlms_uv_count@n ) uv@n : TEXCOORD@n;@end
@property( hlms_vertex_id )
	uint vertexId : SV_VertexID;
@end
	uint drawId : DRAWID;
	@insertpiece( custom_vs_attributes )
};

struct PS_INPUT
{
@insertpiece( VStoPS_block )
	float4 gl_Position : SV_Position;

	@pdiv( full_pso_clip_distances, hlms_pso_clip_distances, 4 )
	@pmod( partial_pso_clip_distances, hlms_pso_clip_distances, 4 )
	@foreach( full_pso_clip_distances, n )
		float4 gl_ClipDistance@n : SV_ClipDistance@n;
	@end
	@property( partial_pso_clip_distances )
		float@value( partial_pso_clip_distances ) gl_ClipDistance@value( full_pso_clip_distances ) : SV_ClipDistance@value( full_pso_clip_distances );
	@end
};

PS_INPUT main( VS_INPUT input )
{
	PS_INPUT outVs;
	@insertpiece( custom_vs_preExecution )
	@insertpiece( DefaultBodyVS )
	@insertpiece( custom_vs_posExecution )

	return outVs;
}
