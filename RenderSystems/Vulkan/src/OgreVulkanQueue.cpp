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

#include "OgreVulkanQueue.h"

#include "OgreVulkanRenderSystem.h"
#include "OgreVulkanWindow.h"
#include "Vao/OgreVulkanVaoManager.h"

#include "OgreException.h"
#include "OgreStringConverter.h"

#include "OgreVulkanUtils.h"

#include <vulkan/vulkan.h>

#define TODO_resetCmdPools
#define TODO_removeCmdBuffer
#define TODO_findRealPresentQueue

namespace Ogre
{
    VulkanQueue::VulkanQueue() :
        mDevice( 0 ),
        mFamily( NumQueueFamilies ),
        mFamilyIdx( 0u ),
        mQueueIdx( 0u ),
        mQueue( 0 ),
        mCurrentCmdBuffer( 0 ),
        mVaoManager( 0 ),
        mRenderSystem( 0 )
    {
    }
    //-------------------------------------------------------------------------
    VulkanQueue::~VulkanQueue()
    {
        if( mDevice )
        {
            vkDeviceWaitIdle( mDevice );

            VkFenceArray::const_iterator itor = mFrameFence.begin();
            VkFenceArray::const_iterator endt = mFrameFence.end();

            while( itor != endt )
                vkDestroyFence( mDevice, *itor++, 0 );

            mFrameFence.clear();
        }
    }
    //-------------------------------------------------------------------------
    void VulkanQueue::setQueueData( QueueFamily family, uint32 familyIdx, uint32 queueIdx )
    {
        mFamily = family;
        mFamilyIdx = familyIdx;
        mQueueIdx = queueIdx;
    }
    //-------------------------------------------------------------------------
    void VulkanQueue::init( VkDevice device, VkQueue queue, VulkanRenderSystem *renderSystem )
    {
        mDevice = device;
        mQueue = queue;
        mVaoManager = static_cast<VulkanVaoManager *>( renderSystem->getVaoManager() );
        mRenderSystem = renderSystem;

        const size_t maxNumFrames = mVaoManager->getDynamicBufferMultiplier();

        VkDeviceQueueCreateInfo queueCreateInfo;
        makeVkStruct( queueCreateInfo, VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO );

        mFrameFence.resize( maxNumFrames );

        VkFenceCreateInfo fenceCi;
        makeVkStruct( fenceCi, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO );
        for( size_t i = 0; i < maxNumFrames; ++i )
        {
            VkResult result = vkCreateFence( mDevice, &fenceCi, 0, &mFrameFence[i] );
            checkVkResult( result, "vkCreateFence" );
        }

        // Create one cmd pool per thread (assume single threaded for now)
        mCommandPools.resize( maxNumFrames );
        VkCommandPoolCreateInfo cmdPoolCreateInfo;
        makeVkStruct( cmdPoolCreateInfo, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO );
        cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        cmdPoolCreateInfo.queueFamilyIndex = mFamilyIdx;

        for( size_t i = 0; i < maxNumFrames; ++i )
            vkCreateCommandPool( mDevice, &cmdPoolCreateInfo, 0, &mCommandPools[i] );

        newCommandBuffer();
    }
    //-------------------------------------------------------------------------
    void VulkanQueue::newCommandBuffer( void )
    {
        TODO_resetCmdPools;
        TODO_removeCmdBuffer;

        const size_t currFrame = mVaoManager->waitForTailFrameToFinish();

        VkCommandBufferAllocateInfo allocateInfo;
        makeVkStruct( allocateInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO );
        allocateInfo.commandPool = mCommandPools[currFrame];
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1u;
        VkResult result = vkAllocateCommandBuffers( mDevice, &allocateInfo, &mCurrentCmdBuffer );
        checkVkResult( result, "vkAllocateCommandBuffers" );

        VkCommandBufferBeginInfo beginInfo;
        makeVkStruct( beginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO );
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer( mCurrentCmdBuffer, &beginInfo );
    }
    //-------------------------------------------------------------------------
    void VulkanQueue::endCommandBuffer( void )
    {
        TODO_removeCmdBuffer;
        if( mCurrentCmdBuffer )
        {
            VkResult result = vkEndCommandBuffer( mCurrentCmdBuffer );
            checkVkResult( result, "vkEndCommandBuffer" );

            mPendingCmds.push_back( mCurrentCmdBuffer );
            mCurrentCmdBuffer = 0;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanQueue::addWindowToWaitFor( VkSemaphore imageAcquisitionSemaph )
    {
        OGRE_ASSERT_MEDIUM( mFamily == Graphics );
        mGpuWaitFlags.push_back( VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT );
        mGpuWaitSemaphForCurrCmdBuff.push_back( imageAcquisitionSemaph );
    }
    //-------------------------------------------------------------------------
    void VulkanQueue::commitAndNextCommandBuffer( void )
    {
        VkSubmitInfo submitInfo;
        makeVkStruct( submitInfo, VK_STRUCTURE_TYPE_SUBMIT_INFO );

        endCommandBuffer();

        if( mPendingCmds.empty() )
            return;

        const size_t windowsSemaphStart = mGpuSignalSemaphForCurrCmdBuff.size();
        const size_t numWindowsPendingSwap = mWindowsPendingSwap.size();
        mVaoManager->getAvailableSempaphores( mGpuSignalSemaphForCurrCmdBuff, numWindowsPendingSwap );

        if( !mGpuWaitSemaphForCurrCmdBuff.empty() )
        {
            submitInfo.waitSemaphoreCount = static_cast<uint32>( mGpuWaitSemaphForCurrCmdBuff.size() );
            submitInfo.pWaitSemaphores = mGpuWaitSemaphForCurrCmdBuff.begin();
            submitInfo.pWaitDstStageMask = mGpuWaitFlags.begin();
        }
        if( !mGpuSignalSemaphForCurrCmdBuff.empty() )
        {
            submitInfo.signalSemaphoreCount =
                static_cast<uint32>( mGpuSignalSemaphForCurrCmdBuff.size() );
            submitInfo.pSignalSemaphores = mGpuSignalSemaphForCurrCmdBuff.begin();
        }
        // clang-format off
        submitInfo.commandBufferCount   = static_cast<uint32>( mPendingCmds.size() );
        submitInfo.pCommandBuffers      = &mPendingCmds[0];
        // clang-format on

        const uint8 dynBufferFrame = mVaoManager->waitForTailFrameToFinish();
        vkQueueSubmit( mQueue, 1u, &submitInfo, mFrameFence[dynBufferFrame] );

        newCommandBuffer();

        for( size_t windowIdx = 0u; windowIdx < numWindowsPendingSwap; ++windowIdx )
        {
            mWindowsPendingSwap[windowIdx]->_swapBuffers(
                mGpuSignalSemaphForCurrCmdBuff[windowsSemaphStart + windowIdx] );
        }
        mWindowsPendingSwap.clear();

        mGpuWaitSemaphForCurrCmdBuff.clear();
        mGpuSignalSemaphForCurrCmdBuff.clear();
    }
}  // namespace Ogre
