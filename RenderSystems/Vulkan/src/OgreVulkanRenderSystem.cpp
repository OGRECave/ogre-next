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
#include "OgreVulkanGpuProgramManager.h"
#include "OgreVulkanTextureGpuManager.h"
#include "OgreVulkanWindow.h"
#include "Vao/OgreVulkanVaoManager.h"

#include "OgreDefaultHardwareBufferManager.h"

#include "Windowing/X11/OgreVulkanXcbWindow.h"

namespace Ogre
{
    //-------------------------------------------------------------------------
    VulkanRenderSystem::VulkanRenderSystem() :
        RenderSystem(),
        mInitialized( false ),
        mHardwareBufferManager( 0 ),
        mShaderManager( 0 )
    {
    }
    //-------------------------------------------------------------------------
    void VulkanRenderSystem::shutdown( void )
    {
        OGRE_DELETE mShaderManager;
        mShaderManager = 0;

        OGRE_DELETE mHardwareBufferManager;
        mHardwareBufferManager = 0;
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
        StringVector requiredExtensions;
        Window *win = OGRE_NEW VulkanXcbWindow( requiredExtensions, name, width, height, fullScreen );
        mWindows.insert( win );

        if( !mInitialized )
        {
            if( miscParams )
            {
                NameValuePairList::const_iterator itOption = miscParams->find( "reverse_depth" );
                if( itOption != miscParams->end() )
                    mReverseDepth = StringConverter::parseBool( itOption->second, true );
            }

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
    }
}  // namespace Ogre
