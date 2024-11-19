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

#include "vulkan/vulkan_core.h"

#ifdef OGRE_VULKAN_USE_SWAPPY
#    include "swappy/swappyVk.h"
#endif

#define TODO_findRealPresentQueue

namespace Ogre
{
    static FastArray<IdString> msInstanceExtensions;

    VulkanDevice::VulkanDevice( VkInstance instance, const String &deviceName,
                                VulkanRenderSystem *renderSystem ) :
        mInstance( instance ),
        mPhysicalDevice( 0 ),
        mDevice( 0 ),
        mPipelineCache( 0 ),
        mPresentQueue( 0 ),
        mVaoManager( 0 ),
        mRenderSystem( renderSystem ),
        mSupportedStages( 0xFFFFFFFF ),
        mIsExternal( false )
    {
        memset( &mDeviceMemoryProperties, 0, sizeof( mDeviceMemoryProperties ) );
        createPhysicalDevice( deviceName );
    }
    //-------------------------------------------------------------------------
    VulkanDevice::VulkanDevice( VkInstance instance, const VulkanExternalDevice &externalDevice,
                                VulkanRenderSystem *renderSystem ) :
        mInstance( instance ),
        mPhysicalDevice( externalDevice.physicalDevice ),
        mDevice( externalDevice.device ),
        mPipelineCache( 0 ),
        mPresentQueue( 0 ),
        mVaoManager( 0 ),
        mRenderSystem( renderSystem ),
        mSupportedStages( 0xFFFFFFFF ),
        mIsExternal( true )
    {
        LogManager::getSingleton().logMessage(
            "Vulkan: Creating Vulkan Device from External VkVulkan handle" );

        memset( &mDeviceMemoryProperties, 0, sizeof( mDeviceMemoryProperties ) );

        vkGetPhysicalDeviceMemoryProperties( mPhysicalDevice, &mDeviceMemoryProperties );

        // mDeviceExtraFeatures gets initialized later, once we analyzed all the extensions
        memset( &mDeviceExtraFeatures, 0, sizeof( mDeviceExtraFeatures ) );
        fillDeviceFeatures();

        mSupportedStages = 0xFFFFFFFF;
        if( !mDeviceFeatures.geometryShader )
            mSupportedStages ^= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
        if( !mDeviceFeatures.tessellationShader )
        {
            mSupportedStages ^= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
                                VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
        }

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
        }

        mPresentQueue = externalDevice.presentQueue;
        mGraphicsQueue.setExternalQueue( this, VulkanQueue::Graphics, externalDevice.graphicsQueue );
        {
            // Filter wrongly-provided extensions
            uint32 numExtensions = 0;
            vkEnumerateDeviceExtensionProperties( mPhysicalDevice, 0, &numExtensions, 0 );

            FastArray<VkExtensionProperties> availableExtensions;
            availableExtensions.resize( numExtensions );
            vkEnumerateDeviceExtensionProperties( mPhysicalDevice, 0, &numExtensions,
                                                  availableExtensions.begin() );
            std::set<String> extensions;
            for( size_t i = 0u; i < numExtensions; ++i )
            {
                const String extensionName = availableExtensions[i].extensionName;
                LogManager::getSingleton().logMessage( "Vulkan: Found device extension: " +
                                                       extensionName );
                extensions.insert( extensionName );
            }

            FastArray<VkExtensionProperties> deviceExtensionsCopy = externalDevice.deviceExtensions;
            FastArray<VkExtensionProperties>::iterator itor = deviceExtensionsCopy.begin();
            FastArray<VkExtensionProperties>::iterator endt = deviceExtensionsCopy.end();

            while( itor != endt )
            {
                if( extensions.find( itor->extensionName ) == extensions.end() )
                {
                    LogManager::getSingleton().logMessage(
                        "Vulkan: [INFO] External Device claims extension " +
                        String( itor->extensionName ) +
                        " is present but it's not. This is normal. Ignoring." );
                    itor = efficientVectorRemove( deviceExtensionsCopy, itor );
                    endt = deviceExtensionsCopy.end();
                }
                else
                {
                    ++itor;
                }
            }

            mDeviceExtensions.reserve( deviceExtensionsCopy.size() );
            itor = deviceExtensionsCopy.begin();
            while( itor != endt )
            {
                LogManager::getSingleton().logMessage(
                    "Vulkan: Externally requested Device Extension: " + String( itor->extensionName ) );
                mDeviceExtensions.push_back( itor->extensionName );
                ++itor;
            }

            std::sort( mDeviceExtensions.begin(), mDeviceExtensions.end() );
        }

