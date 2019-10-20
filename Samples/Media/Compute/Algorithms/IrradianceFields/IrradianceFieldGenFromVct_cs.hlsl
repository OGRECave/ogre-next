@insertpiece( SetCrossPlatformSettings )
@insertpiece( DeclUavCrossPlatform )

#define ushort2 uint2

@insertpiece( PreBindingsHeaderCS )

Buffer<float4> directionsBuffer	: register(t0);

SamplerState probeSampler		: register(s1);

Texture3D vctProbe				: register(t1);

@property( vct_anisotropic )
	Texture3D vctProbeX	: register(t2);
	Texture3D vctProbeY	: register(t3);
	Texture3D vctProbeZ	: register(t4);
@end

RWTexture2D<@insertpiece(uav0_pf_type)> irradianceField			: register(u0);
RWTexture2D<@insertpiece(uav1_pf_type)> irradianceFieldDepth	: register(u1);

groupshared float4 g_diffuseDepth[@value( threads_per_group_x ) * @value( threads_per_group_y ) * @value( threads_per_group_z )];

@insertpiece( HeaderCS )

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
	uint3 gl_GlobalInvocationID		: SV_DispatchThreadId,
	uint gl_LocalInvocationIndex	: SV_GroupIndex
)
{
	@insertpiece( BodyCS )
}
