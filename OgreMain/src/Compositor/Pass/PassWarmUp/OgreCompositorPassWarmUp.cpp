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
#include "Compositor/OgreCompositorWorkspace.h"
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
        mCamera( 0 )
    {
        initialize( rtv );

        const CompositorWorkspace *workspace = parentNode->getWorkspace();

        if( mDefinition->mCameraName != IdString() )
            mCamera = workspace->findCamera( mDefinition->mCameraName );
        else
            mCamera = defaultCamera;
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

        analyzeBarriers();
        executeResourceTransitions();

        setRenderPassDescToCurrent();

        SceneManager *sceneManager = mCamera->getSceneManager();
        sceneManager->_setCamerasInProgress( CamerasInProgress( mCamera ) );
        sceneManager->_setForwardPlusEnabledInPass( mDefinition->mEnableForwardPlus );
        // TODO
        // sceneManager->_setRefractions( mDepthTextureNoMsaa, mRefractionsTexture );
        sceneManager->_setCurrentCompositorPass( this );

        // Fire the listener in case it wants to change anything
        notifyPassPreExecuteListeners();

        RenderSystem *renderSystem = sceneManager->getDestinationRenderSystem();
        renderSystem->executeRenderPassDescriptorDelayedActions();

        sceneManager->_warmUpShaders( mCamera, mDefinition->mVisibilityMask, mDefinition->mFirstRQ,
                                      mDefinition->mLastRQ );

        sceneManager->_setCurrentCompositorPass( 0 );

        notifyPassPosExecuteListeners();

        profilingEnd();
    }
}  // namespace Ogre
