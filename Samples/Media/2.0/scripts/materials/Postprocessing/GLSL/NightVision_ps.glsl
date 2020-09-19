#version ogre_glsl_ver_330

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan_layout( ogre_t1 ) uniform texture3D noiseVol;

vulkan( layout( ogre_s0 ) uniform sampler samplerState0 );
vulkan( layout( ogre_s1 ) uniform sampler samplerState1 );

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform vec4 lum;
	uniform float time;
vulkan( }; )

void main()
{
	vec4 oC;
	oC = texture( vkSampler2D( RT, samplerState0 ), inPs.uv0 );
	
	//obtain luminence value
	oC = vec4(dot(oC,lum));
	
	//add some random noise
	oC += 0.2 *(texture( vkSampler3D( noiseVol, samplerState1 ), vec3(inPs.uv0*5,time) ))- 0.05;
	
	//add lens circle effect
	//(could be optimised by using texture)
	float dist = distance(inPs.uv0, vec2(0.5,0.5));
	oC *= smoothstep(0.5,0.45,dist);
	
	//add rb to the brightest pixels
	oC.rb = vec2(max(oC.r - 0.75, 0)*4);
	
	fragColour = oC;
}
