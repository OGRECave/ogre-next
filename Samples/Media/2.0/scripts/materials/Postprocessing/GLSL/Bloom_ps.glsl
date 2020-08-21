#version ogre_glsl_ver_330

//-------------------------------
// Blends using weights the blurred image with the sharp one
//-------------------------------

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan_layout( ogre_t1 ) uniform texture2D Blur1;

vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform float OriginalImageWeight;
	uniform float BlurWeight;
vulkan( }; )

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

void main()
{
	vec4 sharp	= texture( vkSampler2D( RT, samplerState ), inPs.uv0 );
	vec4 blur	= texture( vkSampler2D( Blur1, samplerState ), inPs.uv0 );
    
	fragColour = ( (blur * BlurWeight) + (sharp * OriginalImageWeight) );
}
