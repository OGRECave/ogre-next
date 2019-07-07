
struct PS_INPUT
{
	nointerpolation float4 voxelColour		: TEXCOORD0;
};

float4 main( PS_INPUT inPs ) : SV_Target
{
	return inPs.voxelColour;
}
