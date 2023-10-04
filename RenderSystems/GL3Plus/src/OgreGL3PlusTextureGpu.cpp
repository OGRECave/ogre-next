/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

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

#include "OgreGL3PlusTextureGpu.h"
#include "OgreGL3PlusMappings.h"
#include "OgreGL3PlusSupport.h"
#include "OgreGL3PlusTextureGpuManager.h"

#include "OgreTextureBox.h"
#include "OgreTextureGpuListener.h"
#include "OgreVector2.h"

#include "OgreRenderSystem.h"
#include "Vao/OgreVaoManager.h"

#include "OgreException.h"

#define TODO_use_StagingTexture_with_GPU_GPU_visibility
#define TODO_sw_fallback

namespace Ogre
{
    GL3PlusTextureGpu::GL3PlusTextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                          VaoManager *vaoManager, IdString name, uint32 textureFlags,
                                          TextureTypes::TextureTypes initialType,
                                          TextureGpuManager *textureManager ) :
        TextureGpu( pageOutStrategy, vaoManager, name, textureFlags, initialType, textureManager ),
        mDisplayTextureName( 0 ),
        mGlTextureTarget( GL_NONE ),
        mFinalTextureName( 0 ),
        mMsaaFramebufferName( 0 )
    {
        // The vtable hasn't yet been populated so
        // GL3PlusTextureGpuWindow::_setToDisplayDummyTexture won't kick in
        if( !isRenderWindowSpecific() )
            _setToDisplayDummyTexture();
    }
    //-----------------------------------------------------------------------------------
    GL3PlusTextureGpu::~GL3PlusTextureGpu() { destroyInternalResourcesImpl(); }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::createInternalResourcesImpl()
    {
        GLenum format = GL3PlusMappings::get( mPixelFormat );

        if( !isMultisample() || !hasMsaaExplicitResolves() )
        {
            OCGE( glGenTextures( 1u, &mFinalTextureName ) );

            mGlTextureTarget = GL3PlusMappings::get( mTextureType, false );

            OCGE( glBindTexture( mGlTextureTarget, mFinalTextureName ) );
            OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_BASE_LEVEL, 0 ) );
            OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_MAX_LEVEL, 0 ) );
            OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST ) );
            OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST ) );
            OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) );
            OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) );
            OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE ) );
            OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_MAX_LEVEL, mNumMipmaps - 1u ) );

            if( mPixelFormat == PFG_A8_UNORM )
            {
                OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_SWIZZLE_R, GL_ZERO ) );
                OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_SWIZZLE_G, GL_ZERO ) );
                OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_SWIZZLE_B, GL_ZERO ) );
                OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_SWIZZLE_A, GL_RED ) );
            }

            switch( mTextureType )
            {
            case TextureTypes::Unknown:
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                             "Texture '" + getNameStr() +
                                 "': "
                                 "Ogre should never hit this path",
                             "GL3PlusTextureGpu::createInternalResourcesImpl" );
                break;
            case TextureTypes::Type1D:
                OCGE(
                    glTexStorage1D( GL_TEXTURE_1D, GLsizei( mNumMipmaps ), format, GLsizei( mWidth ) ) );
                break;
            case TextureTypes::Type1DArray:
                OCGE( glTexStorage2D( GL_TEXTURE_1D_ARRAY, GLsizei( mNumMipmaps ), format,
                                      GLsizei( mWidth ), GLsizei( mDepthOrSlices ) ) );
                break;
            case TextureTypes::Type2D:
                OCGE( glTexStorage2D( GL_TEXTURE_2D, GLsizei( mNumMipmaps ), format, GLsizei( mWidth ),
                                      GLsizei( mHeight ) ) );
                break;
            case TextureTypes::Type2DArray:
                OCGE( glTexStorage3D( GL_TEXTURE_2D_ARRAY, GLsizei( mNumMipmaps ), format,
                                      GLsizei( mWidth ), GLsizei( mHeight ),
                                      GLsizei( mDepthOrSlices ) ) );
                break;
            case TextureTypes::TypeCube:
                OCGE( glTexStorage2D( GL_TEXTURE_CUBE_MAP, GLsizei( mNumMipmaps ), format,
                                      GLsizei( mWidth ), GLsizei( mHeight ) ) );
                break;
            case TextureTypes::TypeCubeArray:
                OCGE( glTexStorage3D( GL_TEXTURE_CUBE_MAP_ARRAY, GLsizei( mNumMipmaps ), format,
                                      GLsizei( mWidth ), GLsizei( mHeight ),
                                      GLsizei( mDepthOrSlices ) ) );
                break;
            case TextureTypes::Type3D:
                OCGE( glTexStorage3D( GL_TEXTURE_3D, GLsizei( mNumMipmaps ), format, GLsizei( mWidth ),
                                      GLsizei( mHeight ), GLsizei( mDepthOrSlices ) ) );
                break;
            }

            // Allocate internal buffers for automipmaps before we load anything into them
            if( allowsAutoMipmaps() )
                OCGE( glGenerateMipmap( mGlTextureTarget ) );

            // Set debug name for RenderDoc and similar tools
            ogreGlObjectLabel( GL_TEXTURE, mFinalTextureName, getNameStr() );
        }

        if( isMultisample() )
        {
            // const GLboolean fixedsamplelocations = mMsaaPattern != MsaaPatterns::Undefined;
            // RENDERBUFFERS have fixedsamplelocations implicitly set to true. Be consistent
            // with non-texture depth buffers.
            const GLboolean fixedsamplelocations = GL_TRUE;

            if( !isTexture() ||
                ( !hasMsaaExplicitResolves() && !PixelFormatGpuUtils::isDepth( mPixelFormat ) ) )
            {
                OCGE( glGenRenderbuffers( 1, &mMsaaFramebufferName ) );
                OCGE( glBindRenderbuffer( GL_RENDERBUFFER, mMsaaFramebufferName ) );
                OCGE( glRenderbufferStorageMultisample( GL_RENDERBUFFER,
                                                        mSampleDescription.getColourSamples(), format,
                                                        GLsizei( mWidth ), GLsizei( mHeight ) ) );
                OCGE( glBindRenderbuffer( GL_RENDERBUFFER, 0 ) );

                // Set debug name for RenderDoc and similar tools
                ogreGlObjectLabel( GL_RENDERBUFFER, mMsaaFramebufferName,
                                   getNameStr() + "/MsaaImplicit" );
            }
            else
            {
                OCGE( glGenTextures( 1u, &mFinalTextureName ) );

                assert( mTextureType == TextureTypes::Type2D ||
                        mTextureType == TextureTypes::Type2DArray );
                mGlTextureTarget = mTextureType == TextureTypes::Type2D
                                       ? GL_TEXTURE_2D_MULTISAMPLE
                                       : GL_TEXTURE_2D_MULTISAMPLE_ARRAY;

                OCGE( glBindTexture( mGlTextureTarget, mFinalTextureName ) );
                OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_BASE_LEVEL, 0 ) );
                OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_MAX_LEVEL, 0 ) );
                OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_MAX_LEVEL, mNumMipmaps - 1u ) );

                if( mPixelFormat == PFG_A8_UNORM )
                {
                    OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_SWIZZLE_R, GL_ZERO ) );
                    OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_SWIZZLE_G, GL_ZERO ) );
                    OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_SWIZZLE_B, GL_ZERO ) );
                    OCGE( glTexParameteri( mGlTextureTarget, GL_TEXTURE_SWIZZLE_A, GL_RED ) );
                }

                if( mTextureType == TextureTypes::Type2D )
                {
                    glTexImage2DMultisample( mGlTextureTarget, mSampleDescription.getColourSamples(),
                                             format, GLsizei( mWidth ), GLsizei( mHeight ),
                                             fixedsamplelocations );
                }
                else
                {
                    glTexImage3DMultisample( mGlTextureTarget, mSampleDescription.getColourSamples(),
                                             format, GLsizei( mWidth ), GLsizei( mHeight ),
                                             GLsizei( mDepthOrSlices ), fixedsamplelocations );
                }

                // Set debug name for RenderDoc and similar tools
                ogreGlObjectLabel( GL_TEXTURE, mFinalTextureName, getNameStr() );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::destroyInternalResourcesImpl()
    {
        if( !hasAutomaticBatching() )
        {
            if( mFinalTextureName )
            {
                glDeleteTextures( 1, &mFinalTextureName );
                mFinalTextureName = 0;
            }
            if( mMsaaFramebufferName )
            {
                if( isMultisample() && !hasMsaaExplicitResolves() )
                    glDeleteRenderbuffers( 1, &mMsaaFramebufferName );
                else
                    glDeleteTextures( 1, &mMsaaFramebufferName );
                mMsaaFramebufferName = 0;
            }
        }
        else
        {
            if( mTexturePool )
            {
                // This will end up calling _notifyTextureSlotChanged,
                // setting mTexturePool & mInternalSliceStart to 0
                mTextureManager->_releaseSlotFromTexture( this );
            }

            mFinalTextureName = 0;
            mMsaaFramebufferName = 0;
        }

        _setToDisplayDummyTexture();
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::notifyDataIsReady()
    {
        assert( mResidencyStatus == GpuResidency::Resident );
        assert( mFinalTextureName || mPixelFormat == PFG_NULL );

        OGRE_ASSERT_LOW( mDataPreparationsPending > 0u &&
                         "Calling notifyDataIsReady too often! Remove this call"
                         "See https://github.com/OGRECave/ogre-next/issues/101" );
        --mDataPreparationsPending;

        mDisplayTextureName = mFinalTextureName;

        notifyAllListenersTextureChanged( TextureGpuListener::ReadyForRendering );
    }
    //-----------------------------------------------------------------------------------
    bool GL3PlusTextureGpu::_isDataReadyImpl() const
    {
        return mDisplayTextureName == mFinalTextureName && mDataPreparationsPending == 0u;
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::_setToDisplayDummyTexture()
    {
        if( !mTextureManager )
        {
            assert( isRenderWindowSpecific() );
            return;  // This can happen if we're a window and we're on shutdown
        }

        GL3PlusTextureGpuManager *textureManagerGl =
            static_cast<GL3PlusTextureGpuManager *>( mTextureManager );
        if( hasAutomaticBatching() )
        {
            mDisplayTextureName = textureManagerGl->getBlankTextureGlName( TextureTypes::Type2DArray );
            mGlTextureTarget = GL_TEXTURE_2D_ARRAY;
        }
        else
        {
            mDisplayTextureName = textureManagerGl->getBlankTextureGlName( mTextureType );
            mGlTextureTarget = GL3PlusMappings::get( mTextureType, false );
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::_notifyTextureSlotChanged( const TexturePool *newPool, uint16 slice )
    {
        TextureGpu::_notifyTextureSlotChanged( newPool, slice );

        _setToDisplayDummyTexture();

        mGlTextureTarget = GL_TEXTURE_2D_ARRAY;

        if( mTexturePool )
        {
            assert( dynamic_cast<GL3PlusTextureGpu *>( mTexturePool->masterTexture ) );
            GL3PlusTextureGpu *masterTexture =
                static_cast<GL3PlusTextureGpu *>( mTexturePool->masterTexture );
            mFinalTextureName = masterTexture->mFinalTextureName;
        }

        notifyAllListenersTextureChanged( TextureGpuListener::PoolTextureSlotChanged );
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::setTextureType( TextureTypes::TextureTypes textureType )
    {
        const TextureTypes::TextureTypes oldType = mTextureType;
        TextureGpu::setTextureType( textureType );

        if( oldType != mTextureType && mDisplayTextureName != mFinalTextureName )
            _setToDisplayDummyTexture();
    }
    //-----------------------------------------------------------------------------------
    bool GL3PlusTextureGpu::isRenderbuffer() const
    {
        const bool isDepth = PixelFormatGpuUtils::isDepth( mPixelFormat );

        return ( isMultisample() && ( ( !hasMsaaExplicitResolves() && !isDepth ) || !isTexture() ) ) ||
               ( isDepth && !isTexture() ) || isRenderWindowSpecific();
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::bindTextureToFrameBuffer( GLenum target, uint8 mipLevel,
                                                      uint32 depthOrSlice )
    {
        GLuint textureName = mFinalTextureName;
        bool bindMsaaColourRenderbuffer =
            isMultisample() && ( !hasMsaaExplicitResolves() || !isTexture() );
        if( bindMsaaColourRenderbuffer )
            textureName = mMsaaFramebufferName;
        bindTextureToFrameBuffer( target, textureName, mipLevel, depthOrSlice,
                                  bindMsaaColourRenderbuffer );
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::bindTextureToFrameBuffer( GLenum target, GLuint textureName, uint8 mipLevel,
                                                      uint32 depthOrSlice,
                                                      bool bindMsaaColourRenderbuffer )
    {
        assert( !isRenderWindowSpecific() );

        const bool isDepth = PixelFormatGpuUtils::isDepth( mPixelFormat );

        if( isDepth )
        {
            if( isTexture() )
            {
                OCGE( glFramebufferTexture( target, GL_DEPTH_ATTACHMENT, textureName, 0 ) );
                if( PixelFormatGpuUtils::isStencil( mPixelFormat ) )
                {
                    OCGE( glFramebufferTexture( target, GL_STENCIL_ATTACHMENT, textureName, 0 ) );
                }
            }
            else
            {
                OCGE( glFramebufferRenderbuffer( target, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                                 textureName ) );
                if( PixelFormatGpuUtils::isStencil( mPixelFormat ) )
                {
                    OCGE( glFramebufferRenderbuffer( target, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                                     textureName ) );
                }
            }
        }
        else
        {
            if( bindMsaaColourRenderbuffer )
            {
                OCGE( glFramebufferRenderbuffer( target, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                                                 textureName ) );
            }
            else
            {
                const bool hasLayers =
                    mTextureType != TextureTypes::Type1D && mTextureType != TextureTypes::Type2D;
                if( !hasLayers )
                {
                    OCGE( glFramebufferTexture( target, GL_COLOR_ATTACHMENT0, textureName, mipLevel ) );
                }
                else
                {
                    OCGE( glFramebufferTextureLayer( target, GL_COLOR_ATTACHMENT0, textureName, mipLevel,
                                                     static_cast<GLint>( depthOrSlice ) ) );
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::copyViaFramebuffer( TextureGpu *dst, const TextureBox &dstBox,
                                                uint8 dstMipLevel, const TextureBox &srcBox,
                                                uint8 srcMipLevel, bool keepResolvedTexSynced )
    {
        RenderSystem *renderSystem = mTextureManager->getRenderSystem();
        renderSystem->endRenderPassDescriptor();

        GL3PlusTextureGpuManager *textureManagerGl =
            static_cast<GL3PlusTextureGpuManager *>( mTextureManager );

        assert( dynamic_cast<GL3PlusTextureGpu *>( dst ) );
        GL3PlusTextureGpu *dstGl = static_cast<GL3PlusTextureGpu *>( dst );

        const bool srcIsFboAble = this->isRenderbuffer() || this->isRenderToTexture();
        const bool dstIsFboAble = dstGl->isRenderbuffer() || dst->isRenderToTexture();

        if( !this->isRenderWindowSpecific() && srcIsFboAble )
        {
            OCGE( glBindFramebuffer( GL_READ_FRAMEBUFFER, textureManagerGl->getTemporaryFbo( 0 ) ) );
        }
        else
        {
            OCGE( glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 ) );
        }

        if( !dst->isRenderWindowSpecific() && dstIsFboAble )
        {
            OCGE( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, textureManagerGl->getTemporaryFbo( 1 ) ) );
        }
        else
        {
            OCGE( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
        }
        OCGE( glViewport( 0, 0, static_cast<GLsizei>( dstBox.width ),
                          static_cast<GLsizei>( dstBox.height ) ) );

        const uint32 depthOrSlices = srcBox.getDepthOrSlices();
        for( uint32 i = 0; i < depthOrSlices; ++i )
        {
            if( srcIsFboAble )
            {
                if( !this->isRenderWindowSpecific() )
                {
                    this->bindTextureToFrameBuffer( GL_READ_FRAMEBUFFER, srcMipLevel,
                                                    srcBox.getZOrSlice() + i );
                    OCGE( glReadBuffer( GL_COLOR_ATTACHMENT0 ) );
                }
                else
                {
                    OCGE( glReadBuffer( GL_BACK ) );
                }
            }
            if( dstIsFboAble )
            {
                if( !dstGl->isRenderWindowSpecific() )
                {
                    dstGl->bindTextureToFrameBuffer( GL_DRAW_FRAMEBUFFER, dstMipLevel,
                                                     dstBox.getZOrSlice() + i );
                    OCGE( glDrawBuffer( GL_COLOR_ATTACHMENT0 ) );
                }
                else
                {
                    OCGE( glDrawBuffer( GL_BACK ) );
                }
            }

            if( srcIsFboAble && !dstIsFboAble )
            {
                // We can use glCopyTexImageXD
                // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glCopyTexImage2D.xhtml
                GLenum texTarget = dstGl->mGlTextureTarget;
                if( dst->getTextureType() == TextureTypes::TypeCube )
                    texTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + dstBox.sliceStart;

                switch( dst->getTextureType() )
                {
                case TextureTypes::Type1D:
                    OCGE( glCopyTexSubImage1D(
                        GL_TEXTURE_1D, dstMipLevel, static_cast<GLint>( dstBox.x ),
                        static_cast<GLint>( srcBox.x ), static_cast<GLint>( srcBox.y ),
                        static_cast<GLsizei>( srcBox.width ) ) );
                    break;
                case TextureTypes::Type1DArray:
                    OCGE( glCopyTexSubImage2D( GL_TEXTURE_1D_ARRAY, dstMipLevel,         //
                                               static_cast<GLint>( dstBox.x ),           //
                                               static_cast<GLint>( dstBox.sliceStart ),  //
                                               static_cast<GLint>( srcBox.x ),           //
                                               static_cast<GLint>( srcBox.sliceStart ),  //
                                               static_cast<GLsizei>( srcBox.width ),     //
                                               static_cast<GLsizei>( srcBox.height ) ) );
                    break;
                case TextureTypes::Unknown:
                case TextureTypes::Type2D:
                case TextureTypes::TypeCube:
                    OCGE( glCopyTexSubImage2D(
                        texTarget, dstMipLevel,                                          //
                        static_cast<GLint>( dstBox.x ), static_cast<GLint>( dstBox.y ),  //
                        static_cast<GLint>( srcBox.x ), static_cast<GLint>( srcBox.y ),  //
                        static_cast<GLsizei>( srcBox.width ), static_cast<GLsizei>( srcBox.height ) ) );
                    break;
                case TextureTypes::Type2DArray:
                case TextureTypes::TypeCubeArray:
                case TextureTypes::Type3D:
                    OCGE( glCopyTexSubImage3D(
                        texTarget, dstMipLevel,                                          //
                        static_cast<GLint>( dstBox.x ), static_cast<GLint>( dstBox.y ),  //
                        static_cast<GLint>( dstBox.getZOrSlice() ),                      //
                        static_cast<GLint>( srcBox.x ), static_cast<GLint>( srcBox.y ),  //
                        static_cast<GLsizei>( srcBox.width ), static_cast<GLsizei>( srcBox.height ) ) );
                    break;
                }
            }
            else if( srcIsFboAble && dstIsFboAble )
            {
                GLenum readStatus, drawStatus;
                OCGE( readStatus = glCheckFramebufferStatus( GL_READ_FRAMEBUFFER ) );
                OCGE( drawStatus = glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER ) );

                const bool supported =
                    readStatus == GL_FRAMEBUFFER_COMPLETE && drawStatus == GL_FRAMEBUFFER_COMPLETE;

                if( supported )
                {
                    GLbitfield bufferBits = 0;
                    if( PixelFormatGpuUtils::isDepth( this->mPixelFormat ) )
                        bufferBits |= GL_DEPTH_BUFFER_BIT;
                    if( PixelFormatGpuUtils::isStencil( this->mPixelFormat ) )
                        bufferBits |= GL_STENCIL_BUFFER_BIT;
                    if( !bufferBits )
                        bufferBits = GL_COLOR_BUFFER_BIT;

                    GLint srcX0 = static_cast<GLint>( srcBox.x );
                    GLint srcX1 = static_cast<GLint>( srcBox.x + srcBox.width );
                    GLint srcY0;
                    GLint srcY1;
                    if( this->isRenderWindowSpecific() )
                    {
                        srcY0 = static_cast<GLint>( this->mHeight - srcBox.y );
                        srcY1 = static_cast<GLint>( this->mHeight - srcBox.y - srcBox.height );
                    }
                    else
                    {
                        srcY0 = static_cast<GLint>( srcBox.y );
                        srcY1 = static_cast<GLint>( srcBox.y + srcBox.height );
                    }
                    GLint dstX0 = static_cast<GLint>( dstBox.x );
                    GLint dstX1 = static_cast<GLint>( dstBox.x + dstBox.width );
                    GLint dstY0;
                    GLint dstY1;
                    if( dst->isRenderWindowSpecific() )
                    {
                        dstY0 = static_cast<GLint>( dstGl->mHeight - dstBox.y );
                        dstY1 = static_cast<GLint>( dstGl->mHeight - dstBox.y - dstBox.height );
                    }
                    else
                    {
                        dstY0 = static_cast<GLint>( dstBox.y );
                        dstY1 = static_cast<GLint>( dstBox.y + dstBox.height );
                    }

                    OCGE( glBlitFramebuffer( srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1,
                                             bufferBits, GL_NEAREST ) );
                }
                else
                {
                    static bool alreadyWarned = false;
                    if( !alreadyWarned )
                    {
                        LogManager::getSingleton().logMessage(
                            "Unsupported FBO in GL3PlusTextureGpu::copyTo. Falling back to "
                            "software copy. This is slow. This message will only appear once.",
                            LML_CRITICAL );
                        alreadyWarned = true;
                    }

                    OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                                 "Unsupported FBO in GL3PlusTextureGpu::copyTo. "
                                 "Fallback to SW copy not implemented",
                                 "GL3PlusTextureGpu::copyTo" );
                    TODO_sw_fallback;
                }
            }
            else
            {
                OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR,
                             "We should have never reached this path. Contact Ogre3D developers.",
                             "GL3PlusTextureGpu::copyTo" );
            }
        }

        if( srcIsFboAble )
        {
            if( !this->isRenderWindowSpecific() )
            {
                OCGE( glFramebufferRenderbuffer( GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                 GL_RENDERBUFFER, 0 ) );
                OCGE( glFramebufferRenderbuffer( GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                                 GL_RENDERBUFFER, 0 ) );
                OCGE( glFramebufferRenderbuffer( GL_READ_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                                 GL_RENDERBUFFER, 0 ) );
            }
            OCGE( glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 ) );
        }
        if( dstIsFboAble )
        {
            if( !this->isRenderWindowSpecific() )
            {
                OCGE( glFramebufferRenderbuffer( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                 GL_RENDERBUFFER, 0 ) );
                OCGE( glFramebufferRenderbuffer( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                                 GL_RENDERBUFFER, 0 ) );
                OCGE( glFramebufferRenderbuffer( GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                                 GL_RENDERBUFFER, 0 ) );
            }
            OCGE( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
        }

        if( dstGl->isMultisample() && !dstGl->hasMsaaExplicitResolves() && keepResolvedTexSynced )
        {
            OCGE( glBindFramebuffer( GL_READ_FRAMEBUFFER, textureManagerGl->getTemporaryFbo( 0 ) ) );
            OCGE( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, textureManagerGl->getTemporaryFbo( 1 ) ) );
            OCGE( glViewport( 0, 0, static_cast<GLsizei>( srcBox.width ),
                              static_cast<GLsizei>( srcBox.height ) ) );

            OCGE( glReadBuffer( GL_COLOR_ATTACHMENT0 ) );
            OCGE( glDrawBuffer( GL_COLOR_ATTACHMENT0 ) );

            for( uint32 i = 0; i < dstBox.numSlices; ++i )
            {
                dstGl->bindTextureToFrameBuffer( GL_READ_FRAMEBUFFER, dstGl->mMsaaFramebufferName, 0,
                                                 dstBox.getZOrSlice() + i, true );
                dstGl->bindTextureToFrameBuffer( GL_DRAW_FRAMEBUFFER, dstGl->mFinalTextureName,
                                                 dstMipLevel, dstBox.getZOrSlice() + i, false );

                OCGE( glBlitFramebuffer(
                    0, 0, static_cast<GLint>( srcBox.width ), static_cast<GLint>( srcBox.height ),  //
                    0, 0, static_cast<GLint>( srcBox.width ), static_cast<GLint>( srcBox.height ),  //
                    GL_COLOR_BUFFER_BIT, GL_NEAREST ) );
            }

            OCGE( glFramebufferRenderbuffer( GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                                             0 ) );
            OCGE( glFramebufferRenderbuffer( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                                             0 ) );
            OCGE( glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 ) );
            OCGE( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::copyTo( TextureGpu *dst, const TextureBox &dstBox, uint8 dstMipLevel,
                                    const TextureBox &srcBox, uint8 srcMipLevel,
                                    bool keepResolvedTexSynced,
                                    CopyEncTransitionMode::CopyEncTransitionMode srcTransitionMode,
                                    CopyEncTransitionMode::CopyEncTransitionMode dstTransitionMode )
    {
        TextureGpu::copyTo( dst, dstBox, dstMipLevel, srcBox, srcMipLevel, srcTransitionMode,
                            dstTransitionMode );

        assert( dynamic_cast<GL3PlusTextureGpu *>( dst ) );

        GL3PlusTextureGpu *dstGl = static_cast<GL3PlusTextureGpu *>( dst );
        GL3PlusTextureGpuManager *textureManagerGl =
            static_cast<GL3PlusTextureGpuManager *>( mTextureManager );
        const GL3PlusSupport &support = textureManagerGl->getGlSupport();

        if( !this->isOpenGLRenderWindow() && !dst->isOpenGLRenderWindow() &&
            ( !this->isMultisample() || !dst->isMultisample() ||
              ( this->hasMsaaExplicitResolves() && dst->hasMsaaExplicitResolves() ) ) )
        {
            if( support.hasMinGLVersion( 4, 3 ) || support.checkExtension( "GL_ARB_copy_image" ) )
            {
                OCGE( glCopyImageSubData(
                    this->mFinalTextureName, this->mGlTextureTarget, srcMipLevel,

                    static_cast<GLint>( srcBox.x ), static_cast<GLint>( srcBox.y ),
                    static_cast<GLint>( srcBox.getZOrSlice() + this->getInternalSliceStart() ),

                    dstGl->mFinalTextureName, dstGl->mGlTextureTarget, dstMipLevel,

                    static_cast<GLint>( dstBox.x ), static_cast<GLint>( dstBox.y ),
                    static_cast<GLint>( dstBox.getZOrSlice() + dstGl->getInternalSliceStart() ),

                    static_cast<GLsizei>( srcBox.width ), static_cast<GLsizei>( srcBox.height ),
                    static_cast<GLsizei>( srcBox.getDepthOrSlices() ) ) );
            }
            else if( support.checkExtension( "GL_NV_copy_image" ) )
            {
                // Initialize the pointer only the first time
                PFNGLCOPYIMAGESUBDATANVPROC local_glCopyImageSubDataNV = nullptr;
                if( !local_glCopyImageSubDataNV )
                {
                    local_glCopyImageSubDataNV =
                        (PFNGLCOPYIMAGESUBDATANVPROC)gl3wGetProcAddress( "glCopyImageSubDataNV" );
                }

                OCGE( local_glCopyImageSubDataNV(
                    this->mFinalTextureName, this->mGlTextureTarget, srcMipLevel,
                    static_cast<GLint>( srcBox.x ), static_cast<GLint>( srcBox.y ),
                    static_cast<GLint>( srcBox.getZOrSlice() + this->getInternalSliceStart() ),
                    dstGl->mFinalTextureName, dstGl->mGlTextureTarget, dstMipLevel,
                    static_cast<GLint>( dstBox.x ), static_cast<GLint>( dstBox.y ),
                    static_cast<GLint>( dstBox.getZOrSlice() + dstGl->getInternalSliceStart() ),
                    static_cast<GLsizei>( srcBox.width ), static_cast<GLsizei>( srcBox.height ),
                    static_cast<GLsizei>( srcBox.getDepthOrSlices() ) ) );
            }
            /*TODO: These are for OpenGL ES 3.0+
            else if( support.checkExtension( "GL_OES_copy_image" ) )
            {
                OCGE( glCopyImageSubDataOES( this->mFinalTextureName, this->mGlTextureTarget,
                                             srcMipLevel, srcBox.x, srcBox.y, srcBox.z,
                                             dstGl->mFinalTextureName, dstGl->mGlTextureTarget,
                                             dstMipLevel, dstBox.x, dstBox.y, dstBox.z,
                                             srcBox.width, srcBox.height, srcBox.getDepthOrSlices() ) );
            }
            else if( support.checkExtension( "GL_EXT_copy_image" ) )
            {
                OCGE( glCopyImageSubDataEXT( this->mFinalTextureName, this->mGlTextureTarget,
                                             srcMipLevel, srcBox.x, srcBox.y, srcBox.z,
                                             dstGl->mFinalTextureName, dstGl->mGlTextureTarget,
                                             dstMipLevel, dstBox.x, dstBox.y, dstBox.z,
                                             srcBox.width, srcBox.height, srcBox.getDepthOrSlices() ) );
            }*/
            else
            {
                //                GLenum format, type;
                //                GL3PlusMappings::getFormatAndType( mPixelFormat, format, type );
                //                glGetTexImage( this->mFinalTextureName, srcMipLevel, format, type,  );
                // glGetCompressedTexImage
                TODO_use_StagingTexture_with_GPU_GPU_visibility;
                OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "", "GL3PlusTextureGpu::copyTo" );
            }
        }
        else
        {
            this->copyViaFramebuffer( dst, dstBox, dstMipLevel, srcBox, srcMipLevel,
                                      keepResolvedTexSynced );
        }

        // Do not perform the sync if notifyDataIsReady hasn't been called yet (i.e. we're
        // still building the HW mipmaps, and the texture will never be ready)
        if( dst->_isDataReadyImpl() &&
            dst->getGpuPageOutStrategy() == GpuPageOutStrategy::AlwaysKeepSystemRamCopy )
        {
            dst->_syncGpuResidentToSystemRam();
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::_autogenerateMipmaps( CopyEncTransitionMode::CopyEncTransitionMode
                                                  /*transitionMode*/ )
    {
        if( !mFinalTextureName )
            return;

        const GLenum texTarget = getGlTextureTarget();
        OCGE( glBindTexture( texTarget, mFinalTextureName ) );
        OCGE( glGenerateMipmap( texTarget ) );
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::getSubsampleLocations( vector<Vector2>::type locations )
    {
        locations.reserve( mSampleDescription.getColourSamples() );
        if( mSampleDescription.getColourSamples() <= 1u )
        {
            locations.push_back( Vector2( 0.0f, 0.0f ) );
        }
        else
        {
            assert( mSampleDescription.getMsaaPattern() != MsaaPatterns::Undefined );

            float vals[2];
            for( uint32 i = 0u; i < mSampleDescription.getColourSamples(); ++i )
            {
                glGetMultisamplefv( GL_SAMPLE_POSITION, i, vals );
                locations.push_back( Vector2( vals[0], vals[1] ) * 2.0f - 1.0f );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::getCustomAttribute( IdString name, void *pData )
    {
        if( name == msFinalTextureBuffer )
            *static_cast<GLuint *>( pData ) = mFinalTextureName;
        else if( name == msMsaaTextureBuffer )
            *static_cast<GLuint *>( pData ) = mMsaaFramebufferName;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    GL3PlusTextureGpuRenderTarget::GL3PlusTextureGpuRenderTarget(
        GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy, VaoManager *vaoManager, IdString name,
        uint32 textureFlags, TextureTypes::TextureTypes initialType,
        TextureGpuManager *textureManager ) :
        GL3PlusTextureGpu( pageOutStrategy, vaoManager, name, textureFlags, initialType,
                           textureManager ),
        mDepthBufferPoolId( 1u ),
        mPreferDepthTexture( false ),
        mDesiredDepthBufferFormat( PFG_UNKNOWN )
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        ,
        mOrientationMode( msDefaultOrientationMode )
#endif
    {
        if( mPixelFormat == PFG_NULL )
            mDepthBufferPoolId = 0;
    }
    //-----------------------------------------------------------------------------------
    GL3PlusTextureGpuRenderTarget::~GL3PlusTextureGpuRenderTarget() { destroyInternalResourcesImpl(); }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuRenderTarget::createInternalResourcesImpl()
    {
        if( mPixelFormat == PFG_NULL )
            return;  // Nothing to do

        if( isTexture() || !PixelFormatGpuUtils::isDepth( mPixelFormat ) )
        {
            GL3PlusTextureGpu::createInternalResourcesImpl();
        }
        else
        {
            OCGE( glGenRenderbuffers( 1, &mFinalTextureName ) );
            OCGE( glBindRenderbuffer( GL_RENDERBUFFER, mFinalTextureName ) );

            GLenum format = GL3PlusMappings::get( mPixelFormat );

            if( !isMultisample() )
            {
                OCGE( glRenderbufferStorage( GL_RENDERBUFFER, format, GLsizei( mWidth ),
                                             GLsizei( mHeight ) ) );
            }
            else
            {
                OCGE( glRenderbufferStorageMultisample(
                    GL_RENDERBUFFER, GLsizei( mSampleDescription.getColourSamples() ), format,
                    GLsizei( mWidth ), GLsizei( mHeight ) ) );
            }

            // Set debug name for RenderDoc and similar tools
            ogreGlObjectLabel( GL_RENDERBUFFER, mFinalTextureName, getNameStr() );
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuRenderTarget::destroyInternalResourcesImpl()
    {
        if( isTexture() || !PixelFormatGpuUtils::isDepth( mPixelFormat ) )
        {
            GL3PlusTextureGpu::destroyInternalResourcesImpl();
            return;
        }

        if( mFinalTextureName )
        {
            glDeleteRenderbuffers( 1, &mFinalTextureName );
            mFinalTextureName = 0;
        }
        _setToDisplayDummyTexture();
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuRenderTarget::_setDepthBufferDefaults(
        uint16 depthBufferPoolId, bool preferDepthTexture, PixelFormatGpu desiredDepthBufferFormat )
    {
        assert( isRenderToTexture() );
        OGRE_ASSERT_MEDIUM( mSourceType != TextureSourceType::SharedDepthBuffer &&
                            "Cannot call _setDepthBufferDefaults on a shared depth buffer!" );
        mDepthBufferPoolId = depthBufferPoolId;
        mPreferDepthTexture = preferDepthTexture;
        mDesiredDepthBufferFormat = desiredDepthBufferFormat;
    }
    //-----------------------------------------------------------------------------------
    uint16 GL3PlusTextureGpuRenderTarget::getDepthBufferPoolId() const { return mDepthBufferPoolId; }
    //-----------------------------------------------------------------------------------
    bool GL3PlusTextureGpuRenderTarget::getPreferDepthTexture() const { return mPreferDepthTexture; }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu GL3PlusTextureGpuRenderTarget::getDesiredDepthBufferFormat() const
    {
        return mDesiredDepthBufferFormat;
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuRenderTarget::setOrientationMode( OrientationMode orientationMode )
    {
        OGRE_ASSERT_LOW( mResidencyStatus == GpuResidency::OnStorage || isRenderWindowSpecific() ||
                         ( isRenderToTexture() && mWidth == mHeight ) );
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        mOrientationMode = orientationMode;
#endif
    }
    //-----------------------------------------------------------------------------------
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
    OrientationMode GL3PlusTextureGpuRenderTarget::getOrientationMode() const
    {
        return mOrientationMode;
    }
#endif
}  // namespace Ogre
