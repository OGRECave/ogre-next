@insertpiece( SetCrossPlatformSettings )
@insertpiece( DeclUavCrossPlatform )

#define ushort2 uvec2

@insertpiece( PreBindingsHeaderCS )

uniform uint probeIdx;
uniform float2 projectionParams;
uniform float3 numProbes;

#define p_probeIdx			probeIdx
#define p_projectionParams	projectionParams
#define p_numProbes			numProbes

uniform samplerCube colourCubemap;
uniform samplerCube depthCubemap;

layout (@insertpiece(uav0_pf_type)) uniform restrict writeonly image2D irradianceField;
layout (@insertpiece(uav1_pf_type)) uniform restrict writeonly image2D irradianceFieldDepth;

layout( local_size_x = @value( threads_per_group_x ),
		local_size_y = @value( threads_per_group_y ),
		local_size_z = @value( threads_per_group_z ) ) in;

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
