#version ogre_glsl_ver_330
#extension GL_ARB_shader_viewport_layer_array : require

vulkan_layout( OGRE_POSITION )	in vec2 vertex;
vulkan_layout( OGRE_NORMAL )	in vec3 normal;

in int gl_InstanceID;

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform float ogreBaseVertex;
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

	// TODO Move the second instance aside. There must be a way to make this more clean. But how?
	gl_Position.x += 3.5f*gl_InstanceID;
    
	gl_Position.z = rsDepthRange.y;
	gl_Position.w = 1.0f;
	outVs.cameraDir.xyz	= normal.xyz;

	gl_ViewportIndex = (gl_VertexID - ogreBaseVertex) >= 2 ? 1 : 0;
}
