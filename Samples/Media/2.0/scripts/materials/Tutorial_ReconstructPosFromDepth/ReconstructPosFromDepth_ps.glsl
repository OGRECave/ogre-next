#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2D depthTexture;
vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
    vec3 cameraDir;
} inPs;

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform vec2 projectionParams;
vulkan( }; )

vulkan_layout( location = 0 )
out vec3 fragColour;

void main()
{
	float fDepth = texture( vkSampler2D( depthTexture, samplerState ), inPs.uv0 ).x;
	
	float linearDepth = projectionParams.y / (fDepth - projectionParams.x);
	
	vec3 viewSpacePosition = inPs.cameraDir * linearDepth;
	
	//For this next line to work cameraPos would have to be an uniform in world space, and cameraDir
	//would have to be sent using the compositor setting "quad_normals camera_far_corners_world_space_centered"
	//vec3 worldSpacePosition = inPs.cameraDir * linearDepth + cameraPos;

	//Scale the values down to get some useful colour debugging in the valid range
	fragColour = vec3( viewSpacePosition.xy / 5, -viewSpacePosition.z / 80 );
    //fragColour = linearDepth.xxx;
}
