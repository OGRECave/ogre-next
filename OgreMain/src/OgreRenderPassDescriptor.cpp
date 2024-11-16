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

#include "OgreStableHeaders.h"

#include "OgreRenderPassDescriptor.h"

#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreStringConverter.h"
#include "OgreTextureGpu.h"

namespace Ogre
{
    RenderPassTargetBase::RenderPassTargetBase() { memset( this, 0, sizeof( *this ) ); }
    bool RenderPassTargetBase::operator!=( const RenderPassTargetBase &other ) const
    {
        return this->texture != other.texture ||                  //
               this->resolveTexture != other.resolveTexture ||    //
               this->mipLevel != other.mipLevel ||                //
               this->resolveMipLevel != other.resolveMipLevel ||  //
               this->slice != other.slice ||                      //
               this->resolveSlice != other.resolveSlice ||        //
               this->loadAction != other.loadAction ||            //
               this->storeAction != other.storeAction;
    }
    bool RenderPassTargetBase::operator<( const RenderPassTargetBase &other ) const
    {
        if( this->texture != other.texture )
            return this->texture < other.texture;
        if( this->resolveTexture != other.resolveTexture )
            return this->resolveTexture < other.resolveTexture;
        if( this->mipLevel != other.mipLevel )
            return this->mipLevel < other.mipLevel;
        if( this->resolveMipLevel != other.resolveMipLevel )
            return this->resolveMipLevel < other.resolveMipLevel;
        if( this->slice != other.slice )
            return this->slice < other.slice;
        if( this->resolveSlice != other.resolveSlice )
            return this->resolveSlice < other.resolveSlice;
        if( this->loadAction != other.loadAction )
            return this->loadAction < other.loadAction;
        if( this->storeAction != other.storeAction )
            return this->storeAction < other.storeAction;

        return false;
    }

    RenderPassColourTarget::RenderPassColourTarget() :
        RenderPassTargetBase(),
        clearColour( ColourValue::Black ),
        allLayers( false )
    {
    }
    RenderPassDepthTarget::RenderPassDepthTarget() :
        RenderPassTargetBase(),
        clearDepth( 1.0f ),
        readOnly( false )
    {
    }
    RenderPassStencilTarget::RenderPassStencilTarget() :
        RenderPassTargetBase(),
        clearStencil( 0 ),
        readOnly( false )
    {
    }

