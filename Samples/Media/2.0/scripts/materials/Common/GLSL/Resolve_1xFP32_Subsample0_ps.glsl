#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2DMS tex;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

vulkan_layout( location = 0 )
out float fragColour;

in vec4 gl_FragCoord;

void main()
{
	fragColour = texelFetch( tex, ivec2( gl_FragCoord.xy ), 0 ).x;
}
