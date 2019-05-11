@insertpiece( SetCrossPlatformSettings )

#define OGRE_imageLoad3D( inImage, iuv ) imageLoad( inImage, int3( iuv ) )
#define OGRE_imageWrite3D4( outImage, iuv, value ) imageStore( outImage, int3( iuv ), value )

uniform sampler3D inLightLowerMip;
layout (@insertpiece(uav0_pf_type)) uniform restrict writeonly image3D outLightHigherMip[3];

layout( local_size_x = @value( threads_per_group_x ),
		local_size_y = @value( threads_per_group_y ),
		local_size_z = @value( threads_per_group_z ) ) in;

//layout( local_size_x = 4,
//		local_size_y = 4,
//		local_size_z = 4 ) in;

uniform int4 lowerMipResolution_higherMipHalfWidth;

#define p_lowerMipResolution lowerMipResolution_higherMipHalfWidth.xyz
#define p_higherMipHalfWidth lowerMipResolution_higherMipHalfWidth.w

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

void main()
{
	@insertpiece( BodyCS )
}
