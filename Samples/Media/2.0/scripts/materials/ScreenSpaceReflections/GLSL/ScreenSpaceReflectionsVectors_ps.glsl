// By Morgan McGuire and Michael Mara at Williams College 2014
// Released as open source under the BSD 2-Clause License
// http://opensource.org/licenses/BSD-2-Clause
//
// Copyright (c) 2014, Morgan McGuire and Michael Mara
// All rights reserved.
//
// From McGuire and Mara, Efficient GPU Screen-Space Ray Tracing,
// Journal of Computer Graphics Techniques, 2014
//
// Small Custom adaptations by Matias N. Goldberg
//
// This software is open source under the "BSD 2-clause license":
//
// Redistribution and use in source and binary forms, with or
// without modification, are permitted provided that the following
// conditions are met:
//
// 1. Redistributions of source code must retain the above
// copyright notice, this list of conditions and the following
// disclaimer.
//
// 2. Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following
// disclaimer in the documentation and/or other materials provided
// with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
// USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
#version ogre_glsl_ver_450

vulkan_layout(location = 0) out vec4 fragColour;

vulkan_layout(location = 0)
in block {
    vec2 uv0;
    vec3 cameraDir;
} inPs;

vulkan_layout(ogre_t0) uniform texture2D depthTexture;
#if !USE_MSAA
vulkan_layout(ogre_t1) uniform texture2D gBuf_normals;
#else
vulkan_layout(ogre_t1) uniform texture2DMS gBuf_normals;
#endif
vulkan_layout(ogre_t2) uniform texture2D prevFrameDepthTexture;
vulkan(layout(ogre_s0) uniform sampler samplerState);

vulkan(layout(ogre_P0) uniform Params { )
    uniform vec4 depthBufferRes;
    uniform vec4 zThickness;
    uniform float nearPlaneZ;
    uniform float stride;
    uniform float maxSteps;
    uniform float maxDistance;
    uniform vec2 projectionParams;
    uniform vec2 invDepthBufferRes;
    uniform mat4 viewToTextureSpaceMatrix;
    uniform mat4 reprojectionMatrix;
    uniform float reprojectionMaxDistanceError;
vulkan( }; )

float saturate(float x) { return clamp(x, 0.0, 1.0); }

float distanceSquared(vec2 a, vec2 b) {
    a -= b;
    return dot(a, a);
}

bool intersectsDepthBuffer(float z, float minZ, float maxZ) {
    float dynamicBias = clamp(z * zThickness.y + zThickness.z, 0.0, zThickness.w);
    z += zThickness.x + dynamicBias;
    return (maxZ >= z) && (minZ - zThickness.x - dynamicBias <= z);
}

void swap(inout float a, inout float b) {
    float t = a; a = b; b = t;
}

float linearizeDepth(float d) {
    return projectionParams.y / (d - projectionParams.x);
}

float linearDepthTexelFetch(texture2D tex, ivec2 coord) {
    return linearizeDepth(texelFetch(vkSampler2D(tex, samplerState), coord, 0).x);
}

