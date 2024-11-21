/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#ifndef _OgreWorkarounds_H_
#define _OgreWorkarounds_H_

#include <stddef.h>

namespace Ogre
{
    struct _OgreExport Workarounds
    {
        static void dump( String &outStr );

#ifdef OGRE_BUILD_RENDERSYSTEM_GLES2

// PowerVR SGX 540 does not correctly transpose matrices in glProgramUniformMatrix4fvEXT,
// so we have to do that manually (which is inconsistent with the way GL3+ works on desktop)
//
// First seen: 2010-02-25 (probably, at least)
// Last seen:  2014-05-30 on Samsung Galaxy Tab 2 (GT-P3110);
//		Android 4.2.2 (Samsung officially discontinued updates for this device)
// Additional Info:
//	https://twitter.com/KeepItFoolish/status/472456232738234368
#    define OGRE_GLES2_WORKAROUND_1 1

// For some reason cubemapping in ES, the vector needs to be rotated 90° around the X axis.
// May be it's a difference in the ES vs GL spec, or an Ogre bug when uploading the faces.
//
// First seen: ???
// Last seen:  2014-08-19 on All devices
#    define OGRE_GLES2_WORKAROUND_2 1

#endif

#ifdef OGRE_BUILD_RENDERSYSTEM_VULKAN

#    if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
// Adreno 505, 506 and many others expose 64kb of UBO memory.
// However binding exactly 65536 bytes of UBO memory causes the GPU to read all 0s.
// We limit const buffer memory to 65472 bytes instead.
//
// Qualcomm claims to have patched this bug.
// As of driver 512.472.0 (Android 10), this bug is still present
//
// First seen: Since the very first driver version
// Last seen: 2020-09-08
#        define OGRE_VK_WORKAROUND_ADRENO_UBO64K
        static bool mAdrenoUbo64kLimitTriggered;
        /// If > 0, then the workaround is active
        /// The value contains the maximum range we will bind
        static size_t mAdrenoUbo64kLimit;

// Adreno 505, 506 and many others get glitchy shadows when using D32_FLOAT.
//
// Qualcomm claims to have patched this bug at version 512.440
// Bug appears to be fixed at version 512.415
//
// First seen: Since the very first driver version
// Last seen: 2020-09-08
#        define OGRE_VK_WORKAROUND_ADRENO_D32_FLOAT
        static bool mAdrenoD32FloatBug;

// Adreno 5xx & 6xx series report the minimum features required by Vulkan.
// However they support much more. In fact they are all DX12 FL12 HW.
//
// Qualcomm has patched this bug on 6xx series, driver version 512.444.0.
// It is unlike Qualcomm will patch this bug for the 5xx series.
//
//      maxTexelBufferElements is reported 65535. At least 128MB is supported
//      16-bit UNORM and SNORM formats are supported. Reported as unsupported.
//
// First seen: Since the very first driver version
// Last seen: 2020-09-08
#        define OGRE_VK_WORKAROUND_ADRENO_5XX_6XX_MINCAPS
        static bool mAdreno5xx6xxMinCaps;

/// Adreno 505 incorrectly scales down non-square 3D textures (e.g. 128x128x32).
/// It is unknown if square 3D textures are downscaled correctly
/// It is unknown if newer GPUs are affected thus we assume all Qualcomm chips are
///
/// The XY downscale is correct but the Z slice is incorrect, as it ignores
/// a lot of slices.
///
/// Qualcomm has been notified of the bug
///
/// As of driver 512.415.0 (Android 10), this bug is still present
///
/// Workaround is to do mipmapping in SW, or via Compute.
/// The flag does not have Adreno's name because it is likely
/// to be reused for other vendors (breaking mipmaps is common...)
///
/// First seen: Unknown
/// Last seen: 2021-08-27
#        define OGRE_VK_WORKAROUND_BAD_3D_BLIT
        static bool mBad3DBlit;

/// Adreno 618 will experience a GPU crash when we send a vkCmdDrawIndirect
/// with 1 command and that command has 0 vertices to render.
/// Normal vkCmdDraw accepts 0 vertices fine.
///
/// Qualcomm has been notified of the bug; but this is likely a HW bug
/// Adreno 640 doesn't exhibit this behavior (same driver version).
///
/// As of Driver 512.502.0 (Android 11), this bug is still present.
///
/// Given the little gains that indirect currently give, and indirect
/// draw sounds like a pandora's box in Android; we force enable
/// this workaround for all Android devices.
///
/// First seen: Unknown
/// Last seen: 2022-04-20
#        define OGRE_VK_WORKAROUND_ADRENO_618_0VERTEX_INDIRECT
        static bool mAdreno618_0VertexIndirect;

/// PowerVR 8xxx & 9xxx will raise a VK_ERROR_OUT_OF_DEVICE_MEMORY in
/// vkQueueSubmit when vkCmdCopyBuffer srcOffset or dstOffset aren't
/// multiple of 16 bytes.
///
/// Imagination has been notified of the bug and has helped us find it.
///
/// As of Driver 1.386.1368, API version 1.1.131 (Android 11),
/// this bug is still present.
///
/// PowerVR has fixed this in driver version 1.426.234
///
/// First seen: Unknown
/// Last seen: 2022-05-13
#        define OGRE_VK_WORKAROUND_PVR_ALIGNMENT
        static uint32 mPowerVRAlignment;
#    endif

/// Swappy causes the Swapchain to be recreated indefinitely, slowing down FPS to a crawl.
/// On Android 14 (latest at the time of writing) Google Pixel (and a few other phones)
/// cause a crash. The blocklist is maintained by hand.
#    define OGRE_VK_WORKAROUND_SWAPPY_CRASH
#endif
    };
}  // namespace Ogre

#endif
