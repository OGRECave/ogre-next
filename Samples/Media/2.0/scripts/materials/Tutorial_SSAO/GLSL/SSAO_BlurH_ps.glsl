#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2D ssaoTexture;
vulkan_layout( ogre_t1 ) uniform texture2D depthTexture;

vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;


vulkan( layout( ogre_P0 ) uniform Params { )
	uniform vec4 texelSize;
	uniform vec2 projectionParams;
vulkan( }; )

const float offsets[9] = float[9]( -8.0, -6.0, -4.0, -2.0, 0.0, 2.0, 4.0, 6.0, 8.0 );

vulkan_layout( location = 0 )
out float fragColour;

float getLinearDepth(vec2 uv)
{
	float fDepth = texture( vkSampler2D( depthTexture, samplerState ), uv ).x;
	float linearDepth = projectionParams.y / (fDepth - projectionParams.x);
	return linearDepth;
}

void main()
{
	float flDepth = getLinearDepth(inPs.uv0);
	
	float weights = 0.0;
	float result = 0.0;

	for (int i = 0; i < 9; ++i)
	{
		vec2 offset = vec2(texelSize.z*offsets[i], 0.0); //Horizontal sample offsets
		vec2 samplePos = inPs.uv0 + offset;

		float slDepth = getLinearDepth(samplePos);

		float weight = (1.0 / (abs(flDepth - slDepth) + 0.0001)); //Calculate weight using depth

		result += texture( vkSampler2D( ssaoTexture, samplerState ), samplePos ).x*weight;

		weights += weight;
	}
	result /= weights;

	fragColour = result;

	//fragColour = texture(vkSampler2D( ssaoTexture, samplerState ), inPs.uv0).x; //Use this to disable blur
}
