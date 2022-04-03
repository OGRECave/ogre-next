/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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

#include "Vct/OgreVctVoxelizerSourceBase.h"

#include "OgreSceneManager.h"
#include "OgreTextureGpuManager.h"
#include "Vao/OgreVaoManager.h"
#include "Vct/OgreVoxelVisualizer.h"

namespace Ogre
{
    //-------------------------------------------------------------------------
    VctVoxelizerSourceBase::VctVoxelizerSourceBase( IdType id, RenderSystem *renderSystem,
                                                    HlmsManager *hlmsManager ) :
        IdObject( id ),
        mAlbedoVox( 0 ),
        mEmissiveVox( 0 ),
        mNormalVox( 0 ),
        mAccumValVox( 0 ),
        mRenderSystem( renderSystem ),
        mVaoManager( renderSystem->getVaoManager() ),
        mHlmsManager( hlmsManager ),
        mTextureGpuManager( renderSystem->getTextureGpuManager() ),
        mDebugVisualizationMode( DebugVisualizationNone ),
        mDebugVoxelVisualizer( 0 ),
        mWidth( 128u ),
        mHeight( 128u ),
        mDepth( 128u ),
        mRegionToVoxelize( Aabb::BOX_ZERO )
    {
    }
    //-------------------------------------------------------------------------
    VctVoxelizerSourceBase::~VctVoxelizerSourceBase()
    {
        setDebugVisualization( DebugVisualizationNone, 0 );
        destroyVoxelTextures();
    }
    //-------------------------------------------------------------------------
    void VctVoxelizerSourceBase::setTextureToDebugVisualizer()
    {
        TextureGpu *trackedTex = mAlbedoVox;
        switch( mDebugVisualizationMode )
        {
        default:
        case DebugVisualizationAlbedo:
            trackedTex = mAlbedoVox;
            break;
        case DebugVisualizationNormal:
            trackedTex = mNormalVox;
            break;
        case DebugVisualizationEmissive:
            trackedTex = mEmissiveVox;
            break;
        }

        BarrierSolver &solver = mRenderSystem->getBarrierSolver();
        ResourceTransitionArray resourceTransitions;
        solver.resolveTransition( resourceTransitions, mAlbedoVox, ResourceLayout::Texture,
                                  ResourceAccess::Read, 1u << VertexShader );
        if( mAlbedoVox != trackedTex )
        {
            solver.resolveTransition( resourceTransitions, trackedTex, ResourceLayout::Texture,
                                      ResourceAccess::Read, 1u << VertexShader );
        }
        mRenderSystem->executeResourceTransition( resourceTransitions );

        mDebugVoxelVisualizer->setTrackingVoxel( mAlbedoVox, trackedTex,
                                                 mDebugVisualizationMode == DebugVisualizationEmissive );
    }
    //-------------------------------------------------------------------------
    void VctVoxelizerSourceBase::destroyVoxelTextures()
    {
        if( mAlbedoVox )
        {
            mTextureGpuManager->destroyTexture( mAlbedoVox );
            mTextureGpuManager->destroyTexture( mEmissiveVox );
            mTextureGpuManager->destroyTexture( mNormalVox );
            mTextureGpuManager->destroyTexture( mAccumValVox );

            mAlbedoVox = 0;
            mEmissiveVox = 0;
            mNormalVox = 0;
            mAccumValVox = 0;

            if( mDebugVoxelVisualizer )
                mDebugVoxelVisualizer->setVisible( false );
        }
    }
    //-------------------------------------------------------------------------
    void VctVoxelizerSourceBase::setDebugVisualization(
        VctVoxelizerSourceBase::DebugVisualizationMode mode, SceneManager *sceneManager )
    {
        if( mDebugVoxelVisualizer )
        {
            SceneNode *sceneNode = mDebugVoxelVisualizer->getParentSceneNode();
            sceneNode->getParentSceneNode()->removeAndDestroyChild( sceneNode );
            OGRE_DELETE mDebugVoxelVisualizer;
            mDebugVoxelVisualizer = 0;
        }

        mDebugVisualizationMode = mode;

        if( mode != DebugVisualizationNone )
        {
            SceneNode *rootNode = sceneManager->getRootSceneNode( SCENE_STATIC );
            SceneNode *visNode = rootNode->createChildSceneNode( SCENE_STATIC );

            mDebugVoxelVisualizer = OGRE_NEW VoxelVisualizer(
                Ogre::Id::generateNewId<Ogre::MovableObject>(),
                &sceneManager->_getEntityMemoryManager( SCENE_STATIC ), sceneManager, 0u );

            setTextureToDebugVisualizer();

            visNode->setPosition( getVoxelOrigin() );
            visNode->setScale( getVoxelCellSize() );
            visNode->attachObject( mDebugVoxelVisualizer );
        }
    }
    //-------------------------------------------------------------------------
    VctVoxelizerSourceBase::DebugVisualizationMode VctVoxelizerSourceBase::getDebugVisualizationMode(
        void ) const
    {
        return mDebugVisualizationMode;
    }
    //-------------------------------------------------------------------------
    Vector3 VctVoxelizerSourceBase::getVoxelOrigin() const { return mRegionToVoxelize.getMinimum(); }
    //-------------------------------------------------------------------------
    Vector3 VctVoxelizerSourceBase::getVoxelCellSize() const
    {
        return mRegionToVoxelize.getSize() / getVoxelResolution();
    }
    //-------------------------------------------------------------------------
    Vector3 VctVoxelizerSourceBase::getVoxelSize() const { return mRegionToVoxelize.getSize(); }
    //-------------------------------------------------------------------------
    Vector3 VctVoxelizerSourceBase::getVoxelResolution() const
    {
        return Vector3( (Real)mWidth, (Real)mHeight, (Real)mDepth );
    }
    //-------------------------------------------------------------------------
    TextureGpuManager *VctVoxelizerSourceBase::getTextureGpuManager() { return mTextureGpuManager; }
    //-------------------------------------------------------------------------
    RenderSystem *VctVoxelizerSourceBase::getRenderSystem() { return mRenderSystem; }
    //-------------------------------------------------------------------------
    HlmsManager *VctVoxelizerSourceBase::getHlmsManager() { return mHlmsManager; }
}  // namespace Ogre
