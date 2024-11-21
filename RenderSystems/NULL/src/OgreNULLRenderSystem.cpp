/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE-Next
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

#include "OgreNULLRenderSystem.h"

#include "OgreDefaultHardwareBufferManager.h"
#include "OgreNULLTextureGpuManager.h"
#include "OgreNULLWindow.h"
#include "OgreRenderPassDescriptor.h"
#include "Vao/OgreNULLVaoManager.h"

namespace Ogre
{
    //-------------------------------------------------------------------------
    NULLRenderSystem::NULLRenderSystem() :
        RenderSystem(),
        mInitialized( false ),
        mHardwareBufferManager( 0 )
    {
    }
    //-------------------------------------------------------------------------
    NULLRenderSystem::~NULLRenderSystem() { shutdown(); }
    //-------------------------------------------------------------------------
    void NULLRenderSystem::shutdown()
    {
        OGRE_DELETE mHardwareBufferManager;
        mHardwareBufferManager = 0;
    }
    //-------------------------------------------------------------------------
    const String &NULLRenderSystem::getName() const
    {
        static String strName( "NULL Rendering Subsystem" );
        return strName;
    }
    //-------------------------------------------------------------------------
    const String &NULLRenderSystem::getFriendlyName() const
    {
        static String strName( "NULL_RS" );
        return strName;
    }
    //-------------------------------------------------------------------------
    HardwareOcclusionQuery *NULLRenderSystem::createHardwareOcclusionQuery()
    {
        return 0;  // TODO
    }
    //-------------------------------------------------------------------------
    RenderSystemCapabilities *NULLRenderSystem::createRenderSystemCapabilities() const
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
    void NULLRenderSystem::reinitialise()
    {
        this->shutdown();
        this->_initialise( true );
    }
    //-------------------------------------------------------------------------
    Window *NULLRenderSystem::_initialise( bool autoCreateWindow, const String &windowTitle )
    {
        Window *autoWindow = 0;
        if( autoCreateWindow )
            autoWindow = _createRenderWindow( windowTitle, 1, 1, false );
        RenderSystem::_initialise( autoCreateWindow, windowTitle );

        return autoWindow;
    }
    //-------------------------------------------------------------------------
    Window *NULLRenderSystem::_createRenderWindow( const String &name, uint32 width, uint32 height,
                                                   bool fullScreen, const NameValuePairList *miscParams )
    {
        Window *win = OGRE_NEW NULLWindow( name, width, height, fullScreen );
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

            mHardwareBufferManager = new v1::DefaultHardwareBufferManager();
            mVaoManager = OGRE_NEW NULLVaoManager();
            mTextureGpuManager = OGRE_NEW NULLTextureGpuManager( mVaoManager, this );

            mInitialized = true;
        }

        win->_initialize( mTextureGpuManager, miscParams );

