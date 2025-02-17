/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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

#include "OgreVulkanRenderSystem.h"

#include "OgreRenderPassDescriptor.h"
#include "OgreVulkanCache.h"
#include "OgreVulkanDelayedFuncs.h"
#include "OgreVulkanDevice.h"
#include "OgreVulkanGpuProgramManager.h"
#include "OgreVulkanMappings.h"
#include "OgreVulkanProgramFactory.h"
#include "OgreVulkanRenderPassDescriptor.h"
#include "OgreVulkanResourceTransition.h"
#include "OgreVulkanRootLayout.h"
#include "OgreVulkanSupport.h"
#include "OgreVulkanTextureGpuManager.h"
#include "OgreVulkanUtils.h"
#include "OgreVulkanWindow.h"
#include "Vao/OgreVulkanVaoManager.h"

#include "OgreDefaultHardwareBufferManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include "CommandBuffer/OgreCbDrawCall.h"
#include "Compositor/OgreCompositorManager2.h"

#include "OgreVulkanHlmsPso.h"
#include "Vao/OgreIndirectBufferPacked.h"
#include "Vao/OgreVulkanBufferInterface.h"
#include "Vao/OgreVulkanConstBufferPacked.h"
#include "Vao/OgreVulkanTexBufferPacked.h"

#include "OgreVulkanHardwareBufferManager.h"
#include "OgreVulkanHardwareIndexBuffer.h"
#include "OgreVulkanHardwareVertexBuffer.h"

#include "OgreVulkanDescriptorSets.h"

#include "OgreVulkanHardwareIndexBuffer.h"
#include "OgreVulkanHardwareVertexBuffer.h"
#include "OgreVulkanTextureGpu.h"
#include "Vao/OgreVulkanUavBufferPacked.h"

#include "OgreDepthBuffer.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreRoot.h"
#include "OgreTimer.h"

#ifdef OGRE_VULKAN_WINDOW_WIN32
#    include "Windowing/win32/OgreVulkanWin32Window.h"
#endif
#ifdef OGRE_VULKAN_WINDOW_XCB
#    include "Windowing/X11/OgreVulkanXcbWindow.h"
#endif
#ifdef OGRE_VULKAN_WINDOW_ANDROID
#    include "Windowing/Android/OgreVulkanAndroidWindow.h"
#endif

#include "OgrePixelFormatGpuUtils.h"

#ifdef OGRE_VULKAN_USE_SWAPPY
#    include "swappy/swappyVk.h"
#endif

#define TODO_addVpCount_to_passpso

#if OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_32
#    define OGRE_HASH128_FUNC MurmurHash3_x86_128
#else
#    define OGRE_HASH128_FUNC MurmurHash3_x64_128
#endif

