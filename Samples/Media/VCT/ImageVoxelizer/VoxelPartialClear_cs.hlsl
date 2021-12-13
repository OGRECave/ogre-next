// Translates a 3D textures by an UVW offset
// Matias N. Goldberg Copyright (c) 2021-present

@insertpiece( SetCrossPlatformSettings )

@insertpiece( DeclUavCrossPlatform )

@insertpiece( PreBindingsHeaderCS )

RWTexture3D<@insertpiece(uav0_pf_type)>	dstTex0 : register(u0);
RWTexture3D<@insertpiece(uav1_pf_type)>	dstTex1 : register(u1);
RWTexture3D<@insertpiece(uav2_pf_type)>	dstTex2 : register(u2);

@insertpiece( HeaderCS )

uniform uint3 startOffset;
uniform uint3 pixelsToClear;

#define p_startOffset startOffset
#define p_pixelsToClear pixelsToClear

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main( uint3 gl_GlobalInvocationID : SV_DispatchThreadId )
{
	@insertpiece( BodyCS )
}
