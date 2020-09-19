#version ogre_glsl_ver_330

vulkan_layout( OGRE_POSITION ) in vec3 vertex;
vulkan_layout( OGRE_NORMAL ) in vec3 normal;

vulkan( layout( ogre_P0 ) uniform Params { )
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
#ifdef OGRE_NO_REVERSE_DEPTH
	gl_Position = (worldViewProj * vec4( vertex, 1.0 )).xyww;
#else
	gl_Position.xyw = (worldViewProj * vec4( vertex, 1.0 )).xyw;
	gl_Position.z = 0.0;
#endif
    outVs.cameraDir.xyz = normal.xyz;
}
