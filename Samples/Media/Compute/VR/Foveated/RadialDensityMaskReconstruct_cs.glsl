@insertpiece( SetCrossPlatformSettings )

@piece( CustomGlslExtensions )
	#extension GL_ARB_shader_group_vote: require
@end

#define short2 ivec2
#define ushort2 uvec2
#define OGRE_imageWrite2D4( outImage, iuv, value ) imageStore( outImage, int2( iuv ), value )

layout (@insertpiece(uav0_pf_type)) uniform restrict writeonly @insertpiece(uav0_data_type) dstTex;

uniform sampler2D srcTex;

layout( local_size_x = @value( threads_per_group_x ),
		local_size_y = @value( threads_per_group_y ),
		local_size_z = @value( threads_per_group_z ) ) in;

@insertpiece( HeaderCS )

void main()
{
	@insertpiece( BodyCS )
}
