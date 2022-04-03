// Translates a 3D textures by an UVW offset
// Matias N. Goldberg Copyright (c) 2021-present

@insertpiece( SetCrossPlatformSettings )

@insertpiece( DeclUavCrossPlatform )

@insertpiece( PreBindingsHeaderCS )

@insertpiece( HeaderCS )

struct Params
{
	uniform uint3 startOffset;
	uniform uint3 pixelsToClear;
};

#define p_startOffset p.startOffset
#define p_pixelsToClear p.pixelsToClear

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
kernel void main_metal
(
	texture3d<@insertpiece(uav0_pf_type), access::write> dstTex0	[[texture(UAV_SLOT_START+0)]],
	texture3d<@insertpiece(uav1_pf_type), access::write> dstTex1	[[texture(UAV_SLOT_START+1)]],
	texture3d<@insertpiece(uav2_pf_type), access::write> dstTex2	[[texture(UAV_SLOT_START+2)]],

	constant Params &p	[[buffer(PARAMETER_SLOT)]],

	ushort3 gl_GlobalInvocationID	[[thread_position_in_grid]],
)
{
	@insertpiece( BodyCS )
}
