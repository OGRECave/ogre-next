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

#include "OgreRenderSystem.h"
#include "OgreHlmsDatablock.h"
#include "OgrePixelFormatGpuUtils.h"

namespace Ogre
{
    GL3PlusRenderPassDescriptor::GL3PlusRenderPassDescriptor( RenderSystem *renderSystem ) :
        mFboName( 0 ),
        mFboMsaaResolve( 0 ),
        mAllClearColoursSetAndIdentical( false ),
        mAnyColourLoadActionsSetToClear( false ),
        mHasRenderWindow( false ),
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

        if( mFboName )
        {
            OCGE( glDeleteFramebuffers( 1, &mFboName ) );
            mFboName = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusRenderPassDescriptor::checkRenderWindowStatus(void)
    {
        if( (mNumColourEntries > 0 && mColour[0].texture->isRenderWindowSpecific()) ||
            (mDepth.texture && !mDepth.texture->isRenderWindowSpecific()) ||
            (mStencil.texture && !mStencil.texture->isRenderWindowSpecific()) )
        {
            if( mNumColourEntries <= 1u )
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
        else if( !mFboName )
        {
            switchToFBO();
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusRenderPassDescriptor::switchToRenderWindow(void)
    {
        if( mFboName )
        {
            OCGE( glDeleteFramebuffers( 1, &mFboName ) );
            mFboName = 0;
        }

        mHasRenderWindow = true;
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusRenderPassDescriptor::switchToFBO(void)
    {
        if( !mFboName )
        {
            OCGE( glGenFramebuffers( 1, &mFboName ) );

            OCGE( glBindFramebuffer( GL_FRAMEBUFFER, mFboName ) );

            //Disable target independent rasterization to let the driver warn us
            //of wrong behavior during regular rendering.
            OCGE( glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, 0 ) );
            OCGE( glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, 0 ) );
        }

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
    void GL3PlusRenderPassDescriptor::colourEntriesModified(void)
    {
        uint8 lastNumColourEntries = mNumColourEntries;

        RenderPassDescriptor::colourEntriesModified();

        checkRenderWindowStatus();

        OCGE( glBindFramebuffer( GL_FRAMEBUFFER, mFboName ) );

        if( mNumColourEntries < lastNumColourEntries && !mHasRenderWindow )
        {
            for( size_t i=mNumColourEntries; i<lastNumColourEntries; ++i )
            {
                //Detach removed colour entries
                OCGE( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                                                 GL_RENDERBUFFER, 0 ) );
            }
        }

        bool needsMsaaResolveFbo = false;

        //Attach colour entries
        for( size_t i=0; i<mNumColourEntries; ++i )
        {
            assert( mColour[i].texture->getResidencyStatus() == GpuResidency::Resident );

            if( !mHasRenderWindow )
            {
                assert( dynamic_cast<GL3PlusTextureGpu*>( mColour[i].texture ) );
                GL3PlusTextureGpu *texture = static_cast<GL3PlusTextureGpu*>( mColour[i].texture );

                if( texture->isRenderWindowSpecific() )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "Cannot use RenderWindow as MRT with other colour textures",
                                 "GL3PlusRenderPassDescriptor::colourEntriesModified" );
                }

                if( mColour[i].allLayers )
                {
                    if( !texture->hasMsaaExplicitResolves() && texture->getMsaa() > 1u )
                    {
                        OCGE( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                                                         texture->getMsaaFramebufferName(), 0 ) );
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
                    if( !texture->hasMsaaExplicitResolves() && texture->getMsaa() > 1u )
                    {
                        OCGE( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                                                         texture->getMsaaFramebufferName(), 0 ) );
                    }
                    else
                    {
                        OCGE( glFramebufferTextureLayer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                                                         texture->getFinalTextureName(),
                                                         mColour[i].mipLevel, mColour[i].slice ) );
                    }
                }
            }

            if( mColour[i].storeAction == StoreAction::MultisampleResolve ||
                mColour[i].storeAction == StoreAction::StoreAndMultisampleResolve )
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
    void GL3PlusRenderPassDescriptor::depthModified(void)
    {
        RenderPassDescriptor::depthModified();
        checkRenderWindowStatus();
        if( mHasRenderWindow )
            return;

        OCGE( glBindFramebuffer( GL_FRAMEBUFFER, mFboName ) );

        if( !mDepth.texture )
        {
            OCGE( glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 0, 0 ) );
            return;
        }

        assert( mDepth.texture->getResidencyStatus() == GpuResidency::Resident );

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
    void GL3PlusRenderPassDescriptor::stencilModified(void)
    {
        RenderPassDescriptor::stencilModified();
        checkRenderWindowStatus();
        if( mHasRenderWindow )
            return;

        OCGE( glBindFramebuffer( GL_FRAMEBUFFER, mFboName ) );

        if( !mStencil.texture )
        {
            OCGE( glFramebufferTexture( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, 0, 0 ) );
            return;
        }

        assert( mStencil.texture->getResidencyStatus() == GpuResidency::Resident );

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
    void GL3PlusRenderPassDescriptor::performLoadActions( uint8 blendChannelMask,
                                                          bool depthWrite,
                                                          uint32 stencilWriteMask )
    {
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

        OCGE( glEnable( GL_FRAMEBUFFER_SRGB ) );

        const RenderSystemCapabilities *capabilities = mRenderSystem->getCapabilities();
        const bool isTiler = capabilities->hasCapability( RSC_IS_TILER );

        bool colourMask = blendChannelMask != HlmsBlendblock::BlendChannelAll;

        GLbitfield flags = 0;
        if( mNumColourEntries > 0 && mAllClearColoursSetAndIdentical )
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
                if( mColour[i].loadAction == LoadAction::Clear ||
                    (isTiler && (mColour[i].loadAction == LoadAction::DontCare ||
                                 mColour[i].loadAction == LoadAction::ClearOnTilers)) )
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

        if( mDepth.loadAction == LoadAction::Clear ||
            (isTiler && (mDepth.loadAction == LoadAction::DontCare ||
                         mDepth.loadAction == LoadAction::ClearOnTilers)) )
        {
            flags |= GL_DEPTH_BUFFER_BIT;
            if( !depthWrite )
            {
                //Enable buffer for writing if it isn't
                OCGE( glDepthMask( GL_TRUE ) );
            }
            OCGE( glClearDepth( mDepth.clearDepth ) );
        }

        if( mStencil.loadAction == LoadAction::Clear ||
            (isTiler && (mStencil.loadAction == LoadAction::DontCare ||
                         mStencil.loadAction == LoadAction::ClearOnTilers)) )
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
        if( colourMask && mNumColourEntries > 0 &&
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
    void GL3PlusRenderPassDescriptor::performStoreActions( bool hasArbInvalidateSubdata )
    {
        GLsizei numAttachments = 0;
        GLenum attachments[OGRE_MAX_MULTIPLE_RENDER_TARGETS+2u];

        for( size_t i=0; i<mNumColourEntries; ++i )
        {
            if( mColour[i].storeAction == StoreAction::MultisampleResolve ||
                mColour[i].storeAction == StoreAction::StoreAndMultisampleResolve )
            {
                assert( mColour[i].resolveTexture->getResidencyStatus() == GpuResidency::Resident );
                assert( dynamic_cast<GL3PlusTextureGpu*>( mColour[i].resolveTexture ) );
                GL3PlusTextureGpu *resolveTexture =
                        static_cast<GL3PlusTextureGpu*>( mColour[i].resolveTexture );

                // Blit from multisample buffer to final buffer, triggers resolve
                uint32 width = mColour[0].texture->getWidth();
                uint32 height = mColour[0].texture->getHeight();
                OCGE( glBindFramebuffer( GL_READ_FRAMEBUFFER, mFboName ) );
                OCGE( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, mFboMsaaResolve ) );
                OCGE( glFramebufferTextureLayer( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                 resolveTexture->getFinalTextureName(),
                                                 mColour[i].resolveMipLevel, mColour[i].resolveSlice ) );
                OCGE( glReadBuffer( GL_COLOR_ATTACHMENT0 + i ) );
                OCGE( glDrawBuffer( GL_COLOR_ATTACHMENT0 + 0 ) );
                OCGE( glBlitFramebuffer( 0, 0, width, height, 0, 0, width, height,
                                         GL_COLOR_BUFFER_BIT, GL_NEAREST ) );
            }
            else if( mColour[i].storeAction == StoreAction::DontCare ||
                     mColour[i].storeAction == StoreAction::MultisampleResolve )
            {
                attachments[numAttachments] = GL_COLOR_ATTACHMENT0 + i;
                ++numAttachments;
            }
        }

        if( mDepth.storeAction == StoreAction::DontCare )
        {
            attachments[numAttachments] = GL_DEPTH_ATTACHMENT;
            ++numAttachments;
        }

        if( mStencil.storeAction == StoreAction::DontCare )
        {
            attachments[numAttachments] = GL_STENCIL_ATTACHMENT;
            ++numAttachments;
        }

        if( numAttachments > 0 && hasArbInvalidateSubdata )
        {
            GLenum target = mHasRenderWindow ? GL_BACK : GL_FRAMEBUFFER;
            OCGE( glInvalidateFramebuffer( target, numAttachments, attachments ) );
        }

        OCGE( glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 ) );
        OCGE( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
    }
}
