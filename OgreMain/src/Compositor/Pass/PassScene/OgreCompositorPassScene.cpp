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

#include "Compositor/Pass/PassScene/OgreCompositorPassScene.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"
#include "Compositor/OgreCompositorShadowNode.h"

#include "OgreViewport.h"
#include "OgreSceneManager.h"

namespace Ogre
{
    CompositorPassScene::CompositorPassScene( const CompositorPassSceneDef *definition,
                                              Camera *defaultCamera, const RenderTargetViewDef *rtv,
                                              CompositorNode *parentNode ) :
                CompositorPass( definition, parentNode ),
                mDefinition( definition ),
                mShadowNode( 0 ),
                mCamera( 0 ),
                mLodCamera( 0 ),
                mCullCamera( 0 ),
                mUpdateShadowNode( false ),
                mPrePassTextures( 0 ),
                mPrePassDepthTexture( 0 ),
                mSsrTexture( 0 ),
                mDepthTextureNoMsaa( 0 ),
                mRefractionsTexture( 0 )
    {
        initialize( rtv );

        CompositorWorkspace *workspace = parentNode->getWorkspace();

        if( mDefinition->mShadowNode != IdString() )
        {
            bool shadowNodeCreated;
            mShadowNode = workspace->findOrCreateShadowNode( mDefinition->mShadowNode,
                                                             shadowNodeCreated );

            // Passes with "first_only" option are set in CompositorWorkspace::setupPassesShadowNodes
            if( mDefinition->mShadowNodeRecalculation != SHADOW_NODE_FIRST_ONLY )
                mUpdateShadowNode = mDefinition->mShadowNodeRecalculation == SHADOW_NODE_RECALCULATE;
        }

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

        if( mDefinition->mPrePassMode == PrePassNone && mDefinition->mGenNormalsGBuf &&
            rtv->colourAttachments.size() < 2u )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Requesting mGenNormalsGBuf (generate normals in GBuffer alongside colour) but "
                         "there is less than 2 colour render textures to render to!",
                         "CompositorPassScene::CompositorPassScene" );
        }

        if( mDefinition->mPrePassMode == PrePassUse && !mDefinition->mPrePassTexture.empty() )
        {
            {
                IdStringVec::const_iterator itPrePassTexName = mDefinition->mPrePassTexture.begin();
                IdStringVec::const_iterator enPrePassTexName = mDefinition->mPrePassTexture.end();
                while( itPrePassTexName != enPrePassTexName )
                {
                    TextureGpu *channel = parentNode->getDefinedTexture( *itPrePassTexName );
                    mPrePassTextures.push_back( channel );
                    ++itPrePassTexName;
                }
            }

            if( mDefinition->mPrePassDepthTexture != IdString() )
            {
                mPrePassDepthTexture =
                    parentNode->getDefinedTexture( mDefinition->mPrePassDepthTexture );
            };

            if( mDefinition->mPrePassSsrTexture != IdString() )
            {
                mSsrTexture = parentNode->getDefinedTexture( mDefinition->mPrePassSsrTexture );
            }
        }

        if( mDefinition->mDepthTextureNoMsaa != IdString() )
        {
            mDepthTextureNoMsaa = parentNode->getDefinedTexture( mDefinition->mDepthTextureNoMsaa );
            if( mDepthTextureNoMsaa->getMsaa() > 1u )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "CompositorPassSceneDef::mDepthTextureNoMsaa must point to a depth buffer "
                             "WITHOUT MSAA",
                             "CompositorPassScene::CompositorPassScene" );
            }
        };

        if( mDefinition->mRefractionsTexture != IdString() )
            mRefractionsTexture = parentNode->getDefinedTexture( mDefinition->mRefractionsTexture );
    }
    //-----------------------------------------------------------------------------------
    CompositorPassScene::~CompositorPassScene()
    {
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassScene::notifyPassSceneAfterShadowMapsListeners(void)
    {
        const CompositorWorkspaceListenerVec& listeners = mParentNode->getWorkspace()->getListeners();

        CompositorWorkspaceListenerVec::const_iterator itor = listeners.begin();
        CompositorWorkspaceListenerVec::const_iterator end  = listeners.end();

        while( itor != end )
        {
            (*itor)->passSceneAfterShadowMaps( this );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassScene::notifyPassSceneAfterFrustumCullingListeners(void)
    {
        const CompositorWorkspaceListenerVec& listeners = mParentNode->getWorkspace()->getListeners();

        CompositorWorkspaceListenerVec::const_iterator itor = listeners.begin();
        CompositorWorkspaceListenerVec::const_iterator end  = listeners.end();

        while( itor != end )
        {
            (*itor)->passSceneAfterFrustumCulling( this );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassScene::execute( const Camera *lodCamera )
    {
        //Execute a limited number of times?
        if( mNumPassesLeft != std::numeric_limits<uint32>::max() )
        {
            if( !mNumPassesLeft )
                return;
            --mNumPassesLeft;
        }

        profilingBegin();

        notifyPassEarlyPreExecuteListeners();

        Camera const *usedLodCamera = mLodCamera;
        if( lodCamera && mDefinition->mLodCameraName == IdString() )
            usedLodCamera = lodCamera;

#if TODO_OGRE_2_2
        //store the viewports current material scheme and use the one set in the scene pass def
        String oldViewportMatScheme = mViewport->getMaterialScheme();
        mViewport->setMaterialScheme(mDefinition->mMaterialScheme);
#endif

        SceneManager *sceneManager = mCamera->getSceneManager();

        const Quaternion oldCameraOrientation( mCamera->getOrientation() );

        //We have to do this first in case usedLodCamera == mCamera
        if( mDefinition->mCameraCubemapReorient )
        {
            uint32 sliceIdx = std::min<uint32>( mDefinition->getRtIndex(), 5 );
            mCamera->setOrientation( oldCameraOrientation * CubemapRotations[sliceIdx] );
        }

        if( mDefinition->mUpdateLodLists )
        {
            sceneManager->updateAllLods( usedLodCamera, mDefinition->mLodBias,
                                         mDefinition->mFirstRQ, mDefinition->mLastRQ );
        }

        //Passes belonging to a ShadowNode should not override their parent.
        CompositorShadowNode* shadowNode = (mShadowNode && mShadowNode->getEnabled()) ? mShadowNode : 0;
        if( mDefinition->mShadowNodeRecalculation != SHADOW_NODE_CASTER_PASS )
        {
            sceneManager->_setCurrentShadowNode( shadowNode, mDefinition->mShadowNodeRecalculation ==
                                                                                    SHADOW_NODE_REUSE );
        }

        Viewport *viewport = sceneManager->getCurrentViewport0();
        viewport->_setVisibilityMask( mDefinition->mVisibilityMask, mDefinition->mLightVisibilityMask );

        //Fire the listener in case it wants to change anything
        notifyPassPreExecuteListeners();

        if( mUpdateShadowNode && shadowNode )
        {
            //We need to prepare for rendering another RT (we broke the contiguous chain)

            //Save the value in case the listener changed it
            const uint32 oldVisibilityMask = viewport->getVisibilityMask();
            const uint32 oldLightVisibilityMask = viewport->getLightVisibilityMask();

            // use culling camera for shadows, so if shadows are re used for slightly different camera (ie VR)
            // shadows are not 'over culled'
            mCullCamera->_notifyViewport(viewport);

            shadowNode->_update(mCullCamera, usedLodCamera, sceneManager);

            //ShadowNode passes may've overriden these settings.
            sceneManager->_setCurrentShadowNode( shadowNode, mDefinition->mShadowNodeRecalculation ==
                                                                                    SHADOW_NODE_REUSE );
            viewport->_setVisibilityMask( oldVisibilityMask, oldLightVisibilityMask );
            mCullCamera->_notifyViewport(viewport);

            if( mDefinition->mFlushCommandBuffersAfterShadowNode )
            {
                RenderSystem *renderSystem = mParentNode->getRenderSystem();
                renderSystem->flushCommands();
            }
            //We need to restore the previous RT's update
        }

        notifyPassSceneAfterShadowMapsListeners();

        executeResourceTransitions();
        setRenderPassDescToCurrent();

        sceneManager->_setForwardPlusEnabledInPass( mDefinition->mEnableForwardPlus );
        sceneManager->_setPrePassMode( mDefinition->mPrePassMode, mPrePassTextures,
                                       mPrePassDepthTexture, mSsrTexture );
        sceneManager->_setRefractions( mDepthTextureNoMsaa, mRefractionsTexture );
        sceneManager->_setCurrentCompositorPass( this );

        viewport->_updateCullPhase01( mCamera, mCullCamera, usedLodCamera,
                                      mDefinition->mFirstRQ, mDefinition->mLastRQ,
                                      mDefinition->mReuseCullData );

        notifyPassSceneAfterFrustumCullingListeners();

#if TODO_OGRE_2_2
        mTarget->setFsaaResolveDirty();
#endif
        viewport->_updateRenderPhase02( mCamera, usedLodCamera,
                                        mDefinition->mFirstRQ, mDefinition->mLastRQ );

        if( mDefinition->mCameraCubemapReorient )
        {
            //Restore orientation
            mCamera->setOrientation( oldCameraOrientation );
        }

#if TODO_OGRE_2_2
        //restore viewport material scheme
        mViewport->setMaterialScheme(oldViewportMatScheme);
#endif

        sceneManager->_setPrePassMode( PrePassNone, TextureGpuVec(), 0, 0 );
        sceneManager->_setCurrentCompositorPass( 0 );

        if( mDefinition->mShadowNodeRecalculation != SHADOW_NODE_CASTER_PASS )
        {
            sceneManager->_setCurrentShadowNode( 0, false );
            sceneManager->_setForwardPlusEnabledInPass( false );
        }

        notifyPassPosExecuteListeners();

        profilingEnd();
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassScene::_placeBarriersAndEmulateUavExecution( BoundUav boundUavs[64],
                                                                    ResourceAccessMap &uavsAccess,
                                                                    ResourceLayoutMap &resourcesLayout )
    {
        if( mShadowNode && mUpdateShadowNode )
        {
            mShadowNode->_placeBarriersAndEmulateUavExecution( boundUavs, uavsAccess,
                                                               resourcesLayout );
        }

        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        const RenderSystemCapabilities *caps = renderSystem->getCapabilities();
        const bool explicitApi = caps->hasCapability( RSC_EXPLICIT_API );

        //Check <anything> -> Texture (GBuffers)
        {
            TextureGpuVec::const_iterator itor = mPrePassTextures.begin();
            TextureGpuVec::const_iterator end  = mPrePassTextures.end();

            while( itor != end )
            {
                TextureGpu *texture = *itor;
                ResourceLayoutMap::iterator currentLayout = resourcesLayout.find( texture );

                if( (currentLayout->second != ResourceLayout::Texture && explicitApi) ||
                    currentLayout->second == ResourceLayout::Uav )
                {
                    addResourceTransition( currentLayout,
                                           ResourceLayout::Texture,
                                           ReadBarrier::Texture );
                }

                ++itor;
            }
        }

        //Check <anything> -> DepthTexture (Depth Texture)
        if( mPrePassDepthTexture )
        {
            TextureGpu *texture = mPrePassDepthTexture;

            ResourceLayoutMap::iterator currentLayout = resourcesLayout.find( texture );

            if( (currentLayout->second != ResourceLayout::Texture && explicitApi) ||
                currentLayout->second == ResourceLayout::Uav )
            {
                addResourceTransition( currentLayout,
                                       ResourceLayout::Texture,
                                       ReadBarrier::Texture );
            }
        }

        //Check <anything> -> DepthTexture (Depth Texture)
        if( mDepthTextureNoMsaa && mDepthTextureNoMsaa != mPrePassDepthTexture )
        {
            TextureGpu *texture = mDepthTextureNoMsaa;

            ResourceLayoutMap::iterator currentLayout = resourcesLayout.find( texture );

            if( (currentLayout->second != ResourceLayout::Texture && explicitApi) ||
                currentLayout->second == ResourceLayout::Uav )
            {
                addResourceTransition( currentLayout,
                                       ResourceLayout::Texture,
                                       ReadBarrier::Texture );
            }
        }

        //Check <anything> -> Texture (SSR Texture)
        if( mSsrTexture )
        {
            TextureGpu *texture = mSsrTexture;

            ResourceLayoutMap::iterator currentLayout = resourcesLayout.find( texture );

            if( (currentLayout->second != ResourceLayout::Texture && explicitApi) ||
                currentLayout->second == ResourceLayout::Uav )
            {
                addResourceTransition( currentLayout,
                                       ResourceLayout::Texture,
                                       ReadBarrier::Texture );
            }
        }

        CompositorPass::_placeBarriersAndEmulateUavExecution( boundUavs, uavsAccess, resourcesLayout );
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassScene::notifyCleared(void)
    {
        mShadowNode = 0; //Allow changes to our shadow nodes too.
        CompositorPass::notifyCleared();
    }
}