        return win;
    }
    //-------------------------------------------------------------------------
    String NULLRenderSystem::getErrorDescription( long errorNumber ) const { return BLANKSTRING; }
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_useLights( const LightList &lights, unsigned short limit ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setWorldMatrix( const Matrix4 &m ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setViewMatrix( const Matrix4 &m ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setProjectionMatrix( const Matrix4 &m ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setSurfaceParams( const ColourValue &ambient, const ColourValue &diffuse,
                                              const ColourValue &specular, const ColourValue &emissive,
                                              Real shininess, TrackVertexColourType tracking )
    {
    }
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setPointSpritesEnabled( bool enabled ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setPointParameters( Real size, bool attenuationEnabled, Real constant,
                                                Real linear, Real quadratic, Real minSize, Real maxSize )
    {
    }
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setCurrentDeviceFromTexture( TextureGpu *texture ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setTexture( size_t unit, TextureGpu *texPtr, bool bDepthReadOnly ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setTextures( uint32 slotStart, const DescriptorSetTexture *set,
                                         uint32 hazardousTexIdx )
    {
    }
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setTextures( uint32 slotStart, const DescriptorSetTexture2 *set ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setSamplers( uint32 slotStart, const DescriptorSetSampler *set ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setTexturesCS( uint32 slotStart, const DescriptorSetTexture *set ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setTexturesCS( uint32 slotStart, const DescriptorSetTexture2 *set ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setSamplersCS( uint32 slotStart, const DescriptorSetSampler *set ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setUavCS( uint32 slotStart, const DescriptorSetUav *set ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setTextureCoordCalculation( size_t unit, TexCoordCalcMethod m,
                                                        const Frustum *frustum )
    {
    }
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setTextureBlendMode( size_t unit, const LayerBlendModeEx &bm ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setTextureMatrix( size_t unit, const Matrix4 &xform ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setIndirectBuffer( IndirectBufferPacked *indirectBuffer ) {}
    //-------------------------------------------------------------------------
    RenderPassDescriptor *NULLRenderSystem::createRenderPassDescriptor()
    {
        RenderPassDescriptor *retVal = OGRE_NEW RenderPassDescriptor();
        mRenderPassDescs.insert( retVal );
        return retVal;
    }
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_beginFrame() {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_endFrame() {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *Samplerblock ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setPipelineStateObject( const HlmsPso *pso ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setComputePso( const HlmsComputePso *pso ) {}
    //-------------------------------------------------------------------------
    VertexElementType NULLRenderSystem::getColourVertexElementType() const { return VET_COLOUR_ARGB; }
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_dispatch( const HlmsComputePso &pso ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setVertexArrayObject( const VertexArrayObject *vao ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_render( const CbDrawCallIndexed *cmd ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_render( const CbDrawCallStrip *cmd ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_renderEmulated( const CbDrawCallIndexed *cmd ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_renderEmulated( const CbDrawCallStrip *cmd ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_setRenderOperation( const v1::CbRenderOp *cmd ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_render( const v1::CbDrawCallIndexed *cmd ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::_render( const v1::CbDrawCallStrip *cmd ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::bindGpuProgramParameters( GpuProgramType gptype,
                                                     GpuProgramParametersSharedPtr params,
                                                     uint16 variabilityMask )
    {
    }
    //-------------------------------------------------------------------------
    void NULLRenderSystem::bindGpuProgramPassIterationParameters( GpuProgramType gptype ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::clearFrameBuffer( RenderPassDescriptor *renderPassDesc, TextureGpu *anyTarget,
                                             uint8 mipLevel )
    {
    }
    //-------------------------------------------------------------------------
    Real NULLRenderSystem::getHorizontalTexelOffset() { return 0.0f; }
    //-------------------------------------------------------------------------
    Real NULLRenderSystem::getVerticalTexelOffset() { return 0.0f; }
    //-------------------------------------------------------------------------
    Real NULLRenderSystem::getMinimumDepthInputValue() { return 0.0f; }
    //-------------------------------------------------------------------------
    Real NULLRenderSystem::getMaximumDepthInputValue() { return 1.0f; }
    //-------------------------------------------------------------------------
    void NULLRenderSystem::preExtraThreadsStarted() {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::postExtraThreadsStarted() {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::registerThread() {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::unregisterThread() {}
    //-------------------------------------------------------------------------
    const PixelFormatToShaderType *NULLRenderSystem::getPixelFormatToShaderType() const
    {
        return &mPixelFormatToShaderType;
    }
    //-------------------------------------------------------------------------
    void NULLRenderSystem::flushCommands() {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::beginProfileEvent( const String &eventName ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::endProfileEvent() {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::markProfileEvent( const String &event ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::initGPUProfiling() {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::deinitGPUProfiling() {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::beginGPUSampleProfile( const String &name, uint32 *hashCache ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::endGPUSampleProfile( const String &name ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::setClipPlanesImpl( const PlaneList &clipPlanes ) {}
    //-------------------------------------------------------------------------
    void NULLRenderSystem::initialiseFromRenderSystemCapabilities( RenderSystemCapabilities *caps,
                                                                   Window *primary )
    {
    }
}  // namespace Ogre
