
//#include "SyntaxHighlightingMisc.h"

@insertpiece( SetCrossPlatformSettings )

// START UNIFORM D3D PRE DECLARATION
@insertpiece( ParticleSystemDeclVS )
// END UNIFORM D3D PRE DECLARATION

@insertpiece( DefaultHeaderVS )
@insertpiece( custom_vs_uniformDeclaration )

struct VS_INPUT
{
@property( !hlms_particle_system )
	float4 vertex : POSITION;
	@property( hlms_normal )	float3 normal : NORMAL;@end
	@property( hlms_qtangent )	float4 qtangent : NORMAL;@end

	@property( normal_map && !hlms_qtangent )
		@property( hlms_tangent4 )float4 tangent	: TANGENT;@end
		@property( !hlms_tangent4 )float3 tangent	: TANGENT;@end
		@property( hlms_binormal )float3 binormal	: BINORMAL;@end
	@end

	@foreach( hlms_uv_count, n )
		float@value( hlms_uv_count@n ) uv@n : TEXCOORD@n;@end
@end /// hlms_particle_system

@property( hlms_skeleton )
	uint4 blendIndices	: BLENDINDICES;
	float4 blendWeights : BLENDWEIGHT;
@end

@property( hlms_vertex_id )
	uint vertexId: SV_VertexID;
@end

	uint drawId : DRAWID;
	@insertpiece( custom_vs_attributes )
};

struct PS_INPUT
{
	@insertpiece( VStoPS_block )
	float4 gl_Position: SV_Position;

	@property( hlms_instanced_stereo )
		uint gl_ViewportIndex : SV_ViewportArrayIndex;
	@end

	@pdiv( full_pso_clip_distances, hlms_pso_clip_distances, 4 )
	@pmod( partial_pso_clip_distances, hlms_pso_clip_distances, 4 )
	@foreach( full_pso_clip_distances, n )
		float4 gl_ClipDistance@n : SV_ClipDistance@n;
	@end
	@property( partial_pso_clip_distances )
		float@value( partial_pso_clip_distances ) gl_ClipDistance@value( full_pso_clip_distances ) : SV_ClipDistance@value( full_pso_clip_distances );
	@end
};

// START UNIFORM D3D DECLARATION
@property( !hlms_particle_system )
	ReadOnlyBuffer( 0, float4, worldMatBuf );
@end
@property( hlms_pose )
	Buffer<float4> poseBuf : register(t@value(poseBuf));
@end
// END UNIFORM D3D DECLARATION

PS_INPUT main( VS_INPUT input )
{
	PS_INPUT outVs;

	@insertpiece( custom_vs_preExecution )
	@insertpiece( DefaultBodyVS )
	@insertpiece( custom_vs_posExecution )

	return outVs;
}
