#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2D depthTexture;
vulkan_layout( ogre_t1 ) uniform texture2D gBuf_normals;
vulkan_layout( ogre_t2 ) uniform texture2D noiseTexture;

vulkan( layout( ogre_s0 ) uniform sampler samplerState0 );
vulkan( layout( ogre_s2 ) uniform sampler samplerState2 );

vulkan_layout( location = 0 )
in block
{
   vec2 uv0;
   vec3 cameraDir;
} inPs;

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform vec2 projectionParams;
	uniform float invKernelSize;
	uniform float kernelRadius;
	uniform vec2 noiseScale;
	uniform mat4 projection;

	uniform vec4 sampleDirs[64];
vulkan( }; )

vulkan_layout( location = 0 )
out float fragColour;

vec3 getScreenSpacePos(vec2 uv, vec3 cameraNormal)
{
	float fDepth = texture( vkSampler2D( depthTexture, samplerState0 ), uv ).x;
	float linearDepth = projectionParams.y / (fDepth - projectionParams.x);
	return (cameraNormal * linearDepth);
}

vec3 reconstructNormal(vec3 posInView)
{
	vec3 dNorm = cross(normalize(dFdy(posInView)), normalize(dFdx(posInView)));
	return dNorm;
}

vec3 getRandomVec(vec2 uv)
{
	vec3 randomVec = texture( vkSampler2D( noiseTexture, samplerState2 ), uv * noiseScale ).xyz;
	return randomVec;
}

void main()
{
    vec3 viewPosition = getScreenSpacePos(inPs.uv0, inPs.cameraDir);
    //vec3 viewNormal = reconstructNormal(viewPosition);
	vec3 viewNormal = normalize( texture( vkSampler2D( gBuf_normals, samplerState0 ),inPs.uv0 ).xyz * 2.0 - 1.0 );
    vec3 randomVec = getRandomVec(inPs.uv0);
   
    vec3 tangent = normalize(randomVec - viewNormal * dot(randomVec, viewNormal));
    vec3 bitangent = cross(viewNormal, tangent);
    mat3 TBN = mat3(tangent, bitangent, viewNormal);
   
    float occlusion = 0.0;
    for(int i = 0; i < 8; ++i)
    {
		for(int a = 0; a < 8; ++a)
		{
			vec3 sNoise = sampleDirs[(a << 3u) + i].xyz;
         
			// get sample position
			vec3 oSample = TBN * sNoise; // From tangent to view-space
			oSample = viewPosition + oSample * kernelRadius;
        
			// project sample position
			vec4 offset = vec4(oSample, 1.0);
			offset = projection * offset; // from view to clip-space
			offset.xyz /= offset.w; // perspective divide
			offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
			offset.y = 1.0 - offset.y;

			float sampleDepth = getScreenSpacePos(offset.xy, inPs.cameraDir).z;

			float rangeCheck = smoothstep(0.0, 1.0, kernelRadius / abs(viewPosition.z - sampleDepth));
			occlusion += (sampleDepth >= oSample.z ? 1.0 : 0.0) * rangeCheck;
			
		}      
    }
    occlusion = 1.0 - (occlusion * invKernelSize);
   
    fragColour = occlusion;
}
