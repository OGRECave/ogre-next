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

#ifndef _OgreVulkanQueue_H_
#define _OgreVulkanQueue_H_

#include "OgreVulkanPrerequisites.h"

#include "Vao/OgreBufferPacked.h"

#include "vulkan/vulkan_core.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class _OgreVulkanExport VulkanQueue
    {
    public:
        enum QueueFamily
        {
            Graphics,
            Compute,
            Transfer,
            NumQueueFamilies
        };

        struct PerFrameData
        {
            VkCommandPool mCmdPool;
            FastArray<VkCommandBuffer> mCommands;
            size_t mCurrentCmdIdx;
            FastArray<VkFence> mProtectingFences;
        };

        enum EncoderState
        {
            EncoderGraphicsOpen,
            EncoderComputeOpen,
            EncoderCopyOpen,
            EncoderClosed
        };

        VkDevice mDevice;
        QueueFamily mFamily;

        uint32 mFamilyIdx;

        uint32 getFamilyIdx() const { return mFamilyIdx; }

        uint32 mQueueIdx;

        VkQueue mQueue;
        VkCommandBuffer mCurrentCmdBuffer;

        VulkanDevice *mOwnerDevice;

    protected:
        // clang-format off
        // One per buffered frame
        FastArray<PerFrameData> mPerFrameData;

        /// Collection of semaphore we need to wait on before our queue executes
        /// pending commands when commitAndNextCommandBuffer is called
        VkSemaphoreArray                mGpuWaitSemaphForCurrCmdBuff;
        FastArray<VkPipelineStageFlags> mGpuWaitFlags;
        /// Collection of semaphore we will signal when our queue
        /// submitted in commitAndNextCommandBuffer is done
        VkSemaphoreArray                mGpuSignalSemaphForCurrCmdBuff;
        // clang-format on

        VkFenceArray mAvailableFences;

    public:
        FastArray<VulkanWindow *> mWindowsPendingSwap;

    protected:
        FastArray<VkCommandBuffer> mPendingCmds;

        VulkanVaoManager *mVaoManager;
        VulkanRenderSystem *mRenderSystem;

        VkFence mCurrentFence;

        typedef map<const BufferPacked *, bool>::type BufferPackedDownloadMap;
        typedef map<VulkanTextureGpu *, bool>::type TextureGpuDownloadMap;

        EncoderState mEncoderState;
        VkAccessFlags mCopyEndReadSrcBufferFlags;
        VkAccessFlags mCopyEndReadDstBufferFlags;
        VkAccessFlags mCopyEndReadDstTextureFlags;
        VkAccessFlags mCopyStartWriteSrcBufferFlags;
        FastArray<VkImageMemoryBarrier> mImageMemBarriers;
        FastArray<TextureGpu *> mImageMemBarrierPtrs;
        /// When mCopyDownloadTextures[buffer] = true, this buffer has been last downloaded
        /// When mCopyDownloadTextures[buffer] = false, this buffer has been last uploaded
        /// When mCopyDownloadTextures[buffer] = not_found, this buffer hasn't been downloaded/uploaded
        TextureGpuDownloadMap mCopyDownloadTextures;
        /// When mCopyDownloadBuffers[buffer] = true, this buffer has been last downloaded
        /// When mCopyDownloadBuffers[buffer] = false, this buffer has been last uploaded
        /// When mCopyDownloadBuffers[buffer] = not_found, this buffer hasn't been downloaded/uploaded
        BufferPackedDownloadMap mCopyDownloadBuffers;

        /// Returns a signaled fence, could be recycled or new
        VkFence getFence( void );

        VkCommandBuffer getCmdBuffer( size_t currFrame );

        static VkPipelineStageFlags deriveStageFromBufferAccessFlags( VkAccessFlags accessFlags );
        static VkPipelineStageFlags deriveStageFromTextureAccessFlags( VkAccessFlags accessFlags );

        /// Schedules a barrier to be issued in endCopyEncoder
        /// This is to RESTORE the texture's layout and force future rendering cmds to wait
        /// for our transfer to finish.
        void insertRestoreBarrier( VulkanTextureGpu *vkTexture, const VkImageLayout newTransferLayout );
        void prepareForUpload( const BufferPacked *buffer, TextureGpu *texture );
        void prepareForDownload( const BufferPacked *buffer, TextureGpu *texture );

    public:
        VulkanQueue();
        ~VulkanQueue();

        void setQueueData( VulkanDevice *owner, QueueFamily family, uint32 familyIdx, uint32 queueIdx );

        void init( VkDevice device, VkQueue queue, VulkanRenderSystem *renderSystem );
        void destroy( void );

        void newCommandBuffer( void );

        void getGraphicsEncoder( void );
        void getComputeEncoder( void );
        /** Call this function when you need to start copy/transfer operations
        @remarks
            buffer and texture pointers cannot be both nullptr at the same time

            You don't have to pair every getCopyEncoder call with an endCopyEncoder call. In
            fact this is discouraged.

            Keep calling getCopyEncoder until you're done with all transfer operations
        @param buffer
            The buffer we're copying from/to. Can be nullptr
        @param texture
            The texture we're copying from/to. Can be nullptr

            If uploading, the texture will be transitioned to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            If downloading, the texture will be transitioned to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        @param bDownload
            True if we plan to do CPU -> GPU transfers
            False if we plan to do GPU -> CPU transfers

            If you want to perform GPU -> GPU transfers, then you need to call:
            @code
                queue->getCopyEncoder( src, src, true );
                queue->getCopyEncoder( dst, dst, false );
            @endcode
        */
        void getCopyEncoder( const BufferPacked *buffer, TextureGpu *texture, const bool bDownload );
        void getCopyEncoderV1Buffer( const bool bDownload );

        void endCopyEncoder( void );
        void endRenderEncoder( const bool endRenderPassDesc = true );
        void endComputeEncoder( void );

        void endAllEncoders( bool endRenderPassDesc = true );

        VkFence getCurrentFence()
        {
            if( mCurrentFence == 0 )
            {
                mCurrentFence = getFence();
            }
            return mCurrentFence;
        }

    public:
        void endCommandBuffer( void );

    public:
        /// When we'll call commitAndNextCommandBuffer, we'll have to wait for
        /// this semaphore on to execute STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        void addWindowToWaitFor( VkSemaphore imageAcquisitionSemaph );

        void _waitOnFrame( uint8 frameIdx );

        void commitAndNextCommandBuffer( bool endingFrame );
    };

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
