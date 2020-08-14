/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2014 Torus Knot Software Ltd

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
#include "OgreVulkanTextureGpuManager.h"
#include "OgreVulkanUtils.h"
#include "OgreVulkanWindow.h"
#include "Vao/OgreVulkanVaoManager.h"

#include "OgreDefaultHardwareBufferManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include "CommandBuffer/OgreCbDrawCall.h"

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

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#    include "Windowing/win32/OgreVulkanWin32Window.h"
#else
#    include "Windowing/X11/OgreVulkanXcbWindow.h"
#endif

#include "OgrePixelFormatGpuUtils.h"

#define TODO_check_layers_exist

#define TODO_addVpCount_to_passpso
#define TODO_depth_readOnly_passes_dont_need_flushing

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
        } else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
            sprintf(message, "DEBUG: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
        } else {
            sprintf(message, "INFORMATION: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
        }

        LogManager::getSingleton().logMessage( message );

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
    VulkanRenderSystem::VulkanRenderSystem() :
        RenderSystem(),
        mInitialized( false ),
        mHardwareBufferManager( 0 ),
        mIndirectBuffer( 0 ),
        mShaderManager( 0 ),
        mVulkanProgramFactory0( 0 ),
        mVulkanProgramFactory1( 0 ),
        mVulkanProgramFactory2( 0 ),
        mVulkanProgramFactory3( 0 ),
        mVkInstance( 0 ),
        mAutoParamsBufferIdx( 0 ),
        mCurrentAutoParamsBufferPtr( 0 ),
        mCurrentAutoParamsBufferSpaceLeft( 0 ),
        mActiveDevice( 0 ),
        mDevice( 0 ),
        mCache( 0 ),
        mPso( 0 ),
        mComputePso( 0 ),
        mTableDirty( false ),
        mDummyBuffer( 0 ),
        mDummyTexBuffer( 0 ),
        mDummyTextureView( 0 ),
        mDummySampler( 0 ),
        mEntriesToFlush( 0u ),
        mVpChanged( false ),
        mInterruptedRenderCommandEncoder( false ),
        CreateDebugReportCallback( 0 ),
        DestroyDebugReportCallback( 0 ),
        mDebugReportCallback( 0 ),
        mCurrentDescriptorSetTexture( 0 )
    {
        memset( &mGlobalTable, 0, sizeof( mGlobalTable ) );
        mGlobalTable.reset();

        for( size_t i = 0u; i < NUM_BIND_TEXTURES; ++i )
            mGlobalTable.textures[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::shutdown( void )
    {
        if( mActiveDevice )
            mActiveDevice->mGraphicsQueue.endAllEncoders();

        if( mDummySampler )
        {
            vkDestroySampler( mActiveDevice->mDevice, mDummySampler, 0 );
            mDummySampler = 0;
        }

        if( mDummyTextureView )
        {
            vkDestroyImageView( mActiveDevice->mDevice, mDummyTextureView, 0 );
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

        OGRE_DELETE mCache;
        mCache = 0;

        OGRE_DELETE mShaderManager;
        mShaderManager = 0;

        OGRE_DELETE mVulkanProgramFactory3;  // LIFO destruction order
        mVulkanProgramFactory3 = 0;
        OGRE_DELETE mVulkanProgramFactory2;
        mVulkanProgramFactory2 = 0;
        OGRE_DELETE mVulkanProgramFactory1;
        mVulkanProgramFactory1 = 0;
        OGRE_DELETE mVulkanProgramFactory0;
        mVulkanProgramFactory0 = 0;

        OGRE_DELETE mHardwareBufferManager;
        mHardwareBufferManager = 0;

        delete mDevice;
        mDevice = 0;

        if( mDebugReportCallback )
        {
            DestroyDebugReportCallback( mVkInstance, mDebugReportCallback, 0 );
            mDebugReportCallback = 0;
        }

        if( mVkInstance )
        {
            vkDestroyInstance( mVkInstance, 0 );
            mVkInstance = 0;
        }
    }
    //-------------------------------------------------------------------------
    const String &VulkanRenderSystem::getName( void ) const
    {
        static String strName( "Vulkan Rendering Subsystem" );
        return strName;
    }
    //-------------------------------------------------------------------------
    const String &VulkanRenderSystem::getFriendlyName( void ) const
    {
        static String strName( "Vulkan_RS" );
        return strName;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::addInstanceDebugCallback( void )
    {
        CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
            mVkInstance, "vkCreateDebugReportCallbackEXT" );
        DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
            mVkInstance, "vkDestroyDebugReportCallbackEXT" );
        if( !CreateDebugReportCallback )
        {
            LogManager::getSingleton().logMessage(
                "[Vulkan] GetProcAddr: Unable to find vkCreateDebugReportCallbackEXT. "
                "Debug reporting won't be available" );
            return;
        }
        if( !DestroyDebugReportCallback )
        {
            LogManager::getSingleton().logMessage(
                "[Vulkan] GetProcAddr: Unable to find vkDestroyDebugReportCallbackEXT. "
                "Debug reporting won't be available" );
            return;
        }
        // DebugReportMessage =
        //    (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr( mVkInstance, "vkDebugReportMessageEXT"
        //    );
        // if( !DebugReportMessage )
        //{
        //    LogManager::getSingleton().logMessage(
        //        "[Vulkan] GetProcAddr: Unable to find DebugReportMessage. "
        //        "Debug reporting won't be available" );
        //}

        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        PFN_vkDebugReportCallbackEXT callback;
        makeVkStruct( dbgCreateInfo, VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT );
        callback = dbgFunc;
        dbgCreateInfo.pfnCallback = callback;
        dbgCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                              VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        VkResult result =
            CreateDebugReportCallback( mVkInstance, &dbgCreateInfo, 0, &mDebugReportCallback );
        switch( result )
        {
        case VK_SUCCESS:
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            OGRE_VK_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, result,
                            "CreateDebugReportCallback: out of host memory",
                            "VulkanDevice::addInstanceDebugCallback" );
        default:
            OGRE_VK_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, result, "vkCreateDebugReportCallbackEXT",
                            "VulkanDevice::addInstanceDebugCallback" );
        }
    }
    //-------------------------------------------------------------------------
    HardwareOcclusionQuery *VulkanRenderSystem::createHardwareOcclusionQuery( void )
    {
        return 0;  // TODO
    }
    //-------------------------------------------------------------------------
    RenderSystemCapabilities *VulkanRenderSystem::createRenderSystemCapabilities( void ) const
    {
        RenderSystemCapabilities *rsc = new RenderSystemCapabilities();
        rsc->setRenderSystemName( getName() );

        // We would like to save the device properties for the device capabilities limits.
        // These limits are needed for buffers' binding alignments.
        VkPhysicalDeviceProperties *vkProperties =
            const_cast<VkPhysicalDeviceProperties *>( &mActiveDevice->mDeviceProperties );
        vkGetPhysicalDeviceProperties( mActiveDevice->mPhysicalDevice, vkProperties );

        VkPhysicalDeviceProperties &properties = mActiveDevice->mDeviceProperties;

        rsc->setDeviceName( properties.deviceName );

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

        if( mActiveDevice->mDeviceFeatures.imageCubeArray )
            rsc->setCapability( RSC_TEXTURE_CUBE_MAP_ARRAY );

        if( mActiveDevice->mDeviceFeatures.depthClamp )
            rsc->setCapability( RSC_DEPTH_CLAMP );

        rsc->setCapability( RSC_HWSTENCIL );
        rsc->setStencilBufferBitDepth( 8 );
        rsc->setNumTextureUnits( 16 );
        rsc->setCapability( RSC_ANISOTROPY );
        rsc->setCapability( RSC_AUTOMIPMAP );
        rsc->setCapability( RSC_BLENDING );
        rsc->setCapability( RSC_DOT3 );
        rsc->setCapability( RSC_CUBEMAPPING );
        rsc->setCapability( RSC_TEXTURE_COMPRESSION );
        rsc->setCapability( RSC_TEXTURE_COMPRESSION_DXT );
        rsc->setCapability( RSC_VBO );
        rsc->setCapability( RSC_TWO_SIDED_STENCIL );
        rsc->setCapability( RSC_STENCIL_WRAP );
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
        rsc->setCapability( RSC_EXPLICIT_API );
        rsc->setMaxPointSize( 256 );

        rsc->setMaximumResolutions( 16384, 4096, 16384 );

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

        return rsc;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::reinitialise( void )
    {
        this->shutdown();
        this->_initialise( true );
    }
    //-------------------------------------------------------------------------
    Window *VulkanRenderSystem::_initialise( bool autoCreateWindow, const String &windowTitle )
    {
        Window *autoWindow = 0;
        if( autoCreateWindow )
            autoWindow = _createRenderWindow( windowTitle, 1280u, 720u, false );
        RenderSystem::_initialise( autoCreateWindow, windowTitle );

        return autoWindow;
    }
    //-------------------------------------------------------------------------
    Window *VulkanRenderSystem::_createRenderWindow( const String &name, uint32 width, uint32 height,
                                                     bool fullScreen,
                                                     const NameValuePairList *miscParams )
    {
        FastArray<const char *> reqInstanceExtensions;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        VulkanWindow *win =
            OGRE_NEW OgreVulkanWin32Window( reqInstanceExtensions, name, width, height, fullScreen );
#else
        VulkanWindow *win =
            OGRE_NEW VulkanXcbWindow( reqInstanceExtensions, name, width, height, fullScreen );
#endif
        mWindows.insert( win );

        if( !mInitialized )
        {
            if( miscParams )
            {
                NameValuePairList::const_iterator itOption = miscParams->find( "reverse_depth" );
                if( itOption != miscParams->end() )
                    mReverseDepth = StringConverter::parseBool( itOption->second, true );
            }

            TODO_check_layers_exist;
            FastArray<const char *> instanceLayers;
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
            instanceLayers.push_back( "VK_LAYER_KHRONOS_validation" );
            reqInstanceExtensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
#endif
            const uint8 dynBufferMultiplier = 3u;

            mVkInstance =
                VulkanDevice::createInstance( name, reqInstanceExtensions, instanceLayers, dbgFunc );
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
            addInstanceDebugCallback();
#endif
            mDevice = new VulkanDevice( mVkInstance, 0u, this );
            mActiveDevice = mDevice;

            mRealCapabilities = createRenderSystemCapabilities();
            mCurrentCapabilities = mRealCapabilities;

            initialiseFromRenderSystemCapabilities( mCurrentCapabilities, 0 );

            mNativeShadingLanguageVersion = 450;

            FastArray<const char *> deviceExtensions;
            mDevice->createDevice( deviceExtensions, 0u, 0u );

            VulkanVaoManager *vaoManager =
                OGRE_NEW VulkanVaoManager( dynBufferMultiplier, mDevice, this );
            mVaoManager = vaoManager;
            mHardwareBufferManager = OGRE_NEW v1::VulkanHardwareBufferManager( mDevice, mVaoManager );

            mActiveDevice->mVaoManager = vaoManager;
            mActiveDevice->initQueues();
            vaoManager->initDrawIdVertexBuffer();

            FastArray<PixelFormatGpu> depthFormatCandidates( 5u );
            if( PixelFormatGpuUtils::isStencil( DepthBuffer::DefaultDepthBufferFormat ) )
            {
                depthFormatCandidates.push_back( PFG_D32_FLOAT_S8X24_UINT );
                depthFormatCandidates.push_back( PFG_D24_UNORM_S8_UINT );
                depthFormatCandidates.push_back( PFG_D32_FLOAT );
                depthFormatCandidates.push_back( PFG_D24_UNORM );
                depthFormatCandidates.push_back( PFG_D16_UNORM );
            }
            else
            {
                depthFormatCandidates.push_back( PFG_D32_FLOAT );
                depthFormatCandidates.push_back( PFG_D24_UNORM );
                depthFormatCandidates.push_back( PFG_D32_FLOAT_S8X24_UINT );
                depthFormatCandidates.push_back( PFG_D24_UNORM_S8_UINT );
                depthFormatCandidates.push_back( PFG_D16_UNORM );
            }

            DepthBuffer::DefaultDepthBufferFormat = findSupportedFormat(
                mDevice->mPhysicalDevice, depthFormatCandidates, VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );

            VulkanTextureGpuManager *textureGpuManager =
                OGRE_NEW VulkanTextureGpuManager( vaoManager, this, mDevice );
            mTextureGpuManager = textureGpuManager;

            uint32 dummyData = 0u;
            mDummyBuffer = vaoManager->createConstBuffer( 4u, BT_IMMUTABLE, &dummyData, false );
            mDummyTexBuffer =
                vaoManager->createTexBuffer( PFG_RGBA8_UNORM, 4u, BT_IMMUTABLE, &dummyData, false );

            {
                VkImage dummyImage =
                    textureGpuManager->getBlankTextureVulkanName( TextureTypes::Type2D );

                VkImageViewCreateInfo imageViewCi;
                makeVkStruct( imageViewCi, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO );
                imageViewCi.image = dummyImage;
                imageViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
                imageViewCi.format = VK_FORMAT_R8G8B8A8_UNORM;
                imageViewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageViewCi.subresourceRange.levelCount = 1u;
                imageViewCi.subresourceRange.layerCount = 1u;

                VkResult result =
                    vkCreateImageView( mActiveDevice->mDevice, &imageViewCi, 0, &mDummyTextureView );
                checkVkResult( result, "vkCreateImageView" );
            }

            {
                VkSamplerCreateInfo samplerDescriptor;
                makeVkStruct( samplerDescriptor, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO );
                float maxAllowedAnisotropy =
                    mActiveDevice->mDeviceProperties.limits.maxSamplerAnisotropy;
                samplerDescriptor.maxAnisotropy = maxAllowedAnisotropy;
                samplerDescriptor.minLod = -std::numeric_limits<float>::max();
                samplerDescriptor.maxLod = std::numeric_limits<float>::max();
                VkResult result =
                    vkCreateSampler( mActiveDevice->mDevice, &samplerDescriptor, 0, &mDummySampler );
                checkVkResult( result, "vkCreateSampler" );
            }

            OGRE_ASSERT_HIGH( dynamic_cast<VulkanConstBufferPacked *>( mDummyBuffer ) );

            for( uint16 i = 0u; i < NumShaderTypes + 1u; ++i )
            {
                VulkanConstBufferPacked *constBuffer =
                    static_cast<VulkanConstBufferPacked *>( mDummyBuffer );
                constBuffer->bindAsParamBuffer( static_cast<GpuProgramType>( i ), 0 );
            }
            for( uint16 i = 0u; i < NUM_BIND_CONST_BUFFERS; ++i )
                mDummyBuffer->bindBufferVS( i );
            for( uint16 i = 0u; i < NUM_BIND_TEX_BUFFERS; ++i )
                mDummyTexBuffer->bindBufferVS( i );
            for( size_t i = 0u; i < NUM_BIND_TEXTURES; ++i )
            {
                mGlobalTable.textures[i].imageView = mDummyTextureView;
                mGlobalTable.textures[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
            for( size_t i = 0u; i < NUM_BIND_SAMPLERS; ++i )
                mGlobalTable.samplers[i].sampler = mDummySampler;

            mInitialized = true;
        }

        win->_setDevice( mActiveDevice );
        win->_initialize( mTextureGpuManager, miscParams );

        return win;
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
    void VulkanRenderSystem::flushUAVs( void )
    {
        /*Log *defaultLog = LogManager::getSingleton().getDefaultLog();
        if( defaultLog )
        {
            defaultLog->logMessage( String( " flushUAVs " ) );
        }*/
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setParamBuffer( GpuProgramType shaderStage,
                                              const VkDescriptorBufferInfo &bufferInfo )
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
    void VulkanRenderSystem::_setCurrentDeviceFromTexture( TextureGpu *texture ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setTexture( size_t unit, TextureGpu *texPtr )
    {
        OGRE_ASSERT_MEDIUM( unit < NUM_BIND_TEXTURES );
        if( texPtr )
        {
            VulkanTextureGpu *tex = static_cast<VulkanTextureGpu *>( texPtr );

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
            if( tex->isDataReady() && tex->mCurrLayout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
            {
                TextureGpu *targetTex;
                uint8 targetMip;
                mCurrentRenderPassDescriptor->findAnyTexture( &targetTex, targetMip );
                String texName = targetTex ? targetTex->getNameStr() : "";
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                             "Texture " + tex->getNameStr() +
                                 " is not in ResourceLayout::Texture. Did you forget to expose it to "
                                 "compositor? Currently rendering to target: " +
                                 texName,
                             "VulkanRenderSystem::_setTexture" );
            }
#endif

            if( mGlobalTable.textures[unit].imageView != tex->getDefaultDisplaySrv() )
            {
                mGlobalTable.textures[unit].imageView = tex->getDefaultDisplaySrv();
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
        uint32 texUnit = slotStart;
        FastArray<const TextureGpu *>::const_iterator itor = set->mTextures.begin();

        for( size_t i = 0u; i < NumShaderTypes; ++i )
        {
            const size_t numTexturesUsed = set->mShaderTypeTexCount[i];
            for( size_t j = 0u; j < numTexturesUsed; ++j )
            {
                if( ( texUnit - slotStart ) == hazardousTexIdx &&
                    mCurrentRenderPassDescriptor->hasAttachment( set->mTextures[hazardousTexIdx] ) )
                {
                    // Hazardous textures can't be in two layouts at the same time
                    _setTexture( texUnit, 0 );
                }
                else
                {
                    _setTexture( texUnit, const_cast<TextureGpu *>( *itor ) );
                }
                ++texUnit;
                ++itor;
            }
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setTextures( uint32 slotStart, const DescriptorSetTexture2 *set )
    {
        Log *defaultLog = LogManager::getSingleton().getDefaultLog();
        if( defaultLog )
        {
            defaultLog->logMessage( String( " _setTextures DescriptorSetTexture2 " ) );
        }

        VulkanDescriptorSetTexture *vulkanSet =
            reinterpret_cast<VulkanDescriptorSetTexture *>( set->mRsData );

        mCurrentDescriptorSetTexture = vulkanSet;

        VkCommandBuffer cmdBuffer = mActiveDevice->mGraphicsQueue.mCurrentCmdBuffer;

        // Bind textures
        {
            uint32 pos = OGRE_VULKAN_TEX_SLOT_START;
            FastArray<VulkanTexRegion>::const_iterator itor = vulkanSet->textures.begin();
            FastArray<VulkanTexRegion>::const_iterator end = vulkanSet->textures.end();

            while( itor != end )
            {
                Range range = itor->range;
                range.location += slotStart;

                uint32 lastPos = range.location + range.length;

                VkDescriptorImageInfo imageInfo;
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                imageInfo.imageView = *itor->textures;
                imageInfo.sampler = 0;
#if VULKAN_HOTSHOT_DISABLED
                mImageInfo[pos++][0] = imageInfo;
#endif
                // uint32 i = 0;
                // for( uint32 texUnit = range.location; texUnit < lastPos; ++texUnit)
                // {
                //     VkDescriptorImageInfo imageInfo;
                //     imageInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                //     imageInfo.imageView = itor->textures[i++];
                //     imageInfo.sampler = 0;
                //     mImageInfo[texUnit][0] = imageInfo;
                // }
                ++itor;
            }
        }

        // Bind buffers
        {
            uint32 pos = 0;
            FastArray<VulkanBufferRegion>::const_iterator itor = vulkanSet->buffers.begin();
            FastArray<VulkanBufferRegion>::const_iterator end = vulkanSet->buffers.end();

            VkDeviceSize offsets[15];
            memset( offsets, 0, sizeof( offsets ) );

            while( itor != end )
            {
                Range range = itor->range;
                // range.location += slotStart + OGRE_VULKAN_TEX_SLOT_START;

                VkDescriptorBufferInfo bufferInfo;
                bufferInfo.buffer = *itor->buffers;
                bufferInfo.offset = range.location;
                bufferInfo.range = range.length;
#if VULKAN_HOTSHOT_DISABLED
                mBufferInfo[pos++][0] = bufferInfo;
#endif

                // for( uint32 texUnit = 0; texUnit < itor->b; ++texUnit )
                // {
                //     VkDescriptorBufferInfo bufferInfo;
                //     bufferInfo.buffer = itor->buffers[texUnit];
                //     bufferInfo.offset = range.location;
                //     bufferInfo.range = range.length;
                //     mBufferInfo[texUnit][0] = bufferInfo;
                // }

                // switch( itor->shaderType )
                // {
                // case VertexShader:
                //     vkCmdBindVertexBuffers( cmdBuffer, range.location, range.length, itor->buffers,
                //                             itor->offsets );
                //     // [mActiveRenderEncoder setVertexBuffers:itor->buffers
                //     //                                offsets:itor->offsets
                //     //                              withRange:range];
                //     break;
                // case PixelShader:
                //     vkCmdBindVertexBuffers( cmdBuffer, range.location, range.length, itor->buffers,
                //                             itor->offsets );
                //     // [mActiveRenderEncoder setFragmentBuffers:itor->buffers
                //     //                                  offsets:itor->offsets
                //     //                                withRange:range];
                //     break;
                // case GeometryShader:
                // case HullShader:
                // case DomainShader:
                // case NumShaderTypes:
                //     break;
                // }

                ++itor;
            }
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setSamplers( uint32 slotStart, const DescriptorSetSampler *set )
    {
        FastArray<const HlmsSamplerblock *>::const_iterator itor = set->mSamplers.begin();

        for( size_t i = 0u; i < NumShaderTypes; ++i )
        {
            const uint32 numSamplersUsed = set->mShaderTypeSamplerCount[i];

            for( size_t j = 0; j < numSamplersUsed; ++j )
            {
                _setHlmsSamplerblock( static_cast<uint8>( slotStart ), *itor );
                ++slotStart;
                ++itor;
            }
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setTexturesCS( uint32 slotStart, const DescriptorSetTexture *set ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setTexturesCS( uint32 slotStart, const DescriptorSetTexture2 *set ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setSamplersCS( uint32 slotStart, const DescriptorSetSampler *set ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setUavCS( uint32 slotStart, const DescriptorSetUav *set ) {}
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
    RenderPassDescriptor *VulkanRenderSystem::createRenderPassDescriptor( void )
    {
        VulkanRenderPassDescriptor *retVal =
            OGRE_NEW VulkanRenderPassDescriptor( &mActiveDevice->mGraphicsQueue, this );
        mRenderPassDescs.insert( retVal );
        return retVal;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_hlmsComputePipelineStateObjectCreated( HlmsComputePso *newPso )
    {
        VkComputePipelineCreateInfo computeInfo;
        makeVkStruct( computeInfo, VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO );

        VulkanProgram *computeShader =
            static_cast<VulkanProgram *>( newPso->computeShader->_getBindingDelegate() );
        computeShader->fillPipelineShaderStageCi( computeInfo.stage );

        VulkanRootLayout *rootLayout = computeShader->getRootLayout();
        computeInfo.layout = rootLayout->createVulkanHandles();

        VkPipeline vulkanPso = 0u;
        VkResult result = vkCreateComputePipelines( mActiveDevice->mDevice, VK_NULL_HANDLE, 1u,
                                                    &computeInfo, 0, &vulkanPso );
        checkVkResult( result, "vkCreateComputePipelines" );

        VulkanHlmsPso *pso = new VulkanHlmsPso( vulkanPso, rootLayout );
        newPso->rsData = pso;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_hlmsComputePipelineStateObjectDestroyed( HlmsComputePso *pso )
    {
        OGRE_ASSERT_LOW( pso->rsData );
        VulkanHlmsPso *vulkanPso = static_cast<VulkanHlmsPso *>( pso->rsData );
        delayed_vkDestroyPipeline( mVaoManager, mActiveDevice->mDevice, vulkanPso->pso, 0 );
        delete vulkanPso;
        pso->rsData = 0;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_beginFrame( void ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_endFrame( void ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_notifyActiveEncoderEnded( bool callEndRenderPassDesc )
    {
        // VulkanQueue::endRenderEncoder gets called either because
        //  * Another encoder was required. Thus we interrupted and callEndRenderPassDesc = true
        //  * endRenderPassDescriptor called us. Thus callEndRenderPassDesc = false
        //  * executeRenderPassDescriptorDelayedActions called us. Thus callEndRenderPassDesc = false
        // In all cases, when callEndRenderPassDesc = true, it also implies rendering was interrupted.
        if( callEndRenderPassDesc )
            endRenderPassDescriptor( true );

        mUavRenderingDirty = true;
        mGlobalTable.setAllDirty();
        mTableDirty = true;
        mPso = 0;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_notifyActiveComputeEnded( void ) { mComputePso = 0; }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_endFrameOnce( void )
    {
        RenderSystem::_endFrameOnce();
        endRenderPassDescriptor( false );
        mActiveDevice->commitAndNextCommandBuffer( true );
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
            VkSampler textureSampler = reinterpret_cast<VkSampler>( samplerblock->mRsData );
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
        if( pso && mActiveDevice->mGraphicsQueue.getEncoderState() != VulkanQueue::EncoderGraphicsOpen )
        {
            OGRE_ASSERT_LOW(
                mInterruptedRenderCommandEncoder &&
                "Encoder can't be in EncoderGraphicsOpen at this stage if rendering was interrupted."
                " Did you call executeRenderPassDescriptorDelayedActions?" );
            executeRenderPassDescriptorDelayedActions( false );
        }

        if( mPso != pso )
        {
            VulkanRootLayout *oldRootLayout = 0;
            if( mPso )
                oldRootLayout = reinterpret_cast<VulkanHlmsPso *>( mPso->rsData )->rootLayout;

            VkCommandBuffer cmdBuffer = mActiveDevice->mGraphicsQueue.mCurrentCmdBuffer;
            OGRE_ASSERT_LOW( pso->rsData );
            VulkanHlmsPso *vulkanPso = reinterpret_cast<VulkanHlmsPso *>( pso->rsData );
            vkCmdBindPipeline( cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPso->pso );
            mPso = pso;

            if( vulkanPso && vulkanPso->rootLayout != oldRootLayout )
            {
                mGlobalTable.setAllDirty();
                mTableDirty = true;
            }
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setComputePso( const HlmsComputePso *pso )
    {
        VulkanRootLayout *oldRootLayout = 0;
        if( mComputePso )
            oldRootLayout = reinterpret_cast<VulkanHlmsPso *>( mComputePso->rsData )->rootLayout;

        mActiveDevice->mGraphicsQueue.getComputeEncoder();

        if( mComputePso != pso )
        {
            OGRE_ASSERT_LOW( pso->rsData );
            VulkanHlmsPso *vulkanPso = reinterpret_cast<VulkanHlmsPso *>( pso->rsData );

            VkCommandBuffer cmdBuffer = mActiveDevice->mGraphicsQueue.mCurrentCmdBuffer;
            vkCmdBindPipeline( cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanPso->pso );

            mComputePso = pso;

            if( vulkanPso && vulkanPso->rootLayout != oldRootLayout )
            {
                mGlobalTable.setAllDirty();
                mTableDirty = true;
            }
        }
    }
    //-------------------------------------------------------------------------
    VertexElementType VulkanRenderSystem::getColourVertexElementType( void ) const
    {
        return VET_COLOUR_ARGB;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_dispatch( const HlmsComputePso &pso )
    {
        flushRootLayout( reinterpret_cast<VulkanHlmsPso *>( mComputePso->rsData ) );

        vkCmdDispatch( mActiveDevice->mGraphicsQueue.mCurrentCmdBuffer, pso.mNumThreadGroups[0],
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

        VkCommandBuffer cmdBuffer = mActiveDevice->mGraphicsQueue.mCurrentCmdBuffer;
        vkCmdBindVertexBuffers( cmdBuffer, 0, static_cast<uint32>( numVertexBuffers ),
                                vulkanVertexBuffers, offsets );

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
    void VulkanRenderSystem::flushDescriptorState(
        VkPipelineBindPoint pipeline_bind_point, const VulkanConstBufferPacked &constBuffer,
        const size_t bindOffset, const size_t bytesToWrite,
        const unordered_map<unsigned, VulkanConstantDefinitionBindingParam>::type &shaderBindings )
    {
#if VULKAN_HOTSHOT_DISABLED
        VulkanHlmsPso *pso = mPso;

        // std::unordered_set<uint32_t> update_descriptor_sets;
        //
        // DescriptorSetLayoutArray::iterator itor = pso->descriptorSets.begin();
        // DescriptorSetLayoutArray::iterator end = pso->descriptorSets.end();
        //
        // while( itor != end )
        // {
        //     VkDescriptorSetLayout &descSet = *itor;
        //
        //     update_descriptor_sets.emplace( descSet );
        //     ++itor;
        // }
        //
        // if( update_descriptor_sets.empty() )
        //     return;

        const VulkanBufferInterface *bufferInterface =
            static_cast<const VulkanBufferInterface *>( constBuffer.getBufferInterface() );

        BindingMap<VkDescriptorBufferInfo> buffer_infos;
        BindingMap<VkDescriptorImageInfo> image_infos;

        DescriptorSetLayoutBindingArray::const_iterator bindingArraySetItor =
            pso->descriptorLayoutBindingSets.begin();
        DescriptorSetLayoutBindingArray::const_iterator bindingArraySetEnd =
            pso->descriptorLayoutBindingSets.end();

        // uint32 set = 0;

        // size_t currentOffset = bindOffset;

        while( bindingArraySetItor != bindingArraySetEnd )
        {
            const FastArray<struct VkDescriptorSetLayoutBinding> bindings = *bindingArraySetItor;

            FastArray<struct VkDescriptorSetLayoutBinding>::const_iterator bindingsItor =
                bindings.begin();
            FastArray<struct VkDescriptorSetLayoutBinding>::const_iterator bindingsItorEnd =
                bindings.end();

            // uint32 arrayElement = 0;

            while( bindingsItor != bindingsItorEnd )
            {
                const VkDescriptorSetLayoutBinding &binding = *bindingsItor;

                if( is_buffer_descriptor_type( binding.descriptorType ) )
                {
                    VkDescriptorBufferInfo buffer_info;

                    VulkanConstantDefinitionBindingParam bindingParam;
                    unordered_map<unsigned, VulkanConstantDefinitionBindingParam>::type::const_iterator
                        constantDefinitionBinding = shaderBindings.find( binding.binding );
                    if( constantDefinitionBinding == shaderBindings.end() )
                    {
                        ++bindingsItor;
                        continue;
                    }
                    else
                    {
                        bindingParam = ( *constantDefinitionBinding ).second;
                    }

                    buffer_info.buffer = bufferInterface->getVboName();
                    buffer_info.offset = bindingParam.offset;
                    buffer_info.range = bindingParam.size;

                    // currentOffset += bytesToWrite;

                    // if( is_dynamic_buffer_descriptor_type( binding_info->descriptorType ) )
                    // {
                    //     dynamic_offsets.push_back( to_u32( buffer_info.offset ) );
                    //
                    //     buffer_info.offset = 0;
                    // }

                    buffer_infos[binding.binding][0] = buffer_info;
                }
                // else if( image_view != nullptr || sampler != VK_NULL_HANDLE )
                // {
                //     // Can be null for input attachments
                //     VkDescriptorImageInfo image_info{};
                //     image_info.sampler = sampler ? sampler->get_handle() : VK_NULL_HANDLE;
                //     image_info.imageView = image_view->get_handle();
                //
                //     if( image_view != nullptr )
                //     {
                //         // Add image layout info based on descriptor type
                //         switch( binding.descriptorType )
                //         {
                //         case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                //         case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                //             if( is_depth_stencil_format( image_view->get_format() ) )
                //             {
                //                 image_info.imageLayout =
                //                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                //             }
                //             else
                //             {
                //                 image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                //             }
                //             break;
                //         case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                //             image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                //             break;
                //
                //         default:
                //             continue;
                //         }
                //     }
                //
                //     image_infos[binding.binding][0] = std::move( image_info );
                // }

                ++bindingsItor;
                // ++arrayElement;
            }

            ++bindingArraySetItor;
            // ++set;
        }

        VulkanDescriptorPool *descriptorPool =
            new VulkanDescriptorPool( mDevice->mDevice, pso->descriptorLayoutSets[0] );

        VulkanDescriptorSet *descriptorSet = new VulkanDescriptorSet(
            mDevice->mDevice, pso->descriptorLayoutSets[0], *descriptorPool, buffer_infos, image_infos );

        VkDescriptorSet descriptorSetHandle = descriptorSet->get_handle();

        // Bind descriptor set
        vkCmdBindDescriptorSets( mDevice->mGraphicsQueue.mCurrentCmdBuffer, pipeline_bind_point,
                                 pso->pipelineLayout, 0, 1, &descriptorSetHandle, 0, 0 );

        // const auto &pipeline_layout = pipeline_state.get_pipeline_layout();
        //
        //
        //
        // // Iterate over the shader sets to check if they have already been bound
        // // If they have, add the set so that the command buffer later updates it
        // for( auto &set_it : pipeline_layout.get_shader_sets() )
        // {
        //     uint32_t descriptor_set_id = set_it.first;
        //
        //     auto descriptor_set_layout_it =
        //         descriptor_set_layout_binding_state.find( descriptor_set_id );
        //
        //     if( descriptor_set_layout_it != descriptor_set_layout_binding_state.end() )
        //     {
        //         if( descriptor_set_layout_it->second->get_handle() !=
        //             pipeline_layout.get_descriptor_set_layout( descriptor_set_id ).get_handle() )
        //         {
        //             update_descriptor_sets.emplace( descriptor_set_id );
        //         }
        //     }
        // }
        //
        // // Validate that the bound descriptor set layouts exist in the pipeline layout
        // for( auto set_it = descriptor_set_layout_binding_state.begin();
        //      set_it != descriptor_set_layout_binding_state.end(); )
        // {
        //     if( !pipeline_layout.has_descriptor_set_layout( set_it->first ) )
        //     {
        //         set_it = descriptor_set_layout_binding_state.erase( set_it );
        //     }
        //     else
        //     {
        //         ++set_it;
        //     }
        // }
        //
        // // Check if a descriptor set needs to be created
        // if( resource_binding_state.is_dirty() || !update_descriptor_sets.empty() )
        // {
        //     resource_binding_state.clear_dirty();
        //
        //     // Iterate over all of the resource sets bound by the command buffer
        //     for( auto &resource_set_it : resource_binding_state.get_resource_sets() )
        //     {
        //         uint32_t descriptor_set_id = resource_set_it.first;
        //         auto &resource_set = resource_set_it.second;
        //
        //         // Don't update resource set if it's not in the update list OR its state hasn't
        //         changed if( !resource_set.is_dirty() && ( update_descriptor_sets.find(
        //         descriptor_set_id ) ==
        //                                           update_descriptor_sets.end() ) )
        //         {
        //             continue;
        //         }
        //
        //         // Clear dirty flag for resource set
        //         resource_binding_state.clear_dirty( descriptor_set_id );
        //
        //         // Skip resource set if a descriptor set layout doesn't exist for it
        //         if( !pipeline_layout.has_descriptor_set_layout( descriptor_set_id ) )
        //         {
        //             continue;
        //         }
        //
        //         auto &descriptor_set_layout =
        //             pipeline_layout.get_descriptor_set_layout( descriptor_set_id );
        //
        //         // Make descriptor set layout bound for current set
        //         descriptor_set_layout_binding_state[descriptor_set_id] = &descriptor_set_layout;
        //
        //         BindingMap<VkDescriptorBufferInfo> buffer_infos;
        //         BindingMap<VkDescriptorImageInfo> image_infos;
        //
        //         std::vector<uint32_t> dynamic_offsets;
        //
        //         // Iterate over all resource bindings
        //         for( auto &binding_it : resource_set.get_resource_bindings() )
        //         {
        //             auto binding_index = binding_it.first;
        //             auto &binding_resources = binding_it.second;
        //
        //             // Check if binding exists in the pipeline layout
        //             if( auto binding_info = descriptor_set_layout.get_layout_binding( binding_index )
        //             )
        //             {
        //                 // Iterate over all binding resources
        //                 for( auto &element_it : binding_resources )
        //                 {
        //                     auto array_element = element_it.first;
        //                     auto &resource_info = element_it.second;
        //
        //                     // Pointer references
        //                     auto &buffer = resource_info.buffer;
        //                     auto &sampler = resource_info.sampler;
        //                     auto &image_view = resource_info.image_view;
        //
        //                     // Get buffer info
        //                     if( buffer != nullptr &&
        //                         is_buffer_descriptor_type( binding_info->descriptorType ) )
        //                     {
        //                         VkDescriptorBufferInfo buffer_info{};
        //
        //                         buffer_info.buffer = resource_info.buffer->get_handle();
        //                         buffer_info.offset = resource_info.offset;
        //                         buffer_info.range = resource_info.range;
        //
        //                         if( is_dynamic_buffer_descriptor_type( binding_info->descriptorType )
        //                         )
        //                         {
        //                             dynamic_offsets.push_back( to_u32( buffer_info.offset ) );
        //
        //                             buffer_info.offset = 0;
        //                         }
        //
        //                         buffer_infos[binding_index][array_element] = buffer_info;
        //                     }
        //
        //                     // Get image info
        //                     else if( image_view != nullptr || sampler != VK_NULL_HANDLE )
        //                     {
        //                         // Can be null for input attachments
        //                         VkDescriptorImageInfo image_info{};
        //                         image_info.sampler = sampler ? sampler->get_handle() : VK_NULL_HANDLE;
        //                         image_info.imageView = image_view->get_handle();
        //
        //                         if( image_view != nullptr )
        //                         {
        //                             // Add image layout info based on descriptor type
        //                             switch( binding_info->descriptorType )
        //                             {
        //                             case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        //                             case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        //                                 if( is_depth_stencil_format( image_view->get_format() ) )
        //                                 {
        //                                     image_info.imageLayout =
        //                                         VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        //                                 }
        //                                 else
        //                                 {
        //                                     image_info.imageLayout =
        //                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        //                                 }
        //                                 break;
        //                             case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        //                                 image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        //                                 break;
        //
        //                             default:
        //                                 continue;
        //                             }
        //                         }
        //
        //                         image_infos[binding_index][array_element] = std::move( image_info );
        //                     }
        //                 }
        //             }
        //         }
        //
        //         auto &descriptor_set = command_pool.get_render_frame()->request_descriptor_set(
        //             descriptor_set_layout, buffer_infos, image_infos, command_pool.get_thread_index()
        //             );
        //
        //         VkDescriptorSet descriptor_set_handle = descriptor_set.get_handle();
        //
        //         // Bind descriptor set
        //         vkCmdBindDescriptorSets( get_handle(), pipeline_bind_point,
        //         pipeline_layout.get_handle(),
        //                                  descriptor_set_id, 1, &descriptor_set_handle,
        //                                  to_u32( dynamic_offsets.size() ), dynamic_offsets.data() );
        //     }
        // }
#endif
    }
    //-------------------------------------------------------------------------
    VulkanHlmsPso *lastPso = 0;
    void VulkanRenderSystem::bindDescriptorSet() const
    {
#if VULKAN_HOTSHOT_DISABLED
        VulkanVaoManager *vaoManager = static_cast<VulkanVaoManager *>( mVaoManager );

        BindingMap<VkDescriptorBufferInfo> buffer_infos;
        BindingMap<VkBufferView> buffer_views;
        const BindingMap<VkDescriptorImageInfo> &image_infos = mImageInfo;

        const vector<VulkanConstBufferPacked *>::type &constBuffers = vaoManager->getConstBuffers();
        vector<VulkanConstBufferPacked *>::type::const_iterator constBuffersIt = constBuffers.begin();
        vector<VulkanConstBufferPacked *>::type::const_iterator constBuffersEnd = constBuffers.end();

        while( constBuffersIt != constBuffersEnd )
        {
            VulkanConstBufferPacked *const constBuffer = *constBuffersIt;
            if( constBuffer->isDirty() )
            {
                const VkDescriptorBufferInfo &bufferInfo = constBuffer->getBufferInfo();
                uint16 binding = constBuffer->getCurrentBinding();
                buffer_infos[binding][0] = bufferInfo;
                constBuffer->resetDirty();
            }
            ++constBuffersIt;
        }

        const vector<VulkanTexBufferPacked *>::type &texBuffers = vaoManager->getTexBuffersPacked();
        vector<VulkanTexBufferPacked *>::type::const_iterator texBuffersIt = texBuffers.begin();
        vector<VulkanTexBufferPacked *>::type::const_iterator texBuffersEnd = texBuffers.end();

        while( texBuffersIt != texBuffersEnd )
        {
            VulkanTexBufferPacked *const texBuffer = *texBuffersIt;
            if( texBuffer->isDirty() )
            {
                VkBufferView bufferView = texBuffer->getBufferView();
                uint16 binding = texBuffer->getCurrentBinding();
                buffer_views[binding][0] = bufferView;
                texBuffer->resetDirty();
            }
            ++texBuffersIt;
        }
        if( lastPso == mPso )
            return;

        VulkanHlmsPso *pso = mPso;
        lastPso = mPso;

        VulkanDescriptorPool *descriptorPool =
            new VulkanDescriptorPool( mDevice->mDevice, pso->descriptorLayoutSets[0] );

        VulkanDescriptorSet *descriptorSet =
            new VulkanDescriptorSet( mDevice->mDevice, pso->descriptorLayoutSets[0], *descriptorPool,
                                     buffer_infos, image_infos, buffer_views );

        VkDescriptorSet descriptorSetHandle = descriptorSet->get_handle();

        // Bind descriptor set
        vkCmdBindDescriptorSets( mDevice->mGraphicsQueue.mCurrentCmdBuffer,
                                 VK_PIPELINE_BIND_POINT_GRAPHICS, pso->pipelineLayout, 0, 1,
                                 &descriptorSetHandle, 0, 0 );
#endif
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::flushRootLayout( VulkanHlmsPso *pso )
    {
        if( !mTableDirty )
            return;

        VulkanVaoManager *vaoManager = static_cast<VulkanVaoManager *>( mVaoManager );
        VulkanRootLayout *rootLayout = pso->rootLayout;
        rootLayout->bind( mDevice, vaoManager, mGlobalTable );
        mTableDirty = false;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_render( const CbDrawCallIndexed *cmd )
    {
        flushRootLayout( reinterpret_cast<VulkanHlmsPso *>( mPso->rsData ) );

        VkCommandBuffer cmdBuffer = mActiveDevice->mGraphicsQueue.mCurrentCmdBuffer;
        vkCmdDrawIndexedIndirect( cmdBuffer, mIndirectBuffer,
                                  reinterpret_cast<VkDeviceSize>( cmd->indirectBufferOffset ),
                                  cmd->numDraws, sizeof( CbDrawIndexed ) );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_render( const CbDrawCallStrip *cmd )
    {
        flushRootLayout( reinterpret_cast<VulkanHlmsPso *>( mPso->rsData ) );

        VkCommandBuffer cmdBuffer = mActiveDevice->mGraphicsQueue.mCurrentCmdBuffer;
        vkCmdDrawIndirect( cmdBuffer, mIndirectBuffer,
                           reinterpret_cast<VkDeviceSize>( cmd->indirectBufferOffset ), cmd->numDraws,
                           sizeof( CbDrawIndexed ) );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_renderEmulated( const CbDrawCallIndexed *cmd )
    {
        flushRootLayout( reinterpret_cast<VulkanHlmsPso *>( mPso->rsData ) );

        CbDrawIndexed *drawCmd = reinterpret_cast<CbDrawIndexed *>( mSwIndirectBufferPtr +
                                                                    (size_t)cmd->indirectBufferOffset );

        VkCommandBuffer cmdBuffer = mActiveDevice->mGraphicsQueue.mCurrentCmdBuffer;

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
        flushRootLayout( reinterpret_cast<VulkanHlmsPso *>( mPso->rsData ) );

        CbDrawStrip *drawCmd =
            reinterpret_cast<CbDrawStrip *>( mSwIndirectBufferPtr + (size_t)cmd->indirectBufferOffset );

        VkCommandBuffer cmdBuffer = mActiveDevice->mGraphicsQueue.mCurrentCmdBuffer;

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

        VkCommandBuffer cmdBuffer = mActiveDevice->mGraphicsQueue.mCurrentCmdBuffer;

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
        flushRootLayout( reinterpret_cast<VulkanHlmsPso *>( mPso->rsData ) );

        VkCommandBuffer cmdBuffer = mActiveDevice->mGraphicsQueue.mCurrentCmdBuffer;
        vkCmdDrawIndexed( cmdBuffer, cmd->primCount, cmd->instanceCount, cmd->firstVertexIndex,
                          (int32_t)mCurrentVertexBuffer->vertexStart, cmd->baseInstance );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_render( const v1::CbDrawCallStrip *cmd )
    {
        flushRootLayout( reinterpret_cast<VulkanHlmsPso *>( mPso->rsData ) );

        VkCommandBuffer cmdBuffer = mActiveDevice->mGraphicsQueue.mCurrentCmdBuffer;
        vkCmdDraw( cmdBuffer, cmd->primCount, cmd->instanceCount, cmd->firstVertexIndex,
                   cmd->baseInstance );
    }

    void VulkanRenderSystem::_render( const v1::RenderOperation &op )
    {
        flushRootLayout( reinterpret_cast<VulkanHlmsPso *>( mPso->rsData ) );

        // Call super class.
        RenderSystem::_render( op );

        const size_t numberOfInstances = op.numberOfInstances;
        const bool hasInstanceData = mCurrentVertexBuffer->vertexBufferBinding->getHasInstanceData();

        VkCommandBuffer cmdBuffer = mActiveDevice->mGraphicsQueue.mCurrentCmdBuffer;

        // Render to screen!
        if( op.useIndexes )
        {
            do
            {
                // Update derived depth bias.
                if( mDerivedDepthBias && mCurrentPassIterationNum > 0 )
                {
                    const float biasSign = mReverseDepth ? 1.0f : -1.0f;
                    vkCmdSetDepthBias( cmdBuffer,
                                       ( mDerivedDepthBiasBase +
                                         mDerivedDepthBiasMultiplier * mCurrentPassIterationNum ) *
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
                    vkCmdSetDepthBias( cmdBuffer,
                                       ( mDerivedDepthBiasBase +
                                         mDerivedDepthBiasMultiplier * mCurrentPassIterationNum ) *
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

        size_t bytesToWrite = shader->getBufferRequiredSize();
        if( shader && bytesToWrite > 0 )
        {
            if( mCurrentAutoParamsBufferSpaceLeft < bytesToWrite )
            {
                if( mAutoParamsBufferIdx >= mAutoParamsBuffer.size() )
                {
                    ConstBufferPacked *constBuffer =
                        mVaoManager->createConstBuffer( std::max<size_t>( 512u * 1024u, bytesToWrite ),
                                                        BT_DYNAMIC_PERSISTENT, 0, false );
                    mAutoParamsBuffer.push_back( constBuffer );
                }

                ConstBufferPacked *constBuffer = mAutoParamsBuffer[mAutoParamsBufferIdx];
                mCurrentAutoParamsBufferPtr =
                    reinterpret_cast<uint8 *>( constBuffer->map( 0, constBuffer->getNumElements() ) );
                mCurrentAutoParamsBufferSpaceLeft = constBuffer->getTotalSizeBytes();

                ++mAutoParamsBufferIdx;
            }

            shader->updateBuffers( params, mCurrentAutoParamsBufferPtr );

            assert( dynamic_cast<VulkanConstBufferPacked *>(
                mAutoParamsBuffer[mAutoParamsBufferIdx - 1u] ) );

            VulkanConstBufferPacked *constBuffer =
                static_cast<VulkanConstBufferPacked *>( mAutoParamsBuffer[mAutoParamsBufferIdx - 1u] );
            const size_t bindOffset =
                constBuffer->getTotalSizeBytes() - mCurrentAutoParamsBufferSpaceLeft;

            constBuffer->bindAsParamBuffer( gptype, bindOffset );

            mCurrentAutoParamsBufferPtr += bytesToWrite;

            const uint8 *oldBufferPos = mCurrentAutoParamsBufferPtr;
            mCurrentAutoParamsBufferPtr = reinterpret_cast<uint8 *>(
                alignToNextMultiple( reinterpret_cast<uintptr_t>( mCurrentAutoParamsBufferPtr ),
                                     mVaoManager->getConstBufferAlignment() ) );
            bytesToWrite += ( size_t )( mCurrentAutoParamsBufferPtr - oldBufferPos );

            // We know that bytesToWrite <= mCurrentAutoParamsBufferSpaceLeft, but that was
            // before padding. After padding this may no longer hold true.
            mCurrentAutoParamsBufferSpaceLeft -=
                std::min( mCurrentAutoParamsBufferSpaceLeft, bytesToWrite );
        }
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
    Real VulkanRenderSystem::getHorizontalTexelOffset( void ) { return 0.0f; }
    //-------------------------------------------------------------------------
    Real VulkanRenderSystem::getVerticalTexelOffset( void ) { return 0.0f; }
    //-------------------------------------------------------------------------
    Real VulkanRenderSystem::getMinimumDepthInputValue( void ) { return 0.0f; }
    //-------------------------------------------------------------------------
    Real VulkanRenderSystem::getMaximumDepthInputValue( void ) { return 1.0f; }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::preExtraThreadsStarted() {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::postExtraThreadsStarted() {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::registerThread() {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::unregisterThread() {}
    //-------------------------------------------------------------------------
    const PixelFormatToShaderType *VulkanRenderSystem::getPixelFormatToShaderType( void ) const
    {
        return &mPixelFormatToShaderType;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::flushCommands( void ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::beginProfileEvent( const String &eventName ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::endProfileEvent( void ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::markProfileEvent( const String &event ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::initGPUProfiling( void ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::deinitGPUProfiling( void ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::beginGPUSampleProfile( const String &name, uint32 *hashCache ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::endGPUSampleProfile( const String &name ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::setClipPlanesImpl( const PlaneList &clipPlanes ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::initialiseFromRenderSystemCapabilities( RenderSystemCapabilities *caps,
                                                                     Window *primary )
    {
        mShaderManager = OGRE_NEW VulkanGpuProgramManager( mActiveDevice );
        mVulkanProgramFactory0 = OGRE_NEW VulkanProgramFactory( mActiveDevice, "glslvk", true );
        mVulkanProgramFactory1 = OGRE_NEW VulkanProgramFactory( mActiveDevice, "glsl", false );
        mVulkanProgramFactory2 = OGRE_NEW VulkanProgramFactory( mActiveDevice, "hlslvk", false );
        mVulkanProgramFactory3 = OGRE_NEW VulkanProgramFactory( mActiveDevice, "hlsl", false );

        HighLevelGpuProgramManager::getSingleton().addFactory( mVulkanProgramFactory0 );
        // HighLevelGpuProgramManager::getSingleton().addFactory( mVulkanProgramFactory1 );
        HighLevelGpuProgramManager::getSingleton().addFactory( mVulkanProgramFactory2 );
        // HighLevelGpuProgramManager::getSingleton().addFactory( mVulkanProgramFactory3 );

        mCache = OGRE_NEW VulkanCache( mActiveDevice );

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
            mActiveDevice->commitAndNextCommandBuffer( false );
        }

        const int oldWidth = mCurrentRenderViewport[0].getActualWidth();
        const int oldHeight = mCurrentRenderViewport[0].getActualHeight();
        const int oldX = mCurrentRenderViewport[0].getActualLeft();
        const int oldY = mCurrentRenderViewport[0].getActualTop();

        VulkanRenderPassDescriptor *currPassDesc =
            static_cast<VulkanRenderPassDescriptor *>( mCurrentRenderPassDescriptor );

        RenderSystem::beginRenderPassDescriptor( desc, anyTarget, mipLevel, viewportSizes, scissors,
                                                 numViewports, overlaysEnabled, warnIfRtvWasFlushed );

        // Calculate the new "lower-left" corner of the viewport to compare with the old one
        const int w = mCurrentRenderViewport[0].getActualWidth();
        const int h = mCurrentRenderViewport[0].getActualHeight();
        const int x = mCurrentRenderViewport[0].getActualLeft();
        const int y = mCurrentRenderViewport[0].getActualTop();

        const bool vpChanged =
            oldX != x || oldY != y || oldWidth != w || oldHeight != h || numViewports > 1u;

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
    void VulkanRenderSystem::executeRenderPassDescriptorDelayedActions( void )
    {
        executeRenderPassDescriptorDelayedActions( true );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::executeRenderPassDescriptorDelayedActions( bool officialCall )
    {
        if( officialCall )
            mInterruptedRenderCommandEncoder = false;

        const bool wasGraphicsOpen =
            mActiveDevice->mGraphicsQueue.getEncoderState() != VulkanQueue::EncoderGraphicsOpen;

        if( mEntriesToFlush )
        {
            mActiveDevice->mGraphicsQueue.endAllEncoders( false );

            VulkanRenderPassDescriptor *newPassDesc =
                static_cast<VulkanRenderPassDescriptor *>( mCurrentRenderPassDescriptor );

            newPassDesc->performLoadActions( mInterruptedRenderCommandEncoder );
        }

        // This is a new command buffer / encoder. State needs to be set again
        if( mEntriesToFlush || !wasGraphicsOpen )
        {
            mActiveDevice->mGraphicsQueue.getGraphicsEncoder();

            VulkanVaoManager *vaoManager = static_cast<VulkanVaoManager *>( mVaoManager );
            vaoManager->bindDrawIdVertexBuffer( mActiveDevice->mGraphicsQueue.mCurrentCmdBuffer );

#if VULKAN_DISABLED
            [mActiveRenderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];

            if( mStencilEnabled )
                [mActiveRenderEncoder setStencilReferenceValue:mStencilRefValue];
#endif
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
                vkVp[i].x = mCurrentRenderViewport[i].getActualLeft();
                vkVp[i].y = mCurrentRenderViewport[i].getActualTop();
                vkVp[i].width = mCurrentRenderViewport[i].getActualWidth();
                vkVp[i].height = mCurrentRenderViewport[i].getActualHeight();
                vkVp[i].minDepth = 0;
                vkVp[i].maxDepth = 1;
            }

            vkCmdSetViewport( mActiveDevice->mGraphicsQueue.mCurrentCmdBuffer, 0u, numViewports, vkVp );
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
            }

            vkCmdSetScissor( mActiveDevice->mGraphicsQueue.mCurrentCmdBuffer, 0u, numViewports,
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
    void VulkanRenderSystem::endRenderPassDescriptor( void ) { endRenderPassDescriptor( false ); }
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
        };
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
                ++numColourAttachments;

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
                    ++attachmentIdx;

                    resolveAttachRefs[numColourAttachments].attachment = attachmentIdx;
                    resolveAttachRefs[numColourAttachments].layout =
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    ++numColourAttachments;
                }
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
        case ResourceLayout::CopyEnd:
            return 0;
        case ResourceLayout::Undefined:
        case ResourceLayout::NumResourceLayouts:
            return 0;
        }
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
    void VulkanRenderSystem::executeResourceTransition( const ResourceTransitionArray &rstCollection )
    {
        if( rstCollection.empty() )
            return;

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
                OGRE_ASSERT_MEDIUM( itor->oldLayout != ResourceLayout::CopyEnd &&
                                    "ResourceLayout::CopyEnd is never in oldLayout" );

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
                        imageBarrier.srcAccessMask = VulkanMappings::getAccessFlags(
                            itor->oldLayout, itor->oldAccess, texture, false );
                    }

                    imageBarrier.dstAccessMask = VulkanMappings::getAccessFlags(
                        itor->newLayout, itor->newAccess, texture, true );

                    if( itor->oldLayout != ResourceLayout::Texture &&
                        itor->oldLayout != ResourceLayout::Uav )
                    {
                        srcStage |= toVkPipelineStageFlags( itor->oldLayout, bIsDepth );
                    }
                    else
                        srcStage |= ogreToVkStageFlags( itor->oldStageMask );
                }

                if( itor->newLayout != ResourceLayout::CopyEnd )
                {
                    if( itor->newLayout != ResourceLayout::Texture &&
                        itor->newLayout != ResourceLayout::Uav )
                    {
                        dstStage |= toVkPipelineStageFlags( itor->newLayout, bIsDepth );
                    }
                    else
                        dstStage |= ogreToVkStageFlags( itor->newStageMask );
                }

                texture->mCurrLayout = imageBarrier.newLayout;

                mImageBarriers.push_back( imageBarrier );
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
            memBarrier.srcAccessMask = bufferSrcBarrierBits;
            memBarrier.dstAccessMask = bufferDstBarrierBits;
            numMemBarriers = 1u;
        }

        if( srcStage == 0 )
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        if( dstStage == 0 )
            dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        mActiveDevice->mGraphicsQueue.endAllEncoders();

        vkCmdPipelineBarrier( mActiveDevice->mGraphicsQueue.mCurrentCmdBuffer, srcStage, dstStage, 0,
                              numMemBarriers, &memBarrier, 0u, 0,
                              static_cast<uint32>( mImageBarriers.size() ), mImageBarriers.begin() );
        mImageBarriers.clear();
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_hlmsPipelineStateObjectCreated( HlmsPso *newPso )
    {
        size_t numShaderStages = 0u;
        VkPipelineShaderStageCreateInfo shaderStages[NumShaderTypes];

        VulkanRootLayout *rootLayout = 0;

        VulkanProgram *vertexShader = 0;
        VulkanProgram *pixelShader = 0;

        if( !newPso->vertexShader.isNull() )
        {
            vertexShader = static_cast<VulkanProgram *>( newPso->vertexShader->_getBindingDelegate() );
            vertexShader->fillPipelineShaderStageCi( shaderStages[numShaderStages++] );
            rootLayout = VulkanRootLayout::findBest( rootLayout, vertexShader->getRootLayout() );
        }

        if( !newPso->geometryShader.isNull() )
        {
            VulkanProgram *shader =
                static_cast<VulkanProgram *>( newPso->geometryShader->_getBindingDelegate() );
            shader->fillPipelineShaderStageCi( shaderStages[numShaderStages++] );
            rootLayout = VulkanRootLayout::findBest( rootLayout, shader->getRootLayout() );
        }

        if( !newPso->tesselationHullShader.isNull() )
        {
            VulkanProgram *shader =
                static_cast<VulkanProgram *>( newPso->tesselationHullShader->_getBindingDelegate() );
            shader->fillPipelineShaderStageCi( shaderStages[numShaderStages++] );
            rootLayout = VulkanRootLayout::findBest( rootLayout, shader->getRootLayout() );
        }

        if( !newPso->tesselationDomainShader.isNull() )
        {
            VulkanProgram *shader =
                static_cast<VulkanProgram *>( newPso->tesselationDomainShader->_getBindingDelegate() );
            shader->fillPipelineShaderStageCi( shaderStages[numShaderStages++] );
            rootLayout = VulkanRootLayout::findBest( rootLayout, shader->getRootLayout() );
        }

        if( !newPso->pixelShader.isNull() )
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
            if( newPso->pixelShader )
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

        if( !newPso->vertexShader.isNull() )
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

        VkPipelineViewportStateCreateInfo viewportStateCi;
        makeVkStruct( viewportStateCi, VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO );
        TODO_addVpCount_to_passpso;
        viewportStateCi.viewportCount = 1u;
        viewportStateCi.scissorCount = 1u;

        VkPipelineRasterizationStateCreateInfo rasterState;
        makeVkStruct( rasterState, VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO );
        rasterState.polygonMode = VulkanMappings::get( newPso->macroblock->mPolygonMode );
        rasterState.cullMode = VulkanMappings::get( newPso->macroblock->mCullMode );
        rasterState.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterState.depthClampEnable = newPso->macroblock->mDepthClamp;
        rasterState.depthBiasEnable = newPso->macroblock->mDepthBiasConstant != 0.0f;
        rasterState.depthBiasConstantFactor = newPso->macroblock->mDepthBiasConstant;
        rasterState.depthBiasClamp = 0.0f;
        rasterState.depthBiasSlopeFactor = newPso->macroblock->mDepthBiasSlopeScale;
        rasterState.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo mssCi;
        makeVkStruct( mssCi, VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO );
        mssCi.rasterizationSamples =
            static_cast<VkSampleCountFlagBits>( newPso->pass.sampleDescription.getColourSamples() );
        mssCi.alphaToCoverageEnable = newPso->blendblock->mAlphaToCoverageEnabled;

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
        blendStates[0].colorWriteMask = newPso->blendblock->mBlendChannelMask;

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

        pipeline.layout = layout;
        pipeline.stageCount = static_cast<uint32>( numShaderStages );
        pipeline.pStages = shaderStages;
        pipeline.pVertexInputState = &vertexFormatCi;
        pipeline.pInputAssemblyState = &inputAssemblyCi;
        pipeline.pTessellationState = &tessStateCi;
        pipeline.pViewportState = &viewportStateCi;
        pipeline.pRasterizationState = &rasterState;
        pipeline.pMultisampleState = &mssCi;
        pipeline.pDepthStencilState = &depthStencilStateCi;
        pipeline.pColorBlendState = &blendStateCi;
        pipeline.pDynamicState = &dynamicStateCi;
        pipeline.renderPass = renderPass;

        VkPipeline vulkanPso = 0;
        VkResult result = vkCreateGraphicsPipelines( mActiveDevice->mDevice, VK_NULL_HANDLE, 1u,
                                                     &pipeline, 0, &vulkanPso );
        checkVkResult( result, "vkCreateGraphicsPipelines" );

        VulkanHlmsPso *pso = new VulkanHlmsPso( vulkanPso, rootLayout );
        newPso->rsData = pso;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_hlmsPipelineStateObjectDestroyed( HlmsPso *pso )
    {
        OGRE_ASSERT_LOW( pso->rsData );
        VulkanHlmsPso *vulkanPso = static_cast<VulkanHlmsPso *>( pso->rsData );
        delayed_vkDestroyPipeline( mVaoManager, mActiveDevice->mDevice, vulkanPso->pso, 0 );
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
        samplerDescriptor.anisotropyEnable = newBlock->mMaxAnisotropy > 0.0f ? VK_TRUE : VK_FALSE;
        float maxAllowedAnisotropy = mActiveDevice->mDeviceProperties.limits.maxSamplerAnisotropy;
        samplerDescriptor.maxAnisotropy = newBlock->mMaxAnisotropy > maxAllowedAnisotropy
                                              ? maxAllowedAnisotropy
                                              : newBlock->mMaxAnisotropy;
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
        VkResult result =
            vkCreateSampler( mActiveDevice->mDevice, &samplerDescriptor, 0, &textureSampler );
        checkVkResult( result, "vkCreateSampler" );

        newBlock->mRsData = textureSampler;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_hlmsSamplerblockDestroyed( HlmsSamplerblock *block )
    {
        assert( block->mRsData );
        VkSampler textureSampler = static_cast<VkSampler>( block->mRsData );
        delayed_vkDestroySampler( mVaoManager, mActiveDevice->mDevice, textureSampler, 0 );
    }
    //-------------------------------------------------------------------------
    template <typename TDescriptorSetTexture, typename TTexSlot, typename TBufferPacked>
    void VulkanRenderSystem::_descriptorSetTextureCreated( TDescriptorSetTexture *newSet,
                                                           const FastArray<TTexSlot> &texContainer,
                                                           uint16 *shaderTypeTexCount )
    {
        VulkanDescriptorSetTexture *vulkanSet = new VulkanDescriptorSetTexture();

        vulkanSet->numTextureViews = 0;

        size_t numBuffers = 0;
        size_t numTextures = 0;

        size_t numBufferRanges = 0;
        size_t numTextureRanges = 0;

        bool needsNewTexRange = true;
        bool needsNewBufferRange = true;

        ShaderType shaderType = VertexShader;
        size_t numProcessedSlots = 0;

        typename FastArray<TTexSlot>::const_iterator itor = texContainer.begin();
        typename FastArray<TTexSlot>::const_iterator end = texContainer.end();

        while( itor != end )
        {
            if( shaderTypeTexCount )
            {
                // We need to break the ranges if we crossed stages
                while( shaderType <= PixelShader && numProcessedSlots >= shaderTypeTexCount[shaderType] )
                {
                    numProcessedSlots = 0;
                    shaderType = static_cast<ShaderType>( shaderType + 1u );
                    needsNewTexRange = true;
                    needsNewBufferRange = true;
                }
            }

            if( itor->isTexture() )
            {
                needsNewBufferRange = true;
                if( needsNewTexRange )
                {
                    ++numTextureRanges;
                    needsNewTexRange = false;
                }

                const typename TDescriptorSetTexture::TextureSlot &texSlot = itor->getTexture();
                if( texSlot.needsDifferentView() )
                    ++vulkanSet->numTextureViews;
                ++numTextures;
            }
            else
            {
                needsNewTexRange = true;
                if( needsNewBufferRange )
                {
                    ++numBufferRanges;
                    needsNewBufferRange = false;
                }

                ++numBuffers;
            }
            ++numProcessedSlots;
            ++itor;
        }

        // Create two contiguous arrays of texture and buffers, but we'll split
        // it into regions as a buffer could be in the middle of two textures.
        VkImageView *srvList = 0;
        VkBuffer *buffers = 0;
        VkDeviceSize *offsets = 0;

        if( vulkanSet->numTextureViews > 0 )
        {
            // Create a third array to hold the strong reference
            // to the reinterpreted versions of textures.
            // Must be init to 0 before ARC sees it.
            void *textureViews = OGRE_MALLOC_SIMD( sizeof( VkImageView * ) * vulkanSet->numTextureViews,
                                                   MEMCATEGORY_RENDERSYS );
            memset( textureViews, 0, sizeof( VkImageView * ) * vulkanSet->numTextureViews );
            vulkanSet->textureViews = (VkImageView *)textureViews;
        }
        if( numTextures > 0 )
        {
            srvList = (VkImageView *)OGRE_MALLOC_SIMD( sizeof( VkImageView * ) * numTextures,
                                                       MEMCATEGORY_RENDERSYS );
        }
        if( numBuffers > 0 )
        {
            buffers =
                (VkBuffer *)OGRE_MALLOC_SIMD( sizeof( VkBuffer * ) * numBuffers, MEMCATEGORY_RENDERSYS );
            offsets = (VkDeviceSize *)OGRE_MALLOC_SIMD( sizeof( VkDeviceSize ) * numBuffers,
                                                        MEMCATEGORY_RENDERSYS );
        }

        vulkanSet->textures.reserve( numTextureRanges );
        vulkanSet->buffers.reserve( numBufferRanges );

        needsNewTexRange = true;
        needsNewBufferRange = true;

        shaderType = VertexShader;
        numProcessedSlots = 0;

        size_t texViewIndex = 0;

        itor = texContainer.begin();
        end = texContainer.end();

        while( itor != end )
        {
            if( shaderTypeTexCount )
            {
                // We need to break the ranges if we crossed stages
                while( shaderType <= PixelShader && numProcessedSlots >= shaderTypeTexCount[shaderType] )
                {
                    numProcessedSlots = 0;
                    shaderType = static_cast<ShaderType>( shaderType + 1u );
                    needsNewTexRange = true;
                    needsNewBufferRange = true;
                }
            }

            if( itor->isTexture() )
            {
                needsNewBufferRange = true;
                if( needsNewTexRange )
                {
                    vulkanSet->textures.push_back( VulkanTexRegion() );
                    VulkanTexRegion &texRegion = vulkanSet->textures.back();
                    texRegion.textures = srvList;
                    texRegion.shaderType = shaderType;
                    texRegion.range.location = itor - texContainer.begin();
                    texRegion.range.length = 0;
                    needsNewTexRange = false;
                }

                const typename TDescriptorSetTexture::TextureSlot &texSlot = itor->getTexture();

                assert( dynamic_cast<VulkanTextureGpu *>( texSlot.texture ) );
                VulkanTextureGpu *vulkanTex = static_cast<VulkanTextureGpu *>( texSlot.texture );
                VkImageView srv = vulkanTex->getDefaultDisplaySrv();

                if( texSlot.needsDifferentView() )
                {
                    vulkanSet->textureViews[texViewIndex] = vulkanTex->createView( texSlot );
                    srv = vulkanSet->textureViews[texViewIndex];
                    ++texViewIndex;
                }

                VulkanTexRegion &texRegion = vulkanSet->textures.back();
                *srvList = srv;
                ++texRegion.range.length;

                ++srvList;
            }
            else
            {
                needsNewTexRange = true;
                if( needsNewBufferRange )
                {
                    vulkanSet->buffers.push_back( VulkanBufferRegion() );
                    VulkanBufferRegion &bufferRegion = vulkanSet->buffers.back();
                    bufferRegion.buffers = buffers;
                    bufferRegion.offsets = offsets;
                    bufferRegion.shaderType = shaderType;
                    bufferRegion.range.location = itor - texContainer.begin();
                    bufferRegion.range.length = VK_WHOLE_SIZE;
                    needsNewBufferRange = false;
                }

                const typename TDescriptorSetTexture::BufferSlot &bufferSlot = itor->getBuffer();

                assert( dynamic_cast<TBufferPacked *>( bufferSlot.buffer ) );
                TBufferPacked *vulkanBuf = static_cast<TBufferPacked *>( bufferSlot.buffer );

                VulkanBufferRegion &bufferRegion = vulkanSet->buffers.back();
                vulkanBuf->bindBufferForDescriptor( buffers, offsets, bufferSlot.offset );
                ++bufferRegion.range.length;

                ++buffers;
                ++offsets;
            }
            ++numProcessedSlots;
            ++itor;
        }

        newSet->mRsData = vulkanSet;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::destroyVulkanDescriptorSetTexture( VulkanDescriptorSetTexture *vulkanSet )
    {
        const size_t numTextureViews = vulkanSet->numTextureViews;
        for( size_t i = 0; i < numTextureViews; ++i )
            vulkanSet->textureViews[i] = 0;  // Let ARC free these pointers

        if( numTextureViews )
        {
            OGRE_FREE_SIMD( vulkanSet->textureViews, MEMCATEGORY_RENDERSYS );
            vulkanSet->textureViews = 0;
        }

        if( !vulkanSet->textures.empty() )
        {
            VulkanTexRegion &texRegion = vulkanSet->textures.front();
            OGRE_FREE_SIMD( texRegion.textures, MEMCATEGORY_RENDERSYS );
        }

        if( !vulkanSet->buffers.empty() )
        {
            VulkanBufferRegion &bufferRegion = vulkanSet->buffers.front();
            OGRE_FREE_SIMD( bufferRegion.buffers, MEMCATEGORY_RENDERSYS );
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_descriptorSetTextureCreated( DescriptorSetTexture *newSet )
    {
        Log *defaultLog = LogManager::getSingleton().getDefaultLog();
        if( defaultLog )
        {
            defaultLog->logMessage( String( " _descriptorSetTextureCreated " ) );
        }
    }

    void VulkanRenderSystem::_descriptorSetTextureDestroyed( DescriptorSetTexture *set ) {}

    void VulkanRenderSystem::_descriptorSetTexture2Created( DescriptorSetTexture2 *newSet )
    {
        Log *defaultLog = LogManager::getSingleton().getDefaultLog();
        if( defaultLog )
        {
            defaultLog->logMessage( String( " _descriptorSetTexture2Created " ) );
        }

        /*_descriptorSetTextureCreated<DescriptorSetTexture2, DescriptorSetTexture2::Slot,
                                     VulkanTexBufferPacked>( newSet, newSet->mTextures,
                                                             newSet->mShaderTypeTexCount );*/
    }

    void VulkanRenderSystem::_descriptorSetTexture2Destroyed( DescriptorSetTexture2 *set )
    {
        assert( set->mRsData );

        VulkanDescriptorSetTexture *metalSet =
            reinterpret_cast<VulkanDescriptorSetTexture *>( set->mRsData );

        destroyVulkanDescriptorSetTexture( metalSet );
        delete metalSet;

        set->mRsData = 0;
    }

    void VulkanRenderSystem::_descriptorSetSamplerCreated( DescriptorSetSampler *newSet )
    {
        Log *defaultLog = LogManager::getSingleton().getDefaultLog();
        if( defaultLog )
        {
            defaultLog->logMessage( String( " _descriptorSetSamplerCreated " ) );
        }
    }

    void VulkanRenderSystem::_descriptorSetSamplerDestroyed( DescriptorSetSampler *set ) {}
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
    bool VulkanRenderSystem::isSameLayout( ResourceLayout::Layout a, ResourceLayout::Layout b,
                                           const TextureGpu *texture ) const
    {
        return VulkanMappings::get( a, texture ) == VulkanMappings::get( b, texture );
    }
}  // namespace Ogre
