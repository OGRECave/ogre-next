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

#include "Compositor/Pass/PassQuad/OgreCompositorPassQuad.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"
#include "OgreCamera.h"
#include "OgreMaterialManager.h"
#include "OgreRectangle2D.h"
#include "OgreSceneManager.h"
#include "OgreTechnique.h"

namespace Ogre
{
    void CompositorPassQuadDef::addQuadTextureSource( size_t texUnitIdx, const String &textureName )
    {
        if( textureName.find( "global_" ) == 0 )
        {
            mParentNodeDef->addTextureSourceName( textureName, 0,
                                                  TextureDefinitionBase::TEXTURE_GLOBAL );
        }
        mTextureSources.push_back( QuadTextureSource( texUnitIdx, textureName ) );
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    CompositorPassQuad::CompositorPassQuad( const CompositorPassQuadDef *definition,
                                            Camera *defaultCamera, CompositorNode *parentNode,
                                            const RenderTargetViewDef *rtv, Real horizonalTexelOffset,
                                            Real verticalTexelOffset ) :
        CompositorPass( definition, parentNode ),
        mDefinition( definition ),
        mFsRect( 0 ),
        mDatablock( 0 ),
        mPass( 0 ),
        mCamera( 0 ),
        mHorizonalTexelOffset( horizonalTexelOffset ),
        mVerticalTexelOffset( verticalTexelOffset )
    {
        initialize( rtv );

        const CompositorWorkspace *workspace = parentNode->getWorkspace();

        if( mDefinition->mUseQuad || mDefinition->mFrustumCorners != CompositorPassQuadDef::NO_CORNERS )
        {
            mFsRect = workspace->getCompositorManager()->getSharedFullscreenQuad();
        }
        else
        {
            mFsRect = workspace->getCompositorManager()->getSharedFullscreenTriangle();
        }

        if( mDefinition->mMaterialIsHlms )
        {
            mFsRect->setDatablock( mDefinition->mMaterialName );
        }
        else
        {
            mMaterial = MaterialManager::getSingleton().getByName( mDefinition->mMaterialName );
            if( !mMaterial )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                             "Cannot find material '" + mDefinition->mMaterialName + "'",
                             "CompositorPassQuad::CompositorPassQuad" );
            }
            mMaterial->load();

            if( !mMaterial->getBestTechnique() )
            {
                OGRE_EXCEPT(
                    Exception::ERR_ITEM_NOT_FOUND,
                    "Cannot find best technique for material '" + mDefinition->mMaterialName + "'",
                    "CompositorPassQuad::CompositorPassQuad" );
            }

            if( !mMaterial->getBestTechnique()->getPass( 0 ) )
            {
                OGRE_EXCEPT(
                    Exception::ERR_ITEM_NOT_FOUND,
                    "Best technique must have a Pass! Material '" + mDefinition->mMaterialName + "'",
                    "CompositorPassQuad::CompositorPassQuad" );
            }

            mPass = mMaterial->getBestTechnique()->getPass( 0 );

            mFsRect->setMaterial( mMaterial );
        }

        mDatablock = mFsRect->getDatablock();

        if( mDefinition->mCameraName != IdString() )
            mCamera = workspace->findCamera( mDefinition->mCameraName );
        else
            mCamera = defaultCamera;

