/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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
#ifndef _OgreDepthBuffer_H_
#define _OgreDepthBuffer_H_

#include "OgrePrerequisites.h"

#include "OgrePixelFormatGpu.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup RenderSystem
     *  @{
     */

    /** An abstract class that contains a depth/stencil buffer.
        Depth Buffers can be attached to render targets. Note we handle Depth & Stencil together.
        DepthBuffer sharing is handled automatically for you. However, there are times where you want
        to specifically control depth buffers to achieve certain effects or increase performance.
        You can control this by hinting Ogre with POOL IDs. Created depth buffers can live in different
        pools, or all together in the same one.
        Usually, a depth buffer can only be attached to a RenderTarget if it's dimensions are bigger
        and have the same bit depth and same multisample settings. Depth Buffers are created
       automatically for new RTs when needed, and stored in the pool where the RenderTarget should have
       drawn from. By default, all RTs have the Id POOL_DEFAULT, which means all depth buffers are stored
       by default in that pool. By choosing a different Pool Id for a specific RenderTarget, that RT will
       only retrieve depth buffers from _that_ pool, therefore not conflicting with sharing depth buffers
        with other RTs (such as shadows maps).
        Setting an RT to POOL_MANUAL_USAGE means Ogre won't manage the DepthBuffer for you (not
       recommended) RTs with POOL_NO_DEPTH are very useful when you don't want to create a DepthBuffer
       for it. You can still manually attach a depth buffer though as internally POOL_NO_DEPTH &
       POOL_MANUAL_USAGE are handled in the same way.

        Behavior is consistent across all render systems, if, and only if, the same RSC flags are set
        RSC flags that affect this class are:
            + RSC_RTT_SEPARATE_DEPTHBUFFER:
                The RTT can create a custom depth buffer different from the main depth buffer. This
       means, an RTT is able to not share it's depth buffer with the main window if it wants to.
            + RSC_RTT_MAIN_DEPTHBUFFER_ATTACHABLE:
                When RSC_RTT_SEPARATE_DEPTHBUFFER is set, some APIs (ie. OpenGL w/ FBO) don't allow using
                the main depth buffer for offscreen RTTs. When this flag is set, the depth buffer can be
                shared between the main window and an RTT.
            + RSC_RTT_DEPTHBUFFER_RESOLUTION_LESSEQUAL:
                When this flag isn't set, the depth buffer can only be shared across RTTs who have the
       EXACT same resolution. When it's set, it can be shared with RTTs as long as they have a resolution
       less or equal than the depth buffer's.

        @remarks
            Design discussion http://www.ogre3d.org/forums/viewtopic.php?f=4&t=53534&p=365582
        @author
            Matias N. Goldberg
        @version
            1.0
     */
    struct _OgreExport DepthBuffer
    {
        enum PoolId
        {
            POOL_NO_DEPTH = 0,
            POOL_MANUAL_USAGE OGRE_DEPRECATED_ENUM_VER( 3 ) = 0,
            POOL_DEFAULT = 1,

            /// The depth buffer doesn't does not have memory backing it.
            /// It will be created with TextureFlags::TilerMemoryless.
            POOL_MEMORYLESS = 65533,

            /// Deprecated.
            ///
            /// Do NOT use this flag directly. It made sense in Ogre 2.1
            /// It no longer makes sense externally in Ogre 2.2
            ///
            /// Depth buffers are now created like regular TextureGpu and are
            /// set directly to RTV (RenderTextureViews) aka RenderPassDescriptor
            ///
            /// See https://forums.ogre3d.org/viewtopic.php?p=548491#p548491
            /// See samples that setup depth RTVs such as:
            ///
            /// Samples/Media/2.0/scripts/Compositors/IrradianceFieldRaster.compositor
            /// Samples/Media/2.0/scripts/Compositors/StencilTest.compositor
            /// Samples/Media/2.0/scripts/Compositors/Tutorial_ReconstructPosFromDepth.compositor
            ///
            /// See https://ogrecave.github.io/ogre-next/api/latest/_ogre22_changes.html
            /// for documentation about RTVs.
            POOL_NON_SHAREABLE OGRE_DEPRECATED_ENUM_VER( 3 ) = 65534,
            /// The depth buffer doesn't come from a pool, either because the TextureGpu
            /// is already a depth buffer, or it's explicitly set in an RTV
            NO_POOL_EXPLICIT_RTV = 65534,
            POOL_INVALID = 65535
        };

        enum DepthFormatsMask
        {
            DFM_D32 = 1u << 0u,
            DFM_D24 = 1u << 1u,
            DFM_D16 = 1u << 2u,
            DFM_S8 = 1u << 3u
        };

        static PixelFormatGpu DefaultDepthBufferFormat;

        /// During initialization DefaultDepthBufferFormat is overriden with a supported format
        ///
        /// This can be troublesome when creating the first render window, as you cannot tell
        /// Ogre which format do you wish to use for that window.
        ///
        /// That's where AvailableDepthFormats comes in:
        ///
        /// Before initialization user can set this mask to inform which formats
        /// they want to use. Ogre will go from best format to worst until a
        /// supported one is found.
        /// The default value is all bits set.
        ///
        /// Set this to 0 to never use depth buffers.
        /// If you only wish render windows to not use depth buffers, then
        /// create the window with miscParam["depth_buffer"] = "no";
        ///
        /// After initialization the mask is left untouched
        ///
        /// See DepthBuffer::DepthFormatsMask
        static uint8 AvailableDepthFormats;
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
