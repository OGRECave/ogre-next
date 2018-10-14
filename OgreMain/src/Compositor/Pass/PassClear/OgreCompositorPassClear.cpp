/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

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

#include "OgreStableHeaders.h"

#include "Compositor/Pass/PassClear/OgreCompositorPassClear.h"
#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"

#include "OgreSceneManager.h"
#include "OgreViewport.h"
#include "OgreSceneManager.h"
#include "OgreRenderTarget.h"
#include "OgrePixelFormatGpuUtils.h"

namespace Ogre
{
    CompositorPassClear::CompositorPassClear( const CompositorPassClearDef *definition,
                                              SceneManager *sceneManager,
                                              const RenderTargetViewDef *rtv,
                                              CompositorNode *parentNode ) :
                CompositorPass( definition, parentNode ),
                mSceneManager( sceneManager ),
                mDefinition( definition )
    {
        initialize( rtv );
    }
    //-----------------------------------------------------------------------------------
    bool CompositorPassClear::allowResolveStoreActionsWithoutResolveTexture(void) const
    {
        return true;
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassClear::postRenderPassDescriptorSetup( RenderPassDescriptor *renderPassDesc )
    {
        //IMPORTANT: We cannot rely on renderPassDesc->getNumColourEntries()
        //because it hasn't been calculated yet.
        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        const RenderSystemCapabilities *capabilities = renderSystem->getCapabilities();

        if( mDefinition->mNonTilersOnly && capabilities->hasCapability( RSC_IS_TILER ) &&
            !capabilities->hasCapability( RSC_TILER_CAN_CLEAR_STENCIL_REGION ) &&
            (renderPassDesc->hasStencilFormat() &&
             (renderPassDesc->mDepth.loadAction == LoadAction::Clear ||
             renderPassDesc->mStencil.loadAction == LoadAction::Clear)) )
        {
            //Normally this clear would be a no-op (because we're on a tiler GPU
            //and this is a non-tiler pass). However depth-stencil formats
            //must be cleared like a non-tiler. We must update our RenderPassDesc to
            //avoid clearing the colour (since that will still behave like tiler)
            for( size_t i=0; i<OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
            {
                if( renderPassDesc->mColour[i].loadAction != LoadAction::Load )
                    renderPassDesc->mColour[i].loadAction = LoadAction::Load;
            }
        }

        //Clears default to writing both to MSAA & resolve texture, but this will cause
        //complaints later on if there is no resolve texture set. Silently set it to Store only.
        for( size_t i=0; i<OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
        {
            if( !renderPassDesc->mColour[i].resolveTexture )
                renderPassDesc->mColour[i].storeAction = StoreAction::Store;
        }

        if( !renderPassDesc->mDepth.resolveTexture )
            renderPassDesc->mDepth.storeAction = StoreAction::Store;
        if( !renderPassDesc->mStencil.resolveTexture )
            renderPassDesc->mStencil.storeAction = StoreAction::Store;
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassClear::execute( const Camera *lodCamera )
    {
        //Execute a limited number of times?
        if( mNumPassesLeft != std::numeric_limits<uint32>::max() )
        {
            if( !mNumPassesLeft )
                return;
            --mNumPassesLeft;
        }

        profilingBegin();

        CompositorWorkspaceListener *listener = mParentNode->getWorkspace()->getListener();
        if( listener )
            listener->passEarlyPreExecute( this );

        executeResourceTransitions();

        //Fire the listener in case it wants to change anything
        if( listener )
            listener->passPreExecute( this );

        RenderSystem *renderSystem = mParentNode->getRenderSystem();

        const RenderSystemCapabilities *capabilities = renderSystem->getCapabilities();
        if( !mDefinition->mNonTilersOnly ||
            !capabilities->hasCapability( RSC_IS_TILER ) ||
            (!capabilities->hasCapability( RSC_TILER_CAN_CLEAR_STENCIL_REGION ) &&
             (mRenderPassDesc->hasStencilFormat() &&
              (mRenderPassDesc->mDepth.loadAction == LoadAction::Clear ||
              mRenderPassDesc->mStencil.loadAction == LoadAction::Clear))) )
        {
            renderSystem->clearFrameBuffer( mRenderPassDesc, mAnyTargetTexture, mAnyMipLevel );
        }

        if( listener )
            listener->passPosExecute( this );

        profilingEnd();
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassClear::_placeBarriersAndEmulateUavExecution( BoundUav boundUavs[64],
                                                                    ResourceAccessMap &uavsAccess,
                                                                    ResourceLayoutMap &resourcesLayout )
    {
        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        const RenderSystemCapabilities *caps = renderSystem->getCapabilities();
        const bool explicitApi = caps->hasCapability( RSC_EXPLICIT_API );

        if( !explicitApi )
            return;

        //Check <anything> -> Clear
        ResourceLayoutMap::iterator currentLayout;
        for( int i=0; i<mRenderPassDesc->getNumColourEntries(); ++i )
        {
            currentLayout = resourcesLayout.find( mRenderPassDesc->mColour[i].texture );
            if( (currentLayout->second != ResourceLayout::RenderTarget && explicitApi) ||
                currentLayout->second == ResourceLayout::Uav )
            {
                addResourceTransition( currentLayout,
                                       ResourceLayout::Clear,
                                       ReadBarrier::RenderTarget );
            }
        }
        if( mRenderPassDesc->mDepth.texture )
        {
            currentLayout = resourcesLayout.find( mRenderPassDesc->mDepth.texture );
            if( currentLayout == resourcesLayout.end() )
            {
                resourcesLayout[mRenderPassDesc->mDepth.texture] = ResourceLayout::Undefined;
                currentLayout = resourcesLayout.find( mRenderPassDesc->mDepth.texture );
            }

            if( (currentLayout->second != ResourceLayout::RenderDepth && explicitApi) ||
                currentLayout->second == ResourceLayout::Uav )
            {
                addResourceTransition( currentLayout,
                                       ResourceLayout::Clear,
                                       ReadBarrier::DepthStencil );
            }
        }
        if( mRenderPassDesc->mStencil.texture &&
            mRenderPassDesc->mStencil.texture != mRenderPassDesc->mDepth.texture )
        {
            currentLayout = resourcesLayout.find( mRenderPassDesc->mStencil.texture );
            if( currentLayout == resourcesLayout.end() )
            {
                resourcesLayout[mRenderPassDesc->mStencil.texture] = ResourceLayout::Undefined;
                currentLayout = resourcesLayout.find( mRenderPassDesc->mStencil.texture );
            }

            if( (currentLayout->second != ResourceLayout::RenderDepth && explicitApi) ||
                currentLayout->second == ResourceLayout::Uav )
            {
                addResourceTransition( currentLayout,
                                       ResourceLayout::Clear,
                                       ReadBarrier::DepthStencil );
            }
        }

        //Do not use base class functionality at all.
        //CompositorPass::_placeBarriersAndEmulateUavExecution();
    }
}
