#version ogre_glsl_ver_330

vulkan_layout( OGRE_POSITION ) in vec3 vertex;
vulkan_layout( OGRE_NORMAL ) in vec3 normal;
vulkan_layout( OGRE_TEXCOORD0 ) in vec2 uv0;

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
	vec2 uv0;
	vec3 cameraDir;
} outVs;

void main()
{
	gl_Position = worldViewProj * vec4( vertex, 1.0 );
	outVs.uv0.xy		= uv0.xy;
	outVs.cameraDir.xyz	= normal.xyz;
}
