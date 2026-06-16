#version ogre_glsl_ver_450

vulkan_layout( location = 0 ) out vec4 fragColour;

vulkan_layout( location = 0 )
in block {
    vec2 uv0;
    vec3 cameraDir;
} inPs;

vulkan_layout( ogre_t0 ) uniform texture2D depthTexture;
vulkan_layout( ogre_t1 ) uniform texture2D gBuf_normals;
vulkan_layout( ogre_t2 ) uniform texture2D gBuf_shadowRoughness;
vulkan_layout( ogre_t3 ) uniform texture2D prevFrame;
vulkan_layout( ogre_t3 ) uniform texture2D rayTraceBuffer;
vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform vec4 depthBufferRes;
    uniform float fadeStart;
    uniform float invFadeRange;
    uniform vec2 projectionParams;
    uniform mat4 textureSpaceToViewSpace;
    uniform mat3 invViewMatCubemap;
vulkan( }; )

float saturate(float x) { return clamp(x, 0.0, 1.0); }

float linearizeDepth(float fDepth)
{
    return projectionParams.y / (fDepth - projectionParams.x);
}

float linearDepthTexelFetch(texture2D depthTex, ivec2 hitPixel)
{
    return linearizeDepth(texelFetch(vkSampler2D(depthTex, samplerState), hitPixel, 0).x);
}

float roughnessToConeAngle(float roughness)
{
    return smoothstep(0.0, 1.0, 1.0 - 1.0 / (1.0 + roughness * roughness));
}

float isoscelesTriangleOpposite(float adjacentLength, float tanConeTheta)
{
    return 2.0 * tanConeTheta * adjacentLength;
}

float isoscelesTriangleInRadius(float a, float h)
{
    float a2 = a * a;
    float fh2 = 4.0 * h * h;
    return (a * (sqrt(a2 + fh2) - a)) / (4.0 * h);
}

vec4 coneSampleWeightedColor(vec2 samplePos, float mipChannel, float gloss)
{
    vec3 sampleColor = textureLod(vkSampler2D(prevFrame, samplerState), samplePos, mipChannel).xyz;
    return vec4(sampleColor * gloss, gloss);
}


void main()
{
#if HQ
    vec4 raySS = texelFetch(vkSampler2D(rayTraceBuffer, samplerState), ivec2(gl_FragCoord.xy), 0);
#else
    vec4 raySS = texture(vkSampler2D(rayTraceBuffer, samplerState), inPs.uv0);
#endif

    float roughness = texelFetch(vkSampler2D(gBuf_shadowRoughness, samplerState), ivec2(gl_FragCoord.xy), 0).y;
    float gloss = 1.0 - roughness;

    float tanConeTheta = roughnessToConeAngle(roughness);

    vec2 positionSS = inPs.uv0;
    vec2 deltaP = raySS.xy - positionSS;
    float adjacentLength = length(deltaP);
    vec2 adjacentUnit = normalize(deltaP);

    vec4 totalColor = vec4(0.0);
    float remainingAlpha = 1.0;
    float glossMult = gloss;

    for(int i = 0; i < 14; ++i)
    {
        float oppositeLength = isoscelesTriangleOpposite(adjacentLength, tanConeTheta);
        float incircleSize = isoscelesTriangleInRadius(oppositeLength, adjacentLength);
        vec2 samplePos = positionSS + adjacentUnit * (adjacentLength - incircleSize);

        float mipChannel = log2(incircleSize * depthBufferRes.w);

        vec4 newColor = coneSampleWeightedColor(samplePos, mipChannel, glossMult);

        remainingAlpha -= newColor.a;
        if(remainingAlpha < 0.0)
            newColor.rgb *= (1.0 - abs(remainingAlpha));
        totalColor += newColor;

        if(totalColor.a >= 1.0)
            break;

        adjacentLength -= incircleSize * 2.0;
        glossMult *= gloss;
    }

    vec2 boundary = abs(raySS.xy - vec2(0.5)) * 2.0;
    float fadeOnBorder = 1.0 - saturate((boundary.x - fadeStart) * invFadeRange);
    fadeOnBorder *= 1.0 - saturate((boundary.y - fadeStart) * invFadeRange);
    fadeOnBorder = smoothstep(0.0, 1.0, fadeOnBorder);
    float fadeOnDistance = raySS.z;
    float fadeOnPerpendicular = saturate(raySS.w * 4.0);
    float fadeOnRoughness = saturate(gloss * 4.0);
    float totalFade = fadeOnBorder * fadeOnDistance * fadeOnPerpendicular *
                      fadeOnRoughness * (1.0 - saturate(remainingAlpha));

    fragColour = vec4(totalColor.rgb, totalFade);
}
