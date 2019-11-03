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

#define TODO_findRealPresentQueue

#define TODO

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

            {
                FastArray<PerFrameData>::iterator itor = mPerFrameData.begin();
                FastArray<PerFrameData>::iterator endt = mPerFrameData.end();

                while( itor != endt )
                {
                    VkFenceArray::const_iterator itFence = itor->mProtectingFences.begin();
                    VkFenceArray::const_iterator enFence = itor->mProtectingFences.end();

                    while( itFence != enFence )
                        vkDestroyFence( mDevice, *itFence++, 0 );

                    itor->mProtectingFences.clear();
                }
            }

            VkFenceArray::const_iterator itor = mAvailableFences.begin();
            VkFenceArray::const_iterator endt = mAvailableFences.end();

            while( itor != endt )
                vkDestroyFence( mDevice, *itor++, 0 );

            mAvailableFences.clear();
        }
    }
    //-------------------------------------------------------------------------
    VkFence VulkanQueue::getFence( void )
    {
        VkFence retVal;
        if( !mAvailableFences.empty() )
        {
            retVal = mAvailableFences.back();
            mAvailableFences.pop_back();
        }
        else
        {
            VkFenceCreateInfo fenceCi;
            makeVkStruct( fenceCi, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO );
            VkResult result = vkCreateFence( mDevice, &fenceCi, 0, &retVal );
            checkVkResult( result, "vkCreateFence" );
        }
        return retVal;
    }
    //-------------------------------------------------------------------------
    VkCommandBuffer VulkanQueue::getCmdBuffer( size_t currFrame )
    {
        PerFrameData &frameData = mPerFrameData[currFrame];

        if( frameData.mCurrentCmdIdx >= frameData.mCommands.size() )
        {
            VkCommandBuffer cmdBuffer;

            VkCommandBufferAllocateInfo allocateInfo;
            makeVkStruct( allocateInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO );
            allocateInfo.commandPool = frameData.mCmdPool;
            allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocateInfo.commandBufferCount = 1u;
            VkResult result = vkAllocateCommandBuffers( mDevice, &allocateInfo, &cmdBuffer );
            checkVkResult( result, "vkAllocateCommandBuffers" );

            frameData.mCommands.push_back( cmdBuffer );
        }
        else if( frameData.mCurrentCmdIdx == 0u )
            vkResetCommandPool( mDevice, frameData.mCmdPool, 0 );

        return frameData.mCommands[frameData.mCurrentCmdIdx++];
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

        // Create at least maxNumFrames fences, though we may need more
        mAvailableFences.resize( maxNumFrames );

        VkFenceCreateInfo fenceCi;
        makeVkStruct( fenceCi, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO );
        for( size_t i = 0; i < maxNumFrames; ++i )
        {
            VkResult result = vkCreateFence( mDevice, &fenceCi, 0, &mAvailableFences[i] );
            checkVkResult( result, "vkCreateFence" );
        }

        // Create one cmd pool per thread (assume single threaded for now)
        mPerFrameData.resize( maxNumFrames );
        VkCommandPoolCreateInfo cmdPoolCreateInfo;
        makeVkStruct( cmdPoolCreateInfo, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO );
        cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        cmdPoolCreateInfo.queueFamilyIndex = mFamilyIdx;

        for( size_t i = 0; i < maxNumFrames; ++i )
            vkCreateCommandPool( mDevice, &cmdPoolCreateInfo, 0, &mPerFrameData[i].mCmdPool );

        newCommandBuffer();
    }
    //-------------------------------------------------------------------------
    void VulkanQueue::newCommandBuffer( void )
    {
        const size_t currFrame = mVaoManager->waitForTailFrameToFinish();
        mCurrentCmdBuffer = getCmdBuffer( currFrame );

        VkCommandBufferBeginInfo beginInfo;
        makeVkStruct( beginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO );
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer( mCurrentCmdBuffer, &beginInfo );
    }
    //-------------------------------------------------------------------------
    void VulkanQueue::endCommandBuffer( void )
    {
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
    void VulkanQueue::_waitOnFrame( uint8 frameIdx )
    {
        FastArray<VkFence> &fences = mPerFrameData[frameIdx].mProtectingFences;

        if( !fences.empty() )
        {
            const uint32 numFences = static_cast<uint32>( fences.size() );
            vkWaitForFences( mDevice, numFences, &fences[0], VK_TRUE, UINT64_MAX );
            vkResetFences( mDevice, numFences, &fences[0] );
            mAvailableFences.appendPOD( fences.begin(), fences.end() );
            fences.clear();
        }
    }
    //-------------------------------------------------------------------------
    void VulkanQueue::commitAndNextCommandBuffer( bool endingFrame )
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
        VkFence fence = getFence();

        vkQueueSubmit( mQueue, 1u, &submitInfo, fence );

        mPerFrameData[dynBufferFrame].mProtectingFences.push_back( fence );

        mPendingCmds.clear();

        if( endingFrame )
        {
            mPerFrameData[dynBufferFrame].mCurrentCmdIdx = 0u;
            mVaoManager->_notifyNewCommandBuffer();
        }

        newCommandBuffer();

        for( size_t windowIdx = 0u; windowIdx < numWindowsPendingSwap; ++windowIdx )
        {
            VkSemaphore semaphore = mGpuSignalSemaphForCurrCmdBuff[windowsSemaphStart + windowIdx];
            mWindowsPendingSwap[windowIdx]->_swapBuffers( semaphore );
            TODO;
            // mVaoManager->notifyWaitSemaphoreSubmitted( semaphore );
        }
        mWindowsPendingSwap.clear();

        TODO;
        // mVaoManager->notifyWaitSemaphoresSubmitted( mGpuWaitSemaphForCurrCmdBuff );

        mGpuWaitSemaphForCurrCmdBuff.clear();
        mGpuSignalSemaphForCurrCmdBuff.clear();
    }
}  // namespace Ogre
