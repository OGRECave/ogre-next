@insertpiece(uav0_data_type) dstTex : register(u0);

@property( !uav0_is_integer )
    uniform float4 fClearValue;
    #define clearValue fClearValue
@else
    @property( !uav0_is_signed )
        uniform uint4 uClearValue;
        #define clearValue uClearValue
    @else
        uniform int4 iClearValue;
        #define clearValue iClearValue
    @end
@end

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main( uint3 gl_GlobalInvocationID : SV_DispatchThreadId )
{
@property( uav0_texture_type == TextureTypes_Type3D || uav0_texture_type == TextureTypes_Type2DArray || uav0_texture_type == TextureTypes_TypeCube || uav0_texture_type == TextureTypes_TypeCubeArray )
    dstTex[gl_GlobalInvocationID.xyz] = clearValue;
@end
@property( uav0_texture_type == TextureTypes_Type2D || uav0_texture_type == TextureTypes_Type1DArray )
    dstTex[gl_GlobalInvocationID.xy] = clearValue;
@end
@property( uav0_texture_type == TextureTypes_Type1D )
    dstTex[gl_GlobalInvocationID.x] = clearValue;
@end
}