        // Initialize mDeviceExtraFeatures
        if( hasInstanceExtension( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME ) )
        {
            VkPhysicalDeviceFeatures2 deviceFeatures2;
            makeVkStruct( deviceFeatures2, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 );
            VkPhysicalDevice16BitStorageFeatures _16BitStorageFeatures;
            makeVkStruct( _16BitStorageFeatures,
                          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES );
            VkPhysicalDeviceShaderFloat16Int8Features shaderFloat16Int8Features;
            makeVkStruct( shaderFloat16Int8Features,
                          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES );

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

            GetPhysicalDeviceFeatures2KHR( mPhysicalDevice, &deviceFeatures2 );

            mDeviceExtraFeatures.storageInputOutput16 = _16BitStorageFeatures.storageInputOutput16;
            mDeviceExtraFeatures.shaderFloat16 = shaderFloat16Int8Features.shaderFloat16;
            mDeviceExtraFeatures.shaderInt8 = shaderFloat16Int8Features.shaderInt8;
        }

        initUtils( mDevice );
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

            if( mPipelineCache )
            {
                vkDestroyPipelineCache( mDevice, mPipelineCache, nullptr );
                mPipelineCache = 0;
            }

            // Must be done externally (this' destructor has yet to free more Vulkan stuff)
            // vkDestroyDevice( mDevice, 0 );
            mDevice = 0;
            mPhysicalDevice = 0;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::fillDeviceFeatures()
    {
#define VK_DEVICEFEATURE_ENABLE_IF( x ) \
    if( features.x ) \
    mDeviceFeatures.x = features.x

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures( mPhysicalDevice, &features );

        // Don't opt in to features we don't want / need.
        memset( &mDeviceFeatures, 0, sizeof( mDeviceFeatures ) );
        // VK_DEVICEFEATURE_ENABLE_IF( robustBufferAccess );
        VK_DEVICEFEATURE_ENABLE_IF( fullDrawIndexUint32 );
        VK_DEVICEFEATURE_ENABLE_IF( imageCubeArray );
        VK_DEVICEFEATURE_ENABLE_IF( independentBlend );
        VK_DEVICEFEATURE_ENABLE_IF( geometryShader );
        VK_DEVICEFEATURE_ENABLE_IF( tessellationShader );
        VK_DEVICEFEATURE_ENABLE_IF( sampleRateShading );
        VK_DEVICEFEATURE_ENABLE_IF( dualSrcBlend );
        // VK_DEVICEFEATURE_ENABLE_IF( logicOp );
        VK_DEVICEFEATURE_ENABLE_IF( multiDrawIndirect );
        VK_DEVICEFEATURE_ENABLE_IF( drawIndirectFirstInstance );
        VK_DEVICEFEATURE_ENABLE_IF( depthClamp );
        VK_DEVICEFEATURE_ENABLE_IF( depthBiasClamp );
        VK_DEVICEFEATURE_ENABLE_IF( fillModeNonSolid );
        VK_DEVICEFEATURE_ENABLE_IF( depthBounds );
        // VK_DEVICEFEATURE_ENABLE_IF( wideLines );
        // VK_DEVICEFEATURE_ENABLE_IF( largePoints );
        VK_DEVICEFEATURE_ENABLE_IF( alphaToOne );
        VK_DEVICEFEATURE_ENABLE_IF( multiViewport );
        VK_DEVICEFEATURE_ENABLE_IF( samplerAnisotropy );
        VK_DEVICEFEATURE_ENABLE_IF( textureCompressionETC2 );
        VK_DEVICEFEATURE_ENABLE_IF( textureCompressionASTC_LDR );
        VK_DEVICEFEATURE_ENABLE_IF( textureCompressionBC );
        // VK_DEVICEFEATURE_ENABLE_IF( occlusionQueryPrecise );
        // VK_DEVICEFEATURE_ENABLE_IF( pipelineStatisticsQuery );
        VK_DEVICEFEATURE_ENABLE_IF( vertexPipelineStoresAndAtomics );
        VK_DEVICEFEATURE_ENABLE_IF( fragmentStoresAndAtomics );
        VK_DEVICEFEATURE_ENABLE_IF( shaderTessellationAndGeometryPointSize );
        VK_DEVICEFEATURE_ENABLE_IF( shaderImageGatherExtended );
        VK_DEVICEFEATURE_ENABLE_IF( shaderStorageImageExtendedFormats );
        VK_DEVICEFEATURE_ENABLE_IF( shaderStorageImageMultisample );
        VK_DEVICEFEATURE_ENABLE_IF( shaderStorageImageReadWithoutFormat );
        VK_DEVICEFEATURE_ENABLE_IF( shaderStorageImageWriteWithoutFormat );
        VK_DEVICEFEATURE_ENABLE_IF( shaderUniformBufferArrayDynamicIndexing );
        VK_DEVICEFEATURE_ENABLE_IF( shaderSampledImageArrayDynamicIndexing );
        VK_DEVICEFEATURE_ENABLE_IF( shaderStorageBufferArrayDynamicIndexing );
        VK_DEVICEFEATURE_ENABLE_IF( shaderStorageImageArrayDynamicIndexing );
        VK_DEVICEFEATURE_ENABLE_IF( shaderClipDistance );
        VK_DEVICEFEATURE_ENABLE_IF( shaderCullDistance );
        // VK_DEVICEFEATURE_ENABLE_IF( shaderFloat64 );
        // VK_DEVICEFEATURE_ENABLE_IF( shaderInt64 );
        VK_DEVICEFEATURE_ENABLE_IF( shaderInt16 );
        // VK_DEVICEFEATURE_ENABLE_IF( shaderResourceResidency );
        VK_DEVICEFEATURE_ENABLE_IF( shaderResourceMinLod );
        // VK_DEVICEFEATURE_ENABLE_IF( sparseBinding );
        // VK_DEVICEFEATURE_ENABLE_IF( sparseResidencyBuffer );
        // VK_DEVICEFEATURE_ENABLE_IF( sparseResidencyImage2D );
        // VK_DEVICEFEATURE_ENABLE_IF( sparseResidencyImage3D );
        // VK_DEVICEFEATURE_ENABLE_IF( sparseResidency2Samples );
        // VK_DEVICEFEATURE_ENABLE_IF( sparseResidency4Samples );
        // VK_DEVICEFEATURE_ENABLE_IF( sparseResidency8Samples );
        // VK_DEVICEFEATURE_ENABLE_IF( sparseResidency16Samples );
        // VK_DEVICEFEATURE_ENABLE_IF( sparseResidencyAliased );
        VK_DEVICEFEATURE_ENABLE_IF( variableMultisampleRate );
        // VK_DEVICEFEATURE_ENABLE_IF( inheritedQueries );
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

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH && !defined OGRE_VULKAN_WINDOW_ANDROID
        // Workaround: skip following code on Android as it causes crash in vkCreateInstance() on Android
        // Emulator 35.1.4, macOS 14.4.1, M1 Pro, despite declared support for rev.10 VK_EXT_debug_report
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
                LogManager::getSingleton().logMessage( "Vulkan: Requesting Instance Extension: " +
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
    void VulkanDevice::addExternalInstanceExtensions( FastArray<VkExtensionProperties> &extensions )
    {
        msInstanceExtensions.clear();
        msInstanceExtensions.reserve( extensions.size() );

        FastArray<VkExtensionProperties>::const_iterator itor = extensions.begin();
        FastArray<VkExtensionProperties>::const_iterator endt = extensions.end();

        while( itor != endt )
        {
            LogManager::getSingleton().logMessage( "Vulkan: Externally requested Instance Extension: " +
                                                   String( itor->extensionName ) );
            msInstanceExtensions.push_back( itor->extensionName );
            ++itor;
        }

        std::sort( msInstanceExtensions.begin(), msInstanceExtensions.end() );
    }
    //-------------------------------------------------------------------------
    void VulkanDevice::createPhysicalDevice( const String &deviceName )
    {
        const auto& devices = mRenderSystem->getVulkanPhysicalDevices();
        size_t deviceIdx = 0;
        for( size_t i = 0; i < devices.size(); ++i )
            if( devices[i].title == deviceName )
            {
                deviceIdx = i;
                break;
            }

        LogManager::getSingleton().logMessage( "Vulkan: Requested \"" + deviceName + "\", selected \"" +
                                               devices[deviceIdx].title + "\"" );

        mPhysicalDevice = devices[deviceIdx].physicalDevice;

        vkGetPhysicalDeviceMemoryProperties( mPhysicalDevice, &mDeviceMemoryProperties );

        // mDeviceExtraFeatures gets initialized later, once we analyzed all the extensions
        memset( &mDeviceExtraFeatures, 0, sizeof( mDeviceExtraFeatures ) );
        fillDeviceFeatures();

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
                LogManager::getSingleton().logMessage( "Vulkan: Requesting Extension: " +
                                                       String( *itor ) );
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
            deviceFeatures2.features = mDeviceFeatures;

            mDeviceExtraFeatures.storageInputOutput16 = _16BitStorageFeatures.storageInputOutput16;
            mDeviceExtraFeatures.shaderFloat16 = shaderFloat16Int8Features.shaderFloat16;
            mDeviceExtraFeatures.shaderInt8 = shaderFloat16Int8Features.shaderInt8;
        }

        VkResult result = vkCreateDevice( mPhysicalDevice, &createInfo, NULL, &mDevice );
        checkVkResult( result, "vkCreateDevice" );

        VkPipelineCacheCreateInfo pipelineCacheCreateInfo;
        makeVkStruct( pipelineCacheCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO );

        result = vkCreatePipelineCache( mDevice, &pipelineCacheCreateInfo, nullptr, &mPipelineCache );
        checkVkResult( result, "vkCreatePipelineCache" );

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
        if( !mIsExternal )
        {
            VkQueue queue = 0;
            vkGetDeviceQueue( mDevice, mGraphicsQueue.mFamilyIdx, mGraphicsQueue.mQueueIdx, &queue );
            mGraphicsQueue.init( mDevice, queue, mRenderSystem );

            TODO_findRealPresentQueue;
            mPresentQueue = mGraphicsQueue.mQueue;
        }
        else
        {
            mGraphicsQueue.init( mDevice, mGraphicsQueue.mQueue, mRenderSystem );
        }

#ifdef OGRE_VULKAN_USE_SWAPPY
        SwappyVk_setQueueFamilyIndex( mDevice, mGraphicsQueue.mQueue, mGraphicsQueue.mFamilyIdx );
#endif
        VkQueue queue = 0;

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
        // We must call flushBoundGpuProgramParameters() now because commitAndNextCommandBuffer() will
        // call flushBoundGpuProgramParameters( SubmissionType::FlushOnly ).
        //
        // Unfortunately that's not enough because that assumes VaoManager::mFrameCount
        // won't change (which normally wouldn't w/ FlushOnly). However our caller is
        // about to do mFrameCount += mDynamicBufferMultiplier.
        //
        // The solution is to tell flushBoundGpuProgramParameters() we're changing frame idx.
        // Calling it again becomes a no-op since mFirstUnflushedAutoParamsBuffer becomes 0.
        //
        // We also *want* mCurrentAutoParamsBufferPtr to become nullptr since it can be reused from
        // scratch. Because we're stalling, it is safe to do so.
        mRenderSystem->flushBoundGpuProgramParameters( SubmissionType::NewFrameIdx );

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
