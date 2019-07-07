
#include <metal_stdlib>
using namespace metal;

struct PS_INPUT
{
	float4 voxelColour [[flat]];
};

fragment float4 main_metal
(
	PS_INPUT inPs [[stage_in]]
)
{
	return inPs.voxelColour;
}
