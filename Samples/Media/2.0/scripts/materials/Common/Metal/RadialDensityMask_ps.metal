#include <metal_stdlib>
using namespace metal;

struct Params
{
	float4 rightEyeStart_radius;
	float4 leftEyeCenter_rightEyeCenter;
	float2 invBlockResolution;
};

#define p_leftEyeCenter			p.leftEyeCenter_rightEyeCenter.xy
#define p_rightEyeCenter		p.leftEyeCenter_rightEyeCenter.zw
#define p_rightEyeStart			p.rightEyeStart_radius.x
#define p_radius				p.rightEyeStart_radius.yzw
#define p_invBlockResolution	p.invBlockResolution

fragment float4 main_metal
(
	float4 gl_FragCoord [[position]],
	constant Params &p [[buffer(PARAMETER_SLOT)]]
)
{
	float2 eyeCenter = gl_FragCoord.x >= p_rightEyeStart ? p_rightEyeCenter : p_leftEyeCenter;

	//We must work in blocks so the reconstruction filter can work properly
	float2 toCenter = trunc(gl_FragCoord.xy * 0.125f) * p_invBlockResolution.xy - eyeCenter;
	toCenter.x *= 2.0f; //Twice because of stereo (each eye is half the size of the full res)
	float distToCenter = length( toCenter );

	uint2 iFragCoordHalf = uint2( gl_FragCoord.xy * 0.5f );
	if( distToCenter < p_radius.x )
		discard_fragment();
	else if( (iFragCoordHalf.x & 0x01u) == (iFragCoordHalf.y & 0x01u) && distToCenter < p_radius.y )
		discard_fragment();
	else if( !((iFragCoordHalf.x & 0x01u) != 0u || (iFragCoordHalf.y & 0x01u) != 0u) && distToCenter < p_radius.z )
		discard_fragment();
	else if( !((iFragCoordHalf.x & 0x03u) != 0u || (iFragCoordHalf.y & 0x03u) != 0u) )
		discard_fragment();

	return float4( 0, 0, 0, 0 );
}
