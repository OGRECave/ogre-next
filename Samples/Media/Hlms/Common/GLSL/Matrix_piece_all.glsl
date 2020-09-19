
@property( !hlms_readonly_is_tex )

@piece( Common_Matrix_DeclUnpackMatrix4x4 )
#define UNPACK_MAT4( matrixBuf, pixelIdx ) mat4( matrixBuf[(pixelIdx) << 2u], matrixBuf[((pixelIdx) << 2u)+1u], matrixBuf[((pixelIdx) << 2u)+2u], matrixBuf[((pixelIdx) << 2u)+3u] )
@end

@piece( Common_Matrix_DeclUnpackMatrix4x3 )
#define UNPACK_MAT4x3( matrixBuf, pixelIdx ) mat3x4( matrixBuf[(pixelIdx) << 2u], matrixBuf[((pixelIdx) << 2u)+1u], matrixBuf[((pixelIdx) << 2u)+2u] )
@end

@piece( Common_Matrix_DeclLoadOgreFloat4x3 )
#define loadOgreFloat4x3( matrixBuf, pixelIdx ) mat3x4( matrixBuf[(pixelIdx)], matrixBuf[(pixelIdx)+1u], matrixBuf[(pixelIdx)+2u] )
#define makeOgreFloat4x3( row0, row1, row2 ) mat3x4( row0, row1, row2 )
@end

@else /// hlms_readonly_is_tex

@property( GL_ARB_texture_buffer_range )

@piece( Common_Matrix_DeclUnpackMatrix4x4 )
mat4 UNPACK_MAT4( samplerBuffer matrixBuf, uint pixelIdx )
{
	vec4 row0 = texelFetch( matrixBuf, int((pixelIdx) << 2u) );
	vec4 row1 = texelFetch( matrixBuf, int(((pixelIdx) << 2u) + 1u) );
	vec4 row2 = texelFetch( matrixBuf, int(((pixelIdx) << 2u) + 2u) );
	vec4 row3 = texelFetch( matrixBuf, int(((pixelIdx) << 2u) + 3u) );
    return mat4( row0, row1, row2, row3 );
}
@end

@piece( Common_Matrix_DeclUnpackMatrix4x3 )
mat3x4 UNPACK_MAT4x3( samplerBuffer matrixBuf, uint pixelIdx )
{
	vec4 row0 = texelFetch( matrixBuf, int((pixelIdx) << 2u) );
	vec4 row1 = texelFetch( matrixBuf, int(((pixelIdx) << 2u) + 1u) );
	vec4 row2 = texelFetch( matrixBuf, int(((pixelIdx) << 2u) + 2u) );
	return mat3x4( row0, row1, row2 );
}
@end

@piece( Common_Matrix_DeclLoadOgreFloat4x3 )
ogre_float4x3 loadOgreFloat4x3( samplerBuffer matrixBuf, uint offsetIdx )
{
	mat3x4 retVal;
	retVal[0] = texelFetch( matrixBuf, int(offsetIdx) );
	retVal[1] = texelFetch( matrixBuf, int(offsetIdx + 1u) );
	retVal[2] = texelFetch( matrixBuf, int(offsetIdx + 2u) );
	return retVal;
}

#define makeOgreFloat4x3( row0, row1, row2 ) mat3x4( row0, row1, row2 )
@end

@else /// !GL_ARB_texture_buffer_range

@piece( Common_Matrix_DeclUnpackMatrix4x4 )
mat4 UNPACK_MAT4( in sampler2D matrixBuf, in uint pixelIdx )
{
    ivec2 pos0 = ivec2(int(((pixelIdx) << 2u) & 2047u), int(((pixelIdx) << 2u) >> 11u));
    ivec2 pos1 = ivec2(int((((pixelIdx) << 2u) + 1u) & 2047u), int((((pixelIdx) << 2u) + 1u) >> 11u));
    ivec2 pos2 = ivec2(int((((pixelIdx) << 2u) + 2u) & 2047u), int((((pixelIdx) << 2u) + 2u) >> 11u));
    ivec2 pos3 = ivec2(int((((pixelIdx) << 2u) + 3u) & 2047u), int((((pixelIdx) << 2u) + 3u) >> 11u));
    vec4 row0 = texelFetch( matrixBuf, pos0, 0 );
    vec4 row1 = texelFetch( matrixBuf, pos1, 0 );
    vec4 row2 = texelFetch( matrixBuf, pos2, 0 );
    vec4 row3 = texelFetch( matrixBuf, pos3, 0 );
    return mat4( row0, row1, row2, row3 );
}
@end

@piece( Common_Matrix_DeclUnpackMatrix4x3 )
mat3x4 UNPACK_MAT4x3( in sampler2D matrixBuf, in uint pixelIdx )
{
    ivec2 pos0 = ivec2(int(((pixelIdx) << 2u) & 2047u), int(((pixelIdx) << 2u) >> 11u));
    ivec2 pos1 = ivec2(int((((pixelIdx) << 2u) + 1u) & 2047u), int((((pixelIdx) << 2u) + 1u) >> 11u));
    ivec2 pos2 = ivec2(int((((pixelIdx) << 2u) + 2u) & 2047u), int((((pixelIdx) << 2u) + 2u) >> 11u));
    vec4 row0 = texelFetch( matrixBuf, pos0, 0 );
    vec4 row1 = texelFetch( matrixBuf, pos1, 0 );
    vec4 row2 = texelFetch( matrixBuf, pos2, 0 );
    return mat3x4( row0, row1, row2 );
}
@end

@piece( Common_Matrix_DeclLoadOgreFloat4x3 )
ogre_float4x3 loadOgreFloat4x3( samplerBuffer matrixBuf, uint offsetIdx )
{
    ivec2 pos0 = ivec2(int(offsetIdx & 2047u), int(offsetIdx >> 11u));
    ivec2 pos1 = ivec2(int((offsetIdx + 1u) & 2047u), int((offsetIdx + 1u) >> 11u));
    ivec2 pos2 = ivec2(int((offsetIdx + 2u) & 2047u), int((offsetIdx + 2u) >> 11u));

	mat3x4 retVal;
	retVal[0] = texelFetch( matrixBuf, pos0, 0 );
	retVal[1] = texelFetch( matrixBuf, pos1, 0 );
	retVal[2] = texelFetch( matrixBuf, pos2, 0 );
	return retVal;
}

#define makeOgreFloat4x3( row0, row1, row2 ) mat3x4( row0, row1, row2 )
@end

@end /// GL_ARB_texture_buffer_range

@end /// hlms_readonly_is_tex
