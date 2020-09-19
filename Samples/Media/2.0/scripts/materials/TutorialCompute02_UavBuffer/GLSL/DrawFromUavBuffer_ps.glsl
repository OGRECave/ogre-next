#version ogre_glsl_ver_430

/*
Ogre's prefab root layouts don't include UAVs.

Create a custom root layout which is compatible with our standard prefab
(see GpuProgram::setPrefabRootLayout) on set idx 0 from our vertex shader,
and place the UAVs in set idx 1

## ROOT LAYOUT BEGIN
{
	"0" :
	{
		"has_params" : ["vs", "ps"],
		"samplers" : [0, 4],
		"textures" : [0, 4]
	},

	"1" :
	{
		"baked" : true,
		"uav_buffers" : [1, 2]
	}
}
## ROOT LAYOUT END
*/

vulkan_layout( location = 0 )
out vec4 fragColour;

#ifdef VULKAN
layout(std430, ogre_U1)
#else
layout(std430, binding = 1)
#endif
buffer PixelBuffer
{
    readonly uint data[];
} pixelBuffer;

in vec4 gl_FragCoord;

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform uvec2 texResolution;
vulkan( }; )

void main()
{
    uint idx = uint(gl_FragCoord.y) * texResolution.x + uint(gl_FragCoord.x);
    fragColour = unpackUnorm4x8( pixelBuffer.data[idx] );
}
