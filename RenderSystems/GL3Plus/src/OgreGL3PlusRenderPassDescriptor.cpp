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

#include "OgreStableHeaders.h"

#include "OgreGL3PlusRenderPassDescriptor.h"

#include "OgreGL3PlusTextureGpu.h"
#include "OgreGL3PlusRenderSystem.h"

#include "OgreHlmsDatablock.h"
#include "OgrePixelFormatGpuUtils.h"

namespace Ogre
{
    GL3PlusRenderPassDescriptor::GL3PlusRenderPassDescriptor( GL3PlusRenderSystem *renderSystem ) :
        mFboName( 0 ),
        mFboMsaaResolve( 0 ),
        mAllClearColoursSetAndIdentical( false ),
        mAnyColourLoadActionsSetToClear( false ),
        mHasRenderWindow( false ),
        mSharedFboItor( renderSystem->_getFrameBufferDescMap().end() ),
        mRenderSystem( renderSystem )
    {
    }
    //-----------------------------------------------------------------------------------
    GL3PlusRenderPassDescriptor::~GL3PlusRenderPassDescriptor()
    {
        if( mFboMsaaResolve )
        {
            OCGE( glDeleteFramebuffers( 1, &mFboMsaaResolve ) );
            mFboMsaaResolve = 0;
        }

        FrameBufferDescMap &frameBufferDescMap = mRenderSystem->_getFrameBufferDescMap();
        if( mSharedFboItor != frameBufferDescMap.end() )
        {
            --mSharedFboItor->second.refCount;
            if( !mSharedFboItor->second.refCount )
            {
                OCGE( glDeleteFramebuffers( 1, &mSharedFboItor->second.fboName ) );
                frameBufferDescMap.erase( mSharedFboItor );
            }
            mSharedFboItor = frameBufferDescMap.end();
            mFboName = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusRenderPassDescriptor::checkRenderWindowStatus(void)
    {
        if( (mNumColourEntries > 0 && mColour[0].texture->isRenderWindowSpecific()) ||
            (mDepth.texture && mDepth.texture->isRenderWindowSpecific()) ||
            (mStencil.texture && mStencil.texture->isRenderWindowSpecific()) )
        {
            if( mNumColourEntries > 1u )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Cannot use RenderWindow as MRT with other colour textures",
                             "GL3PlusRenderPassDescriptor::colourEntriesModified" );
            }

            if( (mNumColourEntries > 0 && !mColour[0].texture->isRenderWindowSpecific()) ||
                (mDepth.texture && !mDepth.texture->isRenderWindowSpecific()) ||
                (mStencil.texture && !mStencil.texture->isRenderWindowSpecific()) )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Cannot mix RenderWindow colour texture with depth or stencil buffer "
                             "that aren't for RenderWindows, or viceversa",
                             "GL3PlusRenderPassDescriptor::checkRenderWindowStatus" );
            }