    RenderPassDescriptor::RenderPassDescriptor() :
        mNumColourEntries( 0 ),
        mRequiresTextureFlipping( false ),
        mReadyWindowForPresent( false ),
        mInformationOnly( false )
    {
    }
    //-----------------------------------------------------------------------------------
    RenderPassDescriptor::~RenderPassDescriptor() {}
    //-----------------------------------------------------------------------------------
    void RenderPassDescriptor::checkWarnIfRtvWasFlushed( uint32 entriesToFlush )
    {
        bool mustWarn = false;

        for( size_t i = 0; i < mNumColourEntries && !mustWarn; ++i )
        {
            if( mColour[i].loadAction == LoadAction::Load &&
                ( entriesToFlush & ( RenderPassDescriptor::Colour0 << i ) ) )
            {
                mustWarn = true;
            }
        }

        if( mDepth.loadAction == LoadAction::Load && mDepth.texture &&
            ( entriesToFlush & RenderPassDescriptor::Depth ) )
        {
            mustWarn = true;
        }

        if( mStencil.loadAction == LoadAction::Load && mStencil.texture &&
            ( entriesToFlush & RenderPassDescriptor::Stencil ) )
        {
            mustWarn = true;
        }

        if( mustWarn )
        {
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "RenderTarget is getting flushed sooner than expected. "
                         "This is a performance and/or correctness warning and "
                         "the developer explicitly told us to warn you. Disable "
                         "warn_if_rtv_was_flushed / CompositorPassDef::mWarnIfRtvWasFlushed "
                         "to ignore this; use a LoadAction::Clear or don't break the two "
                         "passes with something in the middle that causes the flush.",
                         "RenderPassDescriptor::warnIfRtvWasFlushed" );
        }
    }
    //-----------------------------------------------------------------------------------
    void RenderPassDescriptor::checkRequiresTextureFlipping()
    {
        mRequiresTextureFlipping = false;

        bool bResolvingToNonFlipping = false;

        for( size_t i = 0; i < mNumColourEntries && !mRequiresTextureFlipping; ++i )
        {
            TextureGpu *texture = mColour[i].texture;
            mRequiresTextureFlipping = texture->requiresTextureFlipping();
            if( mColour[i].resolveTexture )
                bResolvingToNonFlipping = true;
        }

        if( !mRequiresTextureFlipping && mDepth.texture )
            mRequiresTextureFlipping = mDepth.texture->requiresTextureFlipping();

        if( !mRequiresTextureFlipping && mStencil.texture )
            mRequiresTextureFlipping = mStencil.texture->requiresTextureFlipping();

        if( bResolvingToNonFlipping )
            mRequiresTextureFlipping = mColour[0].resolveTexture->requiresTextureFlipping();
    }
    //-----------------------------------------------------------------------------------
    void RenderPassDescriptor::validateMemorylessTexture( const TextureGpu *texture,
                                                          const LoadAction::LoadAction loadAction,
                                                          const StoreAction::StoreAction storeAction,
                                                          const bool bIsDepthStencil )
    {
        if( !texture->isTilerMemoryless() )
            return;

        if( loadAction != LoadAction::Clear && loadAction != LoadAction::DontCare &&
            loadAction != LoadAction::ClearOnTilers )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Texture '" + texture->getNameStr() +
                             "' is TilerMemoryless. For load actions it can only use clear, "
                             "dont_care or clear_on_tilers.",
                         "RenderPassDescriptor::validateMemorylessTexture" );
        }

        if( !texture->isMultisample() || bIsDepthStencil )
        {
            if( storeAction != StoreAction::DontCare )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Texture '" + texture->getNameStr() +
                                 "' is TilerMemoryless. For store actions it can only use dont_care.",
                             "RenderPassDescriptor::validateMemorylessTexture" );
            }
        }
        else
        {
            if( storeAction != StoreAction::DontCare && storeAction != StoreAction::MultisampleResolve )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "MSAA Texture '" + texture->getNameStr() +
                                 "' is TilerMemoryless. For store actions it can only use dont_care "
                                 "or resolve.",
                             "RenderPassDescriptor::validateMemorylessTexture" );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void RenderPassDescriptor::colourEntriesModified()
    {
        mNumColourEntries = 0;

        mRequiresTextureFlipping = false;

        bool hasRenderWindow = false;

        while( mNumColourEntries < OGRE_MAX_MULTIPLE_RENDER_TARGETS &&
               mColour[mNumColourEntries].texture )
        {
            const RenderPassColourTarget &colourEntry = mColour[mNumColourEntries];

            validateMemorylessTexture( colourEntry.texture, colourEntry.loadAction,
                                       colourEntry.storeAction, false );

            if( colourEntry.texture->isRenderWindowSpecific() )
            {
                RenderPassColourTarget &colourEntryRW = mColour[mNumColourEntries];
                if( !colourEntry.texture->isMultisample() && colourEntry.resolveTexture )
                {
                    colourEntryRW.resolveTexture = 0;
                    colourEntryRW.storeAction = StoreAction::Store;
                }

                hasRenderWindow = true;
            }

            if( colourEntry.storeAction == StoreAction::MultisampleResolve &&
                !colourEntry.resolveTexture )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "When storeAction == StoreAction::MultisampleResolve, "
                             "there MUST be a resolve texture set.",
                             "RenderPassDescriptor::colourEntriesModified" );
            }

            if( colourEntry.resolveTexture )
            {
                if( !colourEntry.texture->isMultisample() )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "Resolve Texture '" + colourEntry.resolveTexture->getNameStr() +
                                     "' specified, but texture to render to '" +
                                     colourEntry.texture->getNameStr() + "' is not MSAA",
                                 "RenderPassDescriptor::colourEntriesModified" );
                }

                if( colourEntry.resolveTexture->isTilerMemoryless() )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "Resolving to Texture '" + colourEntry.resolveTexture->getNameStr() +
                                     "' which is memoryless!",
                                 "RenderPassDescriptor::colourEntriesModified" );
                }

                if( colourEntry.texture != colourEntry.resolveTexture &&
                    !colourEntry.texture->hasMsaaExplicitResolves() )
                {
                    LogManager::getSingleton().logMessage(
                        "[Performance Warning] Resolve Texture '" +
                            colourEntry.resolveTexture->getNameStr() +
                            "' specified, but texture to render to '" +
                            colourEntry.texture->getNameStr() +
                            "' was not created with MsaaExplicitResolve. This is valid and may be "
                            "intended. But if not, then you can save VRAM by creating the latter with "
                            "MsaaExplicitResolve|NotTexture (explicit_resolve and not_texture)",
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
                        LML_CRITICAL  // Draw the dev's attention in debug mode.
#else
                        LML_TRIVIAL
#endif
                    );
                }

                if( colourEntry.resolveTexture == colourEntry.texture &&
                    ( colourEntry.mipLevel != 0 || colourEntry.slice != 0 ) )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "MSAA textures can only render to mipLevel 0 and slice 0 "
                                 "unless using explicit resolves. Texture: " +
                                     colourEntry.texture->getNameStr(),
                                 "RenderPassDescriptor::colourEntriesModified" );
                }

                if( colourEntry.resolveTexture->isRenderWindowSpecific() )
                    hasRenderWindow = true;
            }

            if( colourEntry.allLayers && colourEntry.slice != 0 )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Layered Rendering (i.e. binding 2D array or cubemap) "
                             "only supported if slice = 0. Texture: " +
                                 colourEntry.texture->getNameStr(),
                             "RenderPassDescriptor::colourEntriesModified" );
            }

            ++mNumColourEntries;
        }

        if( !hasRenderWindow )
            mReadyWindowForPresent = false;

        checkRequiresTextureFlipping();
    }
    //-----------------------------------------------------------------------------------
    void RenderPassDescriptor::entriesModified( uint32 entryTypes )
    {
        assert( ( entryTypes & RenderPassDescriptor::All ) != 0 );

        if( entryTypes & RenderPassDescriptor::Colour )
            colourEntriesModified();

        if( mNumColourEntries == 0 && !mDepth.texture && !mStencil.texture )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "RenderPassDescriptor has no colour, depth nor stencil attachments!",
                         "RenderPassDescriptor::entriesModified" );
        }

        if( mDepth.texture && !PixelFormatGpuUtils::isDepth( mDepth.texture->getPixelFormat() ) )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "RenderPassDescriptor depth attachment '" + mDepth.texture->getNameStr() +
                             "' is not a depth format!",
                         "RenderPassDescriptor::entriesModified" );
        }

        if( mStencil.texture && !PixelFormatGpuUtils::isStencil( mStencil.texture->getPixelFormat() ) )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "RenderPassDescriptor stencil attachment '" + mStencil.texture->getNameStr() +
                             "' is not a stencil format!",
                         "RenderPassDescriptor::entriesModified" );
        }

        if( mDepth.texture )
        {
            for( size_t i = 0; i < mNumColourEntries; ++i )
            {
                if( !mDepth.texture->supportsAsDepthBufferFor( mColour[i].texture ) )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "Manually specified depth buffer '" + mDepth.texture->getNameStr() +
                                     "' is incompatible with colour RTT #" +
                                     StringConverter::toString( i ) + "'" +
                                     mColour[i].texture->getNameStr() +
                                     "\nColour: " + mColour[i].texture->getSettingsDesc() +
                                     "\nDepth: " + mDepth.texture->getSettingsDesc(),
                                 "RenderPassDescriptor::entriesModified" );
                }
            }

            validateMemorylessTexture( mDepth.texture, mDepth.loadAction, mDepth.storeAction, true );
        }

        if( mStencil.texture && mStencil.texture != mDepth.texture )
        {
            for( size_t i = 0; i < mNumColourEntries; ++i )
            {
                if( !mStencil.texture->supportsAsDepthBufferFor( mColour[i].texture ) )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "Manually specified stencil buffer '" + mStencil.texture->getNameStr() +
                                     "' is incompatible with colour RTT #" +
                                     StringConverter::toString( i ) + "'" +
                                     mColour[i].texture->getNameStr() +
                                     "'\nColour: " + mColour[i].texture->getSettingsDesc() +
                                     "\nStencil: " + mStencil.texture->getSettingsDesc(),
                                 "RenderPassDescriptor::entriesModified" );
                }
            }

            validateMemorylessTexture( mStencil.texture, mStencil.loadAction, mStencil.storeAction,
                                       true );
        }

        checkRequiresTextureFlipping();
    }
    //-----------------------------------------------------------------------------------
    void RenderPassDescriptor::setClearColour( uint8 idx, const ColourValue &clearColour )
    {
        assert( idx < OGRE_MAX_MULTIPLE_RENDER_TARGETS );
        mColour[idx].clearColour = clearColour;
    }
    //-----------------------------------------------------------------------------------
    void RenderPassDescriptor::setClearDepth( Real clearDepth ) { mDepth.clearDepth = clearDepth; }
    //-----------------------------------------------------------------------------------
    void RenderPassDescriptor::setClearStencil( uint32 clearStencil )
    {
        mStencil.clearStencil = clearStencil;
    }
    //-----------------------------------------------------------------------------------
    void RenderPassDescriptor::setClearColour( const ColourValue &clearColour )
    {
        for( uint8 i = 0; i < mNumColourEntries; ++i )
            setClearColour( i, clearColour );
    }
    //-----------------------------------------------------------------------------------
    bool RenderPassDescriptor::hasSameAttachments( const RenderPassDescriptor *otherPassDesc ) const
    {
        if( !otherPassDesc )
            return false;

        if( this->mNumColourEntries != otherPassDesc->mNumColourEntries )
            return false;

        for( size_t i = 0; i < this->mNumColourEntries; ++i )
        {
            if( this->mColour[i].texture != otherPassDesc->mColour[i].texture ||
                this->mColour[i].resolveTexture != otherPassDesc->mColour[i].resolveTexture ||
                this->mColour[i].mipLevel != otherPassDesc->mColour[i].mipLevel ||
                this->mColour[i].resolveMipLevel != otherPassDesc->mColour[i].resolveMipLevel ||
                this->mColour[i].slice != otherPassDesc->mColour[i].slice ||
                this->mColour[i].resolveSlice != otherPassDesc->mColour[i].resolveSlice )
            {
                return false;
            }
        }

        if( this->mDepth.texture != otherPassDesc->mDepth.texture ||
            this->mStencil.texture != otherPassDesc->mStencil.texture )
        {
            return false;
        }

        return true;
    }
    //-----------------------------------------------------------------------------------
    bool RenderPassDescriptor::hasAttachment( const TextureGpu *texture ) const
    {
        const size_t numColourEntries = mNumColourEntries;
        for( size_t i = 0; i < numColourEntries; ++i )
        {
            if( mColour[i].texture == texture )
                return true;
        }

        if( mDepth.texture == texture || mStencil.texture == texture )
            return true;

        return false;
    }
    //-----------------------------------------------------------------------------------
    bool RenderPassDescriptor::hasStencilFormat() const
    {
        return mStencil.texture ||
               ( mDepth.texture && PixelFormatGpuUtils::isStencil( mDepth.texture->getPixelFormat() ) );
    }
    //-----------------------------------------------------------------------------------
    void RenderPassDescriptor::findAnyTexture( TextureGpu **outAnyTargetTexture, uint8 &outAnyMipLevel )
    {
        TextureGpu *anyTargetTexture = 0;
        uint8 anyMipLevel = 0u;

        for( int i = 0; i < mNumColourEntries && !anyTargetTexture; ++i )
        {
            anyTargetTexture = mColour[i].texture;
            anyMipLevel = mColour[i].mipLevel;
        }
        if( !anyTargetTexture )
        {
            anyTargetTexture = mDepth.texture;
            anyMipLevel = mDepth.mipLevel;
        }
        if( !anyTargetTexture )
        {
            anyTargetTexture = mStencil.texture;
            anyMipLevel = mStencil.mipLevel;
        }

        *outAnyTargetTexture = anyTargetTexture;
        outAnyMipLevel = anyMipLevel;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    FrameBufferDescKey::FrameBufferDescKey() { memset( this, 0, sizeof( *this ) ); }
    //-----------------------------------------------------------------------------------
    FrameBufferDescKey::FrameBufferDescKey( const RenderPassDescriptor &desc )
    {
        memset( this, 0, sizeof( *this ) );
        readyWindowForPresent = desc.mReadyWindowForPresent;
        numColourEntries = desc.getNumColourEntries();

        // Load & Store actions don't matter for generating different FBOs.

        for( size_t i = 0; i < numColourEntries; ++i )
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
    bool FrameBufferDescKey::operator<( const FrameBufferDescKey &other ) const
    {
        if( this->readyWindowForPresent != other.readyWindowForPresent )
            return this->readyWindowForPresent < other.readyWindowForPresent;

        if( this->numColourEntries != other.numColourEntries )
            return this->numColourEntries < other.numColourEntries;

        for( size_t i = 0; i < numColourEntries; ++i )
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
}  // namespace Ogre
