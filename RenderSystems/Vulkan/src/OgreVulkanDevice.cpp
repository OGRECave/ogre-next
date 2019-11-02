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

#include "OgreVulkanDevice.h"

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
    VulkanDevice::VulkanDevice( VkInstance instance, uint32 deviceIdx,
                                VulkanRenderSystem *renderSystem ) :
        mInstance( instance ),
        mPhysicalDevice( 0 ),
        mDevice( 0 ),
        mPresentQueue( 0 ),
        mVaoManager( 0 ),
        mRenderSystem( renderSystem )
    {
        memset( mQueues, 0, sizeof( mQueues ) );
        memset( mCurrentCmdBuffer, 0, sizeof( mCurrentCmdBuffer ) );
        memset( &mMemoryProperties, 0, sizeof( mMemoryProperties ) );
        createPhysicalDevice( deviceIdx );
    }
    //-------------------------------------------------------------------------
    VulkanDevice::~VulkanDevice()
    {
        if( mDevice )
        {
            vkDeviceWaitIdle( mDevice );

            FastArray<FrameFence>::const_iterator itor = mFrameFence.begin();
            FastArray<FrameFence>::const_iterator endt = mFrameFence.end();

            while( itor != endt )
            {
                for( size_t j = 0u; j < NumQueueFamilies; ++j )
                    vkDestroyFence( mDevice, itor->fence[j], 0 );

                ++itor;
            }

            mFrameFence.clear();

            vkDestroyDevice( mDevice, 0 );
            mDevice = 0;
            mPhysicalDevice = 0;
        }
    }
    //-------------------------------------------------------------------------
    VkDebugReportCallbackCreateInfoEXT VulkanDevice::addDebugCallback(
        PFN_vkDebugReportCallbackEXT debugCallback )
    {
        // This is info for a temp callback to use during CreateInstance.
        // After the instance is created, we use the instance-based
        // function to register the final callback.
        VkDebugReportCallbackCreateInfoEXT dbgCreateInfoTemp;
        makeVkStruct( dbgCreateInfoTemp, VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT );
        dbgCreateInfoTemp.pfnCallback = debugCallback;
        dbgCreateInfoTemp.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        return dbgCreateInfoTemp;
    }
    //-------------------------------------------------------------------------
    VkInstance VulkanDevice::createInstance( const String &appName, FastArray<const char *> &extensions,
                                             FastArray<const char *> &layers,
                                             PFN_vkDebugReportCallbackEXT debugCallback )
    {
        VkInstanceCreateInfo createInfo;
        VkApplicationInfo appInfo;
        memset( &createInfo, 0, sizeof( createInfo ) );
        memset( &appInfo, 0, sizeof( appInfo ) );

        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = appName.c_str();
        appInfo.pEngineName = "Ogre3D Vulkan Engine";
        appInfo.engineVersion = OGRE_VERSION;
        appInfo.apiVersion = VK_MAKE_VERSION( 1, 0, 2 );

        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        createInfo.enabledLayerCount = static_cast<uint32>( layers.size() );
        createInfo.ppEnabledLayerNames = layers.begin();

        extensions.push_back( VK_KHR_SURFACE_EXTENSION_NAME );

        createInfo.enabledExtensionCount = static_cast<uint32>( extensions.size() );
        createInfo.ppEnabledExtensionNames = extensions.begin();

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
        VkDebugReportCallbackCreateInfoEXT debugCb = addDebugCallback( debugCallback );
        createInfo.pNext = &debugCb;
#endif

        VkInstance instance;
        VkResult result = vkCreateInstance( &createInfo, 0, &instance );
        checkVkResult( result, "vkCreateInstance" );

        return instance;
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::createPhysicalDevice( uint32 deviceIdx )
    {
        VkResult result = VK_SUCCESS;

        // Note multiple GPUs may be present, and there may be multiple drivers for
        // each GPU hence the number of devices can theoretically get really high
        const uint32_t c_maxDevices = 64u;
        uint32 numDevices = 0u;
        result = vkEnumeratePhysicalDevices( mInstance, &numDevices, NULL );
        checkVkResult( result, "vkEnumeratePhysicalDevices" );

        if( numDevices == 0u )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "No Vulkan devices found.",
                         "VulkanDevice::createPhysicalDevice" );
        }

        numDevices = std::min( numDevices, c_maxDevices );

        const String numDevicesStr = StringConverter::toString( numDevices );
        String deviceIdsStr = StringConverter::toString( deviceIdx );

        LogManager::getSingleton().logMessage( "[Vulkan] Found " + numDevicesStr + " devices" );

        if( deviceIdx >= numDevices )
        {
            LogManager::getSingleton().logMessage( "[Vulkan] Requested device index " + deviceIdsStr +
                                                   " but there's only " +
                                                   StringConverter::toString( numDevices ) + "devices" );
            deviceIdx = 0u;
            deviceIdsStr = "0";
        }

        LogManager::getSingleton().logMessage( "[Vulkan] Selecting device " + deviceIdsStr );

        VkPhysicalDevice pd[c_maxDevices];
        result = vkEnumeratePhysicalDevices( mInstance, &numDevices, pd );
        checkVkResult( result, "vkEnumeratePhysicalDevices" );
        mPhysicalDevice = pd[0];

        vkGetPhysicalDeviceMemoryProperties( mPhysicalDevice, &mMemoryProperties );
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::calculateQueueIdx( QueueFamily family )
    {
        uint32 queueIdx = 0u;
        for( size_t i = 0u; i < family; ++i )
        {
            if( mSelectedQueues[family].familyIdx == mSelectedQueues[i].familyIdx )
                ++queueIdx;
        }
        const size_t familyIdx = mSelectedQueues[family].familyIdx;
        queueIdx = std::min( queueIdx, mQueueProps[familyIdx].queueCount - 1u );
        mSelectedQueues[family].queueIdx = queueIdx;
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::fillQueueSelectionData( VkDeviceQueueCreateInfo *outQueueCreateInfo,
                                               uint32 &outNumQueues )
    {
        uint32 numQueues = 0u;

        bool queueInserted[NumQueueFamilies];
        for( size_t i = 0u; i < NumQueueFamilies; ++i )
            queueInserted[i] = false;

        for( size_t i = 1u; i < NumQueueFamilies; ++i )
            calculateQueueIdx( static_cast<QueueFamily>( i ) );

        for( size_t i = 0u; i < NumQueueFamilies; ++i )
        {
            if( !queueInserted[i] )
            {
                outQueueCreateInfo[numQueues].queueFamilyIndex = mSelectedQueues[i].familyIdx;
                outQueueCreateInfo[numQueues].queueCount = 1u;
                queueInserted[i] = true;
                for( size_t j = i + 1u; j < NumQueueFamilies; ++j )
                {
                    if( mSelectedQueues[i].familyIdx == mSelectedQueues[j].familyIdx )
                    {
                        outQueueCreateInfo[numQueues].queueCount =
                            std::max( mSelectedQueues[i].queueIdx, mSelectedQueues[j].queueIdx ) + 1u;
                        queueInserted[j] = true;
                    }
                }
                ++numQueues;
            }
        }

        outNumQueues = numQueues;
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::createDevice( FastArray<const char *> &extensions, size_t maxNumFrames )
    {
        uint32 numQueues;
        vkGetPhysicalDeviceQueueFamilyProperties( mPhysicalDevice, &numQueues, NULL );
        if( numQueues == 0u )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "Vulkan device is reporting 0 queues!",
                         "VulkanDevice::createDevice" );
        }
        mQueueProps.resize( numQueues );
        vkGetPhysicalDeviceQueueFamilyProperties( mPhysicalDevice, &numQueues, &mQueueProps[0] );

        for( size_t i = 0; i < NumQueueFamilies; ++i )
            mSelectedQueues[i] = SelectedQueue();

        for( uint32 i = 0u; i < numQueues; ++i )
        {
            if( ( mQueueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) &&
                !mSelectedQueues[Graphics].hasValidFamily() )
            {
                mSelectedQueues[Graphics].familyIdx = i;
            }

            if( ( mQueueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT ) )
            {
                // Prefer *not* sharing compute and graphics in the same Queue
                // Note some GPUs may advertise a queue family that has both
                // graphics & compute and support multiples queues. That's fine!
                const uint32 familyIdx = mSelectedQueues[Compute].familyIdx;
                if( !mSelectedQueues[Compute].hasValidFamily() ||
                    ( ( mQueueProps[familyIdx].queueFlags & VK_QUEUE_GRAPHICS_BIT ) &&
                      mQueueProps[familyIdx].queueCount == 1u ) )
                {
                    mSelectedQueues[Compute].familyIdx = i;
                }
            }

            if( ( mQueueProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT ) )
            {
                // Prefer the transfer queue that doesn't share with anything else!
                const uint32 familyIdx = mSelectedQueues[Transfer].familyIdx;
                if( !mSelectedQueues[Transfer].hasValidFamily() ||
                    !( mQueueProps[familyIdx].queueFlags & ( uint32 )( ~VK_QUEUE_TRANSFER_BIT ) ) )
                {
                    mSelectedQueues[Transfer].familyIdx = i;
                }
            }
        }

        // Graphics and Compute queues are implicitly Transfer; and drivers are
        // not required to advertise the transfer bit on those queues.
        if( !mSelectedQueues[Transfer].hasValidFamily() )
            mSelectedQueues[Transfer] = mSelectedQueues[Graphics];

        uint32 numQueuesToCreate = 0u;
        VkDeviceQueueCreateInfo queueCreateInfo[NumQueueFamilies];
        memset( &queueCreateInfo, 0, sizeof( queueCreateInfo ) );
        for( size_t i = 0; i < NumQueueFamilies; ++i )
            queueCreateInfo[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        fillQueueSelectionData( queueCreateInfo, numQueuesToCreate );

        FastArray<float> queuePriorities[NumQueueFamilies];
        for( size_t i = 0u; i < numQueuesToCreate; ++i )
        {
            queuePriorities[i].resize( queueCreateInfo[i].queueCount, 1.0f );
            queueCreateInfo[i].pQueuePriorities = queuePriorities[i].begin();
        }

        extensions.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );

        VkDeviceCreateInfo createInfo;
        memset( &createInfo, 0, sizeof( createInfo ) );

        createInfo.enabledExtensionCount = static_cast<uint32>( extensions.size() );
        createInfo.ppEnabledExtensionNames = extensions.begin();

        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = numQueuesToCreate;
        createInfo.pQueueCreateInfos = queueCreateInfo;

        VkResult result = vkCreateDevice( mPhysicalDevice, &createInfo, NULL, &mDevice );
        checkVkResult( result, "vkCreateDevice" );

        for( uint32 i = 0u; i < NumQueueFamilies; ++i )
        {
            if( mSelectedQueues[i].hasValidFamily() )
            {
                vkGetDeviceQueue( mDevice, mSelectedQueues[i].familyIdx, mSelectedQueues[i].queueIdx,
                                  &mQueues[i] );
            }
            else
            {
                mQueues[i] = 0;
            }
        }

        mFrameFence.resize( maxNumFrames );

        VkFenceCreateInfo fenceCi;
        makeVkStruct( fenceCi, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO );
        for( size_t i = 0; i < maxNumFrames; ++i )
        {
            for( size_t j = 0u; j < NumQueueFamilies; ++j )
            {
                result = vkCreateFence( mDevice, &fenceCi, 0, &mFrameFence[i].fence[j] );
                checkVkResult( result, "vkCreateFence" );
            }
        }

        // Create one cmd pool per queue family, per thread (assume single threaded for now)
        FastArray<VkCommandPool> commandPools;
        commandPools.resize( maxNumFrames );
        VkCommandPoolCreateInfo cmdPoolCreateInfo;
        memset( &cmdPoolCreateInfo, 0, sizeof( cmdPoolCreateInfo ) );
        cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        for( size_t i = 0u; i < numQueuesToCreate; ++i )
        {
            cmdPoolCreateInfo.queueFamilyIndex = createInfo.pQueueCreateInfos[i].queueFamilyIndex;

            for( size_t j = 0; j < maxNumFrames; ++j )
                vkCreateCommandPool( mDevice, &cmdPoolCreateInfo, 0, &commandPools[j] );

            for( size_t j = 0u; j < NumQueueFamilies; ++j )
            {
                if( mSelectedQueues[j].familyIdx == cmdPoolCreateInfo.queueFamilyIndex )
                    mCommandPools[j] = commandPools;
            }
        }

        TODO_findRealPresentQueue;
        mPresentQueue = mQueues[Graphics];
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::newCommandBuffer( QueueFamily family )
    {
        TODO_resetCmdPools;
        TODO_removeCmdBuffer;

        const size_t currFrame = mVaoManager->waitForTailFrameToFinish();

        VkCommandBufferAllocateInfo allocateInfo;
        makeVkStruct( allocateInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO );
        allocateInfo.commandPool = mCommandPools[family][currFrame];
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1u;
        VkResult result = vkAllocateCommandBuffers( mDevice, &allocateInfo, &mCurrentCmdBuffer[family] );
        checkVkResult( result, "vkAllocateCommandBuffers" );

        VkCommandBufferBeginInfo beginInfo;
        makeVkStruct( beginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO );
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer( mCurrentCmdBuffer[family], &beginInfo );
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::endCommandBuffer( VulkanDevice::QueueFamily family )
    {
        TODO_removeCmdBuffer;
        if( mCurrentCmdBuffer[family] )
        {
            VkResult result = vkEndCommandBuffer( mCurrentCmdBuffer[family] );
            checkVkResult( result, "vkEndCommandBuffer" );

            mPendingCmds[family].push_back( mCurrentCmdBuffer[family] );
            mCurrentCmdBuffer[family] = 0;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::addWindowToWaitFor( VkSemaphore imageAcquisitionSemaph )
    {
        mGpuWaitFlags[VulkanDevice::Graphics].push_back( VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT );
        mGpuWaitSemaphForCurrCmdBuff[VulkanDevice::Graphics].push_back( imageAcquisitionSemaph );
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::commitAndNextCommandBuffer( void )
    {
        const uint8 dynBufferFrame = mVaoManager->waitForTailFrameToFinish();

        VkSubmitInfo submitInfo;
        makeVkStruct( submitInfo, VK_STRUCTURE_TYPE_SUBMIT_INFO );

        for( size_t i = 0u; i < NumQueueFamilies; ++i )
        {
            QueueFamily family = static_cast<QueueFamily>( i );

            endCommandBuffer( family );

            if( mPendingCmds[i].empty() )
                continue;

            const size_t windowsSemaphStart = mGpuSignalSemaphForCurrCmdBuff[family].size();
            const size_t numWindowsPendingSwap = mWindowsPendingSwap.size();
            mVaoManager->getAvailableSempaphores( mGpuSignalSemaphForCurrCmdBuff[family],
                                                  numWindowsPendingSwap );

            if( !mGpuWaitSemaphForCurrCmdBuff[family].empty() )
            {
                submitInfo.waitSemaphoreCount =
                    static_cast<uint32>( mGpuWaitSemaphForCurrCmdBuff[family].size() );
                submitInfo.pWaitSemaphores = mGpuWaitSemaphForCurrCmdBuff[family].begin();
                submitInfo.pWaitDstStageMask = mGpuWaitFlags[family].begin();
            }
            if( !mGpuSignalSemaphForCurrCmdBuff[family].empty() )
            {
                submitInfo.signalSemaphoreCount =
                    static_cast<uint32>( mGpuSignalSemaphForCurrCmdBuff[family].size() );
                submitInfo.pSignalSemaphores = mGpuSignalSemaphForCurrCmdBuff[family].begin();
            }
            // clang-format off
            submitInfo.commandBufferCount   = static_cast<uint32>( mPendingCmds[i].size() );
            submitInfo.pCommandBuffers      = &mPendingCmds[i][0];
            // clang-format on

            vkQueueSubmit( mQueues[family], 1u, &submitInfo, mFrameFence[dynBufferFrame].fence[family] );

            newCommandBuffer( family );

            for( size_t windowIdx = 0u; windowIdx < numWindowsPendingSwap; ++windowIdx )
            {
                mWindowsPendingSwap[windowIdx]->_swapBuffers(
                    mGpuSignalSemaphForCurrCmdBuff[family][windowsSemaphStart + windowIdx] );
            }
            mWindowsPendingSwap.clear();

            mGpuWaitSemaphForCurrCmdBuff[family].clear();
            mGpuSignalSemaphForCurrCmdBuff[family].clear();
        }
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::stall( void ) { vkDeviceWaitIdle( mDevice ); }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    VulkanDevice::SelectedQueue::SelectedQueue() :
        familyIdx( std::numeric_limits<uint32>::max() ),
        queueIdx( 0 )
    {
    }
    //-------------------------------------------------------------------------
    bool VulkanDevice::SelectedQueue::hasValidFamily( void ) const
    {
        return familyIdx != std::numeric_limits<uint32>::max();
    }
}  // namespace Ogre
