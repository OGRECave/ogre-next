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
#include "OgreVulkanDevice.h"
#include "OgreVulkanGpuProgramManager.h"
#include "OgreVulkanTextureGpuManager.h"
#include "OgreVulkanWindow.h"
#include "Vao/OgreVulkanVaoManager.h"

#include "OgreDefaultHardwareBufferManager.h"

#include "Windowing/X11/OgreVulkanXcbWindow.h"

#include <vulkan/vulkan.h>

namespace Ogre
{
    //-------------------------------------------------------------------------
    VulkanRenderSystem::VulkanRenderSystem() :
        RenderSystem(),
        mInitialized( false ),
        mHardwareBufferManager( 0 ),
        mShaderManager( 0 ),
        mVkInstance( 0 ),
        mActiveDevice( 0 ),
        mDevice( 0 )
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
        Window *win = OGRE_NEW VulkanXcbWindow( reqInstanceExtensions, name, width, height, fullScreen );
        mWindows.insert( win );

        if( !mInitialized )
        {
            if( miscParams )
            {
                NameValuePairList::const_iterator itOption = miscParams->find( "reverse_depth" );
                if( itOption != miscParams->end() )
                    mReverseDepth = StringConverter::parseBool( itOption->second, true );
            }

            mVkInstance = VulkanDevice::createInstance( name, reqInstanceExtensions );
            mDevice = new VulkanDevice( mVkInstance, 0 );
            mActiveDevice = mDevice;

            mRealCapabilities = createRenderSystemCapabilities();
            mCurrentCapabilities = mRealCapabilities;

            initialiseFromRenderSystemCapabilities( mCurrentCapabilities, 0 );

            mHardwareBufferManager = new v1::DefaultHardwareBufferManager();
            mVaoManager = OGRE_NEW VulkanVaoManager();
            mTextureGpuManager = OGRE_NEW VulkanTextureGpuManager( mVaoManager, this );

            mInitialized = true;
        }

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
        RenderPassDescriptor *retVal = OGRE_NEW RenderPassDescriptor();
        mRenderPassDescs.insert( retVal );
        return retVal;
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_beginFrame( void ) {}
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::_endFrame( void ) {}
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
        mShaderManager = OGRE_NEW VulkanGpuProgramManager();

        Log *defaultLog = LogManager::getSingleton().getDefaultLog();
        if( defaultLog )
        {
            caps->log( defaultLog );
            defaultLog->logMessage( " * Using Reverse Z: " +
                                    StringConverter::toString( mReverseDepth, true ) );
        }
    }
}  // namespace Ogre