            switchToRenderWindow();
        }
        else
        {
            switchToFBO();
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusRenderPassDescriptor::switchToRenderWindow(void)
    {
        FrameBufferDescMap &frameBufferDescMap = mRenderSystem->_getFrameBufferDescMap();
        if( mSharedFboItor != frameBufferDescMap.end() )
        {
            --mSharedFboItor->second.refCount;
            if( !mSharedFboItor->second.refCount )
            {
                OCGE( glDeleteFramebuffers( 1, &mSharedFboItor->second.fboName ) );
                frameBufferDescMap.erase( mSharedFboItor );
            }
            mSharedFboItor = frameBufferDescMap.end();
            mFboName = 0;
        }

        mHasRenderWindow = true;
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusRenderPassDescriptor::switchToFBO(void)
    {
        FrameBufferDescKey key( *this );
        FrameBufferDescMap &frameBufferDescMap = mRenderSystem->_getFrameBufferDescMap();
        FrameBufferDescMap::iterator newItor = frameBufferDescMap.find( key );

        if( newItor == frameBufferDescMap.end() )
        {
            FrameBufferDescValue value;
            value.refCount = 0;

            OCGE( glGenFramebuffers( 1, &value.fboName ) );

            OCGE( glBindFramebuffer( GL_FRAMEBUFFER, value.fboName ) );

            //Disable target independent rasterization to let the driver warn us
            //of wrong behavior during regular rendering.
            OCGE( glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, 0 ) );
            OCGE( glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, 0 ) );
            frameBufferDescMap[key] = value;
            newItor = frameBufferDescMap.find( key );
        }

        ++newItor->second.refCount;

        if( mSharedFboItor != frameBufferDescMap.end() )
        {
            --mSharedFboItor->second.refCount;
            if( !mSharedFboItor->second.refCount )
            {
                OCGE( glDeleteFramebuffers( 1, &mSharedFboItor->second.fboName ) );
                frameBufferDescMap.erase( mSharedFboItor );
            }
        }

        mSharedFboItor = newItor;
        mFboName = mSharedFboItor->second.fboName;
        mHasRenderWindow = false;
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusRenderPassDescriptor::analyzeClearColour(void)
    {
        if( !mNumColourEntries )
        {
            mAllClearColoursSetAndIdentical = false;
            mAnyColourLoadActionsSetToClear = false;
            return;
        }

        const RenderSystemCapabilities *capabilities = mRenderSystem->getCapabilities();
        const bool isTiler = capabilities->hasCapability( RSC_IS_TILER );

        mAllClearColoursSetAndIdentical = true;
        ColourValue lastClearColour = mColour[0].clearColour;

        for( size_t i=0u; i<mNumColourEntries; ++i )
        {
            bool performsClear = mColour[i].loadAction == LoadAction::Clear ||
                                 (isTiler && (mColour[i].loadAction == LoadAction::DontCare ||
                                              mColour[i].loadAction == LoadAction::ClearOnTilers ));
            if( performsClear )
                mAnyColourLoadActionsSetToClear = true;
            if( !performsClear || lastClearColour != mColour[i].clearColour )
                mAllClearColoursSetAndIdentical = false;
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusRenderPassDescriptor::updateColourFbo( uint8 lastNumColourEntries )
    {
        if( mNumColourEntries < lastNumColourEntries && !mHasRenderWindow )
        {
            for( size_t i=mNumColourEntries; i<lastNumColourEntries; ++i )
            {
                //Detach removed colour entries
                OCGE( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                                                 GL_RENDERBUFFER, 0 ) );
            }
        }

        if( !mHasRenderWindow )
        {
            OCGE( glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, 0 ) );
            OCGE( glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, 0 ) );
            OCGE( glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_SAMPLES, 0 ) );
        }

        bool needsMsaaResolveFbo = false;

        //Attach colour entries
        for( size_t i=0; i<mNumColourEntries; ++i )
        {
            if( mColour[i].texture->getResidencyStatus() != GpuResidency::Resident )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "RenderTexture '" +
                             mColour[i].texture->getNameStr() + "' must be resident!",
                             "GL3PlusRenderPassDescriptor::updateColourFbo" );
            }

            if( !mHasRenderWindow && mColour[i].texture->getPixelFormat() != PFG_NULL )
            {
                assert( dynamic_cast<GL3PlusTextureGpu*>( mColour[i].texture ) );
                GL3PlusTextureGpu *texture = static_cast<GL3PlusTextureGpu*>( mColour[i].texture );

                if( texture->isRenderWindowSpecific() )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "Cannot use RenderWindow as MRT with other colour textures",
                                 "GL3PlusRenderPassDescriptor::colourEntriesModified" );
                }

                TextureTypes::TextureTypes textureType = mColour[i].texture->getTextureType();
                const bool hasLayers = textureType != TextureTypes::Type1D &&
                                       textureType != TextureTypes::Type2D;

                if( mColour[i].allLayers || !hasLayers )
                {
                    if( texture->getMsaa() > 1u && (!texture->hasMsaaExplicitResolves() ||
                                                    !texture->isTexture()) )
                    {
                        OCGE( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                                                         GL_RENDERBUFFER,
                                                         texture->getMsaaFramebufferName() ) );
                    }
                    else
                    {
                        OCGE( glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                                                    texture->getFinalTextureName(),
                                                    mColour[i].mipLevel ) );
                    }
                }
                else
                {
                    if( texture->getMsaa() > 1u && (!texture->hasMsaaExplicitResolves() ||
                                                    !texture->isTexture()) )
                    {
                        OCGE( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                                                         GL_RENDERBUFFER,
                                                         texture->getMsaaFramebufferName() ) );
                    }
                    else
                    {
                        OCGE( glFramebufferTextureLayer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                                                         texture->getFinalTextureName(),
                                                         mColour[i].mipLevel, mColour[i].slice ) );
                    }
                }
            }
            else if( mColour[i].texture->getPixelFormat() == PFG_NULL )
            {
                OCGE( glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH,
                                               mColour[i].texture->getWidth() ) );
                OCGE( glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT,
                                               mColour[i].texture->getHeight() ) );

                OCGE( glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_SAMPLES,
                                               mColour[i].texture->getMsaa() > 1u ?
                                                   mColour[i].texture->getMsaa() : 0 ) );
            }

            if( (mColour[i].storeAction == StoreAction::MultisampleResolve ||
                 mColour[i].storeAction == StoreAction::StoreAndMultisampleResolve) &&
                mColour[i].resolveTexture )
            {
                needsMsaaResolveFbo = true;
            }
        }

        analyzeClearColour();

        if( needsMsaaResolveFbo && !mFboMsaaResolve )
        {
            OCGE( glGenFramebuffers( 1, &mFboMsaaResolve ) );
        }
        else if( !needsMsaaResolveFbo && mFboMsaaResolve )
        {
            OCGE( glDeleteFramebuffers( 1, &mFboMsaaResolve ) );
            mFboMsaaResolve = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusRenderPassDescriptor::updateDepthFbo(void)
    {
        if( mHasRenderWindow )
            return;

        if( !mDepth.texture )
        {
            OCGE( glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 0, 0 ) );
            return;
        }

        if( mDepth.texture->getResidencyStatus() != GpuResidency::Resident )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "RenderTexture '" +
                         mDepth.texture->getNameStr() + "' must be resident!",
                         "GL3PlusRenderPassDescriptor::updateDepthFbo" );
        }

        assert( dynamic_cast<GL3PlusTextureGpu*>( mDepth.texture ) );
        GL3PlusTextureGpu *texture = static_cast<GL3PlusTextureGpu*>( mDepth.texture );

        if( texture->isTexture() )
        {
            OCGE( glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                        texture->getFinalTextureName(), 0 ) );
        }
        else
        {
            OCGE( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                             GL_RENDERBUFFER, texture->getFinalTextureName() ) );
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusRenderPassDescriptor::updateStencilFbo(void)
    {
        if( mHasRenderWindow )
            return;

        if( !mStencil.texture )
        {
            OCGE( glFramebufferTexture( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, 0, 0 ) );
            return;
        }

        if( mStencil.texture->getResidencyStatus() != GpuResidency::Resident )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "RenderTexture '" +
                         mStencil.texture->getNameStr() + "' must be resident!",
                         "GL3PlusRenderPassDescriptor::updateStencilFbo" );
        }

        assert( dynamic_cast<GL3PlusTextureGpu*>( mStencil.texture ) );
        GL3PlusTextureGpu *texture = static_cast<GL3PlusTextureGpu*>( mStencil.texture );

        if( texture->isTexture() )
        {
            OCGE( glFramebufferTexture( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                        texture->getFinalTextureName(), 0 ) );
        }
        else
        {
            OCGE( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                             GL_RENDERBUFFER, texture->getFinalTextureName() ) );
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusRenderPassDescriptor::entriesModified( uint32 entryTypes )
    {
        uint8 lastNumColourEntries = mNumColourEntries;
        RenderPassDescriptor::entriesModified( entryTypes );

        checkRenderWindowStatus();

        OCGE( glBindFramebuffer( GL_FRAMEBUFFER, mFboName ) );

        if( entryTypes & RenderPassDescriptor::Colour )
            updateColourFbo( lastNumColourEntries );

        if( entryTypes & RenderPassDescriptor::Depth )
            updateDepthFbo();

        if( entryTypes & RenderPassDescriptor::Stencil )
            updateStencilFbo();
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusRenderPassDescriptor::setClearColour( uint8 idx, const ColourValue &clearColour )
    {
        RenderPassDescriptor::setClearColour( idx, clearColour );
        analyzeClearColour();
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusRenderPassDescriptor::setClearColour( const ColourValue &clearColour )
    {
        for( uint8 i=0; i<mNumColourEntries; ++i )
            RenderPassDescriptor::setClearColour( i, clearColour );

        analyzeClearColour();
    }
    //-----------------------------------------------------------------------------------
    uint32 GL3PlusRenderPassDescriptor::checkForClearActions( GL3PlusRenderPassDescriptor *other ) const
    {
        uint32 entriesToFlush = 0;

        assert( this->mFboName == other->mFboName );
        assert( this->mNumColourEntries == other->mNumColourEntries );

        const RenderSystemCapabilities *capabilities = mRenderSystem->getCapabilities();
        const bool isTiler = capabilities->hasCapability( RSC_IS_TILER );

        for( size_t i=0; i<mNumColourEntries; ++i )
        {
            //this->mColour[i].allLayers doesn't need to be analyzed
            //because it requires a different FBO.
            if( other->mColour[i].loadAction == LoadAction::Clear ||
                (isTiler && mColour[i].loadAction == LoadAction::ClearOnTilers) )
            {
                entriesToFlush |= RenderPassDescriptor::Colour0 << i;
            }
        }

        if( other->mDepth.loadAction == LoadAction::Clear ||
            (isTiler && mDepth.loadAction == LoadAction::ClearOnTilers) )
        {
            entriesToFlush |= RenderPassDescriptor::Depth;
        }

        if( other->mStencil.loadAction == LoadAction::Clear ||
            (isTiler && mStencil.loadAction == LoadAction::ClearOnTilers) )
        {
            entriesToFlush |= RenderPassDescriptor::Stencil;
        }

        return entriesToFlush;
    }
    //-----------------------------------------------------------------------------------
    uint32 GL3PlusRenderPassDescriptor::willSwitchTo( GL3PlusRenderPassDescriptor *newDesc,
                                                      bool warnIfRtvWasFlushed ) const
    {
        uint32 entriesToFlush = 0;

        if( !newDesc ||
            this->mFboName != newDesc->mFboName ||
            this->mInformationOnly || newDesc->mInformationOnly )
        {
            entriesToFlush = RenderPassDescriptor::All;
        }
        else
            entriesToFlush |= checkForClearActions( newDesc );

        if( warnIfRtvWasFlushed )
            newDesc->checkWarnIfRtvWasFlushed( entriesToFlush );

        return entriesToFlush;
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusRenderPassDescriptor::performLoadActions( uint8 blendChannelMask,
                                                          bool depthWrite,
                                                          uint32 stencilWriteMask,
                                                          uint32 entriesToFlush )
    {
        if( mInformationOnly )
            return;

        OCGE( glBindFramebuffer( GL_FRAMEBUFFER, mFboName ) );

        if( mHasRenderWindow )
        {
            if( !mNumColourEntries )
            {
                //Do not render to colour Render Windows.
                OCGE( glDrawBuffer( GL_NONE ) );
            }
            else
            {
                //Make sure colour writes are enabled for RenderWindows.
                OCGE( glDrawBuffer( GL_BACK ) );
            }
        }
        else
        {
            GLenum colourBuffs[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
            for( int i=0; i<mNumColourEntries; ++i )
                colourBuffs[i] = GL_COLOR_ATTACHMENT0 + i;
            OCGE( glDrawBuffers( mNumColourEntries, colourBuffs ) );
        }

        OCGE( glEnable( GL_FRAMEBUFFER_SRGB ) );

        const RenderSystemCapabilities *capabilities = mRenderSystem->getCapabilities();
        const bool isTiler = capabilities->hasCapability( RSC_IS_TILER );

        bool colourMask = blendChannelMask != HlmsBlendblock::BlendChannelAll;

        GLbitfield flags = 0;

        uint32 fullColourMask = (1u << mNumColourEntries) - 1u;
        if( entriesToFlush & RenderPassDescriptor::Colour )
        {
            if( mNumColourEntries > 0 && mAllClearColoursSetAndIdentical &&
                (entriesToFlush & fullColourMask) == fullColourMask )
            {
                //All at the same time
                flags |= GL_COLOR_BUFFER_BIT;

                if( colourMask )
                {
                    //Enable buffer for writing if it isn't
                    OCGE( glColorMask( true, true, true, true ) );
                }
                const ColourValue &clearColour = mColour[0].clearColour;
                OCGE( glClearColor( clearColour.r, clearColour.g, clearColour.b, clearColour.a ) );
            }
            else if( mNumColourEntries > 0 && mAnyColourLoadActionsSetToClear )
            {
                //Clear one at a time
                if( colourMask )
                {
                    //Enable buffer for writing if it isn't
                    OCGE( glColorMask( true, true, true, true ) );
                }

                OCGE( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, mFboName ) );

                for( size_t i=0; i<mNumColourEntries; ++i )
                {
                    if( (entriesToFlush & (RenderPassDescriptor::Colour0 << i)) &&
                        (mColour[i].loadAction == LoadAction::Clear ||
                         (isTiler && (mColour[i].loadAction == LoadAction::DontCare ||
                                      mColour[i].loadAction == LoadAction::ClearOnTilers))) )
                    {
                        GLfloat clearColour[4];
                        clearColour[0] = mColour[i].clearColour.r;
                        clearColour[1] = mColour[i].clearColour.g;
                        clearColour[2] = mColour[i].clearColour.b;
                        clearColour[3] = mColour[i].clearColour.a;
                        OCGE( glClearBufferfv( GL_COLOR, i, clearColour ) );
                    }
                }
            }
        }

        if( (entriesToFlush & RenderPassDescriptor::Depth) &&
            (mDepth.loadAction == LoadAction::Clear ||
             (isTiler && (mDepth.loadAction == LoadAction::DontCare ||
                          mDepth.loadAction == LoadAction::ClearOnTilers))) )
        {
            flags |= GL_DEPTH_BUFFER_BIT;
            if( !depthWrite )
            {
                //Enable buffer for writing if it isn't
                OCGE( glDepthMask( GL_TRUE ) );
            }

            if( !mRenderSystem->isReverseDepth() )
            {
                OCGE( glClearDepth( mDepth.clearDepth ) );
            }
            else
            {
                OCGE( glClearDepth( Real(1.0f) - mDepth.clearDepth ) );
            }
        }

        if( (entriesToFlush & RenderPassDescriptor::Stencil) &&
            (mStencil.loadAction == LoadAction::Clear ||
             (isTiler && (mStencil.loadAction == LoadAction::DontCare ||
                          mStencil.loadAction == LoadAction::ClearOnTilers))) )
        {
            flags |= GL_STENCIL_BUFFER_BIT;
            if( stencilWriteMask != 0xFFFFFFFF )
            {
                //Enable buffer for writing if it isn't
                OCGE( glStencilMask( 0xFFFFFFFF ) );
            }
            OCGE( glClearStencil( mStencil.clearStencil ) );
        }

        if( flags != 0 )
        {
            //Issue the clear (except for glClearBufferfv which was immediate)
            OCGE( glClear( flags ) );
        }

        //Restore state
        if( (entriesToFlush & fullColourMask) == fullColourMask &&
            colourMask && mNumColourEntries > 0 &&
            (mAllClearColoursSetAndIdentical || mAnyColourLoadActionsSetToClear) )
        {
            GLboolean r = (blendChannelMask & HlmsBlendblock::BlendChannelRed) != 0;
            GLboolean g = (blendChannelMask & HlmsBlendblock::BlendChannelGreen) != 0;
            GLboolean b = (blendChannelMask & HlmsBlendblock::BlendChannelBlue) != 0;
            GLboolean a = (blendChannelMask & HlmsBlendblock::BlendChannelAlpha) != 0;
            OCGE( glColorMask( r, g, b, a ) );
        }

        if( !depthWrite && (flags & GL_DEPTH_BUFFER_BIT) )
        {
            OCGE( glDepthMask( GL_FALSE ) );
        }

        if( stencilWriteMask != 0xFFFFFFFF && (flags & GL_STENCIL_BUFFER_BIT) )
        {
            OCGE( glStencilMask( stencilWriteMask ) );
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusRenderPassDescriptor::performStoreActions( bool hasArbInvalidateSubdata,
                                                           uint32 entriesToFlush )
    {
        if( mInformationOnly )
            return;

        GLsizei numAttachments = 0;
        GLenum attachments[OGRE_MAX_MULTIPLE_RENDER_TARGETS+2u];

        bool unbindReadDrawFramebuffers = false;

        if( entriesToFlush & RenderPassDescriptor::Colour && !mHasRenderWindow )
        {
            for( size_t i=0; i<mNumColourEntries; ++i )
            {
                if( entriesToFlush & (RenderPassDescriptor::Colour0 << i) )
                {
                    if( (mColour[i].storeAction == StoreAction::MultisampleResolve ||
                         mColour[i].storeAction == StoreAction::StoreAndMultisampleResolve) &&
                        mColour[i].resolveTexture )
                    {
                        assert( mColour[i].resolveTexture->getResidencyStatus() ==
                                GpuResidency::Resident );
                        assert( dynamic_cast<GL3PlusTextureGpu*>( mColour[i].resolveTexture ) );
                        GL3PlusTextureGpu *resolveTexture =
                                static_cast<GL3PlusTextureGpu*>( mColour[i].resolveTexture );

                        const TextureTypes::TextureTypes resolveTextureType =
                                mColour[i].resolveTexture->getTextureType();
                        const bool hasLayers = resolveTextureType != TextureTypes::Type1D &&
                                               resolveTextureType != TextureTypes::Type2D;

                        // Blit from multisample buffer to final buffer, triggers resolve
                        OCGE( glBindFramebuffer( GL_READ_FRAMEBUFFER, mFboName ) );
                        OCGE( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, mFboMsaaResolve ) );
                        if( !hasLayers )
                        {
                            OCGE( glFramebufferTexture( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                        resolveTexture->getFinalTextureName(),
                                                        mColour[i].mipLevel ) );
                        }
                        else
                        {
                            OCGE( glFramebufferTextureLayer( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                             resolveTexture->getFinalTextureName(),
                                                             mColour[i].resolveMipLevel,
                                                             mColour[i].resolveSlice ) );
                        }

                        const uint32 width  = resolveTexture->getWidth();
                        const uint32 height = resolveTexture->getHeight();

                        OCGE( glReadBuffer( GL_COLOR_ATTACHMENT0 + i ) );
                        OCGE( glDrawBuffer( GL_COLOR_ATTACHMENT0 + 0 ) );
                        OCGE( glBlitFramebuffer( 0, 0, width, height, 0, 0, width, height,
                                                 GL_COLOR_BUFFER_BIT, GL_NEAREST ) );

                        if( mColour[i].storeAction == StoreAction::MultisampleResolve )
                        {
                            attachments[numAttachments] = mHasRenderWindow ? GL_COLOR :
                                                                             GL_COLOR_ATTACHMENT0 + i;
                        }

                        unbindReadDrawFramebuffers = true;
                    }

                    if( mColour[i].storeAction == StoreAction::DontCare ||
                        mColour[i].storeAction == StoreAction::MultisampleResolve )
                    {
                        attachments[numAttachments] = GL_COLOR_ATTACHMENT0 + i;
                        ++numAttachments;
                    }
                }
            }
        }

        if( (entriesToFlush & RenderPassDescriptor::Depth) &&
            mDepth.storeAction == StoreAction::DontCare )
        {
            attachments[numAttachments] = mHasRenderWindow ? GL_DEPTH : GL_DEPTH_ATTACHMENT;
            ++numAttachments;
        }

        if( (entriesToFlush & RenderPassDescriptor::Stencil) &&
            mStencil.storeAction == StoreAction::DontCare )
        {
            attachments[numAttachments] = mHasRenderWindow ? GL_STENCIL : GL_STENCIL_ATTACHMENT;
            ++numAttachments;
        }

        if( numAttachments > 0 && hasArbInvalidateSubdata )
        {
            OCGE( glInvalidateFramebuffer( GL_FRAMEBUFFER, numAttachments, attachments ) );
        }

        if( unbindReadDrawFramebuffers )
        {
            OCGE( glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 ) );
            OCGE( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    FrameBufferDescKey::FrameBufferDescKey()
    {
        memset( this, 0, sizeof( *this ) );
    }
    //-----------------------------------------------------------------------------------
    FrameBufferDescKey::FrameBufferDescKey( const RenderPassDescriptor &desc )
    {
        memset( this, 0, sizeof( *this ) );
        numColourEntries = desc.getNumColourEntries();

        //Load & Store actions don't matter for generating different FBOs.

        for( size_t i=0; i<numColourEntries; ++i )
        {
            colour[i] = desc.mColour[i];
            allLayers[i] = desc.mColour[i].allLayers;
            colour[i].loadAction = LoadAction::DontCare;
            colour[i].storeAction = StoreAction::DontCare;
        }

        depth = desc.mDepth;
        depth.loadAction = LoadAction::DontCare;
        depth.storeAction = StoreAction::DontCare;
        stencil = desc.mStencil;
        stencil.loadAction = LoadAction::DontCare;
        stencil.storeAction = StoreAction::DontCare;
    }
    //-----------------------------------------------------------------------------------
    bool FrameBufferDescKey::operator < ( const FrameBufferDescKey &other ) const
    {
        if( this->numColourEntries != other.numColourEntries )
            return this->numColourEntries < other.numColourEntries;

        for( size_t i=0; i<numColourEntries; ++i )
        {
            if( this->allLayers[i] != other.allLayers[i] )
                return this->allLayers[i] < other.allLayers[i];
            if( this->colour[i] != other.colour[i] )
                return this->colour[i] < other.colour[i];
        }

        if( this->depth != other.depth )
            return this->depth < other.depth;
        if( this->stencil != other.stencil )
            return this->stencil < other.stencil;

        return false;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    FrameBufferDescValue::FrameBufferDescValue() : fboName( 0 ), refCount( 0 ) {}
}
