#version ogre_glsl_ver_330

vulkan_layout( OGRE_POSITION ) in vec3 vertex;

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform mat4 worldViewProj;
	uniform vec4 worldScaledMatrix[3];
	uniform vec3 probeCameraPosScaled;
vulkan( }; )

out gl_PerVertex
{
	vec4 gl_Position;
};

vulkan_layout( location = 0 )
out block
{
	vec3 posLS;
} outVs;

void main()
{
	gl_Position = worldViewProj * vec4( vertex.xyz, 1.0 );
	outVs.posLS.x = dot( worldScaledMatrix[0], vec4( vertex.xyz, 1.0 ) );
	outVs.posLS.y = dot( worldScaledMatrix[1], vec4( vertex.xyz, 1.0 ) );
	outVs.posLS.z = dot( worldScaledMatrix[2], vec4( vertex.xyz, 1.0 ) );
	outVs.posLS = outVs.posLS - probeCameraPosScaled;
	outVs.posLS.z = -outVs.posLS.z; //Left handed
}
