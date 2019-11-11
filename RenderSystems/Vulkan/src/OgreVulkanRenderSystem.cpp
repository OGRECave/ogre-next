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
#include "OgreVulkanDescriptors.h"
#include "OgreVulkanDevice.h"
#include "OgreVulkanGpuProgramManager.h"
#include "OgreVulkanMappings.h"
#include "OgreVulkanProgram.h"
#include "OgreVulkanRenderPassDescriptor.h"
#include "OgreVulkanTextureGpuManager.h"
#include "OgreVulkanUtils.h"
#include "OgreVulkanWindow.h"
#include "Vao/OgreVulkanVaoManager.h"

#include "OgreDefaultHardwareBufferManager.h"

#include "Windowing/X11/OgreVulkanXcbWindow.h"

#define TODO_check_layers_exist

#define TODO_vertex_format
#define TODO_addVpCount_to_passpso
#define TODO_renderPass
#define TODO_psoCaches

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
        // return false;
        return true;
    }

    //-------------------------------------------------------------------------
    VulkanRenderSystem::VulkanRenderSystem() :
        RenderSystem(),
        mInitialized( false ),
        mHardwareBufferManager( 0 ),
        mShaderManager( 0 ),
        mVkInstance( 0 ),
        mActiveDevice( 0 ),
        mDevice( 0 ),
        mEntriesToFlush( 0u ),
        mVpChanged( false ),
        CreateDebugReportCallback( 0 ),
        DestroyDebugReportCallback( 0 ),
        mDebugReportCallback( 0 )
    {
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::shutdown( void )
    {
        OGRE_DELETE mShaderManager;
        mShaderManager = 0;

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

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties( mActiveDevice->mPhysicalDevice, &properties );

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
        VulkanWindow *win =
            OGRE_NEW VulkanXcbWindow( reqInstanceExtensions, name, width, height, fullScreen );
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

            FastArray<const char *> deviceExtensions;
            mDevice->createDevice( deviceExtensions, 0u, 0u );

            mHardwareBufferManager = new v1::DefaultHardwareBufferManager();
            VulkanVaoManager *vaoManager = OGRE_NEW VulkanVaoManager( dynBufferMultiplier, mDevice );
            mVaoManager = vaoManager;
            mTextureGpuManager = OGRE_NEW VulkanTextureGpuManager( mVaoManager, this );

            mActiveDevice->mVaoManager = vaoManager;
            mActiveDevice->initQueues();

            mInitialized = true;
        }

        win->_setDevice( mActiveDevice );
        win->_initialize( mTextureGpuManager );

        return win;
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
    void VulkanRenderSystem::flushUAVs( void ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setCurrentDeviceFromTexture( TextureGpu *texture ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setTexture( size_t unit, TextureGpu *texPtr ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setTextures( uint32 slotStart, const DescriptorSetTexture *set,
                                           uint32 hazardousTexIdx )
    {
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setTextures( uint32 slotStart, const DescriptorSetTexture2 *set ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setSamplers( uint32 slotStart, const DescriptorSetSampler *set ) {}
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
    void VulkanRenderSystem::_setIndirectBuffer( IndirectBufferPacked *indirectBuffer ) {}
    //-------------------------------------------------------------------------
    RenderPassDescriptor *VulkanRenderSystem::createRenderPassDescriptor( void )
    {
        VulkanRenderPassDescriptor *retVal =
            OGRE_NEW VulkanRenderPassDescriptor( &mDevice->mGraphicsQueue, this );
        mRenderPassDescs.insert( retVal );
        return retVal;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_beginFrame( void ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_endFrame( void ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_endFrameOnce( void )
    {
        RenderSystem::_endFrameOnce();
        endRenderPassDescriptor();
        mActiveDevice->commitAndNextCommandBuffer( true );
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *Samplerblock )
    {
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setPipelineStateObject( const HlmsPso *pso ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setComputePso( const HlmsComputePso *pso ) {}
    //-------------------------------------------------------------------------
    VertexElementType VulkanRenderSystem::getColourVertexElementType( void ) const
    {
        return VET_COLOUR_ARGB;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_dispatch( const HlmsComputePso &pso ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setVertexArrayObject( const VertexArrayObject *vao ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_render( const CbDrawCallIndexed *cmd ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_render( const CbDrawCallStrip *cmd ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_renderEmulated( const CbDrawCallIndexed *cmd ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_renderEmulated( const CbDrawCallStrip *cmd ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_setRenderOperation( const v1::CbRenderOp *cmd ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_render( const v1::CbDrawCallIndexed *cmd ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_render( const v1::CbDrawCallStrip *cmd ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::bindGpuProgramParameters( GpuProgramType gptype,
                                                       GpuProgramParametersSharedPtr params,
                                                       uint16 variabilityMask )
    {
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::bindGpuProgramPassIterationParameters( GpuProgramType gptype ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::clearFrameBuffer( RenderPassDescriptor *renderPassDesc,
                                               TextureGpu *anyTarget, uint8 mipLevel )
    {
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
                currPassDesc->performStoreActions();
        }
        else
        {
            entriesToFlush = RenderPassDescriptor::All;
        }

        mEntriesToFlush = entriesToFlush;
        mVpChanged = vpChanged;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::executeRenderPassDescriptorDelayedActions( void )
    {
        if( mEntriesToFlush )
        {
            VulkanRenderPassDescriptor *newPassDesc =
                static_cast<VulkanRenderPassDescriptor *>( mCurrentRenderPassDescriptor );

            newPassDesc->performLoadActions();

#if VULKAN_DISABLED
            [mActiveRenderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];

            if( mStencilEnabled )
                [mActiveRenderEncoder setStencilReferenceValue:mStencilRefValue];
#endif
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

            vkCmdSetViewport( mDevice->mGraphicsQueue.mCurrentCmdBuffer, 0u, numViewports, vkVp );
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

            vkCmdSetScissor( mDevice->mGraphicsQueue.mCurrentCmdBuffer, 0u, numViewports, scissorRect );
        }

        mEntriesToFlush = 0;
        mVpChanged = false;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::endRenderPassDescriptor( void )
    {
        if( mCurrentRenderPassDescriptor )
        {
            VulkanRenderPassDescriptor *passDesc =
                static_cast<VulkanRenderPassDescriptor *>( mCurrentRenderPassDescriptor );
            passDesc->performStoreActions();

            mEntriesToFlush = 0;
            mVpChanged = false;

            RenderSystem::endRenderPassDescriptor();
        }
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
    void VulkanRenderSystem::_hlmsPipelineStateObjectCreated( HlmsPso *newPso )
    {
        size_t numShaderStages = 0u;
        VkPipelineShaderStageCreateInfo shaderStages[GPT_COMPUTE_PROGRAM];

        DescriptorSetLayoutArray descriptorSets;

        if( !newPso->vertexShader.isNull() )
        {
            VulkanProgram *shader =
                static_cast<VulkanProgram *>( newPso->vertexShader->_getBindingDelegate() );
            shader->fillPipelineShaderStageCi( shaderStages[numShaderStages++] );
            VulkanDescriptors::generateAndMergeDescriptorSets( shader, descriptorSets );
        }

        if( !newPso->geometryShader.isNull() )
        {
            VulkanProgram *shader =
                static_cast<VulkanProgram *>( newPso->geometryShader->_getBindingDelegate() );
            shader->fillPipelineShaderStageCi( shaderStages[numShaderStages++] );
            VulkanDescriptors::generateAndMergeDescriptorSets( shader, descriptorSets );
        }

        if( !newPso->tesselationHullShader.isNull() )
        {
            VulkanProgram *shader =
                static_cast<VulkanProgram *>( newPso->tesselationHullShader->_getBindingDelegate() );
            shader->fillPipelineShaderStageCi( shaderStages[numShaderStages++] );
            VulkanDescriptors::generateAndMergeDescriptorSets( shader, descriptorSets );
        }

        if( !newPso->tesselationDomainShader.isNull() )
        {
            VulkanProgram *shader =
                static_cast<VulkanProgram *>( newPso->tesselationDomainShader->_getBindingDelegate() );
            shader->fillPipelineShaderStageCi( shaderStages[numShaderStages++] );
            VulkanDescriptors::generateAndMergeDescriptorSets( shader, descriptorSets );
        }

        if( !newPso->pixelShader.isNull() )
        {
            VulkanProgram *shader =
                static_cast<VulkanProgram *>( newPso->pixelShader->_getBindingDelegate() );
            shader->fillPipelineShaderStageCi( shaderStages[numShaderStages++] );
            VulkanDescriptors::generateAndMergeDescriptorSets( shader, descriptorSets );
        }

        VulkanDescriptors::optimizeDescriptorSets( descriptorSets );
        VkPipelineLayout layout = VulkanDescriptors::generateVkDescriptorSets( descriptorSets );

        VkPipelineVertexInputStateCreateInfo vertexFormatCi;
        makeVkStruct( vertexFormatCi, VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO );
        TODO_vertex_format;

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
        rasterState.depthBiasEnable = newPso->macroblock->mDepthBiasConstant != 0.0f;
        rasterState.depthBiasConstantFactor = newPso->macroblock->mDepthBiasConstant;
        rasterState.depthBiasClamp = 0.0f;
        rasterState.depthBiasSlopeFactor = newPso->macroblock->mDepthBiasSlopeScale;
        rasterState.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo mssCi;
        makeVkStruct( mssCi, VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO );
        mssCi.rasterizationSamples = static_cast<VkSampleCountFlagBits>( newPso->pass.multisampleCount );
        mssCi.alphaToCoverageEnable = newPso->blendblock->mAlphaToCoverageEnabled;

        VkPipelineDepthStencilStateCreateInfo depthStencilStateCi;
        makeVkStruct( depthStencilStateCi, VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO );
        depthStencilStateCi.depthTestEnable = newPso->macroblock->mDepthCheck;
        depthStencilStateCi.depthWriteEnable = newPso->macroblock->mDepthWrite;
        depthStencilStateCi.depthCompareOp = VulkanMappings::get( newPso->macroblock->mDepthFunc );
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
        for( int i = 0; i < OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
        {
            if( newPso->pass.colourFormat[i] != PFG_NULL )
                mrtCount = static_cast<uint8>( i ) + 1u;
        }
        blendStateCi.attachmentCount = mrtCount;
        VkPipelineColorBlendAttachmentState blendStates[OGRE_MAX_MULTIPLE_RENDER_TARGETS];

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

                blendStates[0].colorWriteMask = newPso->blendblock->mBlendChannelMask;
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

                blendStates[0].colorWriteMask = newPso->blendblock->mBlendChannelMask;
            }
        }

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

        TODO_renderPass;
        TODO_psoCaches;

        VkPipeline vulkanPso = 0;
        VkResult result = vkCreateGraphicsPipelines( mActiveDevice->mDevice, VK_NULL_HANDLE, 1u,
                                                     &pipeline, 0, &vulkanPso );
        checkVkResult( result, "vkCreateGraphicsPipelines" );

        //        newPso->rsData = metalPso;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_hlmsPipelineStateObjectDestroyed( HlmsPso *pso )
    {
        assert( pso->rsData );

        //        removeDepthStencilState( pso );

        //        MetalHlmsPso *metalPso = reinterpret_cast<MetalHlmsPso *>( pso->rsData );
        //        delete metalPso;
        pso->rsData = 0;
    }
}  // namespace Ogre
