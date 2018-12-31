// Our terrain has the following pattern:
//
//	 1N  10   11
//		o-o-o
//		|/|/|
//	 0N	o-+-o 01
//		|/|/|
//		o-o-o
//	 NN  N0   N1
//
// We need to calculate the normal of the vertex in
// the center '+', which is shared by 6 triangles.

#include <metal_stdlib>
using namespace metal;

struct PS_INPUT
{
	float2 uv0;
};

struct Params
{
	float2 heightMapResolution;
	float3 vScale;
};

fragment float4 main_metal
(
	PS_INPUT inPs [[stage_in]],

	texture2d<float, access::read> heightMap [[texture(0)]],

	constant Params &p [[buffer(PARAMETER_SLOT)]]
)
{
	int2 iCoord = int2( inPs.uv0 * p.heightMapResolution );

	int3 xN01;
	xN01.x = max( iCoord.x - 1, 0 );
	xN01.y = iCoord.x;
	xN01.z = min( iCoord.x + 1, int(p.heightMapResolution.x) );
	int3 yN01;
	yN01.x = max( iCoord.y - 1, 0 );
	yN01.y = iCoord.y;
	yN01.z = min( iCoord.y + 1, int(p.heightMapResolution.y) );

	//Watch out! It's heightXY, but heightMap.read uses YX.
	float heightNN = heightMap.read( ushort2( xN01.x, yN01.x ) ).x * p.vScale.y;
	float heightN0 = heightMap.read( ushort2( xN01.y, yN01.x ) ).x * p.vScale.y;
	//float heightN1 = heightMap.read( ushort2( xN01.z, yN01.x ) ).x * p.vScale.y;

	float height0N = heightMap.read( ushort2( xN01.x, yN01.y ) ).x * p.vScale.y;
	float height00 = heightMap.read( ushort2( xN01.y, yN01.y ) ).x * p.vScale.y;
	float height01 = heightMap.read( ushort2( xN01.z, yN01.y ) ).x * p.vScale.y;

	//float height1N = heightMap.read( ushort2( xN01.x, yN01.z ) ).x * p.vScale.y;
	float height10 = heightMap.read( ushort2( xN01.y, yN01.z ) ).x * p.vScale.y;
	float height11 = heightMap.read( ushort2( xN01.z, yN01.z ) ).x * p.vScale.y;

	float3 vNN = float3( -p.vScale.x, heightNN, -p.vScale.z );
	float3 vN0 = float3( -p.vScale.x, heightN0,  0 );
	//float3 vN1 = float3( -p.vScale.x, heightN1,  p.vScale.z );

	float3 v0N = float3(  0, height0N, -p.vScale.z );
	float3 v00 = float3(  0, height00,  0 );
	float3 v01 = float3(  0, height01,  p.vScale.z );

	//float3 v1N = float3(  p.vScale.x, height1N, -p.vScale.z );
	float3 v10 = float3(  p.vScale.x, height10,  0 );
	float3 v11 = float3(  p.vScale.x, height11,  p.vScale.z );

	float3 vNormal = float3( 0, 0, 0 );

	vNormal += cross( (v01 - v00), (v11 - v00) );
	vNormal += cross( (v11 - v00), (v10 - v00) );
	vNormal += cross( (v10 - v00), (v0N - v00) );
	vNormal += cross( (v0N - v00), (vNN - v00) );
	vNormal += cross( (vNN - v00), (vN0 - v00) );
	vNormal += cross( (vN0 - v00), (v01 - v00) );

//	vNormal += cross( (v01 - v00), (v11 - v00) );
//	vNormal += cross( (v11 - v00), (v10 - v00) );
//	vNormal += cross( (v10 - v00), (v1N - v00) );
//	vNormal += cross( (v1N - v00), (v0N - v00) );
//	vNormal += cross( (v0N - v00), (vNN - v00) );
//	vNormal += cross( (vNN - v00), (vN0 - v00) );
//	vNormal += cross( (vN0 - v00), (vN1 - v00) );
//	vNormal += cross( (vN1 - v00), (v01 - v00) );

	vNormal = normalize( vNormal );

	//return vNormal.zx;
	return float4( vNormal.zyx * 0.5f + 0.5f, 1.0f );
}
