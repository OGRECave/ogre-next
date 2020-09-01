@insertpiece( SetCrossPlatformSettings )

@insertpiece( PreBindingsHeaderCS )

@property( syntax == glsl )
	#define ogre_U0 binding = 0
@end

layout( std430, ogre_U0 ) restrict buffer instanceBufferLayout
{
	InstanceBuffer instanceBuffer[];
};

layout( local_size_x = @value( threads_per_group_x ),
        local_size_y = @value( threads_per_group_y ),
        local_size_z = @value( threads_per_group_z ) ) in;

@property( syntax == glsl )
	ReadOnlyBufferF( 1, AabbBuffer, inMeshAabb );
@else
	ReadOnlyBufferF( 0, AabbBuffer, inMeshAabb );
@end

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
