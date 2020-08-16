#version ogre_glsl_ver_330

#extension GL_ARB_shader_viewport_layer_array : require

#ifdef VULKAN
	#define gl_VertexID gl_VertexIndex
#endif

vulkan_layout( OGRE_POSITION ) in vec2 vertex;

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform float ogreBaseVertex;
	uniform vec2 rsDepthRange;
	uniform mat4 worldViewProj;
vulkan( }; )

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	gl_Position.xy = (worldViewProj * vec4( vertex.xy, 0, 1.0f )).xy;
	gl_Position.z = rsDepthRange.x;
	gl_Position.w = 1.0f;
	gl_ViewportIndex = (gl_VertexID - ogreBaseVertex) >= (3 * 4) ? 1 : 0;
}
