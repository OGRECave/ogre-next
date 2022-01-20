@piece( DeclQuat_xAxis )
vec3 xAxis( vec4 qQuat )
{
	float fTy  = 2.0 * qQuat.y;
	float fTz  = 2.0 * qQuat.z;
	float fTwy = fTy * qQuat.w;
	float fTwz = fTz * qQuat.w;
	float fTxy = fTy * qQuat.x;
	float fTxz = fTz * qQuat.x;
	float fTyy = fTy * qQuat.y;
	float fTzz = fTz * qQuat.z;

	return vec3( 1.0-(fTyy+fTzz), fTxy+fTwz, fTxz-fTwy );
}

@property( !hlms_high_quality )
	half3 xAxis( half4 qQuat )
	{
		half fTy  = half( 2.0 ) * qQuat.y;
		half fTz  = half( 2.0 ) * qQuat.z;
		half fTwy = fTy * qQuat.w;
		half fTwz = fTz * qQuat.w;
		half fTxy = fTy * qQuat.x;
		half fTxz = fTz * qQuat.x;
		half fTyy = fTy * qQuat.y;
		half fTzz = fTz * qQuat.z;

		return half3( 1.0-(fTyy+fTzz), fTxy+fTwz, fTxz-fTwy );
	}
@end
@end

@piece( DeclQuat_yAxis )
vec3 yAxis( vec4 qQuat )
{
	float fTx  = 2.0 * qQuat.x;
	float fTy  = 2.0 * qQuat.y;
	float fTz  = 2.0 * qQuat.z;
	float fTwx = fTx * qQuat.w;
	float fTwz = fTz * qQuat.w;
	float fTxx = fTx * qQuat.x;
	float fTxy = fTy * qQuat.x;
	float fTyz = fTz * qQuat.y;
	float fTzz = fTz * qQuat.z;

	return vec3( fTxy-fTwz, 1.0-(fTxx+fTzz), fTyz+fTwx );
}

@property( !hlms_high_quality )
	half3 yAxis( half4 qQuat )
	{
		half fTx  = half( 2.0 ) * qQuat.x;
		half fTy  = half( 2.0 ) * qQuat.y;
		half fTz  = half( 2.0 ) * qQuat.z;
		half fTwx = fTx * qQuat.w;
		half fTwz = fTz * qQuat.w;
		half fTxx = fTx * qQuat.x;
		half fTxy = fTy * qQuat.x;
		half fTyz = fTz * qQuat.y;
		half fTzz = fTz * qQuat.z;

		return half3( fTxy-fTwz, 1.0-(fTxx+fTzz), fTyz+fTwx );
	}
@end
@end

@piece( DeclQuat_zAxis )
vec3 zAxis( vec4 qQuat )
{
	float fTx  = 2.0 * qQuat.x;
	float fTy  = 2.0 * qQuat.y;
	float fTz  = 2.0 * qQuat.z;
	float fTwx = fTx * qQuat.w;
	float fTwy = fTy * qQuat.w;
	float fTxx = fTx * qQuat.x;
	float fTxz = fTz * qQuat.x;
	float fTyy = fTy * qQuat.y;
	float fTyz = fTz * qQuat.y;

	return vec3( fTxz+fTwy, fTyz-fTwx, 1.0-(fTxx+fTyy) );
}

@property( !hlms_high_quality )
	half3 zAxis( half4 qQuat )
	{
		half fTx  = half( 2.0 ) * qQuat.x;
		half fTy  = half( 2.0 ) * qQuat.y;
		half fTz  = half( 2.0 ) * qQuat.z;
		half fTwx = fTx * qQuat.w;
		half fTwy = fTy * qQuat.w;
		half fTxx = fTx * qQuat.x;
		half fTxz = fTz * qQuat.x;
		half fTyy = fTy * qQuat.y;
		half fTyz = fTz * qQuat.y;

		return half3( fTxz+fTwy, fTyz-fTwx, 1.0-(fTxx+fTyy) );
	}
@end
@end

@piece( DeclQuat_AllAxis )
@insertpiece( DeclQuat_xAxis )
@insertpiece( DeclQuat_yAxis )
@insertpiece( DeclQuat_zAxis )
@end
