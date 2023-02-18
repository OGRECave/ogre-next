/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#include "Compositor/Pass/PassShadows/OgreCompositorPassShadows.h"

#include "Compositor/OgreCompositorShadowNode.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/Pass/PassShadows/OgreCompositorPassShadowsDef.h"
#include "OgreCamera.h"
#include "OgreRoot.h"

namespace Ogre
{
    CompositorPassShadowsDef::CompositorPassShadowsDef( CompositorNodeDef *parentNodeDef,
                                                        CompositorTargetDef *parentTargetDef ) :
        CompositorPassDef( PASS_SHADOWS, parentTargetDef ),
        mParentNodeDef( parentNodeDef ),
        mVisibilityMask( VisibilityFlags::RESERVED_VISIBILITY_FLAGS ),
        mCameraCubemapReorient( false )
    {
    }
    //-------------------------------------------------------------------------
    void CompositorPassShadowsDef::setVisibilityMask( uint32 visibilityMask )
    {
        mVisibilityMask = visibilityMask & VisibilityFlags::RESERVED_VISIBILITY_FLAGS;
    }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    CompositorPassShadows::CompositorPassShadows( const CompositorPassShadowsDef *definition,
                                                  Camera *defaultCamera, CompositorNode *parentNode,
                                                  const RenderTargetViewDef * /*rtv*/ ) :
        CompositorPass( definition, parentNode ),
        mDefinition( definition ),
        mCamera( 0 ),
        mLodCamera( 0 ),
        mCullCamera( 0 )
    {
        initialize( 0, true );

        CompositorWorkspace *workspace = parentNode->getWorkspace();

        if( mDefinition->mCameraName != IdString() )
            mCamera = workspace->findCamera( mDefinition->mCameraName );
        else
            mCamera = defaultCamera;

        if( mDefinition->mLodCameraName != IdString() )
            mLodCamera = workspace->findCamera( mDefinition->mLodCameraName );
        else
            mLodCamera = mCamera;

        if( mDefinition->mCullCameraName != IdString() )
            mCullCamera = workspace->findCamera( mDefinition->mCullCameraName );
        else
            mCullCamera = mCamera;

        FastArray<IdString>::const_iterator itor = definition->mShadowNodes.begin();
        FastArray<IdString>::const_iterator endt = definition->mShadowNodes.end();

        while( itor != endt )
        {
            bool bShadowNodeCreated;
            CompositorShadowNode *shadowNode =
                workspace->findOrCreateShadowNode( *itor, bShadowNodeCreated );
            mShadowNodes.push_back( shadowNode );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    CompositorPassShadows::~CompositorPassShadows() {}
    //-----------------------------------------------------------------------------------
    void CompositorPassShadows::execute( const Camera *lodCamera )
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
        notifyPassPreExecuteListeners();

        SceneManager *sceneManager = mCamera->getSceneManager();

        Camera const *usedLodCamera = mLodCamera;
        if( lodCamera && mDefinition->mLodCameraName == IdString() )
            usedLodCamera = lodCamera;

        const Quaternion oldCameraOrientation( mCamera->getOrientation() );

        // We have to do this first in case usedLodCamera == mCamera
        if( mDefinition->mCameraCubemapReorient )
        {
            uint32 sliceIdx = std::min<uint32>( mDefinition->getRtIndex(), 5 );
            mCamera->setOrientation( oldCameraOrientation * CubemapRotations[sliceIdx] );
        }

        Viewport *viewport = sceneManager->getCurrentViewport0();

        FastArray<CompositorShadowNode *>::const_iterator itor = mShadowNodes.begin();
        FastArray<CompositorShadowNode *>::const_iterator endt = mShadowNodes.end();

        while( itor != endt )
        {
            viewport->_setVisibilityMask( mDefinition->mVisibilityMask, mDefinition->mVisibilityMask );
            sceneManager->_setCurrentShadowNode( *itor );

            // use culling camera for shadows, so if shadows are re used for slightly different camera
            // (ie VR) shadows are not 'over culled'
            mCullCamera->_notifyViewport( viewport );

            ( *itor )->_update( mCullCamera, usedLodCamera, sceneManager );

            ++itor;
        }

        if( mDefinition->mCameraCubemapReorient )
        {
            // Restore orientation
            mCamera->setOrientation( oldCameraOrientation );
        }

        sceneManager->_setCurrentCompositorPass( 0 );
        sceneManager->_setCurrentShadowNode( 0 );

        notifyPassPosExecuteListeners();

        profilingEnd();
    }
}  // namespace Ogre
