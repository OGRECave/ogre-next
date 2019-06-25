// Heavily modified version based on voxelization shader written by mattatz
//	https://github.com/mattatz/unity-voxel
// Original work Copyright (c) 2018 mattatz under MIT license.
//
// mattatz's work is a full voxelization algorithm.
// We only need to voxelize the shell (aka the contour),
// not to fill the entire voxel.
//
// Adapted for Ogre and for use for Voxel Cone Tracing by
// Matias N. Goldberg Copyright (c) 2019

@insertpiece( SetCrossPlatformSettings )

@piece( CustomGlslExtensions )
	#extension GL_ARB_shader_group_vote: require
@end

#define OGRE_imageLoad3D( inImage, iuv ) imageLoad( inImage, int3( iuv ) )
#define OGRE_imageWrite3D1( outImage, iuv, value ) imageStore( outImage, int3( iuv ), value )
#define OGRE_imageWrite3D4( outImage, iuv, value ) imageStore( outImage, int3( iuv ), value )

@insertpiece( PreBindingsHeaderCS )

layout(std430, binding = 0) readonly restrict buffer vertexBufferLayout
{
	Vertex vertexBuffer[];
};
layout(std430, binding = 1) readonly restrict buffer indexBufferLayout
{
	uint indexBuffer[];
};

layout (@insertpiece(uav2_pf_type)) uniform restrict image3D voxelAlbedoTex;
layout (@insertpiece(uav3_pf_type)) uniform restrict image3D voxelNormalTex;
layout (@insertpiece(uav4_pf_type)) uniform restrict image3D voxelEmissiveTex;
layout (@insertpiece(uav5_pf_type)) uniform restrict uimage3D voxelAccumVal;

layout( local_size_x = @value( threads_per_group_x ),
		local_size_y = @value( threads_per_group_y ),
		local_size_z = @value( threads_per_group_z ) ) in;

//layout (rgba8) uniform restrict writeonly image3D voxelAlbedoTex;
//layout (rgb10_a2) uniform restrict writeonly image3D voxelNormalTex;
//layout (rgba8) uniform restrict writeonly image3D voxelEmissiveTex;

//layout( local_size_x = 4,
//		local_size_y = 4,
//		local_size_z = 4 ) in;


uniform samplerBuffer instanceBuffer;

@property( has_diffuse_tex || has_emissive_tex )
	uniform sampler2DArray texturePool;
@end

@insertpiece( HeaderCS )

uniform uint2 instanceStart_instanceEnd;
uniform float3 voxelOrigin;
uniform float3 voxelCellSize;

#define p_instanceStart instanceStart_instanceEnd.x
#define p_instanceEnd instanceStart_instanceEnd.y
#define p_voxelOrigin voxelOrigin
#define p_voxelCellSize voxelCellSize

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

void main()
{
	@insertpiece( BodyCS )
}
