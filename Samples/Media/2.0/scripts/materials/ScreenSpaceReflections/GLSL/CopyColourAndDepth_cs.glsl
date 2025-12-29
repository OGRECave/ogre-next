#version 430

vulkan_layout(ogre_t0) uniform texture2D srcRtt;
#if !texture1_msaa
vulkan_layout(ogre_t1) uniform texture2D srcDepth;
#else
vulkan_layout(ogre_t1) uniform texture2DMS srcDepth;
#endif
vulkan(layout(ogre_s0) uniform sampler samplerState);

vulkan_layout(ogre_u0) uniform restrict writeonly image2D dstRtt;
vulkan_layout(ogre_u1) uniform restrict writeonly image2D dstDepth;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main()
{
    uvec2 xyPos0 = gl_GlobalInvocationID.xy << 1u;
    uvec2 xyPos1 = xyPos0 + uvec2(1, 0);
    uvec2 xyPos2 = xyPos0 + uvec2(0, 1);
    uvec2 xyPos3 = xyPos0 + uvec2(1, 1);

    vec4 srcRttValue0 = texelFetch(vkSampler2D(srcRtt, samplerState), ivec2(xyPos0), 0);
    vec4 srcRttValue1 = texelFetch(vkSampler2D(srcRtt, samplerState), ivec2(xyPos1), 0);
    vec4 srcRttValue2 = texelFetch(vkSampler2D(srcRtt, samplerState), ivec2(xyPos2), 0);
    vec4 srcRttValue3 = texelFetch(vkSampler2D(srcRtt, samplerState), ivec2(xyPos3), 0);

    vec4 srcDepthValue0 = texelFetch(vkSampler2D(srcDepth, samplerState), ivec2(xyPos0), 0);
    vec4 srcDepthValue1 = texelFetch(vkSampler2D(srcDepth, samplerState), ivec2(xyPos1), 0);
    vec4 srcDepthValue2 = texelFetch(vkSampler2D(srcDepth, samplerState), ivec2(xyPos2), 0);
    vec4 srcDepthValue3 = texelFetch(vkSampler2D(srcDepth, samplerState), ivec2(xyPos3), 0);

    imageStore(dstRtt, ivec2(xyPos0), srcRttValue0);
    imageStore(dstRtt, ivec2(xyPos1), srcRttValue1);
    imageStore(dstRtt, ivec2(xyPos2), srcRttValue2);
    imageStore(dstRtt, ivec2(xyPos3), srcRttValue3);

    imageStore(dstDepth, ivec2(xyPos0), srcDepthValue0);
    imageStore(dstDepth, ivec2(xyPos1), srcDepthValue1);
    imageStore(dstDepth, ivec2(xyPos2), srcDepthValue2);
    imageStore(dstDepth, ivec2(xyPos3), srcDepthValue3);
}
