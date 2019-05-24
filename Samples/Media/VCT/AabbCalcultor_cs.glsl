#version 430

#define float2 vec2
#define float3 vec3
#define float4 vec4

#define int3 ivec3

#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4

#define ogre_float4x3 mat3x4

#define mul( x, y ) ((x) * (y))

@insertpiece( PreBindingsHeaderCS )

layout(std430, binding = 0) buffer vertexBufferLayout
{
	Vertex vertexBuffer[];
};
layout(std430, binding = 1) buffer indexBufferLayout
{
	uint indexBuffer[];
};
layout(std430, binding = 2) buffer instanceBufferLayout
{
	InstanceBuffer instanceBuffer[];
};

layout( local_size_x = @value( threads_per_group_x ),
		local_size_y = @value( threads_per_group_y ),
		local_size_z = @value( threads_per_group_z ) ) in;

@insertpiece( HeaderCS )

uniform uint2 instanceStart_instanceEnd;

#define p_instanceStart instanceStart_instanceEnd.x
#define p_instanceEnd instanceStart_instanceEnd.y

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

void main()
{
	@insertpiece( BodyCS )
}
