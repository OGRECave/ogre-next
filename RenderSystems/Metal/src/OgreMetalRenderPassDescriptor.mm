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

#include "OgreMetalRenderPassDescriptor.h"

#include "OgreMetalRenderSystem.h"
#include "OgreMetalTextureGpu.h"
#include "OgreMetalTextureGpuWindow.h"

#include "OgreHlmsDatablock.h"
#include "OgrePixelFormatGpuUtils.h"

#include <execinfo.h>  //backtrace

namespace Ogre
{
    MetalRenderPassDescriptor::MetalRenderPassDescriptor( MetalDevice *device,
                                                          MetalRenderSystem *renderSystem ) :
        mDepthAttachment( 0 ),
        mStencilAttachment( 0 ),
        mRequiresManualResolve( false ),
        mSharedFboItor( renderSystem->_getFrameBufferDescMap().end() ),
        mDevice( device ),
        mRenderSystem( renderSystem )
#if OGRE_DEBUG_MODE
        ,
        mNumCallstackEntries( 0 )
#endif
    {
#if OGRE_DEBUG_MODE
        memset( mCallstackBacktrace, 0, sizeof( mCallstackBacktrace ) );
#endif
    }
    //-----------------------------------------------------------------------------------
    MetalRenderPassDescriptor::~MetalRenderPassDescriptor()
    {
        for( size_t i = 0u; i < mNumColourEntries; ++i )
        {
            mColourAttachment[i] = 0;
            mResolveColourAttachm[i] = 0;
        }
        mDepthAttachment = 0;
        mStencilAttachment = 0;

        MetalFrameBufferDescMap &frameBufferDescMap = mRenderSystem->_getFrameBufferDescMap();
        if( mSharedFboItor != frameBufferDescMap.end() )
        {
            --mSharedFboItor->second.refCount;
            if( !mSharedFboItor->second.refCount )
                frameBufferDescMap.erase( mSharedFboItor );
            mSharedFboItor = frameBufferDescMap.end();
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalRenderPassDescriptor::checkRenderWindowStatus()
    {
        if( ( mNumColourEntries > 0 && mColour[0].texture->isRenderWindowSpecific() ) ||
            ( mDepth.texture && mDepth.texture->isRenderWindowSpecific() ) ||
            ( mStencil.texture && mStencil.texture->isRenderWindowSpecific() ) )
        {
            if( mNumColourEntries > 1u )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Cannot use RenderWindow as MRT with other colour textures",
                             "MetalRenderPassDescriptor::colourEntriesModified" );
            }

            if( ( mNumColourEntries > 0 && !mColour[0].texture->isRenderWindowSpecific() ) ||
                ( mDepth.texture && !mDepth.texture->isRenderWindowSpecific() ) ||
                ( mStencil.texture && !mStencil.texture->isRenderWindowSpecific() ) )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Cannot mix RenderWindow colour texture with depth or stencil buffer "
                             "that aren't for RenderWindows, or vice-versa",
                             "MetalRenderPassDescriptor::checkRenderWindowStatus" );
            }
        }

        calculateSharedKey();
    }
    //-----------------------------------------------------------------------------------
    void MetalRenderPassDescriptor::calculateSharedKey()
    {
        FrameBufferDescKey key( *this );
        MetalFrameBufferDescMap &frameBufferDescMap = mRenderSystem->_getFrameBufferDescMap();
        MetalFrameBufferDescMap::iterator newItor = frameBufferDescMap.find( key );

        if( newItor == frameBufferDescMap.end() )
        {
            MetalFrameBufferDescValue value;
            value.refCount = 0;
            frameBufferDescMap[key] = value;
            newItor = frameBufferDescMap.find( key );
        }

        ++newItor->second.refCount;

        if( mSharedFboItor != frameBufferDescMap.end() )
        {
            --mSharedFboItor->second.refCount;
            if( !mSharedFboItor->second.refCount )
                frameBufferDescMap.erase( mSharedFboItor );
        }

        mSharedFboItor = newItor;
    }
    //-----------------------------------------------------------------------------------
    MTLLoadAction MetalRenderPassDescriptor::get( LoadAction::LoadAction action )
    {
        switch( action )
        {
        case LoadAction::DontCare:
            return MTLLoadActionDontCare;
        case LoadAction::Clear:
            return MTLLoadActionClear;
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS || \
    ( OGRE_PLATFORM == OGRE_PLATFORM_APPLE && OGRE_CPU == OGRE_CPU_ARM && \
      OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_64 )
        case LoadAction::ClearOnTilers:
            return MTLLoadActionClear;
#else
        case LoadAction::ClearOnTilers:
            return MTLLoadActionLoad;
#endif
        case LoadAction::Load:
            return MTLLoadActionLoad;
        }

        return MTLLoadActionLoad;
    }
    //-----------------------------------------------------------------------------------
    MTLStoreAction MetalRenderPassDescriptor::get( StoreAction::StoreAction action )
    {
        switch( action )
        {
        case StoreAction::DontCare:
            return MTLStoreActionDontCare;
        case StoreAction::Store:
            return MTLStoreActionStore;
        case StoreAction::MultisampleResolve:
            return MTLStoreActionMultisampleResolve;
        case StoreAction::StoreAndMultisampleResolve:
            return MTLStoreActionStoreAndMultisampleResolve;
        case StoreAction::StoreOrResolve:
            assert( false && "StoreOrResolve is invalid. "
                             "Compositor should've set one or the other already!" );
            return MTLStoreActionStore;
        }

        return MTLStoreActionStore;
    }
    //-----------------------------------------------------------------------------------
    void MetalRenderPassDescriptor::sanitizeMsaaResolve( size_t colourIdx )
    {
        const size_t i = colourIdx;

        // iOS_GPUFamily3_v2, OSX_GPUFamily1_v2
        if( mColour[i].storeAction == StoreAction::StoreAndMultisampleResolve &&
            !mRenderSystem->hasStoreAndMultisampleResolve() &&
            ( mColour[i].texture->isMultisample() && mColour[i].resolveTexture ) )
        {
            // Must emulate the behavior (slower)
            mColourAttachment[i].storeAction = MTLStoreActionStore;
            mColourAttachment[i].resolveTexture = nil;
            mResolveColourAttachm[i] = [mColourAttachment[i] copy];
            mResolveColourAttachm[i].loadAction = MTLLoadActionLoad;
            mResolveColourAttachm[i].storeAction = MTLStoreActionMultisampleResolve;

            mRequiresManualResolve = true;
        }
        else if( mColour[i].storeAction == StoreAction::DontCare ||
                 mColour[i].storeAction == StoreAction::Store )
        {
            mColourAttachment[i].resolveTexture = nil;
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalRenderPassDescriptor::updateColourRtv( uint8 lastNumColourEntries )
    {
        mRequiresManualResolve = false;
        if( mNumColourEntries < lastNumColourEntries )
        {
            for( size_t i = mNumColourEntries; i < lastNumColourEntries; ++i )
            {
                mColourAttachment[i] = 0;
                mResolveColourAttachm[i] = 0;
            }
        }

        bool hasRenderWindow = false;

        for( size_t i = 0; i < mNumColourEntries; ++i )
        {
            if( mColour[i].texture->getResidencyStatus() != GpuResidency::Resident )
            {
                OGRE_EXCEPT(
                    Exception::ERR_INVALIDPARAMS,
                    "RenderTexture '" + mColour[i].texture->getNameStr() + "' must be resident!",
                    "MetalRenderPassDescriptor::updateColourRtv" );
            }
            if( i > 0 && hasRenderWindow != mColour[i].texture->isRenderWindowSpecific() )
            {
                // This is a GL restriction actually, which we mimic for consistency
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Cannot use RenderWindow as MRT with other colour textures",
                             "MetalRenderPassDescriptor::updateColourRtv" );
            }

            hasRenderWindow |= mColour[i].texture->isRenderWindowSpecific();

            mColourAttachment[i] = 0;

            if( mColour[i].texture->getPixelFormat() == PFG_NULL )
                continue;

            mColourAttachment[i] = [MTLRenderPassColorAttachmentDescriptor alloc];

            if( !mColour[i].texture->isRenderWindowSpecific() )
            {
                assert( dynamic_cast<MetalTextureGpu *>( mColour[i].texture ) );
                MetalTextureGpu *textureMetal = static_cast<MetalTextureGpu *>( mColour[i].texture );

                if( mColour[i].texture->isMultisample() )
                {
                    MetalTextureGpu *resolveTexture = 0;
                    if( mColour[i].resolveTexture &&
                        !mColour[i].resolveTexture->isRenderWindowSpecific() )
                    {
                        OGRE_ASSERT_HIGH( dynamic_cast<MetalTextureGpu *>( mColour[i].resolveTexture ) );
                        resolveTexture = static_cast<MetalTextureGpu *>( mColour[i].resolveTexture );
                    }

                    if( !mColour[i].texture->hasMsaaExplicitResolves() )
                        mColourAttachment[i].texture = textureMetal->getMsaaFramebufferName();
                    else
                        mColourAttachment[i].texture = textureMetal->getFinalTextureName();

                    if( resolveTexture )
                        mColourAttachment[i].resolveTexture = resolveTexture->getFinalTextureName();
                }
                else
                {
                    mColourAttachment[i].texture = textureMetal->getFinalTextureName();
                }
            }

            mColourAttachment[i].clearColor =
                MTLClearColorMake( mColour[i].clearColour.r, mColour[i].clearColour.g,
                                   mColour[i].clearColour.b, mColour[i].clearColour.a );

            mColourAttachment[i].level = mColour[i].mipLevel;
            mColourAttachment[i].resolveLevel = mColour[i].resolveMipLevel;

            if( mColour[i].texture->getTextureType() == TextureTypes::Type3D )
                mColourAttachment[i].depthPlane = mColour[i].slice;
            else
                mColourAttachment[i].slice = mColour[i].slice;

            if( mColour[i].resolveTexture )
            {
                if( mColour[i].resolveTexture->getTextureType() == TextureTypes::Type3D )
                    mColourAttachment[i].resolveDepthPlane = mColour[i].resolveSlice;
                else
                    mColourAttachment[i].resolveSlice = mColour[i].resolveSlice;
            }

            mColourAttachment[i].loadAction = MetalRenderPassDescriptor::get( mColour[i].loadAction );
            mColourAttachment[i].storeAction = MetalRenderPassDescriptor::get( mColour[i].storeAction );

            if( mColour[i].storeAction == StoreAction::StoreAndMultisampleResolve &&
                ( !mColour[i].texture->isMultisample() || !mColour[i].resolveTexture ) )
            {
                // Ogre allows non-MSAA textures to use this flag. Metal may complain.
                mColourAttachment[i].storeAction = MTLStoreActionStore;
            }

            // iOS_GPUFamily3_v2, OSX_GPUFamily1_v2
            if( mColour[i].storeAction == StoreAction::StoreAndMultisampleResolve &&
                !mRenderSystem->hasStoreAndMultisampleResolve() &&
                ( mColour[i].texture->isMultisample() && mColour[i].resolveTexture ) )
            {
                // Must emulate the behavior (slower)
                mColourAttachment[i].storeAction = MTLStoreActionStore;
                mColourAttachment[i].resolveTexture = nil;
                mResolveColourAttachm[i] = [mColourAttachment[i] copy];
                mResolveColourAttachm[i].loadAction = MTLLoadActionLoad;
                mResolveColourAttachm[i].storeAction = MTLStoreActionMultisampleResolve;

                mRequiresManualResolve = true;
            }

            sanitizeMsaaResolve( i );
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalRenderPassDescriptor::updateDepthRtv()
    {
        mDepthAttachment = 0;

        if( !mDepth.texture )
        {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
            if( mStencil.texture )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Stencil without depth (RenderTexture '" + mStencil.texture->getNameStr() +
                                 "'). This is not supported by macOS",
                             "MetalRenderPassDescriptor::updateDepthRtv" );
            }
#endif
            return;
        }

        if( mDepth.texture->getResidencyStatus() != GpuResidency::Resident )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "RenderTexture '" + mDepth.texture->getNameStr() + "' must be resident!",
                         "MetalRenderPassDescriptor::updateDepthRtv" );
        }

        assert( dynamic_cast<MetalTextureGpu *>( mDepth.texture ) );
        MetalTextureGpu *textureMetal = static_cast<MetalTextureGpu *>( mDepth.texture );

        mDepthAttachment = [MTLRenderPassDepthAttachmentDescriptor alloc];
        mDepthAttachment.texture = textureMetal->getFinalTextureName();

        if( !mRenderSystem->isReverseDepth() )
            mDepthAttachment.clearDepth = mDepth.clearDepth;
        else
            mDepthAttachment.clearDepth = Real( 1.0f ) - mDepth.clearDepth;

        mDepthAttachment.loadAction = MetalRenderPassDescriptor::get( mDepth.loadAction );
        mDepthAttachment.storeAction = MetalRenderPassDescriptor::get( mDepth.storeAction );

        if( mDepth.storeAction == StoreAction::StoreAndMultisampleResolve &&
            ( !mDepth.texture->isMultisample() || !mDepth.resolveTexture ) )
        {
            // Ogre allows non-MSAA textures to use this flag. Metal may complain.
            mDepthAttachment.storeAction = MTLStoreActionStore;
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalRenderPassDescriptor::updateStencilRtv()
    {
        mStencilAttachment = 0;

        if( !mStencil.texture )
            return;

        if( mStencil.texture->getResidencyStatus() != GpuResidency::Resident )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "RenderTexture '" + mStencil.texture->getNameStr() + "' must be resident!",
                         "MetalRenderPassDescriptor::updateStencilRtv" );
        }

        assert( dynamic_cast<MetalTextureGpu *>( mStencil.texture ) );
        MetalTextureGpu *textureMetal = static_cast<MetalTextureGpu *>( mStencil.texture );

        mStencilAttachment = [MTLRenderPassStencilAttachmentDescriptor alloc];
        mStencilAttachment.texture = textureMetal->getFinalTextureName();
        mStencilAttachment.clearStencil = mStencil.clearStencil;

        mStencilAttachment.loadAction = MetalRenderPassDescriptor::get( mStencil.loadAction );
        mStencilAttachment.storeAction = MetalRenderPassDescriptor::get( mStencil.storeAction );

        if( mStencil.storeAction == StoreAction::StoreAndMultisampleResolve &&
            ( !mStencil.texture->isMultisample() || !mStencil.resolveTexture ) )
        {
            // Ogre allows non-MSAA textures to use this flag. Metal may complain.
            mStencilAttachment.storeAction = MTLStoreActionStore;
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalRenderPassDescriptor::entriesModified( uint32 entryTypes )
    {
        uint8 lastNumColourEntries = mNumColourEntries;
        RenderPassDescriptor::entriesModified( entryTypes );

        checkRenderWindowStatus();

        if( entryTypes & RenderPassDescriptor::Colour )
            updateColourRtv( lastNumColourEntries );

        if( entryTypes & RenderPassDescriptor::Depth )
            updateDepthRtv();

        if( entryTypes & RenderPassDescriptor::Stencil )
            updateStencilRtv();
    }
    //-----------------------------------------------------------------------------------
    void MetalRenderPassDescriptor::setClearColour( uint8 idx, const ColourValue &clearColour )
    {
        RenderPassDescriptor::setClearColour( idx, clearColour );

        if( mColourAttachment[idx] )
        {
            mColourAttachment[idx] = [mColourAttachment[idx] copy];
            mColourAttachment[idx].clearColor =
                MTLClearColorMake( clearColour.r, clearColour.g, clearColour.b, clearColour.a );
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalRenderPassDescriptor::setClearDepth( Real clearDepth )
    {
        RenderPassDescriptor::setClearDepth( clearDepth );

        if( mDepthAttachment )
        {
            mDepthAttachment = [mDepthAttachment copy];
            if( !mRenderSystem->isReverseDepth() )
                mDepthAttachment.clearDepth = clearDepth;
            else
                mDepthAttachment.clearDepth = Real( 1.0f ) - clearDepth;
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalRenderPassDescriptor::setClearStencil( uint32 clearStencil )
    {
        RenderPassDescriptor::setClearStencil( clearStencil );

        if( mStencilAttachment )
        {
            mStencilAttachment = [mStencilAttachment copy];
            mStencilAttachment.clearStencil = clearStencil;
        }
    }
    //-----------------------------------------------------------------------------------
    uint32 MetalRenderPassDescriptor::checkForClearActions( MetalRenderPassDescriptor *other ) const
    {
        uint32 entriesToFlush = 0;

        assert( this->mSharedFboItor == other->mSharedFboItor );
        assert( this->mNumColourEntries == other->mNumColourEntries );

        const RenderSystemCapabilities *capabilities = mRenderSystem->getCapabilities();
        const bool isTiler = capabilities->hasCapability( RSC_IS_TILER );

        for( size_t i = 0; i < mNumColourEntries; ++i )
        {
            // this->mColour[i].allLayers doesn't need to be analyzed
            // because it requires a different FBO.
            if( other->mColour[i].loadAction == LoadAction::Clear ||
                ( isTiler && mColour[i].loadAction == LoadAction::ClearOnTilers ) )
            {
                entriesToFlush |= RenderPassDescriptor::Colour0 << i;
            }
        }

        if( other->mDepth.loadAction == LoadAction::Clear ||
            ( isTiler && mDepth.loadAction == LoadAction::ClearOnTilers ) )
        {
            entriesToFlush |= RenderPassDescriptor::Depth;
        }

        if( other->mStencil.loadAction == LoadAction::Clear ||
            ( isTiler && mStencil.loadAction == LoadAction::ClearOnTilers ) )
        {
            entriesToFlush |= RenderPassDescriptor::Stencil;
        }

        return entriesToFlush;
    }
    //-----------------------------------------------------------------------------------
    uint32 MetalRenderPassDescriptor::willSwitchTo( MetalRenderPassDescriptor *newDesc,
                                                    bool warnIfRtvWasFlushed ) const
    {
        uint32 entriesToFlush = 0;

        if( !newDesc || this->mSharedFboItor != newDesc->mSharedFboItor || this->mInformationOnly ||
            newDesc->mInformationOnly )
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
    bool MetalRenderPassDescriptor::cannotInterruptRendering() const
    {
        bool cannotInterrupt = false;

        for( size_t i = 0; i < mNumColourEntries && !cannotInterrupt; ++i )
        {
            if( mColour[i].storeAction != StoreAction::Store &&
                mColour[i].storeAction != StoreAction::StoreAndMultisampleResolve )
            {
                cannotInterrupt = true;
            }
        }

        cannotInterrupt |= ( mDepth.texture && mDepth.storeAction != StoreAction::Store &&
                             mDepth.storeAction != StoreAction::StoreAndMultisampleResolve ) ||
                           ( mStencil.texture && mStencil.storeAction != StoreAction::Store &&
                             mStencil.storeAction != StoreAction::StoreAndMultisampleResolve );

        return cannotInterrupt;
    }
    //-----------------------------------------------------------------------------------
    void MetalRenderPassDescriptor::performLoadActions( MTLRenderPassDescriptor *passDesc,
                                                        bool renderingWasInterrupted )
    {
        if( mInformationOnly )
            return;

        for( size_t i = 0; i < mNumColourEntries; ++i )
            passDesc.colorAttachments[i] = mColourAttachment[i];

        if( mNumColourEntries > 0 &&
            ( mColour[0].texture->isRenderWindowSpecific() ||
              ( mColour[0].resolveTexture && mColour[0].resolveTexture->isRenderWindowSpecific() ) ) )
        {
            // The RenderWindow's MetalDrawable changes every frame. We need a
            // hard copy since the previous descriptor may still be in flight.
            // Also ensure we do not retain current drawable's texture beyond the frame.
            passDesc.colorAttachments[0] = [mColourAttachment[0] copy];
            MetalTextureGpuWindow *textureMetal = 0;

            if( mColour[0].texture->isRenderWindowSpecific() )
            {
                OGRE_ASSERT_HIGH( dynamic_cast<MetalTextureGpuWindow *>( mColour[0].texture ) );
                textureMetal = static_cast<MetalTextureGpuWindow *>( mColour[0].texture );
            }
            else
            {
                OGRE_ASSERT_HIGH( dynamic_cast<MetalTextureGpuWindow *>( mColour[0].resolveTexture ) );
                textureMetal = static_cast<MetalTextureGpuWindow *>( mColour[0].resolveTexture );
            }

            textureMetal->nextDrawable();
            if( mColour[0].texture->isMultisample() )
            {
                if( mColour[0].texture->isRenderWindowSpecific() )
                    passDesc.colorAttachments[0].texture = textureMetal->getMsaaFramebufferName();

                passDesc.colorAttachments[0].resolveTexture = textureMetal->getFinalTextureName();
                if( mColour[0].storeAction == StoreAction::DontCare ||
                    mColour[0].storeAction == StoreAction::Store ||
                    ( mColour[0].storeAction == StoreAction::StoreAndMultisampleResolve &&
                      !mRenderSystem->hasStoreAndMultisampleResolve() ) )
                {
                    passDesc.colorAttachments[0].resolveTexture = nil;
                }
            }
            else
            {
                passDesc.colorAttachments[0].texture = textureMetal->getFinalTextureName();
            }
        }

        passDesc.depthAttachment = mDepthAttachment;
        passDesc.stencilAttachment = mStencilAttachment;

        if( renderingWasInterrupted )
        {
            for( size_t i = 0; i < mNumColourEntries; ++i )
            {
                passDesc.colorAttachments[i] = [passDesc.colorAttachments[i] copy];
                passDesc.colorAttachments[i].loadAction = MTLLoadActionLoad;
            }

            if( passDesc.depthAttachment )
            {
                passDesc.depthAttachment = [passDesc.depthAttachment copy];
                passDesc.depthAttachment.loadAction = MTLLoadActionLoad;
            }

            if( passDesc.stencilAttachment )
            {
                passDesc.stencilAttachment = [passDesc.stencilAttachment copy];
                passDesc.stencilAttachment.loadAction = MTLLoadActionLoad;
            }

            const bool cannotInterrupt = cannotInterruptRendering();

            static bool warnedOnce = false;
            if( !warnedOnce || cannotInterrupt )
            {
                LogManager::getSingleton().logMessage(
                    "WARNING: Rendering was interrupted. Likely because a StagingBuffer "
                    "was used while inside HlmsPbs::fillBuffersFor; but could be caused "
                    "by other reasons such as mipmaps being generated in a listener, "
                    "buffer transfer/copies, manually dispatching a compute shader, etc."
                    " Performance will be degraded. This message will only appear once.",
                    LML_CRITICAL );

#if OGRE_DEBUG_MODE
                LogManager::getSingleton().logMessage(
                    "Dumping callstack at the time rendering was interrupted: ", LML_CRITICAL );

                char **translatedCS =
                    backtrace_symbols( mCallstackBacktrace, (int)mNumCallstackEntries );

                for( size_t i = 0; i < mNumCallstackEntries; ++i )
                    LogManager::getSingleton().logMessage( translatedCS[i], LML_CRITICAL );

                memset( mCallstackBacktrace, 0, sizeof( mCallstackBacktrace ) );
                mNumCallstackEntries = 0;
#endif
                warnedOnce = true;
            }

            if( cannotInterrupt )
            {
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                             "ERROR: We cannot resume rendering because the compositor pass specified "
                             "'DontCare' or 'MultisampleResolve' as StoreAction for colour, depth "
                             "or stencil, therefore rendering will be incorrect!!!\n"
                             "Either remove the buffer manipulation from the rendering hot loop "
                             "that is causing the interruption, or set the store actions to Store, or"
                             "StoreAndMultisampleResolve",
                             "MetalRenderPassDescriptor::performLoadActions" );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalRenderPassDescriptor::performStoreActions( uint32 entriesToFlush,
                                                         bool isInterruptingRendering )
    {
        if( mInformationOnly )
            return;

        if( isInterruptingRendering )
        {
#if OGRE_DEBUG_MODE
            // Save the backtrace to report it later
            const bool cannotInterrupt = cannotInterruptRendering();
            static bool warnedOnce = false;
            if( !warnedOnce || cannotInterrupt )
            {
                mNumCallstackEntries = static_cast<size_t>( backtrace( mCallstackBacktrace, 32 ) );
                warnedOnce = true;
            }
#endif
            return;
        }

        // End (if exists) the render command encoder tied to this RenderPassDesc.
        // Another encoder will have to be created, and don't let ours linger
        // since mCurrentRenderPassDescriptor probably doesn't even point to 'this'
        mDevice->endAllEncoders( false );

        if( !( entriesToFlush & Colour ) )
            return;

        if( !mRequiresManualResolve )
            return;

        if( mRenderSystem->hasStoreAndMultisampleResolve() )
            return;

        MTLRenderPassDescriptor *passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
        size_t currentColourIdx = 0;

        for( size_t i = 0; i < mNumColourEntries; ++i )
        {
            if( mColour[i].resolveTexture &&
                mColour[i].storeAction == StoreAction::StoreAndMultisampleResolve )
            {
                passDesc.colorAttachments[currentColourIdx] = mResolveColourAttachm[i];
                ++currentColourIdx;
            }
        }

        if( mNumColourEntries > 0 &&
            ( mColour[0].texture->isRenderWindowSpecific() ||
              ( mColour[0].resolveTexture && mColour[0].resolveTexture->isRenderWindowSpecific() ) ) )
        {
            // RenderWindows are a special case. We don't retain them so
            // mResolveColourAttachm doesn't have their ptrs.
            passDesc.colorAttachments[0] = [passDesc copy];

            MetalTextureGpuWindow *textureMetal = 0;

            if( mColour[0].texture->isRenderWindowSpecific() )
            {
                OGRE_ASSERT_HIGH( dynamic_cast<MetalTextureGpuWindow *>( mColour[0].texture ) );
                textureMetal = static_cast<MetalTextureGpuWindow *>( mColour[0].texture );
            }
            else
            {
                OGRE_ASSERT_HIGH( dynamic_cast<MetalTextureGpuWindow *>( mColour[0].resolveTexture ) );
                textureMetal = static_cast<MetalTextureGpuWindow *>( mColour[0].resolveTexture );
            }

            if( mColour[0].texture->isRenderWindowSpecific() )
                passDesc.colorAttachments[0].texture = textureMetal->getMsaaFramebufferName();
            passDesc.colorAttachments[0].resolveTexture = textureMetal->getFinalTextureName();
        }

        mDevice->mRenderEncoder =
            [mDevice->mCurrentCommandBuffer renderCommandEncoderWithDescriptor:passDesc];
        mDevice->endRenderEncoder( false );
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    MetalFrameBufferDescValue::MetalFrameBufferDescValue() : refCount( 0 ) {}
}  // namespace Ogre
