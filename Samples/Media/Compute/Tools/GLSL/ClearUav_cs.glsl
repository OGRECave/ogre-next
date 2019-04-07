#version 430

layout (@insertpiece(uav0_pf_type)) uniform restrict writeonly @insertpiece(uav0_data_type) dstTex;

@property( !uav0_is_integer )
    uniform vec4 fClearValue;
    #define clearValue fClearValue
@else
    @property( !uav0_is_signed )
        uniform uvec4 uClearValue;
        #define clearValue uClearValue
    @else
        uniform ivec4 iClearValue;
        #define clearValue iClearValue
    @end
@end

layout( local_size_x = @value( threads_per_group_x ),
        local_size_y = @value( threads_per_group_y ),
        local_size_z = @value( threads_per_group_z ) ) in;

void main()
{
@property( uav0_texture_type == TextureTypes_Type3D || uav0_texture_type == TextureTypes_Type2DArray || uav0_texture_type == TextureTypes_TypeCube || uav0_texture_type == TextureTypes_TypeCubeArray )
    imageStore( dstTex, ivec3(gl_GlobalInvocationID.xyz), clearValue );
@end
@property( uav0_texture_type == TextureTypes_Type2D || uav0_texture_type == TextureTypes_Type1DArray )
    imageStore( dstTex, ivec2(gl_GlobalInvocationID.xy), clearValue );
@end
@property( uav0_texture_type == TextureTypes_Type1D )
    imageStore( dstTex, int(gl_GlobalInvocationID.x), clearValue );
@end
}