        // List all our RTT dependencies
        const CompositorPassQuadDef::TextureSources &textureSources = mDefinition->getTextureSources();
        CompositorPassQuadDef::TextureSources::const_iterator itor = textureSources.begin();
        CompositorPassQuadDef::TextureSources::const_iterator endt = textureSources.end();
        while( itor != endt )
        {
            TextureGpu *channel = mParentNode->getDefinedTexture( itor->textureName );
            CompositorTextureVec::const_iterator it = mTextureDependencies.begin();
            CompositorTextureVec::const_iterator en = mTextureDependencies.end();
            while( it != en && it->name != itor->textureName )
                ++it;

            if( it == en )
                mTextureDependencies.push_back( CompositorTexture( itor->textureName, channel ) );

            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    CompositorPassQuad::~CompositorPassQuad()
    {
        if( mPass )
        {
            // Reset the names of our material textures, so that material could be reloaded later
            const CompositorPassQuadDef::TextureSources &textureSources =
                mDefinition->getTextureSources();
            CompositorPassQuadDef::TextureSources::const_iterator itor = textureSources.begin();
            CompositorPassQuadDef::TextureSources::const_iterator endt = textureSources.end();
            while( itor != endt )
            {
                if( itor->texUnitIdx < mPass->getNumTextureUnitStates() )
                {
                    TextureUnitState *tu = mPass->getTextureUnitState( itor->texUnitIdx );
                    tu->setTextureName( "" );
                }

                ++itor;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassQuad::execute( const Camera *lodCamera )
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

        if( mPass )
        {
            // Set the material textures every frame (we don't clone the material)
            const CompositorPassQuadDef::TextureSources &textureSources =
                mDefinition->getTextureSources();
            CompositorPassQuadDef::TextureSources::const_iterator itor = textureSources.begin();
            CompositorPassQuadDef::TextureSources::const_iterator endt = textureSources.end();
            while( itor != endt )
            {
                if( itor->texUnitIdx < mPass->getNumTextureUnitStates() )
                {
                    TextureUnitState *tu = mPass->getTextureUnitState( itor->texUnitIdx );
                    tu->setTexture( mParentNode->getDefinedTexture( itor->textureName ) );
                }

                ++itor;
            }

            // Force all non-AutomaticBatching textures to be loaded before calling _renderSingleObject,
            // since low level materials need these textures to be loaded (streaming is opt-in)
            const size_t numTUs = mPass->getNumTextureUnitStates();
            for( size_t i = 0u; i < numTUs; ++i )
            {
                TextureUnitState *tu = mPass->getTextureUnitState( i );
                tu->_getTexturePtr();
            }
        }

        if( mHorizonalTexelOffset != 0 || mVerticalTexelOffset != 0 )
        {
            const Real hOffset = 2.0f * mHorizonalTexelOffset / Real( mAnyTargetTexture->getWidth() );
            const Real vOffset = 2.0f * mVerticalTexelOffset / Real( mAnyTargetTexture->getHeight() );

            // The rectangle is shared, set the corners each time
            mFsRect->setCorners( 0.0f + hOffset, 0.0f - vOffset, 1.0f, 1.0f );
        }

#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        {
            const OrientationMode orientationMode = mAnyTargetTexture->getOrientationMode();
            mCamera->setOrientationMode( orientationMode );
        }
#endif

        const Quaternion oldCameraOrientation( mCamera->getOrientation() );

        if( mDefinition->mCameraCubemapReorient )
        {
            uint32 sliceIdx = std::min<uint32>( mDefinition->getRtIndex(), 5 );
            mCamera->setOrientation( oldCameraOrientation * CubemapRotations[sliceIdx] );
        }

        if( mDefinition->mFrustumCorners >= CompositorPassQuadDef::VIEW_SPACE_CORNERS &&
            mDefinition->mFrustumCorners <= CompositorPassQuadDef::VIEW_SPACE_CORNERS_NORMALIZED_LH )
        {
            const Ogre::Matrix4 &viewMat = mCamera->getViewMatrix( true );
            const Vector3 *corners = mCamera->getWorldSpaceCorners();

            Vector3 finalCorners[4] = { viewMat * corners[5], viewMat * corners[6], viewMat * corners[4],
                                        viewMat * corners[7] };

            if( mDefinition->mFrustumCorners >= CompositorPassQuadDef::VIEW_SPACE_CORNERS_NORMALIZED )
            {
                const Real farPlane = mCamera->getFarClipDistance();
                for( int i = 0; i < 4; ++i )
                {
                    // finalCorners[i].z should always be == 1; (ignoring precision errors)
                    finalCorners[i] /= farPlane;
                    if( mDefinition->mFrustumCorners ==
                        CompositorPassQuadDef::VIEW_SPACE_CORNERS_NORMALIZED_LH )
                    {
                        // Make it left handed: Flip Z.
                        finalCorners[i].z = -finalCorners[i].z;
                    }
                }
            }

            mFsRect->setNormals( finalCorners[0], finalCorners[1], finalCorners[2], finalCorners[3] );
        }
        else if( mDefinition->mFrustumCorners == CompositorPassQuadDef::WORLD_SPACE_CORNERS )
        {
            const Vector3 *corners = mCamera->getWorldSpaceCorners();
            mFsRect->setNormals( corners[5], corners[6], corners[4], corners[7] );
        }
        else if( mDefinition->mFrustumCorners == CompositorPassQuadDef::WORLD_SPACE_CORNERS_CENTERED ||
                 mDefinition->mFrustumCorners == CompositorPassQuadDef::CAMERA_DIRECTION )
        {
            const Vector3 *corners = mCamera->getWorldSpaceCorners();
            const Vector3 &cameraPos = mCamera->getDerivedPosition();

            Vector3 cameraDirs[4];
            cameraDirs[0] = corners[5] - cameraPos;
            cameraDirs[1] = corners[6] - cameraPos;
            cameraDirs[2] = corners[4] - cameraPos;
            cameraDirs[3] = corners[7] - cameraPos;

            if( mDefinition->mFrustumCorners == CompositorPassQuadDef::CAMERA_DIRECTION )
            {
                Real invFarPlane = 1.0f / mCamera->getFarClipDistance();
                cameraDirs[0] *= invFarPlane;
                cameraDirs[1] *= invFarPlane;
                cameraDirs[2] *= invFarPlane;
                cameraDirs[3] *= invFarPlane;
            }

            mFsRect->setNormals( cameraDirs[0], cameraDirs[1], cameraDirs[2], cameraDirs[3] );
        }

        analyzeBarriers();
        executeResourceTransitions();

        setRenderPassDescToCurrent();

        // Fire the listener in case it wants to change anything
        notifyPassPreExecuteListeners();

#if TODO_OGRE_2_2
        mTarget->setFsaaResolveDirty();
#endif

        SceneManager *sceneManager = mCamera->getSceneManager();
        sceneManager->_setCamerasInProgress( CamerasInProgress( mCamera ) );
        sceneManager->_setCurrentCompositorPass( this );

        // sceneManager->_injectRenderWithPass( mPass, mFsRect, mCamera, false, false );
        if( mMaterial )
            mFsRect->setMaterial( mMaterial );  // Low level material
        else
            mFsRect->setDatablock( mDatablock );  // Hlms material

        RenderSystem *renderSystem = sceneManager->getDestinationRenderSystem();
        renderSystem->executeRenderPassDescriptorDelayedActions();

        sceneManager->_renderSingleObject( mFsRect, mFsRect, false, false );

        if( mDefinition->mCameraCubemapReorient )
        {
            // Restore orientation
            mCamera->setOrientation( oldCameraOrientation );
        }

        sceneManager->_setCurrentCompositorPass( 0 );

        notifyPassPosExecuteListeners();

        profilingEnd();
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassQuad::analyzeBarriers( const bool bClearBarriers )
    {
        CompositorPass::analyzeBarriers( bClearBarriers );

        if( mDefinition->mAnalyzeAllTextureLayouts && mMaterial )
        {
            const size_t numTexUnits = mPass->getNumTextureUnitStates();
            for( size_t i = 0u; i < numTexUnits; ++i )
            {
                TextureUnitState *tuState = mPass->getTextureUnitState( i );
                TextureGpu *texture = tuState->_getTexturePtr();

                if( texture->isRenderToTexture() || texture->isUav() )
                {
                    resolveTransition( texture, ResourceLayout::Texture, ResourceAccess::Read,
                                       c_allGraphicStagesMask );
                }
            }
        }
    }
}  // namespace Ogre