namespace Ogre
{
    /*static bool checkLayers( const FastArray<layer_properties> &layer_props,
                             const FastArray<const char *> &layer_names )
    {
        uint32_t check_count = layer_names.size();
        uint32_t layer_count = layer_props.size();
        for( uint32_t i = 0; i < check_count; i++ )
        {
            VkBool32 found = 0;
            for( uint32_t j = 0; j < layer_count; j++ )
            {
                if( !strcmp( layer_names[i], layer_props[j].properties.layerName ) )
                {
                    found = 1;
                }
            }
            if( !found )
            {
                std::cout << "Cannot find layer: " << layer_names[i] << std::endl;
                return 0;
            }
        }
        return 1;
    }*/
    VKAPI_ATTR VkBool32 VKAPI_CALL dbgFunc( VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
                                            uint64_t srcObject, size_t location, int32_t msgCode,
                                            const char *pLayerPrefix, const char *pMsg, void *pUserData )
    {
        VulkanRenderSystem *renderSystem = static_cast<VulkanRenderSystem *>( pUserData );

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        if( renderSystem->getActiveVulkanPhysicalDevice().driverID == 23 /* VK_DRIVER_ID_MESA_DOZEN */
            && ( msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) )
        {
            // According to VkQueueFamilyProperties, graphics and compute capable queues must be declared
            // with a minImageTransferGranularity of (1,1,1). Currently, dozen is hardcoded to a value of
            // (0,0,0). https://gitlab.freedesktop.org/mesa/mesa/-/issues/10617
            static String ignoredFalsePositiveErrors[] = {
                "Validation Error: [ VUID-vkCmdCopyImage-srcOffset-01783 ]",
                "Validation Error: [ VUID-vkCmdCopyImage-dstOffset-01784 ]",
                "Validation Error: [ VUID-vkCmdCopyBufferToImage-imageOffset-07738 ]"
            };
            for( auto &err : ignoredFalsePositiveErrors )
                if( 0 == strncmp( pMsg, err.c_str(), err.size() ) )
                    return false;
        }
#endif

        // clang-format off
        char *message = (char *)malloc(strlen(pMsg) + 100);

        assert(message);

        if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
            sprintf(message, "INFORMATION: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
        } else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
            sprintf(message, "WARNING: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
        } else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
            sprintf(message, "PERFORMANCE WARNING: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
        } else if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
            sprintf(message, "ERROR: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
            renderSystem->debugCallback();
        } else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
            sprintf(message, "DEBUG: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
        } else {
            sprintf(message, "INFORMATION: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
        }

        LogManager::getSingleton().logMessage( message,
            msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT ? LML_CRITICAL : LML_NORMAL );

        free(message);

        // clang-format on

        /*
         * false indicates that layer should not bail-out of an
         * API call that had validation failures. This may mean that the
         * app dies inside the driver due to invalid parameter(s).
         * That's what would happen without validation layers, so we'll
         * keep that behavior here.
         */
        return false;
        // return true;
    }
    //-------------------------------------------------------------------------
    void onVulkanFailure( VulkanDevice *device, int result, const char *desc, const char *src,
                          const char *file, long line )
    {
        VkResult vkResult = (VkResult)result;
        if( device != nullptr && device->mDeviceLostReason == VK_SUCCESS &&
            ( vkResult == VK_ERROR_OUT_OF_HOST_MEMORY || vkResult == VK_ERROR_OUT_OF_DEVICE_MEMORY ||
              vkResult == VK_ERROR_DEVICE_LOST ) )
        {
            device->mDeviceLostReason = vkResult;
        }

        ExceptionFactory::throwException( Exception::ERR_RENDERINGAPI_ERROR, vkResult,
                                          desc + ( "\nVkResult = " + vkResultToString( vkResult ) ), src,
                                          file, line );
    }
    //-------------------------------------------------------------------------
    VulkanRenderSystem::VulkanRenderSystem( const NameValuePairList *options ) :
        RenderSystem(),
        mInitialized( false ),
#ifdef OGRE_VULKAN_USE_SWAPPY
        mSwappyFramePacing( true ),
#endif
        mHardwareBufferManager( 0 ),
        mIndirectBuffer( 0 ),
        mShaderManager( 0 ),
        mVulkanProgramFactory0( 0 ),
        mVulkanProgramFactory1( 0 ),
        mVulkanProgramFactory2( 0 ),
        mVulkanProgramFactory3( 0 ),
        mActiveDevice( {} ),
        mFirstUnflushedAutoParamsBuffer( 0 ),
        mAutoParamsBufferIdx( 0 ),
        mCurrentAutoParamsBufferPtr( 0 ),
        mCurrentAutoParamsBufferSpaceLeft( 0 ),
        mDevice( 0 ),
        mCache( 0 ),
        mPso( 0 ),
        mComputePso( 0 ),
        mStencilRefValue( 0u ),
        mStencilEnabled( false ),
        mTableDirty( false ),
        mComputeTableDirty( false ),
        mDummyBuffer( 0 ),
        mDummyTexBuffer( 0 ),
        mDummyTextureView( 0 ),
        mDummySampler( 0 ),
        mEntriesToFlush( 0u ),
        mVpChanged( false ),
        mInterruptedRenderCommandEncoder( false ),
        mValidationError( false )
    {
        mPsoRequestsTimeout = 16;  // ms

        memset( &mGlobalTable, 0, sizeof( mGlobalTable ) );
        mGlobalTable.reset();

        memset( &mComputeTable, 0, sizeof( mComputeTable ) );
        mComputeTable.reset();

        for( size_t i = 0u; i < NUM_BIND_TEXTURES; ++i )
        {
            mGlobalTable.textures[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            mComputeTable.textures[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        mInvertedClipSpaceY = true;

        VulkanExternalInstance *externalInstance = nullptr;
        if( options )
        {
            NameValuePairList::const_iterator itOption = options->find( "external_instance" );
            if( itOption != options->end() )
                externalInstance = reinterpret_cast<VulkanExternalInstance *>(
                    StringConverter::parseSizeT( itOption->second ) );
        }

        VulkanInstance::enumerateExtensionsAndLayers( externalInstance );

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        loadRenderDocApi();
#endif

        mInstance = std::make_shared<VulkanInstance>( Root::getSingleton().getAppName(),
                                                      externalInstance, dbgFunc, this );

        initConfigOptions();

        // Certain drivers (e.g. AMD) or tools did not like when we kept around Vulkan, OpenGL & D3D11
        // instances/devices/context live at the same time (i.e. like when we are probing for support),
        // therefore release VkInstance here, and recreate it later
        if( !externalInstance )
            mInstance.reset();
    }
    //-------------------------------------------------------------------------
    VulkanRenderSystem::~VulkanRenderSystem()
    {
        shutdown();

        for( auto &elem : mAvailableVulkanSupports )
            delete elem.second;

        mAvailableVulkanSupports.clear();
        mVulkanSupport = 0;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::shutdown()
    {
        if( !mDevice )
            return;

        destroyVkResources0();

        {
            // Remove all windows.
            // (destroy primary window last since others may depend on it)
            Window *primary = 0;
            WindowSet::const_iterator itor = mWindows.begin();
            WindowSet::const_iterator endt = mWindows.end();

            while( itor != endt )
            {
                if( !primary && ( *itor )->isPrimary() )
                    primary = *itor;
                else
                    OGRE_DELETE *itor;

                ++itor;
            }

            OGRE_DELETE primary;
            mWindows.clear();
        }

        destroyAllRenderPassDescriptors();
        _cleanupDepthBuffers();
        OGRE_ASSERT_LOW( mSharedDepthBufferRefs.empty() &&
                         "destroyAllRenderPassDescriptors followed by _cleanupDepthBuffers should've "
                         "emptied mSharedDepthBufferRefs. Please report this bug to "
                         "https://github.com/OGRECave/ogre-next/issues/" );

        destroyVkResources1();

        OGRE_DELETE mHardwareBufferManager;
        mHardwareBufferManager = 0;

        OGRE_DELETE mCache;
        mCache = 0;

        OGRE_DELETE mShaderManager;
        mShaderManager = 0;

        OGRE_DELETE mTextureGpuManager;
        mTextureGpuManager = 0;
        OGRE_DELETE mVaoManager;
        mVaoManager = 0;

        OGRE_DELETE mVulkanProgramFactory3;  // LIFO destruction order
        mVulkanProgramFactory3 = 0;
        OGRE_DELETE mVulkanProgramFactory2;
        mVulkanProgramFactory2 = 0;
        OGRE_DELETE mVulkanProgramFactory1;
        mVulkanProgramFactory1 = 0;
        OGRE_DELETE mVulkanProgramFactory0;
        mVulkanProgramFactory0 = 0;

        delete mDevice;
        mDevice = 0;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::createVkResources()
    {
        uint32 dummyData = 0u;
        mDummyBuffer = mVaoManager->createConstBuffer( 4u, BT_IMMUTABLE, &dummyData, false );
        mDummyTexBuffer =
            mVaoManager->createTexBuffer( PFG_RGBA8_UNORM, 4u, BT_IMMUTABLE, &dummyData, false );

        {
            VkImage dummyImage = static_cast<VulkanTextureGpuManager *>( mTextureGpuManager )
                                     ->getBlankTextureVulkanName( TextureTypes::Type2D );

            VkImageViewCreateInfo imageViewCi;
            makeVkStruct( imageViewCi, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO );
            imageViewCi.image = dummyImage;
            imageViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCi.format = VK_FORMAT_R8G8B8A8_UNORM;
            imageViewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCi.subresourceRange.levelCount = 1u;
            imageViewCi.subresourceRange.layerCount = 1u;

            VkResult result = vkCreateImageView( mDevice->mDevice, &imageViewCi, 0, &mDummyTextureView );
            checkVkResult( mDevice, result, "vkCreateImageView" );
        }

        {
            VkSamplerCreateInfo samplerDescriptor;
            makeVkStruct( samplerDescriptor, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO );
            float maxAllowedAnisotropy = mDevice->mDeviceProperties.limits.maxSamplerAnisotropy;
            samplerDescriptor.maxAnisotropy = maxAllowedAnisotropy;
            samplerDescriptor.anisotropyEnable = VK_FALSE;
            samplerDescriptor.minLod = -std::numeric_limits<float>::max();
            samplerDescriptor.maxLod = std::numeric_limits<float>::max();
            VkResult result = vkCreateSampler( mDevice->mDevice, &samplerDescriptor, 0, &mDummySampler );
            checkVkResult( mDevice, result, "vkCreateSampler" );
        }

        resetAllBindings();
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::destroyVkResources0()
    {
        for( ConstBufferPacked *constBuffer : mAutoParamsBuffer )
        {
            if( constBuffer->getMappingState() != MS_UNMAPPED )
                constBuffer->unmap( UO_UNMAP_ALL );
            mVaoManager->destroyConstBuffer( constBuffer );
        }
        mAutoParamsBuffer.clear();
        mFirstUnflushedAutoParamsBuffer = 0u;
        mAutoParamsBufferIdx = 0u;
        mCurrentAutoParamsBufferPtr = 0;
        mCurrentAutoParamsBufferSpaceLeft = 0;

        mDevice->stallIgnoringDeviceLost();
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::destroyVkResources1()
    {
        if( mDummySampler )
        {
            vkDestroySampler( mDevice->mDevice, mDummySampler, 0 );
            mDummySampler = 0;
        }

        if( mDummyTextureView )
        {
            vkDestroyImageView( mDevice->mDevice, mDummyTextureView, 0 );
            mDummyTextureView = 0;
        }

        if( mDummyTexBuffer )
        {
            mVaoManager->destroyTexBuffer( mDummyTexBuffer );
            mDummyTexBuffer = 0;
        }

        if( mDummyBuffer )
        {
            mVaoManager->destroyConstBuffer( mDummyBuffer );
            mDummyBuffer = 0;
        }
    }
    //-------------------------------------------------------------------------
    const String &VulkanRenderSystem::getName() const
    {
        static String strName( "Vulkan Rendering Subsystem" );
        return strName;
    }
    //-------------------------------------------------------------------------
    const String &VulkanRenderSystem::getFriendlyName() const
    {
        static String strName( "Vulkan_RS" );
        return strName;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::initConfigOptions()
    {
        const int numVulkanSupports = Ogre::getNumVulkanSupports();
        for( int i = 0; i < numVulkanSupports; ++i )
        {
            VulkanSupport *vulkanSupport = Ogre::getVulkanSupport( i );
            mAvailableVulkanSupports[vulkanSupport->getInterfaceName()] = vulkanSupport;
        }

#ifdef OGRE_VULKAN_WINDOW_WIN32
        if( VulkanInstance::hasExtension( VulkanWin32Window::getRequiredExtensionName() ) )
            mAvailableVulkanSupports["win32"]->setSupported();
#endif
#ifdef OGRE_VULKAN_WINDOW_XCB
        if( VulkanInstance::hasExtension( VulkanXcbWindow::getRequiredExtensionName() ) )
            mAvailableVulkanSupports["xcb"]->setSupported();
#endif
#ifdef OGRE_VULKAN_WINDOW_ANDROID
        if( VulkanInstance::hasExtension( VulkanAndroidWindow::getRequiredExtensionName() ) )
            mAvailableVulkanSupports["android"]->setSupported();
#endif

        if( mInstance->mVkInstanceIsExternal )
        {
#ifndef OGRE_VULKAN_WINDOW_NULL
            VulkanSupport *vulkanSupport = new VulkanSupport();
            vulkanSupport->setSupported();
            mAvailableVulkanSupports[vulkanSupport->getInterfaceName()] = vulkanSupport;
            mVulkanSupport = vulkanSupport;
#endif
        }
        else
        {
#ifdef OGRE_VULKAN_WINDOW_NULL
            mAvailableVulkanSupports["null"]->setSupported();
#endif

            bool bAnySupported = false;
            for( auto &elem : mAvailableVulkanSupports )
            {
                if( elem.second->isSupported() )
                {
                    bAnySupported = true;
                    continue;
                }
                LogManager::getSingleton().logMessage(
                    "WARNING: Vulkan support for " + elem.second->getInterfaceNameStr() + " not found.",
                    LML_CRITICAL );
            }
            if( !bAnySupported )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Vulkan layer loaded but instance is uncapable of drawing to the screen! "
                             "Cannot continue.",
                             "VulkanRenderSystem::initConfigOptions" );
            }
        }

        for( auto &elem : mAvailableVulkanSupports )
            elem.second->addConfig( this );

        const ConfigOptionMap &configOptions =
            mAvailableVulkanSupports.begin()->second->getConfigOptions( this );
        ConfigOptionMap::const_iterator itInterface = configOptions.find( "Interface" );
        if( itInterface != configOptions.end() )
        {
            const IdString defaultInterface = itInterface->second.currentValue;
            mVulkanSupport = mAvailableVulkanSupports.find( defaultInterface )->second;
        }
        else
        {
            LogManager::getSingleton().logMessage(
                "ERROR: Could NOT find default Interface in Vulkan RenderSystem. Build setting "
                "misconfiguration!?",
                LML_CRITICAL );
            mVulkanSupport = mAvailableVulkanSupports.begin()->second;
        }
    }
    //-------------------------------------------------------------------------
    ConfigOptionMap &VulkanRenderSystem::getConfigOptions()
    {
        return mVulkanSupport->getConfigOptions( this );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::setConfigOption( const String &name, const String &value )
    {
        if( name == "Interface" )
        {
            std::map<IdString, VulkanSupport *>::const_iterator itor =
                mAvailableVulkanSupports.find( value );
            if( itor != mAvailableVulkanSupports.end() )
            {
                mVulkanSupport = itor->second;
                mVulkanSupport->setConfigOption( name, value );
            }
            else
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Option named '" + name + "' does not exist.",
                             "VulkanRenderSystem::setConfigOption" );
            }
        }
        else
        {
            mVulkanSupport->setConfigOption( name, value );
        }
    }
    //-------------------------------------------------------------------------
    const char *VulkanRenderSystem::getPriorityConfigOption( size_t ) const { return "Device"; }
    //-------------------------------------------------------------------------
    size_t VulkanRenderSystem::getNumPriorityConfigOptions() const { return 1u; }
    //-------------------------------------------------------------------------
    bool VulkanRenderSystem::supportsMultithreadedShaderCompilation() const
    {
#ifndef OGRE_SHADER_THREADING_BACKWARDS_COMPATIBLE_API
        return true;
#else
#    ifdef OGRE_SHADER_THREADING_USE_TLS
        return true;
#    else
        return false;
#    endif
#endif
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::loadPipelineCache( DataStreamPtr stream )
    {
        if( stream && stream->size() > sizeof( PipelineCachePrefixHeader ) )
        {
            std::vector<unsigned char> buf;  // PipelineCachePrefixHeader + payload
            buf.resize( stream->size() );
            stream->read( buf.data(), buf.size() );

            // validate blob
            VkResult result = VK_ERROR_FORMAT_NOT_SUPPORTED;
            auto &hdr = *(PipelineCachePrefixHeader *)buf.data();
            if( hdr.magic == ( 'V' | ( 'K' << 8 ) | ( 'P' << 16 ) | ( 'C' << 24 ) ) &&
                hdr.dataSize == buf.size() - sizeof( PipelineCachePrefixHeader ) &&
                hdr.vendorID == mDevice->mDeviceProperties.vendorID &&
                hdr.deviceID == mDevice->mDeviceProperties.deviceID &&
                hdr.driverVersion == mDevice->mDeviceProperties.driverVersion &&
                hdr.driverABI == sizeof( void * ) &&
                0 == memcmp( hdr.uuid, mDevice->mDeviceProperties.pipelineCacheUUID, VK_UUID_SIZE ) )
            {
                auto dataHash = hdr.dataHash;
                hdr.dataHash = 0;
                uint64 hashResult[2] = {};
                OGRE_HASH128_FUNC( buf.data(), (int)buf.size(), IdString::Seed, hashResult );
                if( dataHash == hashResult[0] )
                    result = VK_SUCCESS;
            }

            if( result != VK_SUCCESS )
                LogManager::getSingleton().logMessage( "Vulkan: Pipeline cache outdated, not loaded." );
            else
            {
                VkPipelineCacheCreateInfo pipelineCacheCreateInfo;
                makeVkStruct( pipelineCacheCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO );
                pipelineCacheCreateInfo.initialDataSize =
                    buf.size() - sizeof( PipelineCachePrefixHeader );
                pipelineCacheCreateInfo.pInitialData = buf.data() + sizeof( PipelineCachePrefixHeader );

                VkPipelineCache pipelineCache{};
                result = vkCreatePipelineCache( mDevice->mDevice, &pipelineCacheCreateInfo, nullptr,
                                                &pipelineCache );
                if( VK_SUCCESS == result && pipelineCache != 0 )
                {
                    std::swap( mDevice->mPipelineCache, pipelineCache );
                    LogManager::getSingleton().logMessage( "Vulkan: Pipeline cache loaded, " +
                                                           StringConverter::toString( buf.size() ) +
                                                           " bytes." );
                }
                else
                {
                    LogManager::getSingleton().logMessage(
                        "Vulkan: Pipeline cache loading failed. VkResult = " +
                        vkResultToString( result ) );
                }
                if( pipelineCache != 0 )
                    vkDestroyPipelineCache( mDevice->mDevice, pipelineCache, nullptr );
            }
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::savePipelineCache( DataStreamPtr stream ) const
    {
        OGRE_STATIC_ASSERT( sizeof( PipelineCachePrefixHeader ) == 48 );
        if( mDevice->mPipelineCache )
        {
            size_t size{};
            VkResult result =
                vkGetPipelineCacheData( mDevice->mDevice, mDevice->mPipelineCache, &size, nullptr );
            if( result == VK_SUCCESS && size > 0 && size <= 0x7FFFFFFF )
            {
                std::vector<unsigned char> buf;  // PipelineCachePrefixHeader + payload
                do
                {
                    buf.resize( sizeof( PipelineCachePrefixHeader ) + size );
                    result = vkGetPipelineCacheData( mDevice->mDevice, mDevice->mPipelineCache, &size,
                                                     buf.data() + sizeof( PipelineCachePrefixHeader ) );
                } while( result == VK_INCOMPLETE );

                if( result == VK_SUCCESS && size > 0 && size <= 0x7FFFFFFF )
                {
                    if( sizeof( PipelineCachePrefixHeader ) + size < buf.size() )
                        buf.resize( sizeof( PipelineCachePrefixHeader ) + size );

                    auto &hdr = *(PipelineCachePrefixHeader *)buf.data();
                    hdr.magic = 'V' | ( 'K' << 8 ) | ( 'P' << 16 ) | ( 'C' << 24 );
                    hdr.dataSize = (uint32_t)size;
                    hdr.dataHash = 0;
                    hdr.vendorID = mDevice->mDeviceProperties.vendorID;
                    hdr.deviceID = mDevice->mDeviceProperties.deviceID;
                    hdr.driverVersion = mDevice->mDeviceProperties.driverVersion;
                    hdr.driverABI = sizeof( void * );
                    memcpy( hdr.uuid, mDevice->mDeviceProperties.pipelineCacheUUID, VK_UUID_SIZE );
                    OGRE_STATIC_ASSERT( VK_UUID_SIZE == 16 );

                    uint64 hashResult[2] = {};
                    OGRE_HASH128_FUNC( buf.data(), (int)buf.size(), IdString::Seed, hashResult );
                    hdr.dataHash = hashResult[0];

                    stream->write( buf.data(), buf.size() );
                    LogManager::getSingleton().logMessage( "Vulkan: Pipeline cache saved, " +
                                                           StringConverter::toString( buf.size() ) +
                                                           " bytes." );
                }
            }
            if( result != VK_SUCCESS )
                LogManager::getSingleton().logMessage( "Vulkan: Pipeline cache not saved. VkResult = " +
                                                       vkResultToString( result ) );
        }
    }
    //-------------------------------------------------------------------------
    String VulkanRenderSystem::validateConfigOptions()
    {
        return mVulkanSupport->validateConfigOptions();
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::debugCallback() { mValidationError = true; }
    //-------------------------------------------------------------------------
    HardwareOcclusionQuery *VulkanRenderSystem::createHardwareOcclusionQuery()
    {
        return 0;  // TODO
    }
    //-------------------------------------------------------------------------
    RenderSystemCapabilities *VulkanRenderSystem::createRenderSystemCapabilities() const
    {
        RenderSystemCapabilities *rsc = new RenderSystemCapabilities();
        rsc->setRenderSystemName( getName() );

        // We would like to save the device properties for the device capabilities limits.
        // These limits are needed for buffers' binding alignments.
        const VkPhysicalDeviceProperties &properties = mDevice->mDeviceProperties;

        LogManager::getSingleton().logMessage(
            "Vulkan: API Version: " +
            StringConverter::toString( VK_VERSION_MAJOR( properties.apiVersion ) ) + "." +
            StringConverter::toString( VK_VERSION_MINOR( properties.apiVersion ) ) + "." +
            StringConverter::toString( VK_VERSION_PATCH( properties.apiVersion ) ) + " (" +
            StringConverter::toString( properties.apiVersion, 0, ' ', std::ios::hex ) + ")" );
        LogManager::getSingleton().logMessage(
            "Vulkan: Driver Version (raw): " +
            StringConverter::toString( properties.driverVersion, 0, ' ', std::ios::hex ) );
        LogManager::getSingleton().logMessage(
            "Vulkan: Vendor ID: " +
            StringConverter::toString( properties.vendorID, 0, ' ', std::ios::hex ) );
        LogManager::getSingleton().logMessage(
            "Vulkan: Device ID: " +
            StringConverter::toString( properties.deviceID, 0, ' ', std::ios::hex ) );

        rsc->setDeviceName( properties.deviceName );
        rsc->setDeviceId( properties.deviceID );

        switch( properties.vendorID )
        {
        case 0x10DE:
        {
            rsc->setVendor( GPU_NVIDIA );
            // 10 bits = major version (up to r1023)
            // 8 bits = minor version (up to 255)
            // 8 bits = secondary branch version/build version (up to 255)
            // 6 bits = tertiary branch/build version (up to 63)

            DriverVersion driverVersion;
            driverVersion.major = ( properties.driverVersion >> 22u ) & 0x3ff;
            driverVersion.minor = ( properties.driverVersion >> 14u ) & 0x0ff;
            driverVersion.release = ( properties.driverVersion >> 6u ) & 0x0ff;
            driverVersion.build = ( properties.driverVersion ) & 0x003f;
            rsc->setDriverVersion( driverVersion );
        }
        break;
        case 0x1002:
            rsc->setVendor( GPU_AMD );
            break;
        case 0x8086:
            rsc->setVendor( GPU_INTEL );
            break;
        case 0x13B5:
            rsc->setVendor( GPU_ARM );  // Mali
            break;
        case 0x5143:
            rsc->setVendor( GPU_QUALCOMM );
            break;
        case 0x1010:
            rsc->setVendor( GPU_IMGTEC );  // PowerVR
            break;
        }

        if( rsc->getVendor() != GPU_NVIDIA )
        {
            // Generic version routine that matches SaschaWillems's VulkanCapsViewer
            DriverVersion driverVersion;
            driverVersion.major = ( properties.driverVersion >> 22u ) & 0x3ff;
            driverVersion.minor = ( properties.driverVersion >> 12u ) & 0x3ff;
            driverVersion.release = ( properties.driverVersion ) & 0xfff;
            driverVersion.build = 0;
            rsc->setDriverVersion( driverVersion );
        }

        if( mDevice->mDeviceFeatures.imageCubeArray )
            rsc->setCapability( RSC_TEXTURE_CUBE_MAP_ARRAY );

        if( mDevice->hasDeviceExtension( VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME ) )
            rsc->setCapability( RSC_VP_AND_RT_ARRAY_INDEX_FROM_ANY_SHADER );

        rsc->setCapability( RSC_SHADER_RELAXED_FLOAT );

        if( mDevice->mDeviceExtraFeatures.shaderFloat16 &&
            mDevice->mDeviceExtraFeatures.storageInputOutput16 )
        {
            rsc->setCapability( RSC_SHADER_FLOAT16 );
        }

        if( mDevice->mDeviceFeatures.depthClamp )
            rsc->setCapability( RSC_DEPTH_CLAMP );

        {
            VkFormatProperties props;

            vkGetPhysicalDeviceFormatProperties( mDevice->mPhysicalDevice,
                                                 VulkanMappings::get( PFG_BC1_UNORM ), &props );
            if( props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT )
                rsc->setCapability( RSC_TEXTURE_COMPRESSION_DXT );

            vkGetPhysicalDeviceFormatProperties( mDevice->mPhysicalDevice,
                                                 VulkanMappings::get( PFG_BC4_UNORM ), &props );
            if( props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT )
                rsc->setCapability( RSC_TEXTURE_COMPRESSION_BC4_BC5 );

            vkGetPhysicalDeviceFormatProperties( mDevice->mPhysicalDevice,
                                                 VulkanMappings::get( PFG_BC6H_UF16 ), &props );
            if( props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT )
                rsc->setCapability( RSC_TEXTURE_COMPRESSION_BC6H_BC7 );

            // Vulkan doesn't allow supporting ETC1 without ETC2
            vkGetPhysicalDeviceFormatProperties( mDevice->mPhysicalDevice,
                                                 VulkanMappings::get( PFG_ETC2_RGB8_UNORM ), &props );
            if( props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT )
            {
                rsc->setCapability( RSC_TEXTURE_COMPRESSION_ETC1 );
                rsc->setCapability( RSC_TEXTURE_COMPRESSION_ETC2 );
            }

            vkGetPhysicalDeviceFormatProperties(
                mDevice->mPhysicalDevice, VulkanMappings::get( PFG_ASTC_RGBA_UNORM_4X4_LDR ), &props );
            if( props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT )
                rsc->setCapability( RSC_TEXTURE_COMPRESSION_ASTC );
        }

        const VkPhysicalDeviceLimits &deviceLimits = mDevice->mDeviceProperties.limits;
        rsc->setMaximumResolutions( std::min( deviceLimits.maxImageDimension2D, 16384u ),
                                    std::min( deviceLimits.maxImageDimension3D, 4096u ),
                                    std::min( deviceLimits.maxImageDimensionCube, 16384u ) );
        rsc->setMaxThreadsPerThreadgroupAxis( deviceLimits.maxComputeWorkGroupSize );
        rsc->setMaxThreadsPerThreadgroup( deviceLimits.maxComputeWorkGroupInvocations );

        if( mDevice->mDeviceFeatures.samplerAnisotropy && deviceLimits.maxSamplerAnisotropy > 1u )
        {
            rsc->setCapability( RSC_ANISOTROPY );
            rsc->setMaxSupportedAnisotropy( deviceLimits.maxSamplerAnisotropy );
        }

        {
            uint32 numTexturesInTextureDescriptor[NumShaderTypes + 1];
            for( size_t i = 0u; i < NumShaderTypes + 1; ++i )
                numTexturesInTextureDescriptor[i] = deviceLimits.maxPerStageDescriptorSampledImages;
            rsc->setNumTexturesInTextureDescriptor( numTexturesInTextureDescriptor );
        }

        rsc->setCapability( RSC_STORE_AND_MULTISAMPLE_RESOLVE );
        rsc->setCapability( RSC_TEXTURE_GATHER );

        rsc->setCapability( RSC_COMPUTE_PROGRAM );
        rsc->setCapability( RSC_UAV );
        rsc->setCapability( RSC_TYPED_UAV_LOADS );
        rsc->setCapability( RSC_EXPLICIT_FSAA_RESOLVE );
        rsc->setCapability( RSC_TEXTURE_1D );

        rsc->setCapability( RSC_HWSTENCIL );
        rsc->setStencilBufferBitDepth( 8 );
        rsc->setNumTextureUnits( NUM_BIND_TEXTURES );
        rsc->setCapability( RSC_AUTOMIPMAP );
        rsc->setCapability( RSC_BLENDING );
        rsc->setCapability( RSC_DOT3 );
        rsc->setCapability( RSC_CUBEMAPPING );
        rsc->setCapability( RSC_TEXTURE_COMPRESSION );
        rsc->setCapability( RSC_VBO );
        // VK_INDEX_TYPE_UINT32 is always supported with range at least 2^24-1
        // and even 2^32-1 if mDevice->mDeviceFeatures.fullDrawIndexUint32
        rsc->setCapability( RSC_32BIT_INDEX );
        rsc->setCapability( RSC_TWO_SIDED_STENCIL );
        rsc->setCapability( RSC_STENCIL_WRAP );
        if( mDevice->mDeviceFeatures.shaderClipDistance )
            rsc->setCapability( RSC_USER_CLIP_PLANES );
        rsc->setCapability( RSC_VERTEX_FORMAT_UBYTE4 );
        rsc->setCapability( RSC_INFINITE_FAR_PLANE );
        rsc->setCapability( RSC_TEXTURE_3D );
        rsc->setCapability( RSC_NON_POWER_OF_2_TEXTURES );
        rsc->setNonPOW2TexturesLimited( false );
        rsc->setCapability( RSC_HWRENDER_TO_TEXTURE );
        rsc->setCapability( RSC_TEXTURE_FLOAT );
        rsc->setCapability( RSC_POINT_SPRITES );
        rsc->setCapability( RSC_POINT_EXTENDED_PARAMETERS );
        rsc->setCapability( RSC_TEXTURE_2D_ARRAY );
        rsc->setCapability( RSC_CONST_BUFFER_SLOTS_IN_SHADER );
        rsc->setCapability( RSC_SEPARATE_SAMPLERS_FROM_TEXTURES );
        rsc->setCapability( RSC_ALPHA_TO_COVERAGE );
        rsc->setCapability( RSC_HW_GAMMA );
        rsc->setCapability( RSC_VERTEX_BUFFER_INSTANCE_DATA );
        // We don't support PSO nor VkShaderModule caches yet, but we do have SPIR-V caches.
        rsc->setCapability( RSC_CAN_GET_COMPILED_SHADER_BUFFER );
        rsc->setCapability( RSC_EXPLICIT_API );
        rsc->setMaxPointSize( 256 );

        // check memory properties to determine, if we can use UMA and/or TBDR optimizations
        const VkPhysicalDeviceMemoryProperties &memoryProperties = mDevice->mDeviceMemoryProperties;
        if( mDevice->mDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||
            mDevice->mDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU )
        {
            for( uint32_t typeIndex = 0; typeIndex < memoryProperties.memoryTypeCount; ++typeIndex )
            {
                const VkMemoryType &memoryType = memoryProperties.memoryTypes[typeIndex];
                if( ( memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) != 0 &&
                    ( memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) != 0 &&
                    ( memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) != 0 )
                {
                    rsc->setCapability( RSC_UMA );
                }

                // VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT is a prerequisite for TBDR, and is probably
                // a good heuristic that TBDR mode of buffers clearing is supported efficiently,
                // i.e. RSC_IS_TILER.
                if( ( memoryType.propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT ) != 0 )
                {
                    rsc->setCapability( RSC_IS_TILER );
                    rsc->setCapability( RSC_TILER_CAN_CLEAR_STENCIL_REGION );
                }
            }
        }

        rsc->setVertexProgramConstantFloatCount( 256u );
        rsc->setVertexProgramConstantIntCount( 256u );
        rsc->setVertexProgramConstantBoolCount( 256u );
        rsc->setGeometryProgramConstantFloatCount( 256u );
        rsc->setGeometryProgramConstantIntCount( 256u );
        rsc->setGeometryProgramConstantBoolCount( 256u );
        rsc->setFragmentProgramConstantFloatCount( 256u );
        rsc->setFragmentProgramConstantIntCount( 256u );
        rsc->setFragmentProgramConstantBoolCount( 256u );
        rsc->setTessellationHullProgramConstantFloatCount( 256u );
        rsc->setTessellationHullProgramConstantIntCount( 256u );
        rsc->setTessellationHullProgramConstantBoolCount( 256u );
        rsc->setTessellationDomainProgramConstantFloatCount( 256u );
        rsc->setTessellationDomainProgramConstantIntCount( 256u );
        rsc->setTessellationDomainProgramConstantBoolCount( 256u );
        rsc->setComputeProgramConstantFloatCount( 256u );
        rsc->setComputeProgramConstantIntCount( 256u );
        rsc->setComputeProgramConstantBoolCount( 256u );

        rsc->addShaderProfile( "hlslvk" );
        rsc->addShaderProfile( "hlsl" );
        rsc->addShaderProfile( "glslvk" );
        rsc->addShaderProfile( "glsl" );

        // Turnip is the Mesa driver.
        // These workarounds are for the proprietary driver.
        if( rsc->getVendor() == GPU_QUALCOMM && rsc->getDeviceName().find( "Turnip" ) == String::npos )
        {
#ifdef OGRE_VK_WORKAROUND_BAD_3D_BLIT
            Workarounds::mBad3DBlit = true;
#endif
#ifdef OGRE_VK_WORKAROUND_ADRENO_D32_FLOAT
            Workarounds::mAdrenoD32FloatBug = false;
            if( !rsc->getDriverVersion().hasMinVersion( 512, 415 ) )
                Workarounds::mAdrenoD32FloatBug = true;
#endif
#ifdef OGRE_VK_WORKAROUND_ADRENO_5XX_6XX_MINCAPS
            Workarounds::mAdreno5xx6xxMinCaps = false;

            const uint32 c_adreno5xx6xxDeviceIds[] = {
                0x4010800,  // 418
                0x4030002,  // 430

                0x5000400,  // 504
                0x5000500,  // 505
                0x5000600,  // 506
                0x5000800,  // 508
                0x5000900,  // 509
                0x5010000,  // 510
                0x5010200,  // 512
                0x5030002,  // 530
                0x5040001,  // 540

                0x6010000,  // 610
                0x6010200,  // 612
                0x6010501,  // 615
                0x6010600,  // 616
                0x6010800,  // 618
                0x6010900,  // 619
                0x6020001,  // 620
                0x6030001,  // 630
                0x6040001,  // 640
                0x6050002,  // 650
                // 0x6060001 // 660 (doesn't need workaround)
            };

            const size_t numDevIds =
                sizeof( c_adreno5xx6xxDeviceIds ) / sizeof( c_adreno5xx6xxDeviceIds[0] );
            for( size_t i = 0u; i < numDevIds; ++i )
            {
                if( properties.deviceID == c_adreno5xx6xxDeviceIds[i] )
                {
                    Workarounds::mAdreno5xx6xxMinCaps = true;
                    break;
                }
            }
#endif
        }

        return rsc;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::setSwappyFramePacing( bool bEnable, Window *callingWindow )
    {
#ifdef OGRE_VULKAN_USE_SWAPPY
        const bool bChanged = mSwappyFramePacing != bEnable;
        mSwappyFramePacing = bEnable;

        if( bChanged )
        {
            for( Window *window : mWindows )
            {
                if( window != callingWindow )
                {
                    OGRE_ASSERT_HIGH( dynamic_cast<VulkanAndroidWindow *>( window ) );
                    static_cast<VulkanAndroidWindow *>( window )->_notifySwappyToggled();
                }
            }
        }
#endif
    }
    //-------------------------------------------------------------------------
    bool VulkanRenderSystem::getSwappyFramePacing() const
    {
#ifdef OGRE_VULKAN_USE_SWAPPY
        return mSwappyFramePacing;
#else
        return false;
#endif
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::resetAllBindings()
    {
        OGRE_ASSERT_HIGH( dynamic_cast<VulkanConstBufferPacked *>( mDummyBuffer ) );
        VulkanConstBufferPacked *constBuffer = static_cast<VulkanConstBufferPacked *>( mDummyBuffer );

        VkDescriptorBufferInfo dummyBufferInfo;
        constBuffer->getBufferInfo( dummyBufferInfo );

        for( size_t i = 0u; i < NumShaderTypes + 1u; ++i )
        {
            mGlobalTable.paramsBuffer[i] = dummyBufferInfo;
            mComputeTable.paramsBuffer[i] = dummyBufferInfo;
        }

        for( size_t i = 0u; i < NUM_BIND_CONST_BUFFERS; ++i )
        {
            mGlobalTable.constBuffers[i] = dummyBufferInfo;
            mComputeTable.constBuffers[i] = dummyBufferInfo;
        }

        for( size_t i = 0u; i < NUM_BIND_READONLY_BUFFERS; ++i )
            mGlobalTable.readOnlyBuffers[i] = dummyBufferInfo;

        // Compute (mComputeTable) only uses baked descriptors for Textures and TexBuffers
        // hence no need to clean the emulated bindings
        OGRE_ASSERT_HIGH( dynamic_cast<VulkanTexBufferPacked *>( mDummyTexBuffer ) );
        VulkanTexBufferPacked *texBuffer = static_cast<VulkanTexBufferPacked *>( mDummyTexBuffer );

        VkBufferView texBufferView = texBuffer->_bindBufferCommon( 0u, 0u );
        for( size_t i = 0u; i < NUM_BIND_TEX_BUFFERS; ++i )
            mGlobalTable.texBuffers[i] = texBufferView;

        for( size_t i = 0u; i < NUM_BIND_TEXTURES; ++i )
        {
            mGlobalTable.textures[i].imageView = mDummyTextureView;
            mGlobalTable.textures[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        for( size_t i = 0u; i < NUM_BIND_SAMPLERS; ++i )
            mGlobalTable.samplers[i].sampler = mDummySampler;

        for( size_t i = 0u; i < BakedDescriptorSets::NumBakedDescriptorSets; ++i )
        {
            mGlobalTable.bakedDescriptorSets[i] = 0;
            mComputeTable.bakedDescriptorSets[i] = 0;
        }

        mGlobalTable.setAllDirty();
        mComputeTable.setAllDirty();
        mTableDirty = true;
        mComputeTableDirty = true;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::reinitialise()
    {
        this->shutdown();
        this->_initialise( true );
    }
    //-------------------------------------------------------------------------
    Window *VulkanRenderSystem::_initialise( bool autoCreateWindow, const String &windowTitle )
    {
        Window *autoWindow = 0;
        if( autoCreateWindow )
        {
            NameValuePairList miscParams;

            bool bFullscreen = false;
            uint32 w = 800, h = 600;

            const ConfigOptionMap &options = mVulkanSupport->getConfigOptions( this );
            ConfigOptionMap::const_iterator opt;
            ConfigOptionMap::const_iterator end = options.end();

            if( ( opt = options.find( "Full Screen" ) ) != end )
                bFullscreen = ( opt->second.currentValue == "Yes" );
            if( ( opt = options.find( "Video Mode" ) ) != end )
            {
                String val = opt->second.currentValue;
                String::size_type pos = val.find( 'x' );

                if( pos != String::npos )
                {
                    w = StringConverter::parseUnsignedInt( val.substr( 0, pos ) );
                    h = StringConverter::parseUnsignedInt( val.substr( pos + 1 ) );
                }
            }
            if( ( opt = options.find( "FSAA" ) ) != end )
                miscParams["FSAA"] = opt->second.currentValue;
            if( ( opt = options.find( "VSync" ) ) != end )
                miscParams["vsync"] = opt->second.currentValue;
            if( ( opt = options.find( "sRGB Gamma Conversion" ) ) != end )
                miscParams["gamma"] = opt->second.currentValue;
            if( ( opt = options.find( "VSync Method" ) ) != end )
                miscParams["vsync_method"] = opt->second.currentValue;

            autoWindow = _createRenderWindow( windowTitle, w, h, bFullscreen, &miscParams );
        }
        RenderSystem::_initialise( autoCreateWindow, windowTitle );

        return autoWindow;
    }
    //-------------------------------------------------------------------------
    Window *VulkanRenderSystem::_createRenderWindow( const String &name, uint32 width, uint32 height,
                                                     bool fullScreen,
                                                     const NameValuePairList *miscParams )
    {
        String windowType;
        if( miscParams )
        {
            // Get variable-length params
            NameValuePairList::const_iterator opt = miscParams->find( "windowType" );
            if( opt != miscParams->end() )
                windowType = opt->second;
        }

        VulkanWindow *win = nullptr;
        if( windowType == "null" || mVulkanSupport->getInterfaceName() == "null" )
        {
            win = OGRE_NEW VulkanWindowNull( name, width, height, fullScreen );
        }
        else
        {
#ifdef OGRE_VULKAN_WINDOW_WIN32
            win = OGRE_NEW VulkanWin32Window( name, width, height, fullScreen );
#endif
#ifdef OGRE_VULKAN_WINDOW_XCB
            win = OGRE_NEW VulkanXcbWindow( name, width, height, fullScreen );
#endif
#ifdef OGRE_VULKAN_WINDOW_ANDROID
            win = OGRE_NEW VulkanAndroidWindow( name, width, height, fullScreen );
#endif
        }
        mWindows.insert( win );

        if( !mInitialized )
        {
            VulkanExternalDevice *externalDevice = 0;
            if( miscParams )
            {
                NameValuePairList::const_iterator itOption = miscParams->find( "reverse_depth" );
                if( itOption != miscParams->end() )
                    mReverseDepth = StringConverter::parseBool( itOption->second, true );

                itOption = miscParams->find( "external_device" );
                if( itOption != miscParams->end() )
                {
                    externalDevice = reinterpret_cast<VulkanExternalDevice *>(
                        StringConverter::parseUnsignedLong( itOption->second ) );
                }
            }

            mNativeShadingLanguageVersion = 450;

            if( !mInstance )
            {
                mInstance = std::make_shared<VulkanInstance>( Root::getSingleton().getAppName(), nullptr,
                                                              dbgFunc, this );
            }

            mActiveDevice = externalDevice
                                ? VulkanPhysicalDevice( { externalDevice->physicalDevice,
                                                          { 0ul, 0ul },
                                                          VK_DRIVER_ID_MAX_ENUM,
                                                          0u,
                                                          "[OgreNext] External Device" } )
                                : *mInstance->findByName( mVulkanSupport->getSelectedDeviceName() );

            mDevice = new VulkanDevice( this );
            VulkanVaoManager *vaoManager = OGRE_NEW VulkanVaoManager( mDevice, this, miscParams );
            mVaoManager = vaoManager;
            mDevice->mVaoManager = vaoManager;

            mDevice->setPhysicalDevice( mInstance, mActiveDevice, externalDevice );

            mRealCapabilities = createRenderSystemCapabilities();
            mCurrentCapabilities = mRealCapabilities;

            vaoManager->createVkResources();

            initialiseFromRenderSystemCapabilities( mCurrentCapabilities, 0 );

            mHardwareBufferManager = OGRE_NEW v1::VulkanHardwareBufferManager( mDevice, mVaoManager );

            FastArray<PixelFormatGpu> depthFormatCandidates( 5u );
            if( DepthBuffer::AvailableDepthFormats & DepthBuffer::DFM_S8 )
            {
                if( DepthBuffer::AvailableDepthFormats & DepthBuffer::DFM_D32 )
                    depthFormatCandidates.push_back( PFG_D32_FLOAT_S8X24_UINT );
                if( DepthBuffer::AvailableDepthFormats & DepthBuffer::DFM_D24 )
                    depthFormatCandidates.push_back( PFG_D24_UNORM_S8_UINT );
                if( DepthBuffer::AvailableDepthFormats & DepthBuffer::DFM_D32 )
                    depthFormatCandidates.push_back( PFG_D32_FLOAT );
                if( DepthBuffer::AvailableDepthFormats & DepthBuffer::DFM_D24 )
                    depthFormatCandidates.push_back( PFG_D24_UNORM );
                if( DepthBuffer::AvailableDepthFormats & DepthBuffer::DFM_D16 )
                    depthFormatCandidates.push_back( PFG_D16_UNORM );
            }
            else
            {
                if( DepthBuffer::AvailableDepthFormats & DepthBuffer::DFM_D32 )
                    depthFormatCandidates.push_back( PFG_D32_FLOAT );
                if( DepthBuffer::AvailableDepthFormats & DepthBuffer::DFM_D32 )
                    depthFormatCandidates.push_back( PFG_D32_FLOAT_S8X24_UINT );
                if( DepthBuffer::AvailableDepthFormats & DepthBuffer::DFM_D24 )
                    depthFormatCandidates.push_back( PFG_D24_UNORM );
                if( DepthBuffer::AvailableDepthFormats & DepthBuffer::DFM_D24 )
                    depthFormatCandidates.push_back( PFG_D24_UNORM_S8_UINT );
                if( DepthBuffer::AvailableDepthFormats & DepthBuffer::DFM_D16 )
                    depthFormatCandidates.push_back( PFG_D16_UNORM );
            }

            if( !depthFormatCandidates.empty() )
            {
                DepthBuffer::DefaultDepthBufferFormat = findSupportedFormat(
                    mDevice->mPhysicalDevice, depthFormatCandidates, VK_IMAGE_TILING_OPTIMAL,
                    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
            }
            else
            {
                DepthBuffer::DefaultDepthBufferFormat = PFG_NULL;
            }

            bool bCanRestrictImageViewUsage =
                mDevice->hasDeviceExtension( VK_KHR_MAINTENANCE2_EXTENSION_NAME );

            if( !bCanRestrictImageViewUsage )
            {
                LogManager::getSingleton().logMessage(
                    "WARNING: " VK_KHR_MAINTENANCE2_EXTENSION_NAME
                    " not present. We may have to force the driver to do UAV + SRGB operations "
                    "the GPU should support, but it's not guaranteed to work" );
            }

            VulkanTextureGpuManager *textureGpuManager = OGRE_NEW VulkanTextureGpuManager(
                vaoManager, this, mDevice, bCanRestrictImageViewUsage );
            mTextureGpuManager = textureGpuManager;
            {
                ConfigOptionMap::const_iterator it = getConfigOptions().find( "Allow Memoryless RTT" );
                if( it != getConfigOptions().end() )
                {
                    mTextureGpuManager->setAllowMemoryless(
                        StringConverter::parseBool( it->second.currentValue, true ) );
                }
            }

            createVkResources();

            String workaroundsStr;
            Workarounds::dump( workaroundsStr );
            if( !workaroundsStr.empty() )
            {
                workaroundsStr = "Workarounds applied:" + workaroundsStr;
                LogManager::getSingleton().logMessage( workaroundsStr );
            }

            mInitialized = true;
        }

        win->_setDevice( mDevice );
        win->_initialize( mTextureGpuManager, miscParams );

        return win;
    }
    //-------------------------------------------------------------------------
    const FastArray<VulkanPhysicalDevice> &VulkanRenderSystem::getVulkanPhysicalDevices() const
    {
        return mInstance->mVulkanPhysicalDevices;
    }
    //---------------------------------------------------------------------
    bool VulkanRenderSystem::isDeviceLost() { return mDevice && mDevice->isDeviceLost(); }
    //-------------------------------------------------------------------------
    bool VulkanRenderSystem::validateDevice( bool forceDeviceElection )
    {
        if( mDevice == nullptr || mDevice->mIsExternal || mInstance->mVkInstanceIsExternal )
            return false;

        bool anotherIsElected = false;
        if( forceDeviceElection )
        {
            // elect new physical device
            LogManager::getSingleton().logMessage( "Vulkan: Create new instance for device detection" );
            std::shared_ptr<VulkanInstance> freshInstance = std::make_shared<VulkanInstance>(
                Root::getSingleton().getAppName(), nullptr, dbgFunc, this );
            auto device = freshInstance->findByName( mVulkanSupport->getSelectedDeviceName() );
            if( device == nullptr )
                return false;

            anotherIsElected = device->physicalDeviceID[0] != mActiveDevice.physicalDeviceID[0] ||
                               device->physicalDeviceID[1] != mActiveDevice.physicalDeviceID[1];
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
            anotherIsElected = true;  // simulate switching instance and physical device
#endif
            if( anotherIsElected )
            {
                LogManager::getSingleton().logMessage(
                    "Vulkan: Another device was elected, switching VkInstance and active device" );
                mInstance = freshInstance;
                mActiveDevice = *device;
            }
        }

        // recreate logical device with all resources
        if( anotherIsElected || mDevice->isDeviceLost() )
        {
            handleDeviceLost();

            return !mDevice->isDeviceLost();
        }

        return true;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::handleDeviceLost()
    {
        // recreate logical device with all resources
        LogManager::getSingleton().logMessage( "Vulkan: Device was lost, recreating." );

        Timer timer;
        uint64 startTime = timer.getMicroseconds();

        destroyVkResources0();

        // release device depended resources
        fireEvent( "DeviceLost" );

        Root::getSingleton().getCompositorManager2()->_releaseManualHardwareResources();
        SceneManagerEnumerator::SceneManagerIterator scnIt =
            SceneManagerEnumerator::getSingleton().getSceneManagerIterator();
        while( scnIt.hasMoreElements() )
            scnIt.getNext()->_releaseManualHardwareResources();

        Root::getSingleton().getHlmsManager()->_changeRenderSystem( (RenderSystem *)0 );

        MeshManager::getSingleton().unloadAll( Resource::LF_MARKED_FOR_RELOAD );

        notifyDeviceLost();

        destroyVkResources1();
        static_cast<VulkanTextureGpuManager *>( mTextureGpuManager )->destroyVkResources();
        static_cast<VulkanVaoManager *>( mVaoManager )->destroyVkResources();

        // Release all automatic temporary buffers and free unused
        // temporary buffers, so we doesn't need to recreate them,
        // and they will reallocate on demand.
        v1::HardwareBufferManager::getSingleton()._releaseBufferCopies( true );

        // recreate device
        mDevice->setPhysicalDevice( mInstance, mActiveDevice, nullptr );

        static_cast<VulkanVaoManager *>( mVaoManager )->createVkResources();
        static_cast<VulkanTextureGpuManager *>( mTextureGpuManager )->createVkResources();
        createVkResources();

        // recreate device depended resources
        notifyDeviceRestored();

        Root::getSingleton().getHlmsManager()->_changeRenderSystem( this );

        v1::MeshManager::getSingleton().reloadAll( Resource::LF_PRESERVE_STATE );
        MeshManager::getSingleton().reloadAll( Resource::LF_MARKED_FOR_RELOAD );

        Root::getSingleton().getCompositorManager2()->_restoreManualHardwareResources();
        scnIt = SceneManagerEnumerator::getSingleton().getSceneManagerIterator();
        while( scnIt.hasMoreElements() )
            scnIt.getNext()->_restoreManualHardwareResources();

        fireEvent( "DeviceRestored" );

        uint64 passedTime = ( timer.getMicroseconds() - startTime ) / 1000;
        LogManager::getSingleton().logMessage( "Vulkan: Device was restored in " +
                                               StringConverter::toString( passedTime ) + "ms" );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_notifyDeviceStalled()
    {
        v1::VulkanHardwareBufferManager *hwBufferMgr =
            static_cast<v1::VulkanHardwareBufferManager *>( mHardwareBufferManager );
        VulkanVaoManager *vaoManager = static_cast<VulkanVaoManager *>( mVaoManager );

        hwBufferMgr->_notifyDeviceStalled();
        vaoManager->_notifyDeviceStalled();
    }
    //-------------------------------------------------------------------------
    String VulkanRenderSystem::getErrorDescription( long errorNumber ) const { return BLANKSTRING; }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_useLights( const LightList &lights, unsigned short limit ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setWorldMatrix( const Matrix4 &m ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setViewMatrix( const Matrix4 &m ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setProjectionMatrix( const Matrix4 &m ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setSurfaceParams( const ColourValue &ambient, const ColourValue &diffuse,
                                                const ColourValue &specular, const ColourValue &emissive,
                                                Real shininess, TrackVertexColourType tracking )
    {
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setPointSpritesEnabled( bool enabled ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setPointParameters( Real size, bool attenuationEnabled, Real constant,
                                                  Real linear, Real quadratic, Real minSize,
                                                  Real maxSize )
    {
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::flushUAVs()
    {
        if( mUavRenderingDirty )
        {
            if( !mUavRenderingDescSet )
            {
                if( mGlobalTable.bakedDescriptorSets[BakedDescriptorSets::UavBuffers] )
                {
                    mGlobalTable.bakedDescriptorSets[BakedDescriptorSets::UavBuffers] = 0;
                    mGlobalTable.bakedDescriptorSets[BakedDescriptorSets::UavTextures] = 0;
                    mGlobalTable.dirtyBakedUavs = true;
                    mTableDirty = true;
                }
            }
            else
            {
                VulkanDescriptorSetUav *vulkanSet =
                    reinterpret_cast<VulkanDescriptorSetUav *>( mUavRenderingDescSet->mRsData );
                if( mGlobalTable.bakedDescriptorSets[BakedDescriptorSets::UavBuffers] !=
                    &vulkanSet->mWriteDescSets[0] )
                {
                    mGlobalTable.bakedDescriptorSets[BakedDescriptorSets::UavBuffers] =
                        &vulkanSet->mWriteDescSets[0];
                    mGlobalTable.bakedDescriptorSets[BakedDescriptorSets::UavTextures] =
                        &vulkanSet->mWriteDescSets[1];
                    mGlobalTable.dirtyBakedUavs = true;
                    mTableDirty = true;
                }
            }

            mUavRenderingDirty = false;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setParamBuffer( GpuProgramType shaderStage,
                                              const VkDescriptorBufferInfo &bufferInfo )
    {
        if( shaderStage != GPT_COMPUTE_PROGRAM )
        {
            if( mGlobalTable.paramsBuffer[shaderStage].buffer != bufferInfo.buffer ||
                mGlobalTable.paramsBuffer[shaderStage].offset != bufferInfo.offset ||
                mGlobalTable.paramsBuffer[shaderStage].range != bufferInfo.range )
            {
                mGlobalTable.paramsBuffer[shaderStage] = bufferInfo;
                mGlobalTable.dirtyParamsBuffer = true;
                mTableDirty = true;
            }
        }
        else
        {
            if( mComputeTable.paramsBuffer[shaderStage].buffer != bufferInfo.buffer ||
                mComputeTable.paramsBuffer[shaderStage].offset != bufferInfo.offset ||
                mComputeTable.paramsBuffer[shaderStage].range != bufferInfo.range )
            {
                mComputeTable.paramsBuffer[shaderStage] = bufferInfo;
                mComputeTable.dirtyParamsBuffer = true;
                mComputeTableDirty = true;
            }
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setConstBuffer( size_t slot, const VkDescriptorBufferInfo &bufferInfo )
    {
        OGRE_ASSERT_MEDIUM( slot < NUM_BIND_CONST_BUFFERS );
        if( mGlobalTable.constBuffers[slot].buffer != bufferInfo.buffer ||
            mGlobalTable.constBuffers[slot].offset != bufferInfo.offset ||
            mGlobalTable.constBuffers[slot].range != bufferInfo.range )
        {
            mGlobalTable.constBuffers[slot] = bufferInfo;
            mGlobalTable.minDirtySlotConst = std::min( mGlobalTable.minDirtySlotConst, (uint8)slot );
            mTableDirty = true;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setConstBufferCS( size_t slot, const VkDescriptorBufferInfo &bufferInfo )
    {
        OGRE_ASSERT_MEDIUM( slot < NUM_BIND_CONST_BUFFERS );
        if( mComputeTable.constBuffers[slot].buffer != bufferInfo.buffer ||
            mComputeTable.constBuffers[slot].offset != bufferInfo.offset ||
            mComputeTable.constBuffers[slot].range != bufferInfo.range )
        {
            mComputeTable.constBuffers[slot] = bufferInfo;
            mComputeTable.minDirtySlotConst = std::min( mComputeTable.minDirtySlotConst, (uint8)slot );
            mComputeTableDirty = true;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setTexBuffer( size_t slot, VkBufferView bufferView )
    {
        OGRE_ASSERT_MEDIUM( slot < NUM_BIND_TEX_BUFFERS );
        if( mGlobalTable.texBuffers[slot] != bufferView )
        {
            mGlobalTable.texBuffers[slot] = bufferView;
            mGlobalTable.minDirtySlotTexBuffer =
                std::min( mGlobalTable.minDirtySlotTexBuffer, (uint8)slot );
            mTableDirty = true;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setTexBufferCS( size_t slot, VkBufferView bufferView )
    {
        OGRE_ASSERT_MEDIUM( slot < NUM_BIND_TEX_BUFFERS );
        if( mComputeTable.texBuffers[slot] != bufferView )
        {
            mComputeTable.texBuffers[slot] = bufferView;
            mComputeTable.minDirtySlotTexBuffer =
                std::min( mComputeTable.minDirtySlotTexBuffer, (uint8)slot );
            mComputeTableDirty = true;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setReadOnlyBuffer( size_t slot, const VkDescriptorBufferInfo &bufferInfo )
    {
        OGRE_ASSERT_MEDIUM( slot < NUM_BIND_READONLY_BUFFERS );
        if( mGlobalTable.readOnlyBuffers[slot].buffer != bufferInfo.buffer ||
            mGlobalTable.readOnlyBuffers[slot].offset != bufferInfo.offset ||
            mGlobalTable.readOnlyBuffers[slot].range != bufferInfo.range )
        {
            mGlobalTable.readOnlyBuffers[slot] = bufferInfo;
            mGlobalTable.minDirtySlotReadOnlyBuffer =
                std::min( mGlobalTable.minDirtySlotReadOnlyBuffer, (uint8)slot );
            mTableDirty = true;
        }
    }
#ifdef OGRE_VK_WORKAROUND_ADRENO_6xx_READONLY_IS_TBUFFER
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setReadOnlyBuffer( size_t slot, VkBufferView bufferView )
    {
        OGRE_ASSERT_MEDIUM( slot < NUM_BIND_READONLY_BUFFERS );
        if( mGlobalTable.readOnlyBuffers2[slot] != bufferView )
        {
            mGlobalTable.readOnlyBuffers2[slot] = bufferView;
            mGlobalTable.minDirtySlotReadOnlyBuffer =
                std::min( mGlobalTable.minDirtySlotReadOnlyBuffer, (uint8)slot );
            mTableDirty = true;
        }
    }
#endif
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setCurrentDeviceFromTexture( TextureGpu *texture ) {}
    //-------------------------------------------------------------------------
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
    static void checkTextureLayout( const TextureGpu *texture,
                                    RenderPassDescriptor *currentRenderPassDescriptor )
    {
        OGRE_ASSERT_HIGH( dynamic_cast<const VulkanTextureGpu *>( texture ) );
        const VulkanTextureGpu *tex = static_cast<const VulkanTextureGpu *>( texture );

        if( tex->isDataReady() && tex->mCurrLayout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
            tex->mCurrLayout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL )
        {
            TextureGpu *targetTex;
            uint8 targetMip;
            currentRenderPassDescriptor->findAnyTexture( &targetTex, targetMip );
            String texName = targetTex ? targetTex->getNameStr() : "";
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "Texture " + tex->getNameStr() +
                             " is not in ResourceLayout::Texture nor RenderTargetReadOnly."
                             " Did you forget to expose it to  compositor? "
                             "Currently rendering to target: " +
                             texName,
                         "VulkanRenderSystem::checkTextureLayout" );
        }
    }
#endif
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setTexture( size_t unit, TextureGpu *texPtr, bool bDepthReadOnly )
    {
        OGRE_ASSERT_MEDIUM( unit < NUM_BIND_TEXTURES );
        if( texPtr )
        {
            VulkanTextureGpu *tex = static_cast<VulkanTextureGpu *>( texPtr );

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
            checkTextureLayout( tex, mCurrentRenderPassDescriptor );
#endif
            const VkImageLayout targetLayout = bDepthReadOnly
                                                   ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                                                   : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            if( mGlobalTable.textures[unit].imageView != tex->getDefaultDisplaySrv() ||
                mGlobalTable.textures[unit].imageLayout != targetLayout )
            {
                mGlobalTable.textures[unit].imageView = tex->getDefaultDisplaySrv();
                mGlobalTable.textures[unit].imageLayout = targetLayout;

                mGlobalTable.minDirtySlotTextures =
                    std::min( mGlobalTable.minDirtySlotTextures, (uint8)unit );
                mTableDirty = true;
            }
        }
        else
        {
            if( mGlobalTable.textures[unit].imageView != mDummyTextureView )
            {
                mGlobalTable.textures[unit].imageView = mDummyTextureView;
                mGlobalTable.textures[unit].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                mGlobalTable.minDirtySlotTextures =
                    std::min( mGlobalTable.minDirtySlotTextures, (uint8)unit );
                mTableDirty = true;
            }
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setTextures( uint32 slotStart, const DescriptorSetTexture *set,
                                           uint32 hazardousTexIdx )
    {
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
        {
            FastArray<const TextureGpu *>::const_iterator itor = set->mTextures.begin();
            FastArray<const TextureGpu *>::const_iterator endt = set->mTextures.end();

            while( itor != endt )
            {
                checkTextureLayout( *itor, mCurrentRenderPassDescriptor );
                ++itor;
            }
        }
#endif
        VulkanDescriptorSetTexture *vulkanSet =
            reinterpret_cast<VulkanDescriptorSetTexture *>( set->mRsData );

        VkWriteDescriptorSet *writeDescSet = &vulkanSet->mWriteDescSet;
        if( hazardousTexIdx < set->mTextures.size() &&
            mCurrentRenderPassDescriptor->hasAttachment( set->mTextures[hazardousTexIdx] ) )
        {
            vulkanSet->setHazardousTex( *set, hazardousTexIdx,
                                        static_cast<VulkanTextureGpuManager *>( mTextureGpuManager ) );
            writeDescSet = &vulkanSet->mWriteDescSetHazardous;
        }

        if( mGlobalTable.bakedDescriptorSets[BakedDescriptorSets::Textures] != writeDescSet )
        {
            mGlobalTable.bakedDescriptorSets[BakedDescriptorSets::TexBuffers] = 0;
            mGlobalTable.bakedDescriptorSets[BakedDescriptorSets::Textures] = writeDescSet;
            mGlobalTable.bakedDescriptorSets[BakedDescriptorSets::UavBuffers] = 0;
            mGlobalTable.dirtyBakedTextures = true;
            mTableDirty = true;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setTextures( uint32 slotStart, const DescriptorSetTexture2 *set )
    {
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
        {
            FastArray<DescriptorSetTexture2::Slot>::const_iterator itor = set->mTextures.begin();
            FastArray<DescriptorSetTexture2::Slot>::const_iterator endt = set->mTextures.end();

            while( itor != endt )
            {
                if( itor->isTexture() )
                    checkTextureLayout( itor->getTexture().texture, mCurrentRenderPassDescriptor );
                ++itor;
            }
        }
#endif
        VulkanDescriptorSetTexture2 *vulkanSet =
            reinterpret_cast<VulkanDescriptorSetTexture2 *>( set->mRsData );

        if( mGlobalTable.bakedDescriptorSets[BakedDescriptorSets::ReadOnlyBuffers] !=
            &vulkanSet->mWriteDescSets[0] )
        {
            mGlobalTable.bakedDescriptorSets[BakedDescriptorSets::ReadOnlyBuffers] =
                &vulkanSet->mWriteDescSets[0];
            mGlobalTable.bakedDescriptorSets[BakedDescriptorSets::TexBuffers] =
                &vulkanSet->mWriteDescSets[1];
            mGlobalTable.bakedDescriptorSets[BakedDescriptorSets::Textures] =
                &vulkanSet->mWriteDescSets[2];
            mGlobalTable.dirtyBakedTextures = true;
            mTableDirty = true;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setSamplers( uint32 slotStart, const DescriptorSetSampler *set )
    {
        VulkanDescriptorSetSampler *vulkanSet =
            reinterpret_cast<VulkanDescriptorSetSampler *>( set->mRsData );

        if( mGlobalTable.bakedDescriptorSets[BakedDescriptorSets::Samplers] !=
            &vulkanSet->mWriteDescSet )
        {
            mGlobalTable.bakedDescriptorSets[BakedDescriptorSets::Samplers] = &vulkanSet->mWriteDescSet;
            mGlobalTable.dirtyBakedSamplers = true;
            mTableDirty = true;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setTexturesCS( uint32 slotStart, const DescriptorSetTexture *set )
    {
        VulkanDescriptorSetTexture *vulkanSet =
            reinterpret_cast<VulkanDescriptorSetTexture *>( set->mRsData );

        if( mComputeTable.bakedDescriptorSets[BakedDescriptorSets::Textures] !=
            &vulkanSet->mWriteDescSet )
        {
            mComputeTable.bakedDescriptorSets[BakedDescriptorSets::ReadOnlyBuffers] = 0;
            mComputeTable.bakedDescriptorSets[BakedDescriptorSets::TexBuffers] = 0;
            mComputeTable.bakedDescriptorSets[BakedDescriptorSets::Textures] = &vulkanSet->mWriteDescSet;
            mComputeTable.dirtyBakedSamplers = true;
            mComputeTableDirty = true;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setTexturesCS( uint32 slotStart, const DescriptorSetTexture2 *set )
    {
        VulkanDescriptorSetTexture2 *vulkanSet =
            reinterpret_cast<VulkanDescriptorSetTexture2 *>( set->mRsData );

        if( mComputeTable.bakedDescriptorSets[BakedDescriptorSets::ReadOnlyBuffers] !=
            &vulkanSet->mWriteDescSets[0] )
        {
            mComputeTable.bakedDescriptorSets[BakedDescriptorSets::ReadOnlyBuffers] =
                &vulkanSet->mWriteDescSets[0];
            mComputeTable.bakedDescriptorSets[BakedDescriptorSets::TexBuffers] =
                &vulkanSet->mWriteDescSets[1];
            mComputeTable.bakedDescriptorSets[BakedDescriptorSets::Textures] =
                &vulkanSet->mWriteDescSets[2];
            mComputeTable.dirtyBakedSamplers = true;
            mComputeTableDirty = true;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setSamplersCS( uint32 slotStart, const DescriptorSetSampler *set )
    {
        VulkanDescriptorSetSampler *vulkanSet =
            reinterpret_cast<VulkanDescriptorSetSampler *>( set->mRsData );

        if( mComputeTable.bakedDescriptorSets[BakedDescriptorSets::Samplers] !=
            &vulkanSet->mWriteDescSet )
        {
            mComputeTable.bakedDescriptorSets[BakedDescriptorSets::Samplers] = &vulkanSet->mWriteDescSet;
            mComputeTable.dirtyBakedSamplers = true;
            mComputeTableDirty = true;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setUavCS( uint32 slotStart, const DescriptorSetUav *set )
    {
        VulkanDescriptorSetUav *vulkanSet = reinterpret_cast<VulkanDescriptorSetUav *>( set->mRsData );

        if( mComputeTable.bakedDescriptorSets[BakedDescriptorSets::UavBuffers] !=
            &vulkanSet->mWriteDescSets[0] )
        {
            mComputeTable.bakedDescriptorSets[BakedDescriptorSets::UavBuffers] =
                &vulkanSet->mWriteDescSets[0];
            mComputeTable.bakedDescriptorSets[BakedDescriptorSets::UavTextures] =
                &vulkanSet->mWriteDescSets[1];
            mComputeTable.dirtyBakedUavs = true;
            mComputeTableDirty = true;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setTextureCoordCalculation( size_t unit, TexCoordCalcMethod m,
                                                          const Frustum *frustum )
    {
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setTextureBlendMode( size_t unit, const LayerBlendModeEx &bm ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setTextureMatrix( size_t unit, const Matrix4 &xform ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setIndirectBuffer( IndirectBufferPacked *indirectBuffer )
    {
        if( mVaoManager->supportsIndirectBuffers() )
        {
            if( indirectBuffer )
            {
                VulkanBufferInterface *bufferInterface =
                    static_cast<VulkanBufferInterface *>( indirectBuffer->getBufferInterface() );
                mIndirectBuffer = bufferInterface->getVboName();
            }
            else
            {
                mIndirectBuffer = 0;
            }
        }
        else
        {
            if( indirectBuffer )
                mSwIndirectBufferPtr = indirectBuffer->getSwBufferPtr();
            else
                mSwIndirectBufferPtr = 0;
        }
    }
    //-------------------------------------------------------------------------
    RenderPassDescriptor *VulkanRenderSystem::createRenderPassDescriptor()
    {
        VulkanRenderPassDescriptor *retVal =
            OGRE_NEW VulkanRenderPassDescriptor( &mDevice->mGraphicsQueue, this );
        mRenderPassDescs.insert( retVal );
        return retVal;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_hlmsComputePipelineStateObjectCreated( HlmsComputePso *newPso )
    {
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        debugLogPso( newPso );
#endif

        VkComputePipelineCreateInfo computeInfo;
        makeVkStruct( computeInfo, VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO );

        VulkanProgram *computeShader =
            static_cast<VulkanProgram *>( newPso->computeShader->_getBindingDelegate() );
        computeShader->fillPipelineShaderStageCi( computeInfo.stage );

        VulkanRootLayout *rootLayout = computeShader->getRootLayout();
        computeInfo.layout = rootLayout->createVulkanHandles();

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
        mValidationError = false;
#endif

        VkPipeline vulkanPso = 0u;
        VkResult result = vkCreateComputePipelines( mDevice->mDevice, mDevice->mPipelineCache, 1u,
                                                    &computeInfo, 0, &vulkanPso );
        checkVkResult( mDevice, result, "vkCreateComputePipelines" );

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        if( mValidationError )
        {
            LogManager::getSingleton().logMessage( "Validation error:" );

            if( newPso->computeShader )
            {
                VulkanProgram *shader =
                    static_cast<VulkanProgram *>( newPso->computeShader->_getBindingDelegate() );

                String debugDump;
                shader->debugDump( debugDump );
                LogManager::getSingleton().logMessage( debugDump );
            }
        }
#endif

        VulkanHlmsPso *pso = new VulkanHlmsPso( vulkanPso, rootLayout );
        newPso->rsData = pso;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_hlmsComputePipelineStateObjectDestroyed( HlmsComputePso *pso )
    {
        if( pso == mComputePso )
        {
            mComputePso = 0;
            mComputeTable.setAllDirty();
            mComputeTableDirty = true;
        }

        OGRE_ASSERT_LOW( pso->rsData );
        VulkanHlmsPso *vulkanPso = static_cast<VulkanHlmsPso *>( pso->rsData );
        delayed_vkDestroyPipeline( mVaoManager, mDevice->mDevice, vulkanPso->pso, 0 );
        delete vulkanPso;
        pso->rsData = 0;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_beginFrame() {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_endFrame() {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_notifyActiveEncoderEnded( bool callEndRenderPassDesc )
    {
        // VulkanQueue::endRenderEncoder gets called either because
        //  * Another encoder was required. Thus we interrupted and callEndRenderPassDesc = true
        //  * endRenderPassDescriptor called us. Thus callEndRenderPassDesc = false
        //  * executeRenderPassDescriptorDelayedActions called us. Thus callEndRenderPassDesc = false
        // In all cases, when callEndRenderPassDesc = true, it also implies rendering was
        // interrupted.
        if( callEndRenderPassDesc )
            endRenderPassDescriptor( true );

        mUavRenderingDirty = true;
        mGlobalTable.setAllDirty();
        mTableDirty = true;
        mPso = 0;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_notifyActiveComputeEnded()
    {
        mComputePso = 0;
        mComputeTable.setAllDirty();
        mComputeTableDirty = true;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_endFrameOnce()
    {
        RenderSystem::_endFrameOnce();
        endRenderPassDescriptor( false );
        mDevice->commitAndNextCommandBuffer( SubmissionType::EndFrameAndSwap );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *samplerblock )
    {
        OGRE_ASSERT_MEDIUM( texUnit < NUM_BIND_SAMPLERS );
        if( !samplerblock )
        {
            if( mGlobalTable.samplers[texUnit].sampler != mDummySampler )
            {
                mGlobalTable.samplers[texUnit].sampler = mDummySampler;
                mGlobalTable.minDirtySlotSamplers =
                    std::min( mGlobalTable.minDirtySlotSamplers, texUnit );
                mTableDirty = true;
            }
        }
        else
        {
#if OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_64
            VkSampler textureSampler = static_cast<VkSampler>( samplerblock->mRsData );
#else  // VK handles are always 64bit, even on 32bit systems
            VkSampler textureSampler = *static_cast<VkSampler *>( samplerblock->mRsData );
#endif
            if( mGlobalTable.samplers[texUnit].sampler != textureSampler )
            {
                mGlobalTable.samplers[texUnit].sampler = textureSampler;
                mGlobalTable.minDirtySlotSamplers =
                    std::min( mGlobalTable.minDirtySlotSamplers, texUnit );
                mTableDirty = true;
            }
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setPipelineStateObject( const HlmsPso *pso )
    {
        if( pso && mDevice->mGraphicsQueue.getEncoderState() != VulkanQueue::EncoderGraphicsOpen )
        {
            OGRE_ASSERT_LOW(
                mInterruptedRenderCommandEncoder &&
                "Encoder can't be in EncoderGraphicsOpen at this stage if rendering was interrupted."
                " Did you call executeRenderPassDescriptorDelayedActions?" );
            executeRenderPassDescriptorDelayedActions( false );
        }

        // check, if we deferred pipeline compilation due to the skipped deadline
        // TODO: set some fallback? Dummy here, or more clever at compilation site?
        if( pso && !pso->rsData )
            pso = 0;

        if( mPso != pso )
        {
            if( pso )
            {
                VulkanRootLayout *oldRootLayout = 0;
                if( mPso )
                    oldRootLayout = reinterpret_cast<VulkanHlmsPso *>( mPso->rsData )->rootLayout;

                OGRE_ASSERT_LOW( pso->rsData );
                VulkanHlmsPso *vulkanPso = reinterpret_cast<VulkanHlmsPso *>( pso->rsData );
                VkCommandBuffer cmdBuffer = mDevice->mGraphicsQueue.getCurrentCmdBuffer();
                vkCmdBindPipeline( cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPso->pso );

                if( vulkanPso->rootLayout != oldRootLayout )
                {
                    mGlobalTable.setAllDirty();
                    mTableDirty = true;
                }
            }

            mPso = pso;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setComputePso( const HlmsComputePso *pso )
    {
        mDevice->mGraphicsQueue.getComputeEncoder();

        if( mComputePso != pso )
        {
            if( pso )
            {
                VulkanRootLayout *oldRootLayout = 0;
                if( mComputePso )
                    oldRootLayout = reinterpret_cast<VulkanHlmsPso *>( mComputePso->rsData )->rootLayout;

                OGRE_ASSERT_LOW( pso->rsData );
                VulkanHlmsPso *vulkanPso = reinterpret_cast<VulkanHlmsPso *>( pso->rsData );
                VkCommandBuffer cmdBuffer = mDevice->mGraphicsQueue.getCurrentCmdBuffer();
                vkCmdBindPipeline( cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanPso->pso );

                if( vulkanPso->rootLayout != oldRootLayout )
                {
                    mComputeTable.setAllDirty();
                    mComputeTableDirty = true;
                }
            }

            mComputePso = pso;
        }
    }
    //-------------------------------------------------------------------------
    VertexElementType VulkanRenderSystem::getColourVertexElementType() const { return VET_COLOUR_ABGR; }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_dispatch( const HlmsComputePso &pso )
    {
        flushRootLayoutCS();

        vkCmdDispatch( mDevice->mGraphicsQueue.getCurrentCmdBuffer(), pso.mNumThreadGroups[0],
                       pso.mNumThreadGroups[1], pso.mNumThreadGroups[2] );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setVertexArrayObject( const VertexArrayObject *vao )
    {
        VkBuffer vulkanVertexBuffers[15];
        VkDeviceSize offsets[15];
        memset( offsets, 0, sizeof( offsets ) );

        const VertexBufferPackedVec &vertexBuffers = vao->getVertexBuffers();

        size_t numVertexBuffers = 0;
        VertexBufferPackedVec::const_iterator itor = vertexBuffers.begin();
        VertexBufferPackedVec::const_iterator endt = vertexBuffers.end();

        while( itor != endt )
        {
            VulkanBufferInterface *bufferInterface =
                static_cast<VulkanBufferInterface *>( ( *itor )->getBufferInterface() );
            vulkanVertexBuffers[numVertexBuffers++] = bufferInterface->getVboName();
            ++itor;
        }

        OGRE_ASSERT_LOW( numVertexBuffers < 15u );

        VkCommandBuffer cmdBuffer = mDevice->mGraphicsQueue.getCurrentCmdBuffer();
        if( numVertexBuffers > 0u )
        {
            vkCmdBindVertexBuffers( cmdBuffer, 0, static_cast<uint32>( numVertexBuffers ),
                                    vulkanVertexBuffers, offsets );
        }

        IndexBufferPacked *indexBuffer = vao->getIndexBuffer();
        if( indexBuffer )
        {
            VulkanBufferInterface *bufferInterface =
                static_cast<VulkanBufferInterface *>( indexBuffer->getBufferInterface() );
            vkCmdBindIndexBuffer( cmdBuffer, bufferInterface->getVboName(), 0,
                                  static_cast<VkIndexType>( indexBuffer->getIndexType() ) );
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::flushRootLayout()
    {
        if( !mTableDirty )
            return;

        VulkanVaoManager *vaoManager = static_cast<VulkanVaoManager *>( mVaoManager );
        VulkanRootLayout *rootLayout = reinterpret_cast<VulkanHlmsPso *>( mPso->rsData )->rootLayout;
        rootLayout->bind( mDevice, vaoManager, mGlobalTable );
        mGlobalTable.reset();
        mTableDirty = false;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::flushRootLayoutCS()
    {
        if( !mComputeTableDirty )
            return;

        VulkanVaoManager *vaoManager = static_cast<VulkanVaoManager *>( mVaoManager );
        VulkanRootLayout *rootLayout =
            reinterpret_cast<VulkanHlmsPso *>( mComputePso->rsData )->rootLayout;
        rootLayout->bind( mDevice, vaoManager, mComputeTable );
        mComputeTable.reset();
        mComputeTableDirty = false;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_render( const CbDrawCallIndexed *cmd )
    {
        // check, if we deferred pipeline compilation due to the skipped deadline
        if( mPso == nullptr )
            return;

        flushRootLayout();

        VkCommandBuffer cmdBuffer = mDevice->mGraphicsQueue.getCurrentCmdBuffer();
        vkCmdDrawIndexedIndirect( cmdBuffer, mIndirectBuffer,
                                  reinterpret_cast<VkDeviceSize>( cmd->indirectBufferOffset ),
                                  cmd->numDraws, sizeof( CbDrawIndexed ) );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_render( const CbDrawCallStrip *cmd )
    {
        // check, if we deferred pipeline compilation due to the skipped deadline
        if( mPso == nullptr )
            return;

        flushRootLayout();

        VkCommandBuffer cmdBuffer = mDevice->mGraphicsQueue.getCurrentCmdBuffer();
        vkCmdDrawIndirect( cmdBuffer, mIndirectBuffer,
                           reinterpret_cast<VkDeviceSize>( cmd->indirectBufferOffset ), cmd->numDraws,
                           sizeof( CbDrawStrip ) );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_renderEmulated( const CbDrawCallIndexed *cmd )
    {
        // check, if we deferred pipeline compilation due to the skipped deadline
        if( mPso == nullptr )
            return;

        flushRootLayout();

        CbDrawIndexed *drawCmd = reinterpret_cast<CbDrawIndexed *>( mSwIndirectBufferPtr +
                                                                    (size_t)cmd->indirectBufferOffset );

        VkCommandBuffer cmdBuffer = mDevice->mGraphicsQueue.getCurrentCmdBuffer();

        for( uint32 i = cmd->numDraws; i--; )
        {
            vkCmdDrawIndexed( cmdBuffer, drawCmd->primCount, drawCmd->instanceCount,
                              drawCmd->firstVertexIndex, static_cast<int32>( drawCmd->baseVertex ),
                              drawCmd->baseInstance );
            ++drawCmd;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_renderEmulated( const CbDrawCallStrip *cmd )
    {
        // check, if we deferred pipeline compilation due to the skipped deadline
        if( mPso == nullptr )
            return;

        flushRootLayout();

        CbDrawStrip *drawCmd =
            reinterpret_cast<CbDrawStrip *>( mSwIndirectBufferPtr + (size_t)cmd->indirectBufferOffset );

        VkCommandBuffer cmdBuffer = mDevice->mGraphicsQueue.getCurrentCmdBuffer();

        for( uint32 i = cmd->numDraws; i--; )
        {
            vkCmdDraw( cmdBuffer, drawCmd->primCount, drawCmd->instanceCount, drawCmd->firstVertexIndex,
                       drawCmd->baseInstance );
            ++drawCmd;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setRenderOperation( const v1::CbRenderOp *cmd )
    {
        VulkanVaoManager *vaoManager = static_cast<VulkanVaoManager *>( mVaoManager );

        VkCommandBuffer cmdBuffer = mDevice->mGraphicsQueue.getCurrentCmdBuffer();

        VkBuffer vulkanVertexBuffers[16];
        VkDeviceSize offsets[16];
        memset( vulkanVertexBuffers, 0, sizeof( vulkanVertexBuffers ) );
        memset( offsets, 0, sizeof( offsets ) );

        size_t maxUsedSlot = 0;
        const v1::VertexBufferBinding::VertexBufferBindingMap &binds =
            cmd->vertexData->vertexBufferBinding->getBindings();
        v1::VertexBufferBinding::VertexBufferBindingMap::const_iterator itor = binds.begin();
        v1::VertexBufferBinding::VertexBufferBindingMap::const_iterator end = binds.end();

        while( itor != end )
        {
            v1::VulkanHardwareVertexBuffer *vulkanBuffer =
                reinterpret_cast<v1::VulkanHardwareVertexBuffer *>( itor->second.get() );

            const size_t slot = itor->first;

            OGRE_ASSERT_LOW( slot < 15u );

            size_t offsetStart;
            vulkanVertexBuffers[slot] = vulkanBuffer->getBufferName( offsetStart );
            offsets[slot] = offsetStart;
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            offsets[slot] += cmd->vertexData->vertexStart * vulkanBuffer->getVertexSize();
#endif
            ++itor;
            maxUsedSlot = std::max( maxUsedSlot, slot + 1u );
        }

        VulkanBufferInterface *bufIntf =
            static_cast<VulkanBufferInterface *>( vaoManager->getDrawId()->getBufferInterface() );
        vulkanVertexBuffers[maxUsedSlot] = bufIntf->getVboName();
        offsets[maxUsedSlot] = 0;

        vkCmdBindVertexBuffers( cmdBuffer, 0u, (uint32)maxUsedSlot + 1u, vulkanVertexBuffers, offsets );

        if( cmd->indexData )
        {
            size_t offsetStart = 0u;

            v1::VulkanHardwareIndexBuffer *vulkanBuffer =
                static_cast<v1::VulkanHardwareIndexBuffer *>( cmd->indexData->indexBuffer.get() );
            VkIndexType indexType = static_cast<VkIndexType>( vulkanBuffer->getType() );
            VkBuffer indexBuffer = vulkanBuffer->getBufferName( offsetStart );

            vkCmdBindIndexBuffer( cmdBuffer, indexBuffer, offsetStart, indexType );
        }

        mCurrentIndexBuffer = cmd->indexData;
        mCurrentVertexBuffer = cmd->vertexData;
        mCurrentPrimType = std::min( VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
                                     static_cast<VkPrimitiveTopology>( cmd->operationType - 1u ) );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_render( const v1::CbDrawCallIndexed *cmd )
    {
        // check, if we deferred pipeline compilation due to the skipped deadline
        if( mPso == nullptr )
            return;

        flushRootLayout();

        VkCommandBuffer cmdBuffer = mDevice->mGraphicsQueue.getCurrentCmdBuffer();
        vkCmdDrawIndexed( cmdBuffer, cmd->primCount, cmd->instanceCount, cmd->firstVertexIndex,
                          (int32_t)mCurrentVertexBuffer->vertexStart, cmd->baseInstance );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_render( const v1::CbDrawCallStrip *cmd )
    {
        // check, if we deferred pipeline compilation due to the skipped deadline
        if( mPso == nullptr )
            return;

        flushRootLayout();

        VkCommandBuffer cmdBuffer = mDevice->mGraphicsQueue.getCurrentCmdBuffer();
        vkCmdDraw( cmdBuffer, cmd->primCount, cmd->instanceCount, cmd->firstVertexIndex,
                   cmd->baseInstance );
    }

    void VulkanRenderSystem::_render( const v1::RenderOperation &op )
    {
        // check, if we deferred pipeline compilation due to the skipped deadline
        if( mPso == nullptr )
            return;

        flushRootLayout();

        // Call super class.
        RenderSystem::_render( op );

        const size_t numberOfInstances = op.numberOfInstances;

        VkCommandBuffer cmdBuffer = mDevice->mGraphicsQueue.getCurrentCmdBuffer();

        // Render to screen!
        if( op.useIndexes )
        {
            do
            {
                // Update derived depth bias.
                if( mDerivedDepthBias && mCurrentPassIterationNum > 0 )
                {
                    const float biasSign = mReverseDepth ? 1.0f : -1.0f;
                    vkCmdSetDepthBias(
                        cmdBuffer,
                        ( mDerivedDepthBiasBase +
                          mDerivedDepthBiasMultiplier * float( mCurrentPassIterationNum ) ) *
                            biasSign,
                        0.f, mDerivedDepthBiasSlopeScale * biasSign );
                }

                vkCmdDrawIndexed( cmdBuffer, (uint32)mCurrentIndexBuffer->indexCount,
                                  (uint32)numberOfInstances, (uint32)mCurrentIndexBuffer->indexStart,
                                  (int32)mCurrentVertexBuffer->vertexStart, 0u );
            } while( updatePassIterationRenderState() );
        }
        else
        {
            do
            {
                // Update derived depth bias.
                if( mDerivedDepthBias && mCurrentPassIterationNum > 0 )
                {
                    const float biasSign = mReverseDepth ? 1.0f : -1.0f;
                    vkCmdSetDepthBias(
                        cmdBuffer,
                        ( mDerivedDepthBiasBase +
                          mDerivedDepthBiasMultiplier * float( mCurrentPassIterationNum ) ) *
                            biasSign,
                        0.0f, mDerivedDepthBiasSlopeScale * biasSign );
                }

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
                const uint32 vertexStart = 0;
#else
                const uint32 vertexStart = static_cast<uint32>( mCurrentVertexBuffer->vertexStart );
#endif

                vkCmdDraw( cmdBuffer, (uint32)mCurrentVertexBuffer->vertexCount,
                           (uint32)numberOfInstances, vertexStart, 0u );
            } while( updatePassIterationRenderState() );
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::bindGpuProgramParameters( GpuProgramType gptype,
                                                       GpuProgramParametersSharedPtr params,
                                                       uint16 variabilityMask )
    {
        VulkanProgram *shader = 0;
        switch( gptype )
        {
        case GPT_VERTEX_PROGRAM:
            mActiveVertexGpuProgramParameters = params;
            shader = static_cast<VulkanProgram *>( mPso->vertexShader->_getBindingDelegate() );
            break;
        case GPT_FRAGMENT_PROGRAM:
            mActiveFragmentGpuProgramParameters = params;
            shader = static_cast<VulkanProgram *>( mPso->pixelShader->_getBindingDelegate() );
            break;
        case GPT_GEOMETRY_PROGRAM:
            mActiveGeometryGpuProgramParameters = params;
            shader = static_cast<VulkanProgram *>( mPso->geometryShader->_getBindingDelegate() );
            break;
        case GPT_HULL_PROGRAM:
            mActiveTessellationHullGpuProgramParameters = params;
            shader = static_cast<VulkanProgram *>( mPso->tesselationHullShader->_getBindingDelegate() );
            break;
        case GPT_DOMAIN_PROGRAM:
            mActiveTessellationDomainGpuProgramParameters = params;
            shader =
                static_cast<VulkanProgram *>( mPso->tesselationDomainShader->_getBindingDelegate() );
            break;
        case GPT_COMPUTE_PROGRAM:
            mActiveComputeGpuProgramParameters = params;
            shader = static_cast<VulkanProgram *>( mComputePso->computeShader->_getBindingDelegate() );
            break;
        }

        size_t bytesToWrite = shader ? shader->getBufferRequiredSize() : 0;
        if( shader && bytesToWrite > 0 )
        {
            OGRE_ASSERT_LOW(
                mCurrentAutoParamsBufferSpaceLeft % mVaoManager->getConstBufferAlignment() == 0 );

            size_t bytesToWriteAligned =
                alignToNextMultiple<size_t>( bytesToWrite, mVaoManager->getConstBufferAlignment() );
            if( mCurrentAutoParamsBufferSpaceLeft < bytesToWriteAligned )
            {
                if( mAutoParamsBufferIdx >= mAutoParamsBuffer.size() )
                {
                    // Ask for a coherent buffer to avoid excessive flushing. Note: VaoManager may ignore
                    // this request if the GPU can't provide coherent memory and we must flush anyway.
                    ConstBufferPacked *constBuffer = mVaoManager->createConstBuffer(
                        std::max<size_t>( 512u * 1024u, bytesToWriteAligned ),
                        BT_DYNAMIC_PERSISTENT_COHERENT, 0, false );
                    mAutoParamsBuffer.push_back( constBuffer );
                }

                ConstBufferPacked *constBuffer = mAutoParamsBuffer[mAutoParamsBufferIdx];

                // This should be near-impossible to trigger because most Const Buffers are <= 64kb
                // and we reserver 512kb per const buffer. A Low Level Material using a Params buffer
                // with > 64kb is an edge case we don't care handling.
                OGRE_ASSERT_LOW( bytesToWriteAligned <= constBuffer->getTotalSizeBytes() );

                mCurrentAutoParamsBufferPtr =
                    reinterpret_cast<uint8 *>( constBuffer->map( 0, constBuffer->getNumElements() ) );
                mCurrentAutoParamsBufferSpaceLeft = constBuffer->getTotalSizeBytes();

                ++mAutoParamsBufferIdx;
            }

            shader->updateBuffers( params, mCurrentAutoParamsBufferPtr );

            OGRE_ASSERT_HIGH( dynamic_cast<VulkanConstBufferPacked *>(
                mAutoParamsBuffer[mAutoParamsBufferIdx - 1u] ) );

            VulkanConstBufferPacked *constBuffer =
                static_cast<VulkanConstBufferPacked *>( mAutoParamsBuffer[mAutoParamsBufferIdx - 1u] );
            const size_t bindOffset =
                constBuffer->getTotalSizeBytes() - mCurrentAutoParamsBufferSpaceLeft;

            constBuffer->bindAsParamBuffer( gptype, bindOffset, bytesToWrite );

            mCurrentAutoParamsBufferPtr += bytesToWriteAligned;
            mCurrentAutoParamsBufferSpaceLeft -= bytesToWriteAligned;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::flushBoundGpuProgramParameters(
        const SubmissionType::SubmissionType submissionType )
    {
        bool bWillReuseLastBuffer = false;

        const size_t maxBufferToFlush = mAutoParamsBufferIdx;
        for( size_t i = mFirstUnflushedAutoParamsBuffer; i < maxBufferToFlush; ++i )
        {
            ConstBufferPacked *constBuffer = mAutoParamsBuffer[i];
            if( i + 1u != maxBufferToFlush )
            {
                // Flush whole buffer.
                constBuffer->unmap( UO_KEEP_PERSISTENT );
            }
            else
            {
                // Last buffer. Partial flush.
                const size_t bytesToFlush =
                    constBuffer->getTotalSizeBytes() - mCurrentAutoParamsBufferSpaceLeft;

                constBuffer->unmap( UO_KEEP_PERSISTENT, 0u, bytesToFlush );
                if( submissionType <= SubmissionType::FlushOnly &&
                    mCurrentAutoParamsBufferSpaceLeft >= 4u )
                {
                    // Map again so we can continue from where we left off.

                    // If the assert triggers then getNumElements is not in bytes and our math is wrong.
                    OGRE_ASSERT_LOW( constBuffer->getBytesPerElement() == 1u );
                    constBuffer->regressFrame();
                    mCurrentAutoParamsBufferPtr = reinterpret_cast<uint8 *>(
                        constBuffer->map( bytesToFlush, constBuffer->getNumElements() - bytesToFlush ) );
                    mCurrentAutoParamsBufferSpaceLeft = constBuffer->getNumElements() - bytesToFlush;
                    bWillReuseLastBuffer = true;
                }
                else
                {
                    mCurrentAutoParamsBufferSpaceLeft = 0u;
                    mCurrentAutoParamsBufferPtr = 0;
                }
            }
        }

        if( submissionType >= SubmissionType::NewFrameIdx )
        {
            mAutoParamsBufferIdx = 0u;
            mFirstUnflushedAutoParamsBuffer = 0u;
        }
        else
        {
            // If maxBufferToFlush == 0 then bindGpuProgramParameters() was never called this round
            // and bWillReuseLastBuffer can't be true.
            if( bWillReuseLastBuffer )
                mFirstUnflushedAutoParamsBuffer = maxBufferToFlush - 1u;
            else
                mFirstUnflushedAutoParamsBuffer = maxBufferToFlush;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::flushPendingNonCoherentFlushes(
        const SubmissionType::SubmissionType submissionType )
    {
        flushBoundGpuProgramParameters( submissionType );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::bindGpuProgramPassIterationParameters( GpuProgramType gptype ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::clearFrameBuffer( RenderPassDescriptor *renderPassDesc,
                                               TextureGpu *anyTarget, uint8 mipLevel )
    {
        Vector4 fullVp( 0, 0, 1, 1 );
        beginRenderPassDescriptor( renderPassDesc, anyTarget, mipLevel,  //
                                   &fullVp, &fullVp, 1u, false, false );
        executeRenderPassDescriptorDelayedActions();
    }
    //-------------------------------------------------------------------------
    Real VulkanRenderSystem::getHorizontalTexelOffset() { return 0.0f; }
    //-------------------------------------------------------------------------
    Real VulkanRenderSystem::getVerticalTexelOffset() { return 0.0f; }
    //-------------------------------------------------------------------------
    Real VulkanRenderSystem::getMinimumDepthInputValue() { return 0.0f; }
    //-------------------------------------------------------------------------
    Real VulkanRenderSystem::getMaximumDepthInputValue() { return 1.0f; }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::preExtraThreadsStarted() {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::postExtraThreadsStarted() {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::registerThread() {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::unregisterThread() {}
    //-------------------------------------------------------------------------
    const PixelFormatToShaderType *VulkanRenderSystem::getPixelFormatToShaderType() const
    {
        return &mPixelFormatToShaderType;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::flushCommands()
    {
        mDevice->commitAndNextCommandBuffer( SubmissionType::FlushOnly );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::beginProfileEvent( const String &eventName ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::endProfileEvent() {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::markProfileEvent( const String &event ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::debugAnnotationPush( const String &event )
    {
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        if( !mInstance->CmdBeginDebugUtilsLabelEXT )
            return;  // VK_EXT_debug_utils not available
        VkCommandBuffer cmdBuffer = mDevice->mGraphicsQueue.getCurrentCmdBuffer();
        VkDebugUtilsLabelEXT markerInfo;
        makeVkStruct( markerInfo, VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT );
        markerInfo.pLabelName = event.c_str();
        mInstance->CmdBeginDebugUtilsLabelEXT( cmdBuffer, &markerInfo );
#endif
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::debugAnnotationPop()
    {
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        if( !mInstance->CmdEndDebugUtilsLabelEXT )
            return;  // VK_EXT_debug_utils not available
        VkCommandBuffer cmdBuffer = mDevice->mGraphicsQueue.getCurrentCmdBuffer();
        mInstance->CmdEndDebugUtilsLabelEXT( cmdBuffer );
#endif
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::initGPUProfiling() {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::deinitGPUProfiling() {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::beginGPUSampleProfile( const String &name, uint32 *hashCache ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::endGPUSampleProfile( const String &name ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::endGpuDebuggerFrameCapture( Window *window, const bool bDiscard )
    {
        if( mRenderDocApi && !bDiscard )
            mDevice->commitAndNextCommandBuffer( SubmissionType::FlushOnly );
        RenderSystem::endGpuDebuggerFrameCapture( window, bDiscard );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::getCustomAttribute( const String &name, void *pData )
    {
        if( name == "VkInstance" )
        {
            *(VkInstance *)pData = mDevice->mInstance->mVkInstance;
            return;
        }
        else if( name == "VkPhysicalDevice" )
        {
            *(VkPhysicalDevice *)pData = mDevice->mPhysicalDevice;
            return;
        }
        else if( name == "VulkanDevice" )
        {
            *(VulkanDevice **)pData = mDevice;
            return;
        }
        else if( name == "VkDevice" )
        {
            *(VkDevice *)pData = mDevice->mDevice;
            return;
        }
        else if( name == "mPresentQueue" )
        {
            *(VkQueue *)pData = mDevice->mPresentQueue;
            return;
        }
        else if( name == "VulkanQueue" )
        {
            *(VulkanQueue **)pData = &mDevice->mGraphicsQueue;
            return;
        }

        OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Attribute not found: " + name,
                     "VulkanRenderSystem::getCustomAttribute" );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::setClipPlanesImpl( const PlaneList &clipPlanes ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::initialiseFromRenderSystemCapabilities( RenderSystemCapabilities *caps,
                                                                     Window *primary )
    {
        mShaderManager = OGRE_NEW VulkanGpuProgramManager( mDevice );
        mVulkanProgramFactory0 = OGRE_NEW VulkanProgramFactory( mDevice, "glslvk", true );
        mVulkanProgramFactory1 = OGRE_NEW VulkanProgramFactory( mDevice, "glsl", false );
        mVulkanProgramFactory2 = OGRE_NEW VulkanProgramFactory( mDevice, "hlslvk", false );
        mVulkanProgramFactory3 = OGRE_NEW VulkanProgramFactory( mDevice, "hlsl", false );

        HighLevelGpuProgramManager::getSingleton().addFactory( mVulkanProgramFactory0 );
        // HighLevelGpuProgramManager::getSingleton().addFactory( mVulkanProgramFactory1 );
        HighLevelGpuProgramManager::getSingleton().addFactory( mVulkanProgramFactory2 );
        // HighLevelGpuProgramManager::getSingleton().addFactory( mVulkanProgramFactory3 );

        mCache = OGRE_NEW VulkanCache( mDevice );

        Log *defaultLog = LogManager::getSingleton().getDefaultLog();
        if( defaultLog )
        {
            caps->log( defaultLog );
            defaultLog->logMessage( " * Using Reverse Z: " +
                                    StringConverter::toString( mReverseDepth, true ) );
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::beginRenderPassDescriptor( RenderPassDescriptor *desc,
                                                        TextureGpu *anyTarget, uint8 mipLevel,
                                                        const Vector4 *viewportSizes,
                                                        const Vector4 *scissors, uint32 numViewports,
                                                        bool overlaysEnabled, bool warnIfRtvWasFlushed )
    {
        if( desc->mInformationOnly && desc->hasSameAttachments( mCurrentRenderPassDescriptor ) )
            return;

        const bool mDebugSubmissionValidationLayers = false;
        if( mDebugSubmissionValidationLayers )
        {
            // If we get a validation layer here; then the error was generated by the
            // Pass that last called beginRenderPassDescriptor (i.e. not this one)
            endRenderPassDescriptor( false );
            mDevice->commitAndNextCommandBuffer( SubmissionType::FlushOnly );
        }

        const int oldWidth = mCurrentRenderViewport[0].getActualWidth();
        const int oldHeight = mCurrentRenderViewport[0].getActualHeight();
        const int oldX = mCurrentRenderViewport[0].getActualLeft();
        const int oldY = mCurrentRenderViewport[0].getActualTop();
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        const OrientationMode oldOrientationMode =
            mCurrentRenderViewport[0].getCurrentTarget()
                ? mCurrentRenderViewport[0].getCurrentTarget()->getOrientationMode()
                : OR_DEGREE_0;
#endif

        VulkanRenderPassDescriptor *currPassDesc =
            static_cast<VulkanRenderPassDescriptor *>( mCurrentRenderPassDescriptor );

        RenderSystem::beginRenderPassDescriptor( desc, anyTarget, mipLevel, viewportSizes, scissors,
                                                 numViewports, overlaysEnabled, warnIfRtvWasFlushed );

        // Calculate the new "lower-left" corner of the viewport to compare with the old one
        const int w = mCurrentRenderViewport[0].getActualWidth();
        const int h = mCurrentRenderViewport[0].getActualHeight();
        const int x = mCurrentRenderViewport[0].getActualLeft();
        const int y = mCurrentRenderViewport[0].getActualTop();
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        const OrientationMode orientationMode =
            mCurrentRenderViewport[0].getCurrentTarget()->getOrientationMode();
#endif

        const bool vpChanged = oldX != x || oldY != y || oldWidth != w || oldHeight != h ||
                               numViewports > 1u
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
                               || oldOrientationMode != orientationMode
#endif
            ;

        VulkanRenderPassDescriptor *newPassDesc = static_cast<VulkanRenderPassDescriptor *>( desc );

        // Determine whether:
        //  1. We need to store current active RenderPassDescriptor
        //  2. We need to perform clears when loading the new RenderPassDescriptor
        uint32 entriesToFlush = 0;
        if( currPassDesc )
        {
            entriesToFlush = currPassDesc->willSwitchTo( newPassDesc, warnIfRtvWasFlushed );

            if( entriesToFlush != 0 )
                currPassDesc->performStoreActions( false );

            // If rendering was interrupted but we're still rendering to the same
            // RTT, willSwitchTo will have returned 0 and thus we won't perform
            // the necessary load actions.
            // If RTTs were different, we need to have performStoreActions
            // called by now (i.e. to emulate StoreAndResolve)
            if( mInterruptedRenderCommandEncoder )
            {
                entriesToFlush = RenderPassDescriptor::All;
                if( warnIfRtvWasFlushed )
                    newPassDesc->checkWarnIfRtvWasFlushed( entriesToFlush );
            }
        }
        else
        {
            entriesToFlush = RenderPassDescriptor::All;
        }

        mEntriesToFlush = entriesToFlush;
        mVpChanged = vpChanged;
        mInterruptedRenderCommandEncoder = false;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::executeRenderPassDescriptorDelayedActions()
    {
        executeRenderPassDescriptorDelayedActions( true );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::executeRenderPassDescriptorDelayedActions( bool officialCall )
    {
        if( officialCall )
            mInterruptedRenderCommandEncoder = false;

        const bool wasGraphicsOpen =
            mDevice->mGraphicsQueue.getEncoderState() != VulkanQueue::EncoderGraphicsOpen;

        if( mEntriesToFlush )
        {
            mDevice->mGraphicsQueue.endAllEncoders( false );

            VulkanRenderPassDescriptor *newPassDesc =
                static_cast<VulkanRenderPassDescriptor *>( mCurrentRenderPassDescriptor );

            newPassDesc->performLoadActions( mInterruptedRenderCommandEncoder );
        }

        // This is a new command buffer / encoder. State needs to be set again
        if( mEntriesToFlush || !wasGraphicsOpen )
        {
            mDevice->mGraphicsQueue.getGraphicsEncoder();

            VulkanVaoManager *vaoManager = static_cast<VulkanVaoManager *>( mVaoManager );
            vaoManager->bindDrawIdVertexBuffer( mDevice->mGraphicsQueue.getCurrentCmdBuffer() );

            if( mStencilEnabled )
            {
                vkCmdSetStencilReference( mDevice->mGraphicsQueue.getCurrentCmdBuffer(),
                                          VK_STENCIL_FACE_FRONT_AND_BACK, mStencilRefValue );
            }

            mVpChanged = true;
            mInterruptedRenderCommandEncoder = false;
        }

        flushUAVs();

        const uint32 numViewports = mMaxBoundViewports;

        // If we flushed, viewport and scissor settings got reset.
        if( mVpChanged || numViewports > 1u )
        {
            VkViewport vkVp[16];
            for( size_t i = 0; i < numViewports; ++i )
            {
                vkVp[i].x = (float)mCurrentRenderViewport[i].getActualLeft();
                vkVp[i].y = (float)mCurrentRenderViewport[i].getActualTop();
                vkVp[i].width = (float)mCurrentRenderViewport[i].getActualWidth();
                vkVp[i].height = (float)mCurrentRenderViewport[i].getActualHeight();
                vkVp[i].minDepth = 0;
                vkVp[i].maxDepth = 1;

#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
                if( mCurrentRenderViewport[i].getCurrentTarget()->getOrientationMode() & 0x01 )
                {
                    std::swap( vkVp[i].x, vkVp[i].y );
                    std::swap( vkVp[i].width, vkVp[i].height );
                }
#endif
            }

            vkCmdSetViewport( mDevice->mGraphicsQueue.getCurrentCmdBuffer(), 0u, numViewports, vkVp );
        }

        if( mVpChanged || numViewports > 1u )
        {
            VkRect2D scissorRect[16];
            for( size_t i = 0; i < numViewports; ++i )
            {
                scissorRect[i].offset.x = mCurrentRenderViewport[i].getScissorActualLeft();
                scissorRect[i].offset.y = mCurrentRenderViewport[i].getScissorActualTop();
                scissorRect[i].extent.width =
                    static_cast<uint32>( mCurrentRenderViewport[i].getScissorActualWidth() );
                scissorRect[i].extent.height =
                    static_cast<uint32>( mCurrentRenderViewport[i].getScissorActualHeight() );
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
                if( mCurrentRenderViewport[i].getCurrentTarget()->getOrientationMode() & 0x01 )
                {
                    std::swap( scissorRect[i].offset.x, scissorRect[i].offset.y );
                    std::swap( scissorRect[i].extent.width, scissorRect[i].extent.height );
                }
#endif
            }

            vkCmdSetScissor( mDevice->mGraphicsQueue.getCurrentCmdBuffer(), 0u, numViewports,
                             scissorRect );
        }

        mEntriesToFlush = 0;
        mVpChanged = false;
        mInterruptedRenderCommandEncoder = false;
    }
    //-------------------------------------------------------------------------
    inline void VulkanRenderSystem::endRenderPassDescriptor( bool isInterruptingRender )
    {
        if( mCurrentRenderPassDescriptor )
        {
            VulkanRenderPassDescriptor *passDesc =
                static_cast<VulkanRenderPassDescriptor *>( mCurrentRenderPassDescriptor );
            passDesc->performStoreActions( isInterruptingRender );

            mEntriesToFlush = 0;
            mVpChanged = true;

            mInterruptedRenderCommandEncoder = isInterruptingRender;

            if( !isInterruptingRender )
                RenderSystem::endRenderPassDescriptor();
            else
                mEntriesToFlush = RenderPassDescriptor::All;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::endRenderPassDescriptor() { endRenderPassDescriptor( false ); }
    //-------------------------------------------------------------------------
    TextureGpu *VulkanRenderSystem::createDepthBufferFor( TextureGpu *colourTexture,
                                                          bool preferDepthTexture,
                                                          PixelFormatGpu depthBufferFormat,
                                                          uint16 poolId )
    {
        if( depthBufferFormat == PFG_UNKNOWN )
            depthBufferFormat = DepthBuffer::DefaultDepthBufferFormat;

        return RenderSystem::createDepthBufferFor( colourTexture, preferDepthTexture, depthBufferFormat,
                                                   poolId );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::notifySwapchainCreated( VulkanWindow *window )
    {
        RenderPassDescriptorSet::const_iterator itor = mRenderPassDescs.begin();
        RenderPassDescriptorSet::const_iterator endt = mRenderPassDescs.end();

        while( itor != endt )
        {
            OGRE_ASSERT_HIGH( dynamic_cast<VulkanRenderPassDescriptor *>( *itor ) );
            VulkanRenderPassDescriptor *renderPassDesc =
                static_cast<VulkanRenderPassDescriptor *>( *itor );
            renderPassDesc->notifySwapchainCreated( window );
            ++itor;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::notifySwapchainDestroyed( VulkanWindow *window )
    {
        RenderPassDescriptorSet::const_iterator itor = mRenderPassDescs.begin();
        RenderPassDescriptorSet::const_iterator endt = mRenderPassDescs.end();

        while( itor != endt )
        {
            OGRE_ASSERT_HIGH( dynamic_cast<VulkanRenderPassDescriptor *>( *itor ) );
            VulkanRenderPassDescriptor *renderPassDesc =
                static_cast<VulkanRenderPassDescriptor *>( *itor );
            renderPassDesc->notifySwapchainDestroyed( window );
            ++itor;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::notifyRenderTextureNonResident( VulkanTextureGpu *texture )
    {
        RenderPassDescriptorSet::const_iterator itor = mRenderPassDescs.begin();
        RenderPassDescriptorSet::const_iterator endt = mRenderPassDescs.end();

        while( itor != endt )
        {
            OGRE_ASSERT_HIGH( dynamic_cast<VulkanRenderPassDescriptor *>( *itor ) );
            VulkanRenderPassDescriptor *renderPassDesc =
                static_cast<VulkanRenderPassDescriptor *>( *itor );
            renderPassDesc->notifyRenderTextureNonResident( texture );
            ++itor;
        }
    }
    //-------------------------------------------------------------------------
    VkRenderPass VulkanRenderSystem::getVkRenderPass( HlmsPassPso passPso, uint8 &outMrtCount )
    {
        uint8 mrtCount = 0;
        for( int i = 0; i < OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
        {
            if( passPso.colourFormat[i] != PFG_NULL )
                mrtCount = static_cast<uint8>( i ) + 1u;
        }
        outMrtCount = mrtCount;

        bool usesResolveAttachments = false;

        uint32 attachmentIdx = 0u;
        uint32 numColourAttachments = 0u;
        VkAttachmentDescription attachments[OGRE_MAX_MULTIPLE_RENDER_TARGETS * 2u + 2u];

        VkAttachmentReference colourAttachRefs[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        VkAttachmentReference resolveAttachRefs[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        VkAttachmentReference depthAttachRef;

        memset( attachments, 0, sizeof( attachments ) );
        memset( colourAttachRefs, 0, sizeof( colourAttachRefs ) );
        memset( resolveAttachRefs, 0, sizeof( resolveAttachRefs ) );
        memset( &depthAttachRef, 0, sizeof( depthAttachRef ) );

        for( size_t i = 0; i < mrtCount; ++i )
        {
            resolveAttachRefs[numColourAttachments].attachment = VK_ATTACHMENT_UNUSED;
            resolveAttachRefs[numColourAttachments].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            if( passPso.colourFormat[i] != PFG_NULL )
            {
                colourAttachRefs[numColourAttachments].attachment = attachmentIdx;
                colourAttachRefs[numColourAttachments].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                attachments[attachmentIdx].samples =
                    static_cast<VkSampleCountFlagBits>( passPso.sampleDescription.getColourSamples() );
                attachments[attachmentIdx].format = VulkanMappings::get( passPso.colourFormat[i] );
                attachments[attachmentIdx].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachments[attachmentIdx].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                ++attachmentIdx;

                if( passPso.resolveColourFormat[i] != PFG_NULL )
                {
                    usesResolveAttachments = true;

                    attachments[attachmentIdx].samples = VK_SAMPLE_COUNT_1_BIT;
                    attachments[attachmentIdx].format =
                        VulkanMappings::get( passPso.resolveColourFormat[i] );
                    attachments[attachmentIdx].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    attachments[attachmentIdx].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                    resolveAttachRefs[numColourAttachments].attachment = attachmentIdx;
                    resolveAttachRefs[numColourAttachments].layout =
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    ++attachmentIdx;
                }
                ++numColourAttachments;
            }
        }

        if( passPso.depthFormat != PFG_NULL )
        {
            attachments[attachmentIdx].format = VulkanMappings::get( passPso.depthFormat );
            attachments[attachmentIdx].samples =
                static_cast<VkSampleCountFlagBits>( passPso.sampleDescription.getColourSamples() );
            attachments[attachmentIdx].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments[attachmentIdx].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            depthAttachRef.attachment = attachmentIdx;
            depthAttachRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            ++attachmentIdx;
        }

        VkSubpassDescription subpass;
        memset( &subpass, 0, sizeof( subpass ) );
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.inputAttachmentCount = 0u;
        subpass.colorAttachmentCount = numColourAttachments;
        subpass.pColorAttachments = colourAttachRefs;
        subpass.pResolveAttachments = usesResolveAttachments ? resolveAttachRefs : 0;
        subpass.pDepthStencilAttachment = ( passPso.depthFormat != PFG_NULL ) ? &depthAttachRef : 0;

        VkRenderPassCreateInfo renderPassCreateInfo;
        makeVkStruct( renderPassCreateInfo, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO );
        renderPassCreateInfo.attachmentCount = attachmentIdx;
        renderPassCreateInfo.pAttachments = attachments;
        renderPassCreateInfo.subpassCount = 1u;
        renderPassCreateInfo.pSubpasses = &subpass;

        VkRenderPass retVal = mCache->getRenderPass( renderPassCreateInfo );
        return retVal;
    }
    //-------------------------------------------------------------------------
    static VkPipelineStageFlags toVkPipelineStageFlags( ResourceLayout::Layout layout,
                                                        const bool bIsDepth )
    {
        switch( layout )
        {
        case ResourceLayout::PresentReady:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case ResourceLayout::RenderTarget:
        case ResourceLayout::RenderTargetReadOnly:
        case ResourceLayout::Clear:
            if( bIsDepth )
                return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                       VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            else
                return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case ResourceLayout::Texture:
        case ResourceLayout::Uav:
            return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                   VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
                   VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
                   VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        case ResourceLayout::MipmapGen:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case ResourceLayout::CopySrc:
        case ResourceLayout::CopyDst:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case ResourceLayout::ResolveDest:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case ResourceLayout::Undefined:
        case ResourceLayout::CopyEncoderManaged:
        case ResourceLayout::NumResourceLayouts:
            return 0;
        }

        return 0;
    }
    //-------------------------------------------------------------------------
    static VkPipelineStageFlags ogreToVkStageFlags( uint8 ogreStageMask )
    {
        VkPipelineStageFlags retVal = 0u;
        if( ogreStageMask & ( 1u << VertexShader ) )
            retVal |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        if( ogreStageMask & ( 1u << PixelShader ) )
            retVal |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        if( ogreStageMask & ( 1u << GeometryShader ) )
            retVal |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
        if( ogreStageMask & ( 1u << HullShader ) )
            retVal |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
        if( ogreStageMask & ( 1u << DomainShader ) )
            retVal |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
        if( ogreStageMask & ( 1u << GPT_COMPUTE_PROGRAM ) )
            retVal |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

        return retVal;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::endCopyEncoder() { mDevice->mGraphicsQueue.endCopyEncoder(); }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::executeResourceTransition( const ResourceTransitionArray &rstCollection )
    {
        if( rstCollection.empty() )
            return;

        // Needs to be done now, as it may change layouts of textures we're about to change
        mDevice->mGraphicsQueue.endAllEncoders();

        VkPipelineStageFlags srcStage = 0u;
        VkPipelineStageFlags dstStage = 0u;

        VkMemoryBarrier memBarrier;

        uint32 bufferSrcBarrierBits = 0u;
        uint32 bufferDstBarrierBits = 0u;

        ResourceTransitionArray::const_iterator itor = rstCollection.begin();
        ResourceTransitionArray::const_iterator endt = rstCollection.end();

        while( itor != endt )
        {
            if( itor->resource && itor->resource->isTextureGpu() )
            {
                OGRE_ASSERT_MEDIUM( itor->oldLayout != ResourceLayout::CopyEncoderManaged &&
                                    "ResourceLayout::CopyEncoderManaged is never in oldLayout" );
                OGRE_ASSERT_MEDIUM( itor->newLayout != ResourceLayout::CopyEncoderManaged &&
                                    "ResourceLayout::CopyEncoderManaged is never in newLayout" );

                VulkanTextureGpu *texture = static_cast<VulkanTextureGpu *>( itor->resource );
                VkImageMemoryBarrier imageBarrier = texture->getImageMemoryBarrier();
                imageBarrier.oldLayout = VulkanMappings::get( itor->oldLayout, texture );
                imageBarrier.newLayout = VulkanMappings::get( itor->newLayout, texture );

                const bool bIsDepth = PixelFormatGpuUtils::isDepth( texture->getPixelFormat() );

                // If oldAccess == ResourceAccess::Undefined then this texture is used for
                // the first time on a new frame (but not necessarily the first time ever)
                // thus there are no caches needed to flush.
                //
                // dstStage only needs to wait for the transition to happen though
                if( itor->oldAccess != ResourceAccess::Undefined )
                {
                    if( itor->oldAccess & ResourceAccess::Write )
                    {
                        imageBarrier.srcAccessMask =
                            VulkanMappings::getAccessFlags( itor->oldLayout, itor->oldAccess, texture,
                                                            false ) &
                            c_srcValidAccessFlags;
                    }

                    if( itor->oldLayout != ResourceLayout::Texture &&
                        itor->oldLayout != ResourceLayout::Uav )
                    {
                        srcStage |= toVkPipelineStageFlags( itor->oldLayout, bIsDepth );
                    }

                    if( itor->oldStageMask != 0u )
                        srcStage |= ogreToVkStageFlags( itor->oldStageMask );
                }

                imageBarrier.dstAccessMask =
                    VulkanMappings::getAccessFlags( itor->newLayout, itor->newAccess, texture, true );

                if( itor->newLayout != ResourceLayout::Texture &&
                    itor->newLayout != ResourceLayout::Uav )
                {
                    dstStage |= toVkPipelineStageFlags( itor->newLayout, bIsDepth );
                }

                if( itor->newStageMask != 0u )
                    dstStage |= ogreToVkStageFlags( itor->newStageMask );

                texture->mCurrLayout = imageBarrier.newLayout;

                mImageBarriers.push_back( imageBarrier );

                if( texture->isMultisample() && !texture->hasMsaaExplicitResolves() )
                {
                    // Rare case where we render to an implicit resolve without resolving
                    // (otherwise newLayout = ResolveDest), or more common case if we need
                    // to copy to/from an MSAA texture. We can also try to sample from texture.
                    // In all these cases keep MSAA texture in predictable layout.
                    //
                    // This cannot catch all use cases, but if you fall into something this
                    // doesn't catch, then you should probably be using explicit resolves
                    bool useNewLayoutForMsaa = itor->newLayout == ResourceLayout::RenderTarget ||
                                               itor->newLayout == ResourceLayout::ResolveDest ||
                                               itor->newLayout == ResourceLayout::CopySrc ||
                                               itor->newLayout == ResourceLayout::CopyDst;
                    bool useOldLayoutForMsaa = itor->oldLayout == ResourceLayout::RenderTarget ||
                                               itor->oldLayout == ResourceLayout::ResolveDest ||
                                               itor->oldLayout == ResourceLayout::CopySrc ||
                                               itor->oldLayout == ResourceLayout::CopyDst;
                    if( !useNewLayoutForMsaa )
                        imageBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    if( !useOldLayoutForMsaa )
                        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    imageBarrier.image = texture->getMsaaFramebufferName();
                    mImageBarriers.push_back( imageBarrier );
                }
            }
            else
            {
                srcStage |= ogreToVkStageFlags( itor->oldStageMask );
                dstStage |= ogreToVkStageFlags( itor->newStageMask );

                if( itor->oldAccess & ResourceAccess::Write )
                    bufferSrcBarrierBits |= VK_ACCESS_SHADER_WRITE_BIT;

                if( itor->newAccess & ResourceAccess::Read )
                    bufferDstBarrierBits |= VK_ACCESS_SHADER_READ_BIT;
                if( itor->newAccess & ResourceAccess::Write )
                    bufferDstBarrierBits |= VK_ACCESS_SHADER_WRITE_BIT;
            }
            ++itor;
        }

        uint32 numMemBarriers = 0u;
        if( bufferSrcBarrierBits || bufferDstBarrierBits )
        {
            makeVkStruct( memBarrier, VK_STRUCTURE_TYPE_MEMORY_BARRIER );
            memBarrier.srcAccessMask = bufferSrcBarrierBits & c_srcValidAccessFlags;
            memBarrier.dstAccessMask = bufferDstBarrierBits;
            numMemBarriers = 1u;
        }

        if( srcStage == 0 )
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        if( dstStage == 0 )
            dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        vkCmdPipelineBarrier( mDevice->mGraphicsQueue.getCurrentCmdBuffer(),
                              srcStage & mDevice->mSupportedStages, dstStage & mDevice->mSupportedStages,
                              0, numMemBarriers, &memBarrier, 0u, 0,
                              static_cast<uint32>( mImageBarriers.size() ), mImageBarriers.begin() );
        mImageBarriers.clear();
    }
    //-------------------------------------------------------------------------
    bool VulkanRenderSystem::_hlmsPipelineStateObjectCreated( HlmsPso *newPso, uint64 deadline )
    {
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        debugLogPso( newPso );
#endif

        if( ( newPso->geometryShader && !mDevice->mDeviceFeatures.geometryShader ) ||
            ( newPso->tesselationHullShader && !mDevice->mDeviceFeatures.tessellationShader ) ||
            ( newPso->tesselationDomainShader && !mDevice->mDeviceFeatures.tessellationShader ) )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Geometry or tesselation shaders are not supported",
                         "VulkanRenderSystem::_hlmsPipelineStateObjectCreated" );
        }

        size_t numShaderStages = 0u;
        VkPipelineShaderStageCreateInfo shaderStages[NumShaderTypes];

        VulkanRootLayout *rootLayout = 0;

        VulkanProgram *vertexShader = 0;
        VulkanProgram *pixelShader = 0;

        if( newPso->vertexShader )
        {
            vertexShader = static_cast<VulkanProgram *>( newPso->vertexShader->_getBindingDelegate() );
            vertexShader->fillPipelineShaderStageCi( shaderStages[numShaderStages++] );
            rootLayout = VulkanRootLayout::findBest( rootLayout, vertexShader->getRootLayout() );
        }

        if( newPso->geometryShader )
        {
            VulkanProgram *shader =
                static_cast<VulkanProgram *>( newPso->geometryShader->_getBindingDelegate() );
            shader->fillPipelineShaderStageCi( shaderStages[numShaderStages++] );
            rootLayout = VulkanRootLayout::findBest( rootLayout, shader->getRootLayout() );
        }

        if( newPso->tesselationHullShader )
        {
            VulkanProgram *shader =
                static_cast<VulkanProgram *>( newPso->tesselationHullShader->_getBindingDelegate() );
            shader->fillPipelineShaderStageCi( shaderStages[numShaderStages++] );
            rootLayout = VulkanRootLayout::findBest( rootLayout, shader->getRootLayout() );
        }

        if( newPso->tesselationDomainShader )
        {
            VulkanProgram *shader =
                static_cast<VulkanProgram *>( newPso->tesselationDomainShader->_getBindingDelegate() );
            shader->fillPipelineShaderStageCi( shaderStages[numShaderStages++] );
            rootLayout = VulkanRootLayout::findBest( rootLayout, shader->getRootLayout() );
        }

        if( newPso->pixelShader &&
            newPso->blendblock->mBlendChannelMask != HlmsBlendblock::BlendChannelForceDisabled )
        {
            pixelShader = static_cast<VulkanProgram *>( newPso->pixelShader->_getBindingDelegate() );
            pixelShader->fillPipelineShaderStageCi( shaderStages[numShaderStages++] );
            rootLayout = VulkanRootLayout::findBest( rootLayout, pixelShader->getRootLayout() );
        }

        if( !rootLayout )
        {
            String shaderNames =
                "The following shaders cannot be linked. Their Root Layouts are incompatible:\n";
            if( newPso->vertexShader )
            {
                shaderNames += newPso->vertexShader->getName() + "\n";
                static_cast<VulkanProgram *>( newPso->vertexShader->_getBindingDelegate() )
                    ->getRootLayout()
                    ->dump( shaderNames );
            }
            if( newPso->geometryShader )
            {
                shaderNames += newPso->geometryShader->getName() + "\n";
                static_cast<VulkanProgram *>( newPso->geometryShader->_getBindingDelegate() )
                    ->getRootLayout()
                    ->dump( shaderNames );
            }
            if( newPso->tesselationHullShader )
            {
                shaderNames += newPso->tesselationHullShader->getName() + "\n";
                static_cast<VulkanProgram *>( newPso->tesselationHullShader->_getBindingDelegate() )
                    ->getRootLayout()
                    ->dump( shaderNames );
            }
            if( newPso->tesselationDomainShader )
            {
                shaderNames += newPso->tesselationDomainShader->getName() + "\n";
                static_cast<VulkanProgram *>( newPso->tesselationDomainShader->_getBindingDelegate() )
                    ->getRootLayout()
                    ->dump( shaderNames );
            }
            if( newPso->pixelShader &&
                newPso->blendblock->mBlendChannelMask != HlmsBlendblock::BlendChannelForceDisabled )
            {
                shaderNames += newPso->pixelShader->getName() + "\n";
                static_cast<VulkanProgram *>( newPso->pixelShader->_getBindingDelegate() )
                    ->getRootLayout()
                    ->dump( shaderNames );
            }

            LogManager::getSingleton().logMessage( shaderNames, LML_CRITICAL );
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Shaders cannot be linked together. Their Root Layouts are incompatible. See "
                         "Ogre.log for more info",
                         "VulkanRenderSystem::_hlmsPipelineStateObjectCreated" );
        }

        VkPipelineLayout layout = rootLayout->createVulkanHandles();

        VkPipelineVertexInputStateCreateInfo vertexFormatCi;
        makeVkStruct( vertexFormatCi, VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO );
        FastArray<VkVertexInputBindingDescription> bindingDescriptions;
        FastArray<VkVertexInputAttributeDescription> attributeDescriptions;

        if( newPso->vertexShader )
        {
            VulkanProgram *shader =
                static_cast<VulkanProgram *>( newPso->vertexShader->_getBindingDelegate() );

            shader->getLayoutForPso( newPso->vertexElements, bindingDescriptions,
                                     attributeDescriptions );

            vertexFormatCi.vertexBindingDescriptionCount =
                static_cast<uint32_t>( bindingDescriptions.size() );
            vertexFormatCi.vertexAttributeDescriptionCount =
                static_cast<uint32_t>( attributeDescriptions.size() );
            vertexFormatCi.pVertexBindingDescriptions = bindingDescriptions.begin();
            vertexFormatCi.pVertexAttributeDescriptions = attributeDescriptions.begin();
        }

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyCi;
        makeVkStruct( inputAssemblyCi, VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO );
        inputAssemblyCi.topology = VulkanMappings::get( newPso->operationType );
        inputAssemblyCi.primitiveRestartEnable = newPso->enablePrimitiveRestart;

        VkPipelineTessellationStateCreateInfo tessStateCi;
        makeVkStruct( tessStateCi, VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO );
        tessStateCi.patchControlPoints = 1u;
        bool useTesselationState = mDevice->mDeviceFeatures.tessellationShader &&
                                   ( newPso->tesselationHullShader || newPso->tesselationDomainShader );

        VkPipelineViewportStateCreateInfo viewportStateCi;
        makeVkStruct( viewportStateCi, VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO );
        TODO_addVpCount_to_passpso;
        viewportStateCi.viewportCount = 1u;
        viewportStateCi.scissorCount = 1u;

        const float biasSign = mReverseDepth ? 1.0f : -1.0f;

        VkPipelineRasterizationStateCreateInfo rasterState;
        makeVkStruct( rasterState, VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO );
        rasterState.polygonMode = VulkanMappings::get( newPso->macroblock->mPolygonMode );
        rasterState.cullMode = VulkanMappings::get( newPso->macroblock->mCullMode );
        rasterState.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterState.depthClampEnable = newPso->macroblock->mDepthClamp;
        rasterState.depthBiasEnable = newPso->macroblock->mDepthBiasConstant != 0.0f;
        rasterState.depthBiasConstantFactor = newPso->macroblock->mDepthBiasConstant * biasSign;
        rasterState.depthBiasClamp = 0.0f;
        rasterState.depthBiasSlopeFactor = newPso->macroblock->mDepthBiasSlopeScale * biasSign;
        rasterState.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo mssCi;
        makeVkStruct( mssCi, VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO );
        mssCi.rasterizationSamples =
            static_cast<VkSampleCountFlagBits>( newPso->pass.sampleDescription.getColourSamples() );
        mssCi.alphaToCoverageEnable =
            newPso->blendblock->mAlphaToCoverage == HlmsBlendblock::A2cEnabled ||
            ( newPso->blendblock->mAlphaToCoverage == HlmsBlendblock::A2cEnabledMsaaOnly &&
              newPso->pass.sampleDescription.isMultisample() );

        VkPipelineDepthStencilStateCreateInfo depthStencilStateCi;
        makeVkStruct( depthStencilStateCi, VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO );
        depthStencilStateCi.depthTestEnable = newPso->macroblock->mDepthCheck;
        depthStencilStateCi.depthWriteEnable = newPso->macroblock->mDepthWrite;
        CompareFunction depthFunc = newPso->macroblock->mDepthFunc;
        if( mReverseDepth )
            depthFunc = reverseCompareFunction( depthFunc );
        depthStencilStateCi.depthCompareOp = VulkanMappings::get( depthFunc );
        depthStencilStateCi.stencilTestEnable = newPso->pass.stencilParams.enabled;
        if( newPso->pass.stencilParams.enabled )
        {
            depthStencilStateCi.front.failOp =
                VulkanMappings::get( newPso->pass.stencilParams.stencilFront.stencilFailOp );
            depthStencilStateCi.front.passOp =
                VulkanMappings::get( newPso->pass.stencilParams.stencilFront.stencilPassOp );
            depthStencilStateCi.front.depthFailOp =
                VulkanMappings::get( newPso->pass.stencilParams.stencilFront.stencilDepthFailOp );
            depthStencilStateCi.front.compareOp =
                VulkanMappings::get( newPso->pass.stencilParams.stencilFront.compareOp );
            depthStencilStateCi.front.compareMask = newPso->pass.stencilParams.readMask;
            depthStencilStateCi.front.writeMask = newPso->pass.stencilParams.writeMask;
            depthStencilStateCi.front.reference = 0;  // Dynamic state

            depthStencilStateCi.back.failOp =
                VulkanMappings::get( newPso->pass.stencilParams.stencilBack.stencilFailOp );
            depthStencilStateCi.back.passOp =
                VulkanMappings::get( newPso->pass.stencilParams.stencilBack.stencilPassOp );
            depthStencilStateCi.back.depthFailOp =
                VulkanMappings::get( newPso->pass.stencilParams.stencilBack.stencilDepthFailOp );
            depthStencilStateCi.back.compareOp =
                VulkanMappings::get( newPso->pass.stencilParams.stencilBack.compareOp );
            depthStencilStateCi.back.compareMask = newPso->pass.stencilParams.readMask;
            depthStencilStateCi.back.writeMask = newPso->pass.stencilParams.writeMask;
            depthStencilStateCi.back.reference = 0;  // Dynamic state
        }
        depthStencilStateCi.minDepthBounds = 0.0f;
        depthStencilStateCi.maxDepthBounds = 1.0f;

        VkPipelineColorBlendStateCreateInfo blendStateCi;
        makeVkStruct( blendStateCi, VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO );
        blendStateCi.logicOpEnable = false;
        uint8 mrtCount = 0;
        VkRenderPass renderPass = getVkRenderPass( newPso->pass, mrtCount );
        blendStateCi.attachmentCount = mrtCount;
        VkPipelineColorBlendAttachmentState blendStates[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        memset( blendStates, 0, sizeof( blendStates ) );

        if( newPso->blendblock->mSeparateBlend )
        {
            if( newPso->blendblock->mSourceBlendFactor == SBF_ONE &&
                newPso->blendblock->mDestBlendFactor == SBF_ZERO &&
                newPso->blendblock->mSourceBlendFactorAlpha == SBF_ONE &&
                newPso->blendblock->mDestBlendFactorAlpha == SBF_ZERO )
            {
                blendStates[0].blendEnable = false;
            }
            else
            {
                blendStates[0].blendEnable = true;
                blendStates[0].srcColorBlendFactor =
                    VulkanMappings::get( newPso->blendblock->mSourceBlendFactor );
                blendStates[0].dstColorBlendFactor =
                    VulkanMappings::get( newPso->blendblock->mDestBlendFactor );
                blendStates[0].colorBlendOp = VulkanMappings::get( newPso->blendblock->mBlendOperation );

                blendStates[0].srcAlphaBlendFactor =
                    VulkanMappings::get( newPso->blendblock->mSourceBlendFactorAlpha );
                blendStates[0].dstAlphaBlendFactor =
                    VulkanMappings::get( newPso->blendblock->mDestBlendFactorAlpha );
                blendStates[0].alphaBlendOp = blendStates[0].colorBlendOp;
            }
        }
        else
        {
            if( newPso->blendblock->mSourceBlendFactor == SBF_ONE &&
                newPso->blendblock->mDestBlendFactor == SBF_ZERO )
            {
                blendStates[0].blendEnable = false;
            }
            else
            {
                blendStates[0].blendEnable = true;
                blendStates[0].srcColorBlendFactor =
                    VulkanMappings::get( newPso->blendblock->mSourceBlendFactor );
                blendStates[0].dstColorBlendFactor =
                    VulkanMappings::get( newPso->blendblock->mDestBlendFactor );
                blendStates[0].colorBlendOp = VulkanMappings::get( newPso->blendblock->mBlendOperation );

                blendStates[0].srcAlphaBlendFactor = blendStates[0].srcColorBlendFactor;
                blendStates[0].dstAlphaBlendFactor = blendStates[0].dstColorBlendFactor;
                blendStates[0].alphaBlendOp = blendStates[0].colorBlendOp;
            }
        }
        blendStates[0].colorWriteMask =
            newPso->blendblock->mBlendChannelMask & HlmsBlendblock::BlendChannelAll;

        for( int i = 1; i < mrtCount; ++i )
            blendStates[i] = blendStates[0];

        blendStateCi.pAttachments = blendStates;

        // Having viewport hardcoded into PSO is crazy.
        // It could skyrocket the number of required PSOs and heavily neutralize caches.
        const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,
                                                 VK_DYNAMIC_STATE_STENCIL_REFERENCE };

        VkPipelineDynamicStateCreateInfo dynamicStateCi;
        makeVkStruct( dynamicStateCi, VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO );
        dynamicStateCi.dynamicStateCount = sizeof( dynamicStates ) / sizeof( dynamicStates[0] );
        dynamicStateCi.pDynamicStates = dynamicStates;

        VkGraphicsPipelineCreateInfo pipeline;
        makeVkStruct( pipeline, VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO );

        bool deadlineMissed =
            deadline != UINT64_MAX &&
            (int64)( Root::getSingleton().getTimer()->getMilliseconds() - deadline ) > 0;
        pipeline.flags = deadlineMissed && mDevice->mDeviceExtraFeatures.pipelineCreationCacheControl
                             ? VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_EXT
                             : 0;
        pipeline.layout = layout;
        pipeline.stageCount = static_cast<uint32>( numShaderStages );
        pipeline.pStages = shaderStages;
        pipeline.pVertexInputState = &vertexFormatCi;
        pipeline.pInputAssemblyState = &inputAssemblyCi;
        pipeline.pTessellationState = useTesselationState ? &tessStateCi : nullptr;
        pipeline.pViewportState = &viewportStateCi;
        pipeline.pRasterizationState = &rasterState;
        pipeline.pMultisampleState = &mssCi;
        pipeline.pDepthStencilState = &depthStencilStateCi;
        pipeline.pColorBlendState = &blendStateCi;
        pipeline.pDynamicState = &dynamicStateCi;
        pipeline.renderPass = renderPass;

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
        mValidationError = false;
#endif

        VkPipeline vulkanPso = 0;
        VkResult result = deadlineMissed && !mDevice->mDeviceExtraFeatures.pipelineCreationCacheControl
                              ? VK_PIPELINE_COMPILE_REQUIRED_EXT
                              : vkCreateGraphicsPipelines( mDevice->mDevice, mDevice->mPipelineCache, 1u,
                                                           &pipeline, 0, &vulkanPso );
        if( result == VK_PIPELINE_COMPILE_REQUIRED_EXT )
            return false;

        checkVkResult( mDevice, result, "vkCreateGraphicsPipelines" );

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        if( mValidationError )
        {
            LogManager::getSingleton().logMessage( "Validation error:" );

            // This isn't legal C++ but it's a debug function not intended for production
            GpuProgramPtr *shaders = &newPso->vertexShader;
            for( size_t i = 0u; i < NumShaderTypes; ++i )
            {
                if( !shaders[i] )
                    continue;

                VulkanProgram *shader =
                    static_cast<VulkanProgram *>( shaders[i]->_getBindingDelegate() );

                String debugDump;
                shader->debugDump( debugDump );
                LogManager::getSingleton().logMessage( debugDump );
            }
        }
#endif

        VulkanHlmsPso *pso = new VulkanHlmsPso( vulkanPso, rootLayout );
        newPso->rsData = pso;
        return true;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_hlmsPipelineStateObjectDestroyed( HlmsPso *pso )
    {
        if( pso == mPso )
        {
            mUavRenderingDirty = true;
            mGlobalTable.setAllDirty();
            mTableDirty = true;
            mPso = 0;
        }

        OGRE_ASSERT_LOW( pso->rsData );
        VulkanHlmsPso *vulkanPso = static_cast<VulkanHlmsPso *>( pso->rsData );
        delayed_vkDestroyPipeline( mVaoManager, mDevice->mDevice, vulkanPso->pso, 0 );
        delete vulkanPso;
        pso->rsData = 0;
    }

    void VulkanRenderSystem::_hlmsMacroblockCreated( HlmsMacroblock *newBlock )
    {
        Log *defaultLog = LogManager::getSingleton().getDefaultLog();
        if( defaultLog )
        {
            defaultLog->logMessage( String( " _hlmsMacroblockCreated " ) );
        }
    }

    void VulkanRenderSystem::_hlmsMacroblockDestroyed( HlmsMacroblock *block ) {}

    void VulkanRenderSystem::_hlmsBlendblockCreated( HlmsBlendblock *newBlock )
    {
        Log *defaultLog = LogManager::getSingleton().getDefaultLog();
        if( defaultLog )
        {
            defaultLog->logMessage( String( " _hlmsBlendblockCreated " ) );
        }
    }

    void VulkanRenderSystem::_hlmsBlendblockDestroyed( HlmsBlendblock *block ) {}

    void VulkanRenderSystem::_hlmsSamplerblockCreated( HlmsSamplerblock *newBlock )
    {
        Log *defaultLog = LogManager::getSingleton().getDefaultLog();
        if( defaultLog )
        {
            defaultLog->logMessage( String( " _hlmsSamplerblockCreated " ) );
        }

        VkSamplerCreateInfo samplerDescriptor;
        makeVkStruct( samplerDescriptor, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO );
        samplerDescriptor.minFilter = VulkanMappings::get( newBlock->mMinFilter );
        samplerDescriptor.magFilter = VulkanMappings::get( newBlock->mMagFilter );
        samplerDescriptor.mipmapMode = VulkanMappings::getMipFilter( newBlock->mMipFilter );
        samplerDescriptor.mipLodBias = newBlock->mMipLodBias;
        float maxAllowedAnisotropy = mDevice->mDeviceProperties.limits.maxSamplerAnisotropy;
        samplerDescriptor.maxAnisotropy = newBlock->mMaxAnisotropy > maxAllowedAnisotropy
                                              ? maxAllowedAnisotropy
                                              : newBlock->mMaxAnisotropy;
        samplerDescriptor.anisotropyEnable =
            ( mDevice->mDeviceFeatures.samplerAnisotropy == VK_TRUE ) &&
            ( samplerDescriptor.maxAnisotropy > 1.0f ? VK_TRUE : VK_FALSE );
        samplerDescriptor.addressModeU = VulkanMappings::get( newBlock->mU );
        samplerDescriptor.addressModeV = VulkanMappings::get( newBlock->mV );
        samplerDescriptor.addressModeW = VulkanMappings::get( newBlock->mW );
        samplerDescriptor.unnormalizedCoordinates = VK_FALSE;
        samplerDescriptor.minLod = newBlock->mMinLod;
        samplerDescriptor.maxLod = newBlock->mMaxLod;

        if( newBlock->mCompareFunction != NUM_COMPARE_FUNCTIONS )
        {
            samplerDescriptor.compareEnable = VK_TRUE;
            samplerDescriptor.compareOp = VulkanMappings::get( newBlock->mCompareFunction );
        }

        VkSampler textureSampler;
        VkResult result = vkCreateSampler( mDevice->mDevice, &samplerDescriptor, 0, &textureSampler );
        checkVkResult( mDevice, result, "vkCreateSampler" );

#if OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_64
        newBlock->mRsData = textureSampler;
#else  // VK handles are always 64bit, even on 32bit systems
        newBlock->mRsData = new uint64( textureSampler );
#endif
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_hlmsSamplerblockDestroyed( HlmsSamplerblock *block )
    {
        assert( block->mRsData );
#if OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_64
        VkSampler textureSampler = static_cast<VkSampler>( block->mRsData );
#else  // VK handles are always 64bit, even on 32bit systems
        VkSampler textureSampler = *static_cast<VkSampler *>( block->mRsData );
        delete(uint64 *)block->mRsData;
#endif
        delayed_vkDestroySampler( mVaoManager, mDevice->mDevice, textureSampler, 0 );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_descriptorSetTextureCreated( DescriptorSetTexture *newSet )
    {
        VulkanDescriptorSetTexture *vulkanSet = new VulkanDescriptorSetTexture( *newSet );
        newSet->mRsData = vulkanSet;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_descriptorSetTextureDestroyed( DescriptorSetTexture *set )
    {
        OGRE_ASSERT_LOW( set->mRsData );
        VulkanDescriptorSetTexture *vulkanSet =
            static_cast<VulkanDescriptorSetTexture *>( set->mRsData );
        delete vulkanSet;
        set->mRsData = 0;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_descriptorSetTexture2Created( DescriptorSetTexture2 *newSet )
    {
        VulkanDescriptorSetTexture2 *vulkanSet = new VulkanDescriptorSetTexture2( *newSet );
        newSet->mRsData = vulkanSet;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_descriptorSetTexture2Destroyed( DescriptorSetTexture2 *set )
    {
        OGRE_ASSERT_LOW( set->mRsData );
        VulkanDescriptorSetTexture2 *vulkanSet =
            static_cast<VulkanDescriptorSetTexture2 *>( set->mRsData );
        vulkanSet->destroy( mVaoManager, mDevice->mDevice, *set );
        delete vulkanSet;
        set->mRsData = 0;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_descriptorSetSamplerCreated( DescriptorSetSampler *newSet )
    {
        VulkanDescriptorSetSampler *vulkanSet = new VulkanDescriptorSetSampler( *newSet, mDummySampler );
        newSet->mRsData = vulkanSet;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_descriptorSetSamplerDestroyed( DescriptorSetSampler *set )
    {
        OGRE_ASSERT_LOW( set->mRsData );
        VulkanDescriptorSetSampler *vulkanSet =
            static_cast<VulkanDescriptorSetSampler *>( set->mRsData );
        delete vulkanSet;
        set->mRsData = 0;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_descriptorSetUavCreated( DescriptorSetUav *newSet )
    {
        VulkanDescriptorSetUav *vulkanSet = new VulkanDescriptorSetUav( *newSet );
        newSet->mRsData = vulkanSet;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_descriptorSetUavDestroyed( DescriptorSetUav *set )
    {
        OGRE_ASSERT_LOW( set->mRsData );

        VulkanDescriptorSetUav *vulkanSet = reinterpret_cast<VulkanDescriptorSetUav *>( set->mRsData );
        vulkanSet->destroy( *set );
        delete vulkanSet;

        set->mRsData = 0;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::setStencilBufferParams( uint32 refValue,
                                                     const StencilParams &stencilParams )
    {
        RenderSystem::setStencilBufferParams( refValue, stencilParams );

        // There are two main cases:
        // 1. The active render encoder is valid and will be subsequently used for drawing.
        //      We need to set the stencil reference value on this encoder. We do this below.
        // 2. The active render is invalid or is about to go away.
        //      In this case, we need to set the stencil reference value on the new encoder when it
        //      is created (In this case, the setStencilReferenceValue below in this wasted, but it
        //      is inexpensive).

        // Save this info so we can transfer it into a new encoder if necessary
        mStencilEnabled = stencilParams.enabled;
        if( mStencilEnabled )
        {
            mStencilRefValue = refValue;

            if( mDevice->mGraphicsQueue.getEncoderState() == VulkanQueue::EncoderGraphicsOpen )
            {
                vkCmdSetStencilReference( mDevice->mGraphicsQueue.getCurrentCmdBuffer(),
                                          VK_STENCIL_FACE_FRONT_AND_BACK, mStencilRefValue );
            }
        }
    }
    //-------------------------------------------------------------------------
    inline bool isPowerOf2( uint32 x ) { return ( x & ( x - 1u ) ) == 0u; }
    SampleDescription VulkanRenderSystem::validateSampleDescription( const SampleDescription &sampleDesc,
                                                                     PixelFormatGpu format,
                                                                     uint32 textureFlags )
    {
        if( !mDevice )
        {
            // TODO
            return sampleDesc;
        }

        // TODO: Spec says framebufferColorSampleCounts & co. contains the base minimum for all formats
        // and we can use vkGetPhysicalDeviceImageFormatProperties to check for MSAA settings that may go
        // BEYOND that mask for a specific format (honestly I don't trust Android drivers to implement
        // vkGetPhysicalDeviceImageFormatProperties properly).
        const VkPhysicalDeviceLimits &deviceLimits = mDevice->mDeviceProperties.limits;

        if( sampleDesc.getCoverageSamples() != 0u )
        {
            // EQAA / CSAA.
            // TODO: Support VK_AMD_mixed_attachment_samples & VK_NV_framebuffer_mixed_samples.
            return validateSampleDescription(
                SampleDescription( sampleDesc.getMaxSamples(), sampleDesc.getMsaaPattern() ), format,
                textureFlags );
        }
        else
        {
            // MSAA.
            VkSampleCountFlags supportedSampleCounts = ( VK_SAMPLE_COUNT_64_BIT << 1 ) - 1;

            if( format == PFG_NULL )
            {
                // PFG_NULL is always NotTexture and can't be Uav,
                // let's just return to the user what they intended to ask.
                supportedSampleCounts = deviceLimits.framebufferNoAttachmentsSampleCounts;
            }
            else if( textureFlags & TextureFlags::Uav )
            {
                supportedSampleCounts &= deviceLimits.storageImageSampleCounts;
            }
            else
            {
                const bool isDepth = PixelFormatGpuUtils::isDepth( format );
                const bool isStencil = PixelFormatGpuUtils::isStencil( format );
                const bool isInteger = PixelFormatGpuUtils::isInteger( format );

                if( textureFlags & ( TextureFlags::NotTexture | TextureFlags::RenderToTexture |
                                     TextureFlags::RenderWindowSpecific ) )
                {
                    // frame buffer
                    if( !isDepth && !isStencil && !isInteger )
                        supportedSampleCounts &= deviceLimits.framebufferColorSampleCounts;
                    if( isDepth || ( textureFlags & TextureFlags::RenderWindowSpecific ) )
                        supportedSampleCounts &= deviceLimits.framebufferDepthSampleCounts;
                    if( isStencil || ( textureFlags & TextureFlags::RenderWindowSpecific ) )
                        supportedSampleCounts &= deviceLimits.framebufferStencilSampleCounts;
                    if( isInteger )
                    {
                        // TODO: Query Vulkan 1.2 / extensions to get
                        // framebufferIntegerColorSampleCounts.
                        supportedSampleCounts &= VK_SAMPLE_COUNT_1_BIT;
                    }
                }

                if( 0 == ( textureFlags & TextureFlags::NotTexture ) )
                {
                    // sampled image
                    if( !isDepth && !isStencil && !isInteger )
                        supportedSampleCounts &= deviceLimits.sampledImageColorSampleCounts;
                    if( isDepth || ( textureFlags & TextureFlags::RenderWindowSpecific ) )
                        supportedSampleCounts &= deviceLimits.sampledImageDepthSampleCounts;
                    if( isStencil || ( textureFlags & TextureFlags::RenderWindowSpecific ) )
                        supportedSampleCounts &= deviceLimits.sampledImageStencilSampleCounts;
                    if( isInteger )
                        supportedSampleCounts &= deviceLimits.sampledImageIntegerSampleCounts;
                }
            }

            uint8 samples = sampleDesc.getColourSamples();
            OGRE_ASSERT_LOW( isPowerOf2( samples ) );
            while( samples > 0u )
            {
                if( supportedSampleCounts & samples )
                    return SampleDescription( samples, sampleDesc.getMsaaPattern() );
                samples = samples >> 1u;
            }

            // Ouch. The format is not supported. Return "something".
            return SampleDescription( 0u, sampleDesc.getMsaaPattern() );
        }
    }
    //-------------------------------------------------------------------------
    bool VulkanRenderSystem::isSameLayout( ResourceLayout::Layout a, ResourceLayout::Layout b,
                                           const TextureGpu *texture, bool bIsDebugCheck ) const
    {
        return VulkanMappings::get( a, texture ) == VulkanMappings::get( b, texture );
    }
}  // namespace Ogre
