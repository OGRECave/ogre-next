/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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
#include "OgreGL3PlusTextureGpuManager.h"
#include "OgreGL3PlusSupport.h"

#include "OgreTextureGpuListener.h"
#include "OgreTextureBox.h"
#include "OgreVector2.h"

#include "Vao/OgreVaoManager.h"
#include "OgreRenderSystem.h"

#include "OgreException.h"

#define TODO_use_StagingTexture_with_GPU_GPU_visibility 1
#define TODO_sw_fallback 1

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
        _setToDisplayDummyTexture();
    }
    //-----------------------------------------------------------------------------------
    GL3PlusTextureGpu::~GL3PlusTextureGpu()
    {
        destroyInternalResourcesImpl();
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::createInternalResourcesImpl(void)
    {
        GLenum format = GL3PlusMappings::get( mPixelFormat );

        if( mMsaa <= 1u || !hasMsaaExplicitResolves() )
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
                             "Texture '" + getNameStr() + "': "
                             "Ogre should never hit this path",
                             "GL3PlusTextureGpu::createInternalResourcesImpl" );
                break;
            case TextureTypes::Type1D:
                OCGE( glTexStorage1D( GL_TEXTURE_1D, GLsizei(mNumMipmaps), format, GLsizei(mWidth) ) );
                break;
            case TextureTypes::Type1DArray:
                OCGE( glTexStorage2D( GL_TEXTURE_1D_ARRAY, GLsizei(mNumMipmaps), format,
                                      GLsizei(mWidth), GLsizei(mDepthOrSlices) ) );
                break;
            case TextureTypes::Type2D:
                OCGE( glTexStorage2D( GL_TEXTURE_2D, GLsizei(mNumMipmaps), format,
                                      GLsizei(mWidth), GLsizei(mHeight) ) );
                break;
            case TextureTypes::Type2DArray:
                OCGE( glTexStorage3D( GL_TEXTURE_2D_ARRAY, GLsizei(mNumMipmaps), format,
                                      GLsizei(mWidth), GLsizei(mHeight), GLsizei(mDepthOrSlices) ) );
                break;
            case TextureTypes::TypeCube:
                OCGE( glTexStorage2D( GL_TEXTURE_CUBE_MAP, GLsizei(mNumMipmaps), format,
                                      GLsizei(mWidth), GLsizei(mHeight) ) );
                break;
            case TextureTypes::TypeCubeArray:
                OCGE( glTexStorage3D( GL_TEXTURE_CUBE_MAP_ARRAY, GLsizei(mNumMipmaps), format,
                                      GLsizei(mWidth), GLsizei(mHeight), GLsizei(mDepthOrSlices * 6u) ) );
                break;
            case TextureTypes::Type3D:
                OCGE( glTexStorage3D( GL_TEXTURE_3D, GLsizei(mNumMipmaps), format,
                                      GLsizei(mWidth), GLsizei(mHeight), GLsizei(mDepthOrSlices) ) );
                break;
            }

            //Allocate internal buffers for automipmaps before we load anything into them
            if( allowsAutoMipmaps() )
                OCGE( glGenerateMipmap( mGlTextureTarget ) );

            //Set debug name for RenderDoc and similar tools
            ogreGlObjectLabel( GL_TEXTURE, mFinalTextureName, getNameStr() );
        }

        if( mMsaa > 1u )
        {
            //const GLboolean fixedsamplelocations = mMsaaPattern != MsaaPatterns::Undefined;
            //RENDERBUFFERS have fixedsamplelocations implicitly set to true. Be consistent
            //with non-texture depth buffers.
            const GLboolean fixedsamplelocations = GL_TRUE;

            if( !isTexture() ||
                (!hasMsaaExplicitResolves() && !PixelFormatGpuUtils::isDepth( mPixelFormat )) )
            {
                OCGE( glGenRenderbuffers( 1, &mMsaaFramebufferName ) );
                OCGE( glBindRenderbuffer( GL_RENDERBUFFER, mMsaaFramebufferName ) );
                OCGE( glRenderbufferStorageMultisample( GL_RENDERBUFFER, mMsaa, format,
                                                        GLsizei(mWidth), GLsizei(mHeight) ) );
                OCGE( glBindRenderbuffer( GL_RENDERBUFFER, 0 ) );

                //Set debug name for RenderDoc and similar tools
                ogreGlObjectLabel( GL_RENDERBUFFER, mMsaaFramebufferName,
                                   getNameStr() + "/MsaaImplicit" );
            }
            else
            {
                OCGE( glGenTextures( 1u, &mFinalTextureName ) );

                assert( mTextureType == TextureTypes::Type2D ||
                        mTextureType == TextureTypes::Type2DArray );
                mGlTextureTarget =
                        mTextureType == TextureTypes::Type2D ? GL_TEXTURE_2D_MULTISAMPLE :
                                                               GL_TEXTURE_2D_MULTISAMPLE_ARRAY;

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
                    glTexImage2DMultisample( mGlTextureTarget, mMsaa, format,
                                             GLsizei(mWidth), GLsizei(mHeight), fixedsamplelocations );
                }
                else
                {
                    glTexImage3DMultisample( mGlTextureTarget, mMsaa, format,
                                             GLsizei(mWidth), GLsizei(mHeight), GLsizei(mDepthOrSlices),
                                             fixedsamplelocations );
                }

                //Set debug name for RenderDoc and similar tools
                ogreGlObjectLabel( GL_TEXTURE, mFinalTextureName, getNameStr() );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::destroyInternalResourcesImpl(void)
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
                if( mMsaa > 1u && !hasMsaaExplicitResolves() )
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
                //This will end up calling _notifyTextureSlotChanged,
                //setting mTexturePool & mInternalSliceStart to 0
                mTextureManager->_releaseSlotFromTexture( this );
            }

            mFinalTextureName = 0;
            mMsaaFramebufferName = 0;
        }

        _setToDisplayDummyTexture();
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::notifyDataIsReady(void)
    {
        assert( mResidencyStatus == GpuResidency::Resident );
        assert( mFinalTextureName || mPixelFormat == PFG_NULL );

        mDisplayTextureName = mFinalTextureName;

        notifyAllListenersTextureChanged( TextureGpuListener::ReadyForRendering );
    }
    //-----------------------------------------------------------------------------------
    bool GL3PlusTextureGpu::_isDataReadyImpl(void) const
    {
        return mDisplayTextureName == mFinalTextureName;
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::_setToDisplayDummyTexture(void)
    {
        if( !mTextureManager )
        {
            assert( isRenderWindowSpecific() );
            return; //This can happen if we're a window and we're on shutdown
        }

        GL3PlusTextureGpuManager *textureManagerGl =
                static_cast<GL3PlusTextureGpuManager*>( mTextureManager );
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
            assert( dynamic_cast<GL3PlusTextureGpu*>( mTexturePool->masterTexture ) );
            GL3PlusTextureGpu *masterTexture = static_cast<GL3PlusTextureGpu*>(mTexturePool->masterTexture);
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
    bool GL3PlusTextureGpu::isRenderbuffer(void) const
    {
        const bool isDepth = PixelFormatGpuUtils::isDepth( mPixelFormat );

        return  ( mMsaa > 1u && ((!hasMsaaExplicitResolves() && !isDepth) || !isTexture()) ) ||
                ( isDepth && !isTexture() ) ||
                isRenderWindowSpecific();
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::bindTextureToFrameBuffer( GLenum target, uint8 mipLevel,
                                                      uint32 depthOrSlice )
    {
        bindTextureToFrameBuffer( target, mFinalTextureName, mipLevel, depthOrSlice );
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::bindTextureToFrameBuffer( GLenum target, GLuint textureName,
                                                      uint8 mipLevel, uint32 depthOrSlice )
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
                OCGE( glFramebufferRenderbuffer( target, GL_DEPTH_ATTACHMENT,
                                                 GL_RENDERBUFFER, textureName ) );
                if( PixelFormatGpuUtils::isStencil( mPixelFormat ) )
                {
                    OCGE( glFramebufferRenderbuffer( target, GL_STENCIL_ATTACHMENT,
                                                     GL_RENDERBUFFER, textureName ) );
                }
            }
        }
        else
        {
            if( mMsaa > 1u && (!hasMsaaExplicitResolves() || !isTexture()) )
            {
                OCGE( glFramebufferRenderbuffer( target, GL_COLOR_ATTACHMENT0,
                                                 GL_RENDERBUFFER, mMsaaFramebufferName ) );
            }
            else
            {
                const bool hasLayers = mTextureType != TextureTypes::Type1D &&
                                       mTextureType != TextureTypes::Type2D;
                if( !hasLayers )
                {
                    OCGE( glFramebufferTexture( target, GL_COLOR_ATTACHMENT0,
                                                textureName, mipLevel ) );
                }
                else
                {
                    OCGE( glFramebufferTextureLayer( target, GL_COLOR_ATTACHMENT0,
                                                     textureName, mipLevel, depthOrSlice ) );
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::copyViaFramebuffer( TextureGpu *dst, const TextureBox &dstBox,
                                                uint8 dstMipLevel, const TextureBox &srcBox,
                                                uint8 srcMipLevel )
    {
        RenderSystem *renderSystem = mTextureManager->getRenderSystem();
        renderSystem->endRenderPassDescriptor();

        GL3PlusTextureGpuManager *textureManagerGl =
                static_cast<GL3PlusTextureGpuManager*>( mTextureManager );

        assert( dynamic_cast<GL3PlusTextureGpu*>( dst ) );
        GL3PlusTextureGpu *dstGl = static_cast<GL3PlusTextureGpu*>( dst );

        const bool srcIsFboAble = this->isRenderbuffer() || this->isRenderToTexture();
        const bool dstIsFboAble = dstGl->isRenderbuffer() || dst->isRenderToTexture();

        if( !this->isRenderWindowSpecific() && srcIsFboAble )
        {
            OCGE( glBindFramebuffer( GL_READ_FRAMEBUFFER, textureManagerGl->getTemporaryFbo(0) ) );
        }
        else
        {
            OCGE( glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 ) );
        }

        if( !dst->isRenderWindowSpecific() && dstIsFboAble )
        {
            OCGE( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, textureManagerGl->getTemporaryFbo(1) ) );
        }
        else
        {
            OCGE( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
        }
        OCGE( glViewport( 0, 0, dstBox.width, dstBox.height ) );

        size_t depthOrSlices = srcBox.getDepthOrSlices();
        for( size_t i=0; i<depthOrSlices; ++i )
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
                //We can use glCopyTexImageXD
                //https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glCopyTexImage2D.xhtml
                GLenum texTarget = dstGl->mGlTextureTarget;
                if( dst->getTextureType() == TextureTypes::TypeCube )
                    texTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + dstBox.sliceStart;

                switch( dst->getTextureType() )
                {
                case TextureTypes::Type1D:
                    OCGE( glCopyTexSubImage1D( GL_TEXTURE_1D, dstMipLevel,
                                               dstBox.x,
                                               srcBox.x, srcBox.y,
                                               srcBox.width ) );
                    break;
                case TextureTypes::Type1DArray:
                    OCGE( glCopyTexSubImage2D( GL_TEXTURE_1D_ARRAY, dstMipLevel,
                                               dstBox.x, dstBox.sliceStart,
                                               srcBox.x, srcBox.sliceStart,
                                               srcBox.width, srcBox.height ) );
                    break;
                case TextureTypes::Unknown:
                case TextureTypes::Type2D:
                case TextureTypes::TypeCube:
                    OCGE( glCopyTexSubImage2D( texTarget, dstMipLevel,
                                               dstBox.x, dstBox.y,
                                               srcBox.x, srcBox.y,
                                               srcBox.width, srcBox.height ) );
                    break;
                case TextureTypes::Type2DArray:
                case TextureTypes::TypeCubeArray:
                case TextureTypes::Type3D:
                    OCGE( glCopyTexSubImage3D( texTarget, dstMipLevel,
                                               dstBox.x, dstBox.y, dstBox.getZOrSlice(),
                                               srcBox.x, srcBox.y,
                                               srcBox.width, srcBox.height ) );
                    break;
                }
            }
            else if( srcIsFboAble && dstIsFboAble )
            {
                GLenum readStatus, drawStatus;
                OCGE( readStatus = glCheckFramebufferStatus( GL_READ_FRAMEBUFFER ) );
                OCGE( drawStatus = glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER ) );

                const bool supported = readStatus == GL_FRAMEBUFFER_COMPLETE &&
                                       drawStatus == GL_FRAMEBUFFER_COMPLETE;

                if( supported )
                {
                    GLbitfield bufferBits = 0;
                    if( PixelFormatGpuUtils::isDepth( this->mPixelFormat ) )
                        bufferBits |= GL_DEPTH_BUFFER_BIT;
                    if( PixelFormatGpuUtils::isStencil( this->mPixelFormat ) )
                        bufferBits |= GL_STENCIL_BUFFER_BIT;
                    if( !bufferBits )
                        bufferBits = GL_COLOR_BUFFER_BIT;

                    GLint srcX0 = srcBox.x;
                    GLint srcX1 = srcBox.x + srcBox.width;;
                    GLint srcY0;
                    GLint srcY1;
                    if( this->isRenderWindowSpecific() )
                    {
                        srcY0 = this->mHeight - srcBox.y;
                        srcY1 = this->mHeight - srcBox.y - srcBox.height;
                    }
                    else
                    {
                        srcY0 = srcBox.y;
                        srcY1 = srcBox.y + srcBox.height;
                    }
                    GLint dstX0 = dstBox.x;
                    GLint dstX1 = dstBox.x + dstBox.width;
                    GLint dstY0;
                    GLint dstY1;
                    if( dst->isRenderWindowSpecific() )
                    {
                        dstY0 = dstGl->mHeight - dstBox.y;
                        dstY1 = dstGl->mHeight - dstBox.y - dstBox.height;
                    }
                    else
                    {
                        dstY0 = dstBox.y;
                        dstY1 = dstBox.y + dstBox.height;
                    }

                    OCGE( glBlitFramebuffer( srcX0, srcY0, srcX1, srcY1,
                                             dstX0, dstY0, dstX1, dstY1,
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
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::copyTo( TextureGpu *dst, const TextureBox &dstBox, uint8 dstMipLevel,
                                    const TextureBox &srcBox, uint8 srcMipLevel )
    {
        TextureGpu::copyTo( dst, dstBox, dstMipLevel, srcBox, srcMipLevel );

        assert( dynamic_cast<GL3PlusTextureGpu*>( dst ) );

        GL3PlusTextureGpu *dstGl = static_cast<GL3PlusTextureGpu*>( dst );
        GL3PlusTextureGpuManager *textureManagerGl =
                static_cast<GL3PlusTextureGpuManager*>( mTextureManager );
        const GL3PlusSupport &support = textureManagerGl->getGlSupport();

        if( !this->isRenderWindowSpecific() && !dst->isRenderWindowSpecific() )
        {
            GLuint srcTextureName = this->mFinalTextureName;
            GLuint dstTextureName = dstGl->mFinalTextureName;

            //Source has explicit resolves. If destination doesn't,
            //we must copy to its internal MSAA surface.
            if( this->mMsaa > 1u && this->hasMsaaExplicitResolves() )
            {
                if( !dstGl->hasMsaaExplicitResolves() )
                    dstTextureName = dstGl->mMsaaFramebufferName;
            }
            //Destination has explicit resolves. If source doesn't,
            //we must copy from its internal MSAA surface.
            if( dstGl->mMsaa > 1u && dstGl->hasMsaaExplicitResolves() )
            {
                if( !this->hasMsaaExplicitResolves() )
                    srcTextureName = this->mMsaaFramebufferName;
            }

            if( support.hasMinGLVersion( 4, 3 ) || support.checkExtension( "GL_ARB_copy_image" ) )
            {
                OCGE( glCopyImageSubData( this->mFinalTextureName, this->mGlTextureTarget,
                                          srcMipLevel, srcBox.x, srcBox.y,
                                          srcBox.getZOrSlice() + this->getInternalSliceStart(),
                                          dstGl->mFinalTextureName, dstGl->mGlTextureTarget,
                                          dstMipLevel, dstBox.x, dstBox.y,
                                          dstBox.getZOrSlice() + dstGl->getInternalSliceStart(),
                                          srcBox.width, srcBox.height, srcBox.getDepthOrSlices() ) );
            }
            /*TODO
            else if( support.checkExtension( "GL_NV_copy_image" ) )
            {
                OCGE( glCopyImageSubDataNV( this->mFinalTextureName, this->mGlTextureTarget,
                                            srcMipLevel, srcBox.x, srcBox.y, srcBox.z,
                                            dstGl->mFinalTextureName, dstGl->mGlTextureTarget,
                                            dstMipLevel, dstBox.x, dstBox.y, dstBox.z,
                                            srcBox.width, srcBox.height, srcBox.getDepthOrSlices() ) );
            }*/
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
                //glGetCompressedTexImage
                TODO_use_StagingTexture_with_GPU_GPU_visibility;
                OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "", "GL3PlusTextureGpu::copyTo" );
            }

            //Must keep the resolved texture up to date.
            if( dstGl->mMsaa > 1u && !dstGl->hasMsaaExplicitResolves() )
            {
                OCGE( glBindFramebuffer( GL_READ_FRAMEBUFFER, textureManagerGl->getTemporaryFbo(0) ) );
                OCGE( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, textureManagerGl->getTemporaryFbo(1) ) );
                OCGE( glViewport( 0, 0, srcBox.width, srcBox.height ) );

                OCGE( glReadBuffer( GL_COLOR_ATTACHMENT0 ) );
                OCGE( glDrawBuffer( GL_COLOR_ATTACHMENT0 ) );

                for( size_t i=0; i<dstBox.numSlices; ++i )
                {
                    dstGl->bindTextureToFrameBuffer( GL_READ_FRAMEBUFFER, dstGl->mMsaaFramebufferName,
                                                     0, dstBox.getZOrSlice() + i );
                    dstGl->bindTextureToFrameBuffer( GL_DRAW_FRAMEBUFFER, dstGl->mFinalTextureName,
                                                     dstMipLevel, dstBox.getZOrSlice() + i );

                    OCGE( glBlitFramebuffer( 0, 0, srcBox.width, srcBox.height,
                                             0, 0, srcBox.width, srcBox.height,
                                             GL_COLOR_BUFFER_BIT, GL_NEAREST ) );
                }

                OCGE( glFramebufferRenderbuffer( GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                 GL_RENDERBUFFER, 0 ) );
                OCGE( glFramebufferRenderbuffer( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                 GL_RENDERBUFFER, 0 ) );
                OCGE( glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 ) );
                OCGE( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
            }
        }
        else
        {
            this->copyViaFramebuffer( dst, dstBox, dstMipLevel, srcBox, srcMipLevel );
        }

        //Do not perform the sync if notifyDataIsReady hasn't been called yet (i.e. we're
        //still building the HW mipmaps, and the texture will never be ready)
        if( dst->_isDataReadyImpl() &&
            dst->getGpuPageOutStrategy() == GpuPageOutStrategy::AlwaysKeepSystemRamCopy )
        {
            dst->_syncGpuResidentToSystemRam();
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpu::_autogenerateMipmaps(void)
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
        locations.reserve( mMsaa );
        if( mMsaa <= 1u )
        {
            locations.push_back( Vector2( 0.0f, 0.0f ) );
        }
        else
        {
            assert( mMsaaPattern != MsaaPatterns::Undefined );

            float vals[2];
            for( int i=0; i<mMsaa; ++i )
            {
                glGetMultisamplefv( GL_SAMPLE_POSITION, i, vals );
                locations.push_back( Vector2( vals[0], vals[1] ) * 2.0f - 1.0f );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    GL3PlusTextureGpuRenderTarget::GL3PlusTextureGpuRenderTarget(
            GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
            VaoManager *vaoManager, IdString name, uint32 textureFlags,
            TextureTypes::TextureTypes initialType,
            TextureGpuManager *textureManager ) :
        GL3PlusTextureGpu( pageOutStrategy, vaoManager, name,
                           textureFlags, initialType, textureManager ),
        mDepthBufferPoolId( 1u ),
        mPreferDepthTexture( false ),
        mDesiredDepthBufferFormat( PFG_UNKNOWN )
    {
        if( mPixelFormat == PFG_NULL )
            mDepthBufferPoolId = 0;
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuRenderTarget::createInternalResourcesImpl(void)
    {
        if( mPixelFormat == PFG_NULL )
            return; //Nothing to do

        if( isTexture() || !PixelFormatGpuUtils::isDepth( mPixelFormat ) )
        {
            GL3PlusTextureGpu::createInternalResourcesImpl();
        }
        else
        {
            OCGE( glGenRenderbuffers( 1, &mFinalTextureName ) );
            OCGE( glBindRenderbuffer( GL_RENDERBUFFER, mFinalTextureName ) );

            GLenum format = GL3PlusMappings::get( mPixelFormat );

            if( mMsaa <= 1u )
            {
                OCGE( glRenderbufferStorage( GL_RENDERBUFFER, format,
                                             GLsizei(mWidth), GLsizei(mHeight) ) );
            }
            else
            {
                OCGE( glRenderbufferStorageMultisample( GL_RENDERBUFFER, GLsizei(mMsaa), format,
                                                        GLsizei(mWidth), GLsizei(mHeight) ) );
            }

            //Set debug name for RenderDoc and similar tools
            ogreGlObjectLabel( GL_RENDERBUFFER, mFinalTextureName, getNameStr() );
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuRenderTarget::destroyInternalResourcesImpl(void)
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
        mDepthBufferPoolId          = depthBufferPoolId;
        mPreferDepthTexture         = preferDepthTexture;
        mDesiredDepthBufferFormat   = desiredDepthBufferFormat;
    }
    //-----------------------------------------------------------------------------------
    uint16 GL3PlusTextureGpuRenderTarget::getDepthBufferPoolId(void) const
    {
        return mDepthBufferPoolId;
    }
    //-----------------------------------------------------------------------------------
    bool GL3PlusTextureGpuRenderTarget::getPreferDepthTexture(void) const
    {
        return mPreferDepthTexture;
    }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu GL3PlusTextureGpuRenderTarget::getDesiredDepthBufferFormat(void) const
    {
        return mDesiredDepthBufferFormat;
    }
}
