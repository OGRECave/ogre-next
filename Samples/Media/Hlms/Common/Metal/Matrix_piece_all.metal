@piece( Common_Matrix_DeclUnpackMatrix4x4 )
inline float4x4 UNPACK_MAT4( device const float4 *matrixBuf, uint pixelIdx )
{
	float4 row0 = matrixBuf[(pixelIdx << 2u)];
	float4 row1 = matrixBuf[(pixelIdx << 2u) + 1u];
	float4 row2 = matrixBuf[(pixelIdx << 2u) + 2u];
	float4 row3 = matrixBuf[(pixelIdx << 2u) + 3u];
	return float4x4( row0, row1, row2, row3 );
}
@end

@piece( Common_Matrix_DeclUnpackMatrix4x3 )
inline float3x4 UNPACK_MAT4x3( device const float4 *matrixBuf, uint pixelIdx )
{
	float4 row0 = matrixBuf[(pixelIdx << 2u)];
	float4 row1 = matrixBuf[(pixelIdx << 2u) + 1u];
	float4 row2 = matrixBuf[(pixelIdx << 2u) + 2u];
	return float3x4( row0, row1, row2 );
}
@end

@piece( Common_Matrix_DeclLoadOgreFloat4x3 )
ogre_float4x3 loadOgreFloat4x3( device const float4 *matrixBuf, uint offsetIdx )
{
	float4 row0 = matrixBuf[offsetIdx];
	float4 row1 = matrixBuf[offsetIdx + 1u];
	float4 row2 = matrixBuf[offsetIdx + 2u];
	return float3x4( row0, row1, row2 );
}

#define makeOgreFloat4x3( row0, row1, row2 ) float3x4( row0, row1, row2 )
@end
