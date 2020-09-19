#version ogre_glsl_ver_330

vulkan_layout( OGRE_POSITION )	in vec2 vertex;
vulkan_layout( OGRE_NORMAL )	in vec3 normal;

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform vec2 rsDepthRange;
	uniform mat4 worldViewProj;
vulkan( }; )

out gl_PerVertex
{
	vec4 gl_Position;
};

vulkan_layout( location = 0 )
out block
{
	vec3 cameraDir;
} outVs;

void main()
{
	gl_Position.xy = (worldViewProj * vec4( vertex.xy, 0, 1.0f )).xy;
	gl_Position.z = rsDepthRange.y;
	gl_Position.w = 1.0f;
	outVs.cameraDir.xyz	= normal.xyz;
}
