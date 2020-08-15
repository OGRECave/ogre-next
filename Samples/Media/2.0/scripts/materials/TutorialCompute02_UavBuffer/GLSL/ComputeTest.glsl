@property( syntax != glslvk )
	#version 430
@else
	#version 450
@end

@property( syntax != glslvk )
layout(std430, binding = 0) buffer PixelBuffer
@else
layout(std430, ogre_U0) buffer PixelBuffer
@end
{
    writeonly uint data[];
} pixelBuffer;

layout( local_size_x = @value( threads_per_group_x ),
        local_size_y = @value( threads_per_group_y ),
        local_size_z = @value( threads_per_group_z ) ) in;

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

vulkan( layout( ogre_P0 ) uniform Params { )
//uniform vec3 cameraPos;
uniform uvec2 texResolution;
vulkan( }; )

void main()
{
	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y )
	{
		uint idx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;
		pixelBuffer.data[idx] = packUnorm4x8( vec4( vec2(gl_LocalInvocationID.xy) / 16.0f, 0.0f, 1.0f ) );
	}
}
