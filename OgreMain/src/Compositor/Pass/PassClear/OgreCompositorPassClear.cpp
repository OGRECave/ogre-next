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
                CompositorPass( definition, rtv, parentNode ),
                mSceneManager( sceneManager ),
                mDefinition( definition )
    {
        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        const RenderSystemCapabilities *capabilities = renderSystem->getCapabilities();

        if( mDefinition->mNonTilersOnly && capabilities->hasCapability( RSC_IS_TILER ) &&
            !capabilities->hasCapability( RSC_TILER_CAN_CLEAR_STENCIL_REGION ) &&
            (mRenderPassDesc->hasStencilFormat() &&
             (mRenderPassDesc->mDepth.loadAction == LoadAction::Clear ||
             mRenderPassDesc->mStencil.loadAction == LoadAction::Clear)) )
        {
            //Normally this clear would be a no-op (because we're on a tiler GPU
            //and this is a non-tiler pass). However depth-stencil formats
            //must be cleared like a non-tiler. We must update our RenderPassDesc to
            //avoid clearing the colour (since that will still behave like tiler)
            uint32 entryTypes = 0;
            for( size_t i=0; i<mRenderPassDesc->getNumColourEntries(); ++i )
            {
                if( mRenderPassDesc->mColour[i].loadAction != LoadAction::Load )
                {
                    entryTypes |= RenderPassDescriptor::Colour0 << i;
                    mRenderPassDesc->mColour[i].loadAction = LoadAction::Load;
                }
            }

            if( entryTypes )
                mRenderPassDesc->entriesModified( entryTypes );
        }
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
            renderSystem->clearFrameBuffer( mRenderPassDesc, mAnyTargetTexture );
        }

        if( listener )
            listener->passPosExecute( this );
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
