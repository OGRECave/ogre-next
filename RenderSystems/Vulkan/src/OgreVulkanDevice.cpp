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

#include "OgreVulkanDevice.h"

#include "OgreVulkanRenderSystem.h"
#include "OgreVulkanWindow.h"
#include "Vao/OgreVulkanVaoManager.h"

#include "OgreException.h"
#include "OgreStringConverter.h"

#include "OgreVulkanUtils.h"

#include <vulkan/vulkan.h>

#define TODO_findRealPresentQueue

namespace Ogre
{
    static FastArray<IdString> msInstanceExtensions;

    VulkanDevice::VulkanDevice( VkInstance instance, uint32 deviceIdx,
                                VulkanRenderSystem *renderSystem ) :
        mInstance( instance ),
        mPhysicalDevice( 0 ),
        mDevice( 0 ),
        mPresentQueue( 0 ),
        mVaoManager( 0 ),
        mRenderSystem( renderSystem ),
        mSupportedStages( 0xFFFFFFFF )
    {
        memset( &mDeviceMemoryProperties, 0, sizeof( mDeviceMemoryProperties ) );
        createPhysicalDevice( deviceIdx );
    }
    //-------------------------------------------------------------------------
    VulkanDevice::~VulkanDevice()
    {
        if( mDevice )
        {
            vkDeviceWaitIdle( mDevice );

            mGraphicsQueue.destroy();
            destroyQueues( mComputeQueues );
            destroyQueues( mTransferQueues );

            // Must be done externally (this' destructor has yet to free more Vulkan stuff)
            // vkDestroyDevice( mDevice, 0 );
            mDevice = 0;
            mPhysicalDevice = 0;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::destroyQueues( FastArray<VulkanQueue> &queueArray )
    {
        FastArray<VulkanQueue>::iterator itor = queueArray.begin();
        FastArray<VulkanQueue>::iterator endt = queueArray.end();

        while( itor != endt )
        {
            itor->destroy();
            ++itor;
        }
        queueArray.clear();
    }
    //-------------------------------------------------------------------------
    VkDebugReportCallbackCreateInfoEXT VulkanDevice::addDebugCallback(
        PFN_vkDebugReportCallbackEXT debugCallback, RenderSystem *renderSystem )
    {
        // This is info for a temp callback to use during CreateInstance.
        // After the instance is created, we use the instance-based
        // function to register the final callback.
        VkDebugReportCallbackCreateInfoEXT dbgCreateInfoTemp;
        makeVkStruct( dbgCreateInfoTemp, VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT );
        dbgCreateInfoTemp.pfnCallback = debugCallback;
        dbgCreateInfoTemp.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        dbgCreateInfoTemp.pUserData = renderSystem;
        return dbgCreateInfoTemp;
    }
    //-------------------------------------------------------------------------
    VkInstance VulkanDevice::createInstance( const String &appName, FastArray<const char *> &extensions,
                                             FastArray<const char *> &layers,
                                             PFN_vkDebugReportCallbackEXT debugCallback,
                                             RenderSystem *renderSystem )
    {
        VkInstanceCreateInfo createInfo;
        VkApplicationInfo appInfo;
        memset( &createInfo, 0, sizeof( createInfo ) );
        memset( &appInfo, 0, sizeof( appInfo ) );

        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        if( !appName.empty() )
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
        VkDebugReportCallbackCreateInfoEXT debugCb = addDebugCallback( debugCallback, renderSystem );
        createInfo.pNext = &debugCb;
#endif

        {
            msInstanceExtensions.clear();
            msInstanceExtensions.reserve( extensions.size() );

            FastArray<const char *>::const_iterator itor = extensions.begin();
            FastArray<const char *>::const_iterator endt = extensions.end();

            while( itor != endt )
            {
                LogManager::getSingleton().logMessage( "Requesting Instance Extension: " +
                                                       String( *itor ) );
                msInstanceExtensions.push_back( *itor );
                ++itor;
            }

            std::sort( msInstanceExtensions.begin(), msInstanceExtensions.end() );
        }

        VkInstance instance;
        VkResult result = vkCreateInstance( &createInfo, 0, &instance );
        checkVkResult( result, "vkCreateInstance" );

        return instance;
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::createPhysicalDevice( uint32 deviceIdx )
    {
        VkResult result = VK_SUCCESS;

        uint32 numDevices = 0u;
        result = vkEnumeratePhysicalDevices( mInstance, &numDevices, NULL );
        checkVkResult( result, "vkEnumeratePhysicalDevices" );

        if( numDevices == 0u )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "No Vulkan devices found.",
                         "VulkanDevice::createPhysicalDevice" );
        }

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

        FastArray<VkPhysicalDevice> pd;
        pd.resize( numDevices );
        result = vkEnumeratePhysicalDevices( mInstance, &numDevices, pd.begin() );
        checkVkResult( result, "vkEnumeratePhysicalDevices" );
        mPhysicalDevice = pd[deviceIdx];

        vkGetPhysicalDeviceMemoryProperties( mPhysicalDevice, &mDeviceMemoryProperties );

        // mDeviceExtraFeatures gets initialized later, once we analyzed all the extensions
        memset( &mDeviceExtraFeatures, 0, sizeof( mDeviceExtraFeatures ) );
        vkGetPhysicalDeviceFeatures( mPhysicalDevice, &mDeviceFeatures );

        mSupportedStages = 0xFFFFFFFF;
        if( !mDeviceFeatures.geometryShader )
            mSupportedStages ^= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
        if( !mDeviceFeatures.tessellationShader )
        {
            mSupportedStages ^= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
                                VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::findGraphicsQueue( FastArray<uint32> &inOutUsedQueueCount )
    {
        const size_t numQueues = mQueueProps.size();
        for( size_t i = 0u; i < numQueues; ++i )
        {
            if( mQueueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
                inOutUsedQueueCount[i] < mQueueProps[i].queueCount )
            {
                mGraphicsQueue.setQueueData( this, VulkanQueue::Graphics, static_cast<uint32>( i ),
                                             inOutUsedQueueCount[i] );
                ++inOutUsedQueueCount[i];
                return;
            }
        }

        OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                     "GPU does not expose Graphics queue. Cannot be used for rendering",
                     "VulkanQueue::findGraphicsQueue" );
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::findComputeQueue( FastArray<uint32> &inOutUsedQueueCount, uint32 maxNumQueues )
    {
        const size_t numQueues = mQueueProps.size();
        for( size_t i = 0u; i < numQueues && mComputeQueues.size() < maxNumQueues; ++i )
        {
            if( mQueueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT &&
                inOutUsedQueueCount[i] < mQueueProps[i].queueCount )
            {
                mComputeQueues.push_back( VulkanQueue() );
                mComputeQueues.back().setQueueData( this, VulkanQueue::Compute, static_cast<uint32>( i ),
                                                    inOutUsedQueueCount[i] );
                ++inOutUsedQueueCount[i];
            }
        }
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::findTransferQueue( FastArray<uint32> &inOutUsedQueueCount, uint32 maxNumQueues )
    {
        const size_t numQueues = mQueueProps.size();
        for( size_t i = 0u; i < numQueues && mTransferQueues.size() < maxNumQueues; ++i )
        {
            if( mQueueProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT &&
                !( mQueueProps[i].queueFlags & ( VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) ) &&
                inOutUsedQueueCount[i] < mQueueProps[i].queueCount )
            {
                mTransferQueues.push_back( VulkanQueue() );
                mTransferQueues.back().setQueueData( this, VulkanQueue::Transfer,
                                                     static_cast<uint32>( i ), inOutUsedQueueCount[i] );
                ++inOutUsedQueueCount[i];
            }
        }
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::fillQueueCreationInfo( uint32 maxComputeQueues, uint32 maxTransferQueues,
                                              FastArray<VkDeviceQueueCreateInfo> &outQueueCiArray )
    {
        const size_t numQueueFamilies = mQueueProps.size();

        FastArray<uint32> usedQueueCount;
        usedQueueCount.resize( numQueueFamilies, 0u );

        findGraphicsQueue( usedQueueCount );
        findComputeQueue( usedQueueCount, maxComputeQueues );
        findTransferQueue( usedQueueCount, maxTransferQueues );

        VkDeviceQueueCreateInfo queueCi;
        makeVkStruct( queueCi, VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO );

        for( size_t i = 0u; i < numQueueFamilies; ++i )
        {
            queueCi.queueFamilyIndex = static_cast<uint32>( i );
            queueCi.queueCount = usedQueueCount[i];
            if( queueCi.queueCount > 0u )
                outQueueCiArray.push_back( queueCi );
        }
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::createDevice( FastArray<const char *> &extensions, uint32 maxComputeQueues,
                                     uint32 maxTransferQueues )
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

        // Setup queue creation
        FastArray<VkDeviceQueueCreateInfo> queueCreateInfo;
        fillQueueCreationInfo( maxComputeQueues, maxTransferQueues, queueCreateInfo );

        FastArray<FastArray<float> > queuePriorities;
        queuePriorities.resize( queueCreateInfo.size() );

        for( size_t i = 0u; i < queueCreateInfo.size(); ++i )
        {
            queuePriorities[i].resize( queueCreateInfo[i].queueCount, 1.0f );
            queueCreateInfo[i].pQueuePriorities = queuePriorities[i].begin();
        }

        extensions.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );

        VkDeviceCreateInfo createInfo;
        makeVkStruct( createInfo, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO );

        createInfo.enabledExtensionCount = static_cast<uint32>( extensions.size() );
        createInfo.ppEnabledExtensionNames = extensions.begin();

        createInfo.queueCreateInfoCount = static_cast<uint32>( queueCreateInfo.size() );
        createInfo.pQueueCreateInfos = &queueCreateInfo[0];

        if( hasInstanceExtension( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME ) )
            createInfo.pEnabledFeatures = nullptr;
        else
            createInfo.pEnabledFeatures = &mDeviceFeatures;

        {
            mDeviceExtensions.clear();
            mDeviceExtensions.reserve( extensions.size() );

            FastArray<const char *>::const_iterator itor = extensions.begin();
            FastArray<const char *>::const_iterator endt = extensions.end();

            while( itor != endt )
            {
                LogManager::getSingleton().logMessage( "Requesting Extension: " + String( *itor ) );
                mDeviceExtensions.push_back( *itor );
                ++itor;
            }

            std::sort( mDeviceExtensions.begin(), mDeviceExtensions.end() );
        }

        // Declared at this scope because they must be alive for createInfo.pNext
        VkPhysicalDeviceFeatures2 deviceFeatures2;
        makeVkStruct( deviceFeatures2, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 );
        VkPhysicalDevice16BitStorageFeatures _16BitStorageFeatures;
        makeVkStruct( _16BitStorageFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES );
        VkPhysicalDeviceShaderFloat16Int8Features shaderFloat16Int8Features;
        makeVkStruct( shaderFloat16Int8Features,
                      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES );

        // Initialize mDeviceExtraFeatures
        if( hasInstanceExtension( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME ) )
        {
            PFN_vkGetPhysicalDeviceFeatures2KHR GetPhysicalDeviceFeatures2KHR =
                (PFN_vkGetPhysicalDeviceFeatures2KHR)vkGetInstanceProcAddr(
                    mInstance, "vkGetPhysicalDeviceFeatures2KHR" );

            void **lastNext = &deviceFeatures2.pNext;

            if( this->hasDeviceExtension( VK_KHR_16BIT_STORAGE_EXTENSION_NAME ) )
            {
                *lastNext = &_16BitStorageFeatures;
                lastNext = &_16BitStorageFeatures.pNext;
            }
            if( this->hasDeviceExtension( VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME ) )
            {
                *lastNext = &shaderFloat16Int8Features;
                lastNext = &shaderFloat16Int8Features.pNext;
            }

            // Send the same chain to vkCreateDevice
            createInfo.pNext = &deviceFeatures2;

            GetPhysicalDeviceFeatures2KHR( mPhysicalDevice, &deviceFeatures2 );

            mDeviceExtraFeatures.storageInputOutput16 = _16BitStorageFeatures.storageInputOutput16;
            mDeviceExtraFeatures.shaderFloat16 = shaderFloat16Int8Features.shaderFloat16;
            mDeviceExtraFeatures.shaderInt8 = shaderFloat16Int8Features.shaderInt8;
        }

        VkResult result = vkCreateDevice( mPhysicalDevice, &createInfo, NULL, &mDevice );
        checkVkResult( result, "vkCreateDevice" );

        initUtils( mDevice );
    }
    //-------------------------------------------------------------------------
    bool VulkanDevice::hasDeviceExtension( const IdString extension ) const
    {
        FastArray<IdString>::const_iterator itor =
            std::lower_bound( mDeviceExtensions.begin(), mDeviceExtensions.end(), extension );
        return itor != mDeviceExtensions.end() && *itor == extension;
    }
    //-------------------------------------------------------------------------
    bool VulkanDevice::hasInstanceExtension( const IdString extension )
    {
        FastArray<IdString>::const_iterator itor =
            std::lower_bound( msInstanceExtensions.begin(), msInstanceExtensions.end(), extension );
        return itor != msInstanceExtensions.end() && *itor == extension;
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::initQueues()
    {
        VkQueue queue = 0;
        vkGetDeviceQueue( mDevice, mGraphicsQueue.mFamilyIdx, mGraphicsQueue.mQueueIdx, &queue );
        mGraphicsQueue.init( mDevice, queue, mRenderSystem );

        FastArray<VulkanQueue>::iterator itor = mComputeQueues.begin();
        FastArray<VulkanQueue>::iterator endt = mComputeQueues.end();

        while( itor != endt )
        {
            vkGetDeviceQueue( mDevice, itor->mFamilyIdx, itor->mQueueIdx, &queue );
            itor->init( mDevice, queue, mRenderSystem );
            ++itor;
        }

        itor = mTransferQueues.begin();
        endt = mTransferQueues.end();

        while( itor != endt )
        {
            vkGetDeviceQueue( mDevice, itor->mFamilyIdx, itor->mQueueIdx, &queue );
            itor->init( mDevice, queue, mRenderSystem );
            ++itor;
        }

        TODO_findRealPresentQueue;
        mPresentQueue = mGraphicsQueue.mQueue;
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::commitAndNextCommandBuffer( SubmissionType::SubmissionType submissionType )
    {
        mGraphicsQueue.endAllEncoders();
        mGraphicsQueue.commitAndNextCommandBuffer( submissionType );
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::stall()
    {
        // We must flush the cmd buffer and our bindings because we take the
        // moment to delete all delayed buffers and API handles after a stall.
        //
        // We can't have potentially dangling API handles in a cmd buffer.
        // We must submit our current pending work so far and wait until that's done.
        commitAndNextCommandBuffer( SubmissionType::FlushOnly );
        mRenderSystem->resetAllBindings();

        vkDeviceWaitIdle( mDevice );

        mRenderSystem->_notifyDeviceStalled();
    }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    VulkanDevice::SelectedQueue::SelectedQueue() :
        usage( VulkanQueue::NumQueueFamilies ),
        familyIdx( std::numeric_limits<uint32>::max() ),
        queueIdx( 0 )
    {
    }
}  // namespace Ogre