bool traceScreenSpaceRay(vec3 origin, vec3 dir, float jitter, out vec2 hitPixel, out vec3 hitPoint) {
    float rayLength = ((origin.z + dir.z * maxDistance) < nearPlaneZ) ? 
                      (nearPlaneZ - origin.z) / dir.z : maxDistance;
    vec3 endPoint = origin + dir * rayLength;

    vec4 H0 = viewToTextureSpaceMatrix * vec4(origin, 1.0);
    vec4 H1 = viewToTextureSpaceMatrix * vec4(endPoint, 1.0);
    H0.xy *= depthBufferRes.xy;
    H1.xy *= depthBufferRes.xy;

    float k0 = 1.0 / H0.w, k1 = 1.0 / H1.w;
    vec3 Q0 = origin * k0, Q1 = endPoint * k1;
    vec2 P0 = H0.xy * k0, P1 = H1.xy * k1;

    if (distanceSquared(P0, P1) < 0.0001)
        P1 += vec2(0.01);

    vec2 delta = P1 - P0;
    bool permute = false;
    if (abs(delta.x) < abs(delta.y)) {
        permute = true;
        delta = delta.yx; P0 = P0.yx; P1 = P1.yx;
    }

    float dirSign = sign(delta.x), invDx = dirSign / delta.x;
    vec3 dQ = (Q1 - Q0) * invDx;
    float dk = (k1 - k0) * invDx;
    vec2 dP = vec2(dirSign, delta.y * invDx);

    dP *= stride; dQ *= stride; dk *= stride;
    P0 += dP * jitter; Q0 += dQ * jitter; k0 += dk * jitter;

    vec4 PQk = vec4(P0, Q0.z, k0), dPQk = vec4(dP, dQ.z, dk);
    vec3 Q = Q0; float endX = P1.x * dirSign;
    float prevZMax = origin.z, rayMin = prevZMax, rayMax = prevZMax;
    float sceneZ = rayMax + 100.0;
    float steps = 0.0;

    for (; (PQk.x * dirSign) <= endX && steps < maxSteps &&
           !intersectsDepthBuffer(sceneZ, rayMin, rayMax) && sceneZ != 0.0;
           ++steps)
    {
        rayMin = prevZMax;
        rayMax = (dPQk.z * 0.5 + PQk.z) / (dPQk.w * 0.5 + PQk.w);
        prevZMax = rayMax;
        if (rayMin > rayMax) swap(rayMin, rayMax);
        hitPixel = permute ? PQk.yx : PQk.xy;
        sceneZ = linearDepthTexelFetch(depthTexture, ivec2(hitPixel));
        PQk += dPQk;
    }

    Q.xy += dQ.xy * steps;
    hitPoint = Q / PQk.w;
    return intersectsDepthBuffer(sceneZ, rayMin, rayMax);
}

vulkan_layout(location = 1) in vec4 gl_FragCoord;

void main() {
#if HQ
    vec2 sampleCoord = gl_FragCoord.xy;
#else
    vec2 sampleCoord = gl_FragCoord.xy * 2.0;
#endif

    vec3 normalVS = normalize(texelFetch(vkSampler2D(gBuf_normals, samplerState), ivec2(sampleCoord), 0).xyz * 2.0 - 1.0);
    normalVS.z = -normalVS.z;
    if (normalVS == vec3(0)) {
        fragColour = vec4(0.0);
        return;
    }

    float depth = texelFetch(vkSampler2D(depthTexture, samplerState), ivec2(sampleCoord), 0).x;
    vec3 origin = inPs.cameraDir * linearizeDepth(depth);
    vec3 toPosition = normalize(origin);
    vec3 rayDir = normalize(reflect(toPosition, normalVS));
    float rDotV = dot(rayDir, toPosition);

    vec2 hitPixel; vec3 hitPoint;
    float jitter = stride > 1.0 ? float(int(gl_FragCoord.x + gl_FragCoord.y) & 1) * 0.5 : 0.0;
    bool hit = traceScreenSpaceRay(origin, rayDir, jitter, hitPixel, hitPoint);

    vec3 reflNormal = normalize(texelFetch(vkSampler2D(gBuf_normals, samplerState), ivec2(hitPixel), 0).xyz * 2.0 - 1.0);
    reflNormal.z = -reflNormal.z;
    rDotV *= smoothstep(-0.17, 0.0, dot(normalVS, -reflNormal)) * 0.25;

    float currDepth = texelFetch(vkSampler2D(depthTexture, samplerState), ivec2(hitPixel), 0).x;
    hitPixel *= invDepthBufferRes;
    vec4 reprojPos = reprojectionMatrix * vec4(hitPixel, currDepth, 1.0);
    reprojPos.xyz /= reprojPos.w;

    float prevDepth = texelFetch(vkSampler2D(prevFrameDepthTexture, samplerState),
                                 ivec2(reprojPos.xy * depthBufferRes.xy), 0).x;
    hitPixel = reprojPos.xy;
    bool reprojFail = (linearizeDepth(reprojPos.z) - linearizeDepth(prevDepth)) > reprojectionMaxDistanceError;

    hit = hit && hitPixel.x >= 0.0 && hitPixel.x <= 1.0 &&
               hitPixel.y >= 0.0 && hitPixel.y <= 1.0 &&
               !reprojFail;

    float fade = 1.0 - distance(hitPoint, origin) / maxDistance;
    fragColour = hit ? vec4(hitPixel, fade, rDotV) : vec4(0.0);
}
