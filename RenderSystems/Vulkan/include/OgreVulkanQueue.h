/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#include "OgreTextureGpu.h"
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
        struct RefCountedFence
        {
            uint32 refCount;
            /// When true, the VkFence is no longer inside PerFrameData::mProtectingFences and it is
            /// VulkanQueue::releaseFence's responsibility to put the fence back into mAvailableFences
            bool recycleAfterRelease;
            RefCountedFence() {}
            RefCountedFence( uint32 _refCount ) : refCount( _refCount ), recycleAfterRelease( false ) {}
        };

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

        typedef map<VkFence, RefCountedFence>::type RefCountedFenceMap;

        VkFenceArray mAvailableFences;
        /// Fences which haven't been released.
        /// mCurrentFence is not included even if mCurrentFenceRefCount > 0
        RefCountedFenceMap mRefCountedFences;

    public:
        FastArray<VulkanWindow *> mWindowsPendingSwap;

    protected:
        FastArray<VkCommandBuffer> mPendingCmds;

        VulkanVaoManager *mVaoManager;
        VulkanRenderSystem *mRenderSystem;

        VkFence mCurrentFence;
        /// Starts at 0. Increases with each acquireCurrentFence
        /// If commitAndNextCommandBuffer gets called with mCurrentFenceRefCount == 0,
        /// then mCurrentFence can enter into the recycle pool
        uint32 mCurrentFenceRefCount;

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
        VkFence getFence();
        /// Puts all input fences into mAvailableFences for recycling,
        /// unless their external reference count isn't 0
        ///
        /// Clears fences.
        void recycleFences( FastArray<VkFence> &fences );

        inline VkFence getCurrentFence();

        VkCommandBuffer getCmdBuffer( size_t currFrame );

        static VkPipelineStageFlags deriveStageFromBufferAccessFlags( VkAccessFlags accessFlags );
        static VkPipelineStageFlags deriveStageFromTextureAccessFlags( VkAccessFlags accessFlags );

        /// Schedules a barrier to be issued in endCopyEncoder
        /// This is to RESTORE the texture's layout and force future rendering cmds to wait
        /// for our transfer to finish.
        void insertRestoreBarrier( VulkanTextureGpu *vkTexture, const VkImageLayout newTransferLayout );

        /** There will be two barriers that prepareForUpload will generate:

                1. We must wait until previous commands are done reading from/writing to dst
                   before we can proceed with a copy command what writes to dst.<br/>
                   That includes previous copy commands, but we don't because we assume two
                   consecutive uploads to the same buffer (or textures) are always done to
                   non-overlapping regions.
                2. At endCopyEncoder: we must ensure future GPU commands wait for our copy
                   command to end writing to dst before they can start.

            prepareForUpload tries its best to generate no more than 2 barriers (except for
            texture layout transitions) between getCopyEncoder and endCopyEncoder, but sometimes
            more than 2 barriers may end up being issued
        @param buffer
        @param texture
        */
        void prepareForUpload( const BufferPacked *buffer, TextureGpu *texture,
                               CopyEncTransitionMode::CopyEncTransitionMode transitionMode );

        /** There between zero to two barriers that prepareForDownload will generate:

                1. We must wait until previous commands are done writing to src before we can
                   proceed with a copy command that reads from src. Some buffers/textures are
                   guaranteed to be read-only, in this case we can avoid this barrier.
                   Textures may still need to transition their layout though.
                2. At endCopyEncoder: we must ensure future commands don't write to src before
                   we're done reading from src. Again, buffers/textures which are guaranteed to
                   be read-only don't need this.
                   Textures may still need to transition their layout though

            prepareForDownload tries its best to generate no more than 2 barriers (except for
            texture layout transitions) between getCopyEncoder and endCopyEncoder, but sometimes
            more than 2 barriers may end up being issued
        @param buffer
        @param texture
        */
        void prepareForDownload( const BufferPacked *buffer, TextureGpu *texture,
                                 CopyEncTransitionMode::CopyEncTransitionMode transitionMode );

    public:
        VulkanQueue();
        ~VulkanQueue();

        void setQueueData( VulkanDevice *owner, QueueFamily family, uint32 familyIdx, uint32 queueIdx );

        void init( VkDevice device, VkQueue queue, VulkanRenderSystem *renderSystem );
        void destroy();

    protected:
        void newCommandBuffer();
        void endCommandBuffer();

    public:
        EncoderState getEncoderState() const { return mEncoderState; }

        void getGraphicsEncoder();
        void getComputeEncoder();
        /** Call this function when you need to start copy/transfer operations
        @remarks
            buffer and texture pointers cannot be both nullptr at the same time

            You don't have to pair every getCopyEncoder call with an endCopyEncoder call. In
            fact this is discouraged.

            Keep calling getCopyEncoder until you're done with all transfer operations

            @see    VulkanQueue::prepareForUpload
            @see    VulkanQueue::prepareForDownload
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
        @param transitionMode
            Only used if texture != nullptr
            See CopyEncTransitionMode::CopyEncTransitionMode
        */
        void getCopyEncoder( const BufferPacked *buffer, TextureGpu *texture, const bool bDownload,
                             CopyEncTransitionMode::CopyEncTransitionMode transitionMode );
        void getCopyEncoderV1Buffer( const bool bDownload );

        void endCopyEncoder();
        void endRenderEncoder( const bool endRenderPassDesc = true );
        void endComputeEncoder();

        void endAllEncoders( bool endRenderPassDesc = true );

        void notifyTextureDestroyed( VulkanTextureGpu *texture );

        VkFence acquireCurrentFence();
        void releaseFence( VkFence fence );

        /// When we'll call commitAndNextCommandBuffer, we'll have to wait for
        /// this semaphore on to execute STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        void addWindowToWaitFor( VkSemaphore imageAcquisitionSemaph );

        /// If this function returns false, waiting on the fence would cause a deadlock since
        /// it will never signal. You must call commitAndNextCommandBuffer before waiting.
        bool isFenceFlushed( VkFence fence ) const;

        void _waitOnFrame( uint8 frameIdx );
        bool _isFrameFinished( uint8 frameIdx );

        void commitAndNextCommandBuffer(
            SubmissionType::SubmissionType submissionType = SubmissionType::FlushOnly );

        VulkanVaoManager *getVaoManager() { return mVaoManager; }
    };

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
