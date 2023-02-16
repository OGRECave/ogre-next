/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2023 Torus Knot Software Ltd

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

#include "Compositor/Pass/PassWarmUp/OgreCompositorPassWarmUp.h"

#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorShadowNode.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"
#include "Compositor/Pass/PassWarmUp/OgreCompositorPassWarmUpDef.h"
#include "OgreCamera.h"
#include "OgreSceneManager.h"

namespace Ogre
{
    //-----------------------------------------------------------------------------------
    CompositorPassWarmUp::CompositorPassWarmUp( const CompositorPassWarmUpDef *definition,
                                                Camera *defaultCamera, CompositorNode *parentNode,
                                                const RenderTargetViewDef *rtv ) :
        CompositorPass( definition, parentNode ),
        mDefinition( definition ),
        mShadowNode( 0 ),
        mCamera( 0 )
    {
        initialize( rtv );

        CompositorWorkspace *workspace = parentNode->getWorkspace();

        if( mDefinition->mShadowNode != IdString() )
        {
            bool shadowNodeCreated;
            mShadowNode =
                workspace->findOrCreateShadowNode( mDefinition->mShadowNode, shadowNodeCreated );
        }

        if( mDefinition->mCameraName != IdString() )
            mCamera = workspace->findCamera( mDefinition->mCameraName );
        else
            mCamera = defaultCamera;
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassWarmUp::notifyPassSceneAfterShadowMapsListeners()
    {
        const CompositorWorkspaceListenerVec &listeners = mParentNode->getWorkspace()->getListeners();
        for( CompositorWorkspaceListener *listener : listeners )
            listener->passSceneAfterShadowMaps( nullptr );
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassWarmUp::execute( const Camera *lodCamera )
    {
        // Execute a limited number of times?
        if( mNumPassesLeft != std::numeric_limits<uint32>::max() )
        {
            if( !mNumPassesLeft )
                return;
            --mNumPassesLeft;
        }

        profilingBegin();

        notifyPassEarlyPreExecuteListeners();

        SceneManager *sceneManager = mCamera->getSceneManager();

        Viewport *viewport = sceneManager->getCurrentViewport0();
        viewport->_setVisibilityMask( mDefinition->mVisibilityMask, 0xFFFFFFFFu );

        CompositorShadowNode *shadowNode =
            ( mShadowNode && mShadowNode->getEnabled() ) ? mShadowNode : 0;
        sceneManager->_setCurrentShadowNode( shadowNode );

        // Fire the listener in case it wants to change anything
        notifyPassPreExecuteListeners();

        if( shadowNode )
        {
            // We need to prepare for rendering another RT (we broke the contiguous chain)
            if( mDefinition->mSkipLoadStoreSemantics )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "mSkipLoadStoreSemantics can't be true if updating the shadow node. You "
                             "can use shadow_node reuse",
                             "CompositorPassScene::execute" );
            }

            // Save the value in case the listener changed it
            const uint32 oldVisibilityMask = viewport->getVisibilityMask();
            const uint32 oldLightVisibilityMask = viewport->getLightVisibilityMask();

            // use culling camera for shadows, so if shadows are re used for slightly different camera
            // (ie VR) shadows are not 'over culled'
            mCamera->_notifyViewport( viewport );

            shadowNode->_update( mCamera, lodCamera, sceneManager );

            // ShadowNode passes may've overriden these settings.
            sceneManager->_setCurrentShadowNode( shadowNode );
            viewport->_setVisibilityMask( oldVisibilityMask, oldLightVisibilityMask );
            mCamera->_notifyViewport( viewport );
            // We need to restore the previous RT's update
        }

        notifyPassSceneAfterShadowMapsListeners();

        analyzeBarriers();
        executeResourceTransitions();

        setRenderPassDescToCurrent();

        sceneManager->_setCamerasInProgress( CamerasInProgress( mCamera ) );
        sceneManager->_setForwardPlusEnabledInPass( mDefinition->mEnableForwardPlus );
        // TODO
        // sceneManager->_setRefractions( mDepthTextureNoMsaa, mRefractionsTexture );
        sceneManager->_setCurrentCompositorPass( this );

        RenderSystem *renderSystem = sceneManager->getDestinationRenderSystem();
        renderSystem->executeRenderPassDescriptorDelayedActions();

        sceneManager->_warmUpShaders( mCamera, mDefinition->mVisibilityMask, mDefinition->mFirstRQ,
                                      mDefinition->mLastRQ );

        sceneManager->_setCurrentCompositorPass( 0 );

        notifyPassPosExecuteListeners();

        profilingEnd();
    }
}  // namespace Ogre
