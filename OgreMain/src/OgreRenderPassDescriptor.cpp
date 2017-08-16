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

#include "OgreRenderPassDescriptor.h"
#include "OgreTextureGpu.h"
#include "OgreException.h"

namespace Ogre
{
    RenderPassTargetBase::RenderPassTargetBase()
    {
        memset( this, 0, sizeof(*this) );
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
        mRequiresTextureFlipping( false )
    {
    }
    //-----------------------------------------------------------------------------------
    RenderPassDescriptor::~RenderPassDescriptor()
    {
    }
    //-----------------------------------------------------------------------------------
    void RenderPassDescriptor::checkRequiresTextureFlipping(void)
    {
        mRequiresTextureFlipping = false;

        for( size_t i=0; i<mNumColourEntries && !mRequiresTextureFlipping; ++i )
        {
            TextureGpu *texture = mColour[i].texture;
            mRequiresTextureFlipping = texture->requiresTextureFlipping();
        }

        if( !mRequiresTextureFlipping && mDepth.texture )
            mRequiresTextureFlipping = mDepth.texture->requiresTextureFlipping();

        if( !mRequiresTextureFlipping && mStencil.texture )
            mRequiresTextureFlipping = mStencil.texture->requiresTextureFlipping();
    }
    //-----------------------------------------------------------------------------------
    void RenderPassDescriptor::colourEntriesModified(void)
    {
        mNumColourEntries = 0;

        mRequiresTextureFlipping = false;

        while( mNumColourEntries < OGRE_MAX_MULTIPLE_RENDER_TARGETS &&
               mColour[mNumColourEntries].texture )
        {
            const RenderPassColourTarget &colourEntry = mColour[mNumColourEntries];

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
                if( colourEntry.texture->getMsaa() <= 1u )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "Resolve Texture '" + colourEntry.resolveTexture->getNameStr() +
                                 "' specified, but texture to render to '" +
                                 colourEntry.texture->getNameStr() + "' is not MSAA",
                                 "RenderPassDescriptor::colourEntriesModified" );
                }
                if( colourEntry.resolveTexture == colourEntry.texture &&
                    (colourEntry.mipLevel != 0 || colourEntry.slice != 0) )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "MSAA textures can only render to mipLevel 0 and slice 0 "
                                 "unless using explicit resolves. Texture: " +
                                 colourEntry.texture->getNameStr(),
                                 "RenderPassDescriptor::colourEntriesModified" );
                }
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

        checkRequiresTextureFlipping();
    }
    //-----------------------------------------------------------------------------------
    void RenderPassDescriptor::entriesModified( uint32 entryTypes )
    {
        assert( (entryTypes & RenderPassDescriptor::All) != 0 );

        if( entryTypes & RenderPassDescriptor::Colour )
            colourEntriesModified();
        checkRequiresTextureFlipping();
    }
    //-----------------------------------------------------------------------------------
    void RenderPassDescriptor::setClearColour( uint8 idx, const ColourValue &clearColour )
    {
        assert( idx < OGRE_MAX_MULTIPLE_RENDER_TARGETS );
        mColour[idx].clearColour = clearColour;
    }
    //-----------------------------------------------------------------------------------
    void RenderPassDescriptor::setClearDepth( Real clearDepth )
    {
        mDepth.clearDepth = clearDepth;
    }
    //-----------------------------------------------------------------------------------
    void RenderPassDescriptor::setClearStencil( uint32 clearStencil )
    {
        mStencil.clearStencil = clearStencil;
    }
    //-----------------------------------------------------------------------------------
    void RenderPassDescriptor::setClearColour( const ColourValue &clearColour )
    {
        for( uint8 i=0; i<mNumColourEntries; ++i )
            setClearColour( i, clearColour );
    }
}
