@insertpiece( SetCrossPlatformSettings )
#extension GL_ARB_shader_group_vote: require

#define OGRE_imageLoad3D( inImage, iuv ) imageLoad( inImage, int3( iuv ) )
#define OGRE_imageWrite3D4( outImage, iuv, value ) imageStore( outImage, int3( iuv ), value )

@insertpiece( PreBindingsHeaderCS )

uniform sampler3D voxelAlbedoTex;
uniform sampler3D voxelNormalTex;
uniform sampler3D vctProbe;

@property( vct_anisotropic )
    uniform sampler3D vctProbeX;
    uniform sampler3D vctProbeY;
    uniform sampler3D vctProbeZ;
@end

layout (@insertpiece(uav0_pf_type)) uniform restrict writeonly image3D lightVoxel;

layout( local_size_x = @value( threads_per_group_x ),
        local_size_y = @value( threads_per_group_y ),
        local_size_z = @value( threads_per_group_z ) ) in;

uniform float3 voxelCellSize;
uniform float3 invVoxelResolution;
uniform float iterationDampening;
uniform float2 startBias_invStartBias;

#define p_voxelCellSize voxelCellSize
#define p_invVoxelResolution invVoxelResolution
#define p_iterationDampening iterationDampening
#define p_vctStartBias startBias_invStartBias.x
#define p_vctInvStartBias startBias_invStartBias.y

@insertpiece( HeaderCS )

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

void main()
{
    @insertpiece( BodyCS )
}
