
//#include "SyntaxHighlightingMisc.h"

@insertpiece( SetCrossPlatformSettings )

@insertpiece( DefaultHeaderVS )
@insertpiece( custom_vs_uniformDeclaration )

struct VS_INPUT
{
	float4 vertex : POSITION;
@property( hlms_normal )	float3 normal : NORMAL;@end
@property( hlms_qtangent )	float4 qtangent : NORMAL;@end

@property( normal_map && !hlms_qtangent )
	float3 tangent	: TANGENT;
	@property( hlms_binormal )float3 binormal	: BINORMAL;@end
@end

@property( hlms_skeleton )
	uint4 blendIndices	: BLENDINDICES;
	float4 blendWeights : BLENDWEIGHT;
@end

@property( hlms_vertex_id )
	uint vertexId: SV_VertexID;
@end

@foreach( hlms_uv_count, n )
	float@value( hlms_uv_count@n ) uv@n : TEXCOORD@n;@end
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
Buffer<float4> worldMatBuf : register(t0);
@property( hlms_pose )
	Buffer<float4> poseBuf : register(t4);
@end
// END UNIFORM D3D DECLARATION

PS_INPUT main( VS_INPUT input )
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
