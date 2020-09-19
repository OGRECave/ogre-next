#version ogre_glsl_ver_330

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

vulkan_layout( ogre_t0 ) uniform texture2D Image;
vulkan_layout( ogre_t1 ) uniform texture3D Rand;
vulkan_layout( ogre_t2 ) uniform texture3D Noise;

vulkan( layout( ogre_s0 ) uniform sampler samplerState0 );
vulkan( layout( ogre_s1 ) uniform sampler samplerState1 );
vulkan( layout( ogre_s2 ) uniform sampler samplerState2 );

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform float distortionFreq;
	uniform float distortionScale;
	uniform float distortionRoll;
	uniform float interference;
	uniform float frameLimit;
	uniform float frameShape;
	uniform float frameSharpness;
	uniform float time_0_X;
	uniform float sin_time_0_X;
vulkan( }; )

void main()
{
    // Define a frame shape
	//vec2 pos = abs( (inPs.uv0 - 0.5) * 2.0 );
	vec2 pos = abs( (inPs.uv0 - 0.5) * 2.0 );
    float f = (1 - pos.x * pos.x) * (1 - pos.y * pos.y);
    float frame = clamp(frameSharpness * (pow(f, frameShape) - frameLimit), 0.0, 1.0);

    // Interference ... just a texture filled with rand()
	float rand = texture( vkSampler3D( Rand, samplerState1 ),
						  vec3(1.5 * inPs.uv0.xy * 2.0, time_0_X) ).x - 0.2;

    // Some signed noise for the distortion effect
	float noisy = texture( vkSampler3D( Noise, samplerState2 ),
						   vec3(0, 0.5 * pos.y, 0.1 * time_0_X) ).x - 0.5;

    // Repeat a 1 - x^2 (0 < x < 1) curve and roll it with sinus.
    float dst = fract(pos.y * distortionFreq + distortionRoll * sin_time_0_X);
    dst *= (1 - dst);
    // Make sure distortion is highest in the center of the image
    dst /= 1 + distortionScale * abs(pos.y);

    // ... and finally distort
	vec2 inUv = inPs.uv0;
    inUv.x += distortionScale * noisy * dst;
	vec4 image = texture( vkSampler2D( Image, samplerState0 ), inUv );

    // Combine frame, distorted image and interference
    fragColour = frame * (interference * rand + image);
}
