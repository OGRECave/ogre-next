#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2D sceneTexture;
vulkan_layout( ogre_t1 ) uniform texture2D distortionTexture;

vulkan( layout( ogre_s0 ) uniform sampler samplerState0 );

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform float u_DistortionStrenght;
vulkan( }; )

vulkan_layout( location = 0 )
out vec4 fragColour;

void main()
{
	//Sample vector from distortion pass
	vec4 dVec = texture( vkSampler2D( distortionTexture, samplerState0 ), inPs.uv0 );
    
	// Distortion texture is in range [0-1] so we will make it to range [-1.0 - 1.0] so we will get left-right vector and up-down vector
    dVec.xy = (dVec.xy - 0.5)*2.0;
    
	//Calculate base coordinate offset using texture color values, alpha and strenght
    vec2 offset = dVec.xy * dVec.w * u_DistortionStrenght;
    
	//We will add more realism by offsetting each color channel separetly for different amount.
	//That is because violet/blue light has shorter wave lenght than red light so they refract differently.
	
    vec2 ofR = offset*0.80; //Red channel refracts 80% of offset
    vec2 ofG = offset*0.90; //Green channel refracts 90% of offset
    vec2 ofB = offset; //Blue channel refracts 100% of offset
    
	//We have to sample three times to get each color channel.
	// Distortion is made by offsetting UV coordinates with each channel offset value.
	float cR = texture( vkSampler2D( sceneTexture, samplerState0 ), inPs.uv0 + ofR ).r;
	float cG = texture( vkSampler2D( sceneTexture, samplerState0 ), inPs.uv0 + ofG ).g;
	float cB = texture( vkSampler2D( sceneTexture, samplerState0 ), inPs.uv0 + ofB ).b;
    
	// Combine colors to result
    fragColour = vec4(cR, cG, cB, 1.0);
}
