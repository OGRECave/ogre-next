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

#include "Cubemaps/OgreCubemapProbe.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compute/OgreComputeTools.h"
#include "Cubemaps/OgreParallaxCorrectedCubemap.h"
#include "Cubemaps/OgreParallaxCorrectedCubemapBase.h"
#include "OgreCamera.h"
#include "OgreHlmsManager.h"
#include "OgreId.h"
#include "OgreInternalCubemapProbe.h"
#include "OgreLogManager.h"
#include "OgreLwString.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreSceneManager.h"
#include "OgreTextureGpuManager.h"
#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreVaoManager.h"

namespace Ogre
{
    CubemapProbe::CubemapProbe( ParallaxCorrectedCubemapBase *creator ) :
        mProbeCameraPos( Vector3::ZERO ),
        mArea( Aabb::BOX_ZERO ),
        mAreaInnerRegion( Vector3::ZERO ),
        mOrientation( Matrix3::IDENTITY ),
        mInvOrientation( Matrix3::IDENTITY ),
        mProbeShape( Aabb::BOX_ZERO ),
        mTexture( 0 ),
        mCubemapArrayIdx( std::numeric_limits<uint16>::max() ),
        mClearWorkspace( 0 ),
        mWorkspace( 0 ),
        mCamera( 0 ),
        mCreator( creator ),
        mInternalProbe( 0 ),
        mConstBufferForManualProbes( 0 ),
        mNumDatablockUsers( 0 ),
        mPriority( 10u ),
        mStatic( true ),
        mCullLights( true ),
        mEnabled( true ),
        mDirty( true ),
        mNumIterations( 8 ),
        mMask( 0xffffffff )
    {
    }
    //-----------------------------------------------------------------------------------
    CubemapProbe::~CubemapProbe()
    {
        destroyWorkspace();
        destroyTexture();

        assert( !mNumDatablockUsers &&
                "There's still datablocks using this probe! Pointers will become dangling!" );
        _releaseManualHardwareResources();
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::_releaseManualHardwareResources()
    {
        if( mConstBufferForManualProbes )
        {
            SceneManager *sceneManager = mCreator->getSceneManager();
            VaoManager *vaoManager = sceneManager->getDestinationRenderSystem()->getVaoManager();
            vaoManager->destroyConstBuffer( mConstBufferForManualProbes );
            mConstBufferForManualProbes = 0;
            mCreator->_removeManuallyActiveProbe( this );
        }
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::_restoreManualHardwareResources()
    {
        if( mNumDatablockUsers && !mConstBufferForManualProbes )
        {
            OGRE_ASSERT_LOW( !mCreator->getAutomaticMode() );

            SceneManager *sceneManager = mCreator->getSceneManager();
            VaoManager *vaoManager = sceneManager->getDestinationRenderSystem()->getVaoManager();
            mConstBufferForManualProbes = vaoManager->createConstBuffer(
                ParallaxCorrectedCubemap::getConstBufferSizeStatic(), BT_DEFAULT, 0, false );
            mCreator->_addManuallyActiveProbe( this );
        }
        mDirty = true;
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::destroyWorkspace()
    {
        if( mWorkspace )
        {
            if( !mCreator->getAutomaticMode() )
            {
                // A Workspace without a valid texture is never valid. If it happens it
                // could mean the workspace is internally referencing a dangling pointer.
                OGRE_ASSERT_LOW( mTexture );

                const bool useManual = mTexture->getNumMipmaps() > 1u;
                if( useManual )
                {
                    TextureGpu *channel = mWorkspace->getExternalRenderTargets()[0];
                    mCreator->releaseTmpRtt( channel );
                }
            }

            if( mCreator->getAutomaticMode() )
            {
                TextureGpu *channel = mWorkspace->getExternalRenderTargets()[0];
                mCreator->releaseTmpRtt( channel );
            }

            CompositorManager2 *compositorManager = mWorkspace->getCompositorManager();
            compositorManager->removeWorkspace( mWorkspace );
            mWorkspace = 0;
        }

        if( mClearWorkspace )
        {
            CompositorManager2 *compositorManager = mClearWorkspace->getCompositorManager();
            compositorManager->removeWorkspace( mClearWorkspace );
            mClearWorkspace = 0;
        }

        if( mTexture && mTexture->getResidencyStatus() != GpuResidency::OnStorage &&
            !mCreator->getAutomaticMode() )
        {
            mTexture->_transitionTo( GpuResidency::OnStorage, (uint8 *)0 );
        }

        if( mCamera )
        {
            SceneManager *sceneManager = mCamera->getSceneManager();
            sceneManager->destroyCamera( mCamera );
            mCamera = 0;
        }

        releaseTextureAuto();
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::destroyTexture()
    {
        assert( !mWorkspace );
        if( mTexture )
        {
            if( !mCreator->getAutomaticMode() )
            {
                SceneManager *sceneManager = mCreator->getSceneManager();
                TextureGpuManager *textureManager =
                    sceneManager->getDestinationRenderSystem()->getTextureGpuManager();
                textureManager->destroyTexture( mTexture );
            }
            mTexture = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::acquireTextureAuto()
    {
        if( !mCreator->getAutomaticMode() )
            return;

        releaseTextureAuto();
        mTexture = mCreator->_acquireTextureSlot( mCubemapArrayIdx );

        if( mTexture )
        {
            createInternalProbe();
        }
        else
        {
            LogManager::getSingleton().logMessage(
                "Warning: CubemapProbe::acquireTextureAuto failed. "
                "You ran out of slots in the cubemap array. "
                "Disabling this probe" );
        }
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::releaseTextureAuto()
    {
        if( !mCreator->getAutomaticMode() )
            return;

        if( mTexture )
        {
            destroyInternalProbe();
            mCreator->_releaseTextureSlot( mTexture, mCubemapArrayIdx );
            mTexture = 0;
            mCubemapArrayIdx = std::numeric_limits<uint16>::max();
        }
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::createInternalProbe()
    {
        destroyInternalProbe();
        if( !mCreator->getAutomaticMode() )
            return;

        SceneManager *sceneManager = mCreator->getSceneManager();
        const SceneMemoryMgrTypes sceneType = mStatic ? SCENE_STATIC : SCENE_DYNAMIC;
        mInternalProbe = sceneManager->_createCubemapProbe( sceneType );

        SceneNode *sceneNode =
            sceneManager->getRootSceneNode( sceneType )->createChildSceneNode( sceneType );
        sceneNode->attachObject( mInternalProbe );
        sceneNode->setIndestructibleByClearScene( true );

        syncInternalProbe();
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::destroyInternalProbe()
    {
        if( !mInternalProbe )
            return;

        SceneNode *sceneNode = mInternalProbe->getParentSceneNode();
        sceneNode->getParentSceneNode()->removeAndDestroyChild( sceneNode );
        mCreator->getSceneManager()->_destroyCubemapProbe( mInternalProbe );
        mInternalProbe = 0;
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::switchInternalProbeStaticValue()
    {
        if( mInternalProbe && mInternalProbe->isStatic() != mStatic )
        {
            SceneNode *sceneNode = mInternalProbe->getParentSceneNode();
            sceneNode->getParent()->removeChild( sceneNode );

            sceneNode->setStatic( mStatic );

            SceneManager *sceneManager = mCreator->getSceneManager();
            SceneNode *rootNode =
                sceneManager->getRootSceneNode( mStatic ? SCENE_STATIC : SCENE_DYNAMIC );
            rootNode->addChild( sceneNode );
        }
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::syncInternalProbe()
    {
        if( !mInternalProbe )
            return;

        Vector3 restrictedAabbMin( mArea.getMinimum() );
        Vector3 restrictedAabbMax( mArea.getMaximum() );
        restrictedAabbMin.makeCeil( mProbeShape.getMinimum() );
        restrictedAabbMax.makeFloor( mProbeShape.getMaximum() );

        Quaternion qRot( mOrientation );
        SceneNode *sceneNode = mInternalProbe->getParentSceneNode();
        sceneNode->setPosition( ( restrictedAabbMin + restrictedAabbMax ) * 0.5f );
        sceneNode->setScale( restrictedAabbMax - restrictedAabbMin );
        sceneNode->setOrientation( qRot );

        mCreator->fillConstBufferData( *this, Matrix4::IDENTITY, Matrix3::IDENTITY,
                                       reinterpret_cast<float *>( mInternalProbe->mGpuData ) );

        uint32 finalValue = uint32( mPriority << 16u ) | mCubemapArrayIdx;
        memcpy( &mInternalProbe->mGpuData[3][3], &finalValue, sizeof( finalValue ) );

        Vector3 probeToAreaCenterOffsetLS = mInvOrientation * ( mArea.mCenter - mProbeShape.mCenter );
        mInternalProbe->mGpuData[4][3] = probeToAreaCenterOffsetLS.x;
        mInternalProbe->mGpuData[5][3] = probeToAreaCenterOffsetLS.y;
        mInternalProbe->mGpuData[6][3] = probeToAreaCenterOffsetLS.z;

        mInternalProbe->mGpuData[6][0] = mArea.mHalfSize.x * mAreaInnerRegion.x;
        mInternalProbe->mGpuData[6][1] = mArea.mHalfSize.y * mAreaInnerRegion.y;
        mInternalProbe->mGpuData[6][2] = mArea.mHalfSize.z * mAreaInnerRegion.z;

        mInternalProbe->mGpuData[7][0] = mArea.mHalfSize.x;
        mInternalProbe->mGpuData[7][1] = mArea.mHalfSize.y;
        mInternalProbe->mGpuData[7][2] = mArea.mHalfSize.z;

        if( mStatic )
        {
            SceneManager *sceneManager = mCreator->getSceneManager();
            sceneManager->notifyStaticDirty( sceneNode );
        }
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::restoreFromClearScene( SceneNode *rootNode )
    {
        if( mCamera )
            rootNode->attachObject( mCamera );

        if( mInternalProbe )
        {
            SceneManager *sceneManager = mCreator->getSceneManager();
            const SceneMemoryMgrTypes sceneType = mStatic ? SCENE_STATIC : SCENE_DYNAMIC;
            sceneManager->getRootSceneNode( sceneType )->addChild( mInternalProbe->getParentNode() );
        }
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::setTextureParams( uint32 width, uint32 height, bool useManual, PixelFormatGpu pf,
                                         bool isStatic, SampleDescription sampleDesc )
    {
        if( !mCreator->getAutomaticMode() )
        {
            float cameraNear = 0.5;
            float cameraFar = 1000;

            if( mCamera )
            {
                cameraNear = mCamera->getNearClipDistance();
                cameraFar = mCamera->getFarClipDistance();
            }

            const bool reinitWorkspace = isInitialized();
            destroyWorkspace();
            destroyTexture();

            char tmpBuffer[64];
            LwString texName( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
            texName.a( "CubemapProbe_", Id::generateNewId<CubemapProbe>() );

            uint32 flags = TextureFlags::RenderToTexture;
            uint8 numMips = 1u;

            if( useManual )
            {
                numMips = mCreator->getIblNumMipmaps( width, height );
                flags = mCreator->getIblTargetTextureFlags( pf );
            }

            mSampleDescription = sampleDesc;
            sampleDesc = isStatic ? SampleDescription() : sampleDesc;

            SceneManager *sceneManager = mCreator->getSceneManager();
            TextureGpuManager *textureManager =
                sceneManager->getDestinationRenderSystem()->getTextureGpuManager();
            mTexture = textureManager->createTexture( texName.c_str(), GpuPageOutStrategy::Discard,
                                                      flags, TextureTypes::TypeCube );
            mTexture->setResolution( width, height );
            mTexture->setPixelFormat( pf );
            mTexture->setNumMipmaps( numMips );
            mTexture->setSampleDescription( sampleDesc );

            mStatic = isStatic;
            mDirty = true;

            if( reinitWorkspace && !mCreator->getAutomaticMode() )
                initWorkspace( cameraNear, cameraFar, mWorkspaceDefName );
        }
        else
        {
            mStatic = isStatic;
            mDirty = true;

            switchInternalProbeStaticValue();
        }
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::initWorkspace( float cameraNear, float cameraFar, IdString workspaceDefOverride,
                                      const CompositorChannelVec &additionalChannels,
                                      uint8 executionMask )
    {
        assert( ( mTexture != 0 || mCreator->getAutomaticMode() ) && "Call setTextureParams first!" );

        destroyWorkspace();
        acquireTextureAuto();

        if( !mTexture )
            return;  // acquireTextureAuto failed. There are no available slots

        CompositorWorkspaceDef const *workspaceDef = mCreator->getDefaultWorkspaceDef();
        CompositorManager2 *compositorManager = workspaceDef->getCompositorManager();

        if( workspaceDefOverride != IdString() )
            workspaceDef = compositorManager->getWorkspaceDefinition( workspaceDefOverride );

        mWorkspaceDefName = workspaceDef->getName();
        SceneManager *sceneManager = mCreator->getSceneManager();
        mCamera = sceneManager->createCamera(
            mTexture->getNameStr() + StringConverter::toString( mCubemapArrayIdx ), true, true );
        mCamera->setFOVy( Degree( 90 ) );
        mCamera->setAspectRatio( 1 );
        mCamera->setFixedYawAxis( false );
        mCamera->setNearClipDistance( cameraNear );
        mCamera->setFarClipDistance( cameraFar );

        TextureGpu *rtt = mTexture;
        TextureGpu *ibl = mTexture;

        if( mCreator->getAutomaticMode() )
        {
            rtt = mCreator->findTmpRtt( mTexture );
            ibl = mCreator->findIbl( mTexture );
        }
        else
        {
            const bool useManual = mTexture->getNumMipmaps() > 1u;
            if( useManual )
                rtt = mCreator->findTmpRtt( mTexture );
        }

        if( mStatic )
        {
            // Set camera to skip light culling (efficiency)
            mCamera->setLightCullingVisibility( false, false );
        }
        else
        {
            mCamera->setLightCullingVisibility( true, true );
        }

        if( !mCreator->getAutomaticMode() )
            mTexture->_transitionTo( GpuResidency::Resident, (uint8 *)0 );

        CompositorChannelVec channels;
        channels.reserve( 2u + additionalChannels.size() );
        channels.push_back( rtt );
        channels.push_back( ibl );
        channels.insert( channels.end(), additionalChannels.begin(), additionalChannels.end() );
        mWorkspace = compositorManager->addWorkspace(
            sceneManager, channels, mCamera, mWorkspaceDefName, false, -1, (UavBufferPackedVec *)0,
            (ResourceStatusMap *)0, Vector4::ZERO, 0x00, executionMask );
        mWorkspace->addListener( mCreator );

        if( !mStatic && !mCreator->getAutomaticMode() && mTexture->isRenderToTexture() )
        {
            mClearWorkspace = compositorManager->addWorkspace(
                sceneManager, mTexture, mCamera, "AutoGen_ParallaxCorrectedCubemapClear_Workspace",
                false );
        }
    }
    //-----------------------------------------------------------------------------------
    bool CubemapProbe::isInitialized() const { return mWorkspace != 0; }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::set( const Vector3 &cameraPos, const Aabb &area, const Vector3 &areaInnerRegion,
                            const Matrix3 &orientation, const Aabb &probeShape )
    {
        mProbeCameraPos = cameraPos;
        mArea = area;
        mAreaInnerRegion = areaInnerRegion;
        mOrientation = orientation;
        mInvOrientation = mOrientation.Inverse();
        mProbeShape = probeShape;

        // Add some padding.
        Real padding = 1.005f;
        mArea.mHalfSize *= padding;
        mProbeShape.mHalfSize *= padding;

        mAreaInnerRegion.makeCeil( Vector3::ZERO );
        mAreaInnerRegion.makeFloor( Vector3::UNIT_SCALE );

        Aabb areaLocalToShape = mArea;
        areaLocalToShape.mCenter -= mProbeShape.mCenter;
        areaLocalToShape.mCenter = mInvOrientation * areaLocalToShape.mCenter;
        areaLocalToShape.mCenter += mProbeShape.mCenter;

        if( ( !mProbeShape.contains( mArea ) && !mCreator->getAutomaticMode() ) ||
            ( !mProbeShape.intersects( mArea ) && mCreator->getAutomaticMode() ) )
        {
            if( !mCreator->getAutomaticMode() )
            {
                LogManager::getSingleton().logMessage(
                    "WARNING: Area must be fully inside probe's shape otherwise "
                    "artifacts appear. Forcing area to be inside probe" );
            }
            else
            {
                LogManager::getSingleton().logMessage(
                    "WARNING: Area must intersect with the probe's shape otherwise "
                    "PCC will not have any effect. Forcing intersection" );
            }
            Vector3 vMin = mArea.getMinimum() * 0.98f;
            Vector3 vMax = mArea.getMaximum() * 0.98f;

            vMin.makeCeil( mProbeShape.getMinimum() );
            vMax.makeFloor( mProbeShape.getMaximum() );
            mArea.setExtents( vMin, vMax );
        }

        syncInternalProbe();

        mDirty = true;
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::setStatic( bool isStatic )
    {
        if( mStatic != isStatic && mTexture )
        {
            setTextureParams( mTexture->getWidth(), mTexture->getHeight(), mTexture->getNumMipmaps() > 0,
                              mTexture->getPixelFormat(), isStatic, mTexture->getSampleDescription() );
        }
        else
        {
            // We're not initialized yet, but still save the intention...
            mStatic = isStatic;
        }
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::setPriority( uint16 priority )
    {
        OGRE_ASSERT_LOW( mCreator->getAutomaticMode() );
        priority = std::max<uint16>( priority, 1u );
        if( mPriority != priority )
        {
            mPriority = priority;
            syncInternalProbe();
        }
    }
    //-----------------------------------------------------------------------------------
    uint16_t CubemapProbe::getPriority() const { return mPriority; }
    //-----------------------------------------------------------------------------------
    Real CubemapProbe::getNDF( const Vector3 &posLS ) const
    {
        // Work in the upper left corner of the box. (Like Aabb::distance)
        Vector3 dist;
        dist.x = Math::Abs( posLS.x );
        dist.y = Math::Abs( posLS.y );
        dist.z = Math::Abs( posLS.z );

        const Vector3 innerRange = mArea.mHalfSize * mAreaInnerRegion;
        const Vector3 outerRange = mArea.mHalfSize;

        // 1e-6f avoids division by zero.
        Vector3 ndf = ( dist - innerRange ) / ( outerRange - innerRange + Real( 1e-6f ) );

        return std::max( std::max( ndf.x, ndf.y ), ndf.z );
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::_prepareForRendering()
    {
        if( mCamera )
        {
            mCamera->setPosition( mProbeCameraPos );
            mCamera->setOrientation( Quaternion( mOrientation ) );
            if( mStatic && mCullLights )
                mCamera->setLightCullingVisibility( true, true );
        }
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::_clearCubemap( HlmsManager *hlmsManager )
    {
        if( !mClearWorkspace && mWorkspace )
        {
            if( mTexture->isRenderToTexture() )
            {
                CompositorWorkspaceDef const *workspaceDef = mCreator->getDefaultWorkspaceDef();
                CompositorManager2 *compositorManager = workspaceDef->getCompositorManager();

                SceneManager *sceneManager = mCreator->getSceneManager();
                mClearWorkspace = compositorManager->addWorkspace(
                    sceneManager, mTexture, mCamera, "AutoGen_ParallaxCorrectedCubemapClear_Workspace",
                    false );
            }
            else
            {
                ComputeTools computeTools( hlmsManager->getComputeHlms() );
                const float clearValue[4] = {
                    0.0f,
                    0.0f,
                    0.0f,
                    0.0f,
                };
                computeTools.clearUavFloat( mTexture, clearValue );
            }
        }

        if( !mClearWorkspace )
            return;

        mClearWorkspace->_update();

        if( mStatic )
        {
            CompositorManager2 *compositorManager = mClearWorkspace->getCompositorManager();
            compositorManager->removeWorkspace( mClearWorkspace );
            mClearWorkspace = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::_updateRender()
    {
        assert( mDirty || !mStatic );

        const bool automaticMode = mCreator->getAutomaticMode();

        if( automaticMode )
            mCreator->_setIsRendering( true );

        mCreator->_setProbeRenderInProgress( this );
        mWorkspace->_update();
        mCreator->_setProbeRenderInProgress( 0 );

        if( automaticMode )
            mCreator->_setIsRendering( false );

        if( mStatic )
            mCamera->setLightCullingVisibility( false, false );

        mCreator->_copyRenderTargetToCubemap( mCubemapArrayIdx );
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::_addReference()
    {
        OGRE_ASSERT_LOW( !mCreator->getAutomaticMode() );

        ++mNumDatablockUsers;

        if( !mConstBufferForManualProbes )
        {
            SceneManager *sceneManager = mCreator->getSceneManager();
            VaoManager *vaoManager = sceneManager->getDestinationRenderSystem()->getVaoManager();
            mConstBufferForManualProbes = vaoManager->createConstBuffer(
                ParallaxCorrectedCubemap::getConstBufferSizeStatic(), BT_DEFAULT, 0, false );
            mCreator->_addManuallyActiveProbe( this );
        }
    }
    //-----------------------------------------------------------------------------------
    void CubemapProbe::_removeReference()
    {
        OGRE_ASSERT_LOW( !mCreator->getAutomaticMode() );

        --mNumDatablockUsers;
        if( !mNumDatablockUsers )
        {
            assert( mConstBufferForManualProbes );
            if( mConstBufferForManualProbes )
            {
                SceneManager *sceneManager = mCreator->getSceneManager();
                VaoManager *vaoManager = sceneManager->getDestinationRenderSystem()->getVaoManager();
                vaoManager->destroyConstBuffer( mConstBufferForManualProbes );
                mConstBufferForManualProbes = 0;
                mCreator->_removeManuallyActiveProbe( this );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    const SceneNode *CubemapProbe::getInternalCubemapProbeSceneNode() const
    {
        SceneNode const *retVal = 0;
        if( mInternalProbe )
            retVal = mInternalProbe->getParentSceneNode();
        return retVal;
    }
}  // namespace Ogre
