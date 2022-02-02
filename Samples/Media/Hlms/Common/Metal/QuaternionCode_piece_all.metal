@piece( DeclQuat_xAxis )
	@property( precision_mode != relaxed )
		float3 xAxis( float4 qQuat )
		{
			float fTy  = 2.0 * qQuat.y;
			float fTz  = 2.0 * qQuat.z;
			float fTwy = fTy * qQuat.w;
			float fTwz = fTz * qQuat.w;
			float fTxy = fTy * qQuat.x;
			float fTxz = fTz * qQuat.x;
			float fTyy = fTy * qQuat.y;
			float fTzz = fTz * qQuat.z;

			return float3( 1.0-(fTyy+fTzz), fTxy+fTwz, fTxz-fTwy );
		}
	@end
	@property( precision_mode != full32 )
		midf3 xAxis( midf4 qQuat )
		{
			midf fTy  = _h( 2.0 ) * qQuat.y;
			midf fTz  = _h( 2.0 ) * qQuat.z;
			midf fTwy = fTy * qQuat.w;
			midf fTwz = fTz * qQuat.w;
			midf fTxy = fTy * qQuat.x;
			midf fTxz = fTz * qQuat.x;
			midf fTyy = fTy * qQuat.y;
			midf fTzz = fTz * qQuat.z;

			return midf3_c( _h( 1.0 )-(fTyy+fTzz), fTxy+fTwz, fTxz-fTwy );
		}
	@end
@end

@piece( DeclQuat_yAxis )
	@property( precision_mode != relaxed )
		float3 yAxis( float4 qQuat )
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

			return float3( fTxy-fTwz, 1.0-(fTxx+fTzz), fTyz+fTwx );
		}
	@end
	@property( precision_mode != full32 )
		midf3 yAxis( midf4 qQuat )
		{
			midf fTx  = _h( 2.0 ) * qQuat.x;
			midf fTy  = _h( 2.0 ) * qQuat.y;
			midf fTz  = _h( 2.0 ) * qQuat.z;
			midf fTwx = fTx * qQuat.w;
			midf fTwz = fTz * qQuat.w;
			midf fTxx = fTx * qQuat.x;
			midf fTxy = fTy * qQuat.x;
			midf fTyz = fTz * qQuat.y;
			midf fTzz = fTz * qQuat.z;

			return midf3_c( fTxy-fTwz, _h( 1.0 )-(fTxx+fTzz), fTyz+fTwx );
		}
	@end
@end

@piece( DeclQuat_zAxis )
	@property( precision_mode != relaxed )
		float3 zAxis( float4 qQuat )
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

			return float3( fTxz+fTwy, fTyz-fTwx, 1.0-(fTxx+fTyy) );
		}
	@end
	@property( precision_mode != full32 )
		midf3 zAxis( midf4 qQuat )
		{
			midf fTx  = _h( 2.0 ) * qQuat.x;
			midf fTy  = _h( 2.0 ) * qQuat.y;
			midf fTz  = _h( 2.0 ) * qQuat.z;
			midf fTwx = fTx * qQuat.w;
			midf fTwy = fTy * qQuat.w;
			midf fTxx = fTx * qQuat.x;
			midf fTxz = fTz * qQuat.x;
			midf fTyy = fTy * qQuat.y;
			midf fTyz = fTz * qQuat.y;

			return midf3_c( fTxz+fTwy, fTyz-fTwx, _h( 1.0 )-(fTxx+fTyy) );
		}
	@end
@end

@piece( DeclQuat_AllAxis )
@insertpiece( DeclQuat_xAxis )
@insertpiece( DeclQuat_yAxis )
@insertpiece( DeclQuat_zAxis )
@end
