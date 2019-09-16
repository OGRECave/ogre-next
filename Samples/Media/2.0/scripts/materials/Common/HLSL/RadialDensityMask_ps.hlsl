
#define p_leftEyeCenter			leftEyeCenter_rightEyeCenter.xy
#define p_rightEyeCenter		leftEyeCenter_rightEyeCenter.zw
#define p_rightEyeStart			rightEyeStart_radius.x
#define p_radius				rightEyeStart_radius.yzw
#define p_invBlockResolution	invBlockResolution

float4 main
(
	float4 gl_FragCoord : SV_Position,

	uniform float4 rightEyeStart_radius,
	uniform float4 leftEyeCenter_rightEyeCenter,
	uniform float2 invBlockResolution
) : SV_Target
{
	float2 eyeCenter = gl_FragCoord.x >= p_rightEyeStart ? p_rightEyeCenter : p_leftEyeCenter;

	//We must work in blocks so the reconstruction filter can work properly
	float2 toCenter = trunc(gl_FragCoord.xy * 0.125f) * p_invBlockResolution.xy - eyeCenter;
	toCenter.x *= 2.0f; //Twice because of stereo (each eye is half the size of the full res)
	float distToCenter = length( toCenter );

	uint2 iFragCoordHalf = uint2( gl_FragCoord.xy * 0.5f );
	if( distToCenter < p_radius.x )
		discard;
	else if( (iFragCoordHalf.x & 0x01u) == (iFragCoordHalf.y & 0x01u) && distToCenter < p_radius.y )
		discard;
	else if( !((iFragCoordHalf.x & 0x01u) != 0u || (iFragCoordHalf.y & 0x01u) != 0u) && distToCenter < p_radius.z )
		discard;
	else if( !((iFragCoordHalf.x & 0x03u) != 0u || (iFragCoordHalf.y & 0x03u) != 0u) )
		discard;

	return float4( 0, 0, 0, 0 );
}
