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

        VkDevice mDevice;
        QueueFamily mFamily;

        uint32 mFamilyIdx;
        uint32 mQueueIdx;

        VkQueue mQueue;
        VkCommandBuffer mCurrentCmdBuffer;

    protected:
        // clang-format off
        // One per buffered frame
        FastArray<VkCommandPool>    mCommandPools;

        /// Collection of semaphore we need to wait on before our queue executes
        /// pending commands when commitAndNextCommandBuffer is called
        VkSemaphoreArray                mGpuWaitSemaphForCurrCmdBuff;
        FastArray<VkPipelineStageFlags> mGpuWaitFlags;

        /// Collection of semaphore we will signal when our queue
        /// submitted in commitAndNextCommandBuffer is done
        VkSemaphoreArray                mGpuSignalSemaphForCurrCmdBuff;
        VkFenceArray                    mFrameFence;
        // clang-format on

    public:
        FastArray<VulkanWindow *> mWindowsPendingSwap;

    protected:
        FastArray<VkCommandBuffer> mPendingCmds;

        VulkanVaoManager *mVaoManager;
        VulkanRenderSystem *mRenderSystem;

    public:
        VulkanQueue();
        ~VulkanQueue();

        void setQueueData( QueueFamily family, uint32 familyIdx, uint32 queueIdx );

        void init( VkDevice device, VkQueue queue, VulkanRenderSystem *renderSystem );
        void destroy( void );

        void newCommandBuffer( void );

    protected:
        void endCommandBuffer( void );

    public:
        /// When we'll call commitAndNextCommandBuffer, we'll have to wait for
        /// this semaphore on to execute STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        void addWindowToWaitFor( VkSemaphore imageAcquisitionSemaph );

        void commitAndNextCommandBuffer( void );
    };

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
