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

#define OGRE_imageLoad3D( inImage, iuv ) inImage.Load( int3( iuv ) )
#define OGRE_imageWrite3D1( outImage, iuv, value ) outImage[int3( iuv )] = value.x
#define OGRE_imageWrite3D4( outImage, iuv, value ) outImage[int3( iuv )] = value

@insertpiece( PreBindingsHeaderCS )

StructuredBuffer<Vertex> vertexBuffer	: register(u0);
StructuredBuffer<uint> indexBuffer		: register(u1);

RWTexture3D<@insertpiece(uav2_pf_type)> voxelAlbedoTex		: register(u2);
RWTexture3D<@insertpiece(uav3_pf_type)> voxelNormalTex		: register(u3);
RWTexture3D<@insertpiece(uav4_pf_type)> voxelEmissiveTex	: register(u4);
RWTexture3D<@insertpiece(uav5_pf_type)> voxelAccumVal		: register(u5);

Buffer<float4> instanceBuffer : register(t0);

@pset( samplerRegister, 1 )
@pset( texRegister, 1 )

@property( has_diffuse_tex || emissive_is_diffuse_tex )
	@property( has_diffuse_tex || !emissive_sampler_separate )
		SamplerState	diffuseSampler	: register(@counter(samplerRegister));
	@end
	Texture2DArray		diffuseTex		: register(@counter(texRegister));
@end
@property( has_emissive_tex )
	@property( !emissive_sampler_separate )
		#define emissiveSampler diffuseSampler
	@else
		SamplerState	emissiveSampler	: register(@counter(samplerRegister));
	@end
	Texture2DArray		emissiveTex		: register(@counter(texRegister));
@end

@insertpiece( HeaderCS )

uniform uint2 instanceStart_instanceEnd;
uniform float3 voxelOrigin;
uniform float3 voxelCellSize;

#define p_instanceStart instanceStart_instanceEnd.x
#define p_instanceEnd instanceStart_instanceEnd.y
#define p_numInstances numInstances
#define p_voxelOrigin voxelOrigin
#define p_voxelCellSize voxelCellSize

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
	uint3 gl_GlobalInvocationID		: SV_DispatchThreadId,
	uint3 gl_WorkGroupID			: SV_GroupID,
	uint gl_LocalInvocationIndex	: SV_GroupIndex
)
{
	@insertpiece( BodyCS )
}
