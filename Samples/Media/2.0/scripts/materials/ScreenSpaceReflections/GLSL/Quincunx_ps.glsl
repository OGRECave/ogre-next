#version ogre_glsl_ver_450

vulkan_layout(location = 0) out vec4 fragColour;

vulkan_layout(location = 0)
in block {
    vec2 uv0;
} inPs;

vulkan_layout(ogre_t0) uniform texture2D tex;
vulkan(layout(ogre_s0) uniform sampler samplerState);
vulkan_layout(location = 1) in vec4 gl_FragCoord;

void main()
{
    vec4 finalColour = texelFetch(vkSampler2D(tex, samplerState), ivec2(gl_FragCoord.xy), 0);

    finalColour += texelFetch(vkSampler2D(tex, samplerState), ivec2(gl_FragCoord.xy) + ivec2(-1,  1), 0);
    finalColour += texelFetch(vkSampler2D(tex, samplerState), ivec2(gl_FragCoord.xy) + ivec2( 1,  1), 0);
    finalColour += texelFetch(vkSampler2D(tex, samplerState), ivec2(gl_FragCoord.xy) + ivec2(-1, -1), 0);
    finalColour += texelFetch(vkSampler2D(tex, samplerState), ivec2(gl_FragCoord.xy) + ivec2( 1, -1), 0);

    finalColour /= 5.0;

    fragColour = finalColour;
}
