/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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

#ifndef _OgreVulkanRenderPassDescriptor_H_
#define _OgreVulkanRenderPassDescriptor_H_

#include "OgreVulkanPrerequisites.h"

#include "OgreCommon.h"
#include "OgrePixelFormatGpu.h"
#include "OgreRenderPassDescriptor.h"

#include "vulkan/vulkan_core.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */

    struct VulkanFrameBufferDescValue
    {
        uint16 refCount;
        VulkanFrameBufferDescValue();
    };

    typedef map<FrameBufferDescKey, VulkanFrameBufferDescValue>::type VulkanFrameBufferDescMap;

    class _OgreVulkanExport VulkanRenderPassDescriptor : public RenderPassDescriptor
    {
    protected:
        VkRenderPass mRenderPass;

        uint8 mDepthStencilIdxToAttachmIdx;
        VkClearValue mClearValues[OGRE_MAX_MULTIPLE_RENDER_TARGETS * 2u + 2u];
        VkImageView mImageViews[OGRE_MAX_MULTIPLE_RENDER_TARGETS * 2u + 2u];
        uint32 mNumImageViews;
        FastArray<VkImageView> mWindowImageViews;  // Only used by windows
        // We normally need just one, but Windows are a special case
        // because we need to have one per swapchain image, hence FastArray
        FastArray<VkFramebuffer> mFramebuffers;

        VulkanFrameBufferDescMap::iterator mSharedFboItor;

        uint32 mTargetWidth;
        uint32 mTargetHeight;

        VulkanQueue *mQueue;
        VulkanRenderSystem *mRenderSystem;

#if OGRE_DEBUG_MODE && OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        void *mCallstackBacktrace[32];
        size_t mNumCallstackEntries;
#endif

        void checkRenderWindowStatus( void );
        void calculateSharedKey( void );

        static VkAttachmentLoadOp get( LoadAction::LoadAction action );
        static VkAttachmentStoreOp get( StoreAction::StoreAction action );
        static VkClearColorValue getClearColour( const ColourValue &clearColour,
                                                 PixelFormatGpu pixelFormat );

        void sanitizeMsaaResolve( size_t colourIdx );
        VkImageView setupColourAttachment( VkAttachmentDescription &attachment, VkImage texName,
                                           const RenderPassColourTarget &colour, bool resolveTex );
        VkImageView setupDepthAttachment( VkAttachmentDescription &attachment );
        void updateFbo( void );
        void destroyFbo( void );

        /// Returns a mask of RenderPassDescriptor::EntryTypes bits set that indicates
        /// if 'other' wants to perform clears on colour, depth and/or stencil values.
        /// If using MRT, each colour is evaluated independently (only the ones marked
        /// as clear will be cleared).
        uint32 checkForClearActions( VulkanRenderPassDescriptor *other ) const;
        bool cannotInterruptRendering( void ) const;

    public:
        VulkanRenderPassDescriptor( VulkanQueue *graphicsQueue, VulkanRenderSystem *renderSystem );
        virtual ~VulkanRenderPassDescriptor();

        void notifySwapchainCreated( VulkanWindow *window );
        void notifySwapchainDestroyed( VulkanWindow *window );

        virtual void entriesModified( uint32 entryTypes );

        virtual void setClearColour( uint8 idx, const ColourValue &clearColour );
        virtual void setClearDepth( Real clearDepth );
        virtual void setClearStencil( uint32 clearStencil );

        /// Sets the clear colour to all entries. In some APIs may be faster
        /// than calling setClearColour( idx, clearColour ) for each entry
        /// individually.
        virtual void setClearColour( const ColourValue &clearColour );

        uint32 willSwitchTo( VulkanRenderPassDescriptor *newDesc, bool warnIfRtvWasFlushed ) const;

        void performLoadActions( bool renderingWasInterrupted );
        void performStoreActions( bool isInterruptingRendering );
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
