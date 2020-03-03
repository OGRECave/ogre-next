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

#include "IrradianceField/OgreIrradianceFieldRaster.h"

#include "IrradianceField/OgreIfdProbeVisualizer.h"
#include "IrradianceField/OgreIrradianceField.h"
#include "OgreHlmsPbs.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "OgreDepthBuffer.h"
#include "OgreRoot.h"
#include "OgreCamera.h"

#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"
#include "OgreHlmsManager.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"
#include "OgreTextureGpuManager.h"
#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreTexBufferPacked.h"
#include "Vao/OgreVaoManager.h"

#define TODO_final_memoryBarrier

namespace Ogre
{
    IrradianceFieldRaster::IrradianceFieldRaster( IrradianceField *creator ) :
        mCreator( creator ),
        mCubemap( 0 ),
        mDepthCubemap( 0 ),
        mRenderWorkspace( 0 ),
        mConvertToIfdWorkspace( 0 ),
        mIfdIntegrationWorkspace( 0 ),
        mConvertToIfdJob( 0 ),
        mShaderParamsConvertToIfd( 0 ),
        mProbeIdxParam( 0 ),
        mProjectionABParam( 0 ),
        mNumProbesParam( 0 ),
        mCamera( 0 )
    {
        HlmsCompute *hlmsCompute = mCreator->mRoot->getHlmsManager()->getComputeHlms();
        mConvertToIfdJob = hlmsCompute->findComputeJobNoThrow( "IrradianceField/CubemapToIfd" );

        if( !mConvertToIfdJob )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "To use IrradianceFieldRaster, you must include the resources bundled at "
                         "Samples/Media/Compute\n"
                         "Could not find IrradianceField/CubemapToIfd",
                         "IrradianceFieldRaster::IrradianceFieldRaster" );
        }
    }
    //-------------------------------------------------------------------------
    IrradianceFieldRaster::~IrradianceFieldRaster() { destroyWorkspace(); }
    //-------------------------------------------------------------------------
    void IrradianceFieldRaster::createWorkspace( void )
    {
        destroyWorkspace();

        const RasterParams &rasterParams = mCreator->mSettings.mRasterParams;
        SceneManager *sceneManager = mCreator->mSceneManager;

        TextureGpuManager *textureManager =
            sceneManager->getDestinationRenderSystem()->getTextureGpuManager();

        uint32 cubemapRes = mCreator->mSettings.mDepthProbeResolution * 2u;
        cubemapRes = Math::ClosestPow2( cubemapRes );

        mCubemap = textureManager->createTexture(
            "IrradianceFieldRaster/Temp/" + StringConverter::toString( mCreator->getId() ),
            GpuPageOutStrategy::Discard, TextureFlags::RenderToTexture, TextureTypes::TypeCube );
        mCubemap->setResolution( cubemapRes, cubemapRes );
        mCubemap->setPixelFormat( rasterParams.mPixelFormat );

        mDepthCubemap = textureManager->createTexture(
            "IrradianceFieldRaster/Depth/" + StringConverter::toString( mCreator->getId() ),
            GpuPageOutStrategy::Discard, TextureFlags::RenderToTexture, TextureTypes::TypeCube );
        mDepthCubemap->copyParametersFrom( mCubemap );
        mDepthCubemap->setPixelFormat( PFG_R32_FLOAT );
        mDepthCubemap->_setDepthBufferDefaults( DepthBuffer::POOL_NO_DEPTH, false, PFG_UNKNOWN );

        mCubemap->scheduleTransitionTo( GpuResidency::Resident );
        mDepthCubemap->scheduleTransitionTo( GpuResidency::Resident );

        mCamera = sceneManager->createCamera(
            "IrradianceFieldRaster/" + StringConverter::toString( mCreator->getId() ), true, true );
        mCamera->setFOVy( Degree( 90 ) );
        mCamera->setAspectRatio( 1 );
        mCamera->setFixedYawAxis( false );
        mCamera->setNearClipDistance( rasterParams.mCameraNear );
        mCamera->setFarClipDistance( rasterParams.mCameraFar );

        CompositorManager2 *compositorManager = mCreator->mRoot->getCompositorManager2();
        ResourceLayoutMap initialLayouts;
        ResourceAccessMap initialUavAccess;

        CompositorChannelVec channels;
        channels.reserve( 2u );
        channels.push_back( mCubemap );
        channels.push_back( mDepthCubemap );
        mRenderWorkspace = compositorManager->addWorkspace( sceneManager, channels, mCamera,
                                                            rasterParams.mWorkspaceName, false, -1, 0,
                                                            &initialLayouts, &initialUavAccess );

        channels.push_back( mCreator->mIrradianceTex );
        channels.push_back( mCreator->mDepthVarianceTex );
        mConvertToIfdWorkspace = compositorManager->addWorkspace(
            sceneManager, channels, mCamera, "IrradianceField/CubemapToIfd/Workspace", false, -1, 0,
            &initialLayouts, &initialUavAccess );

        channels.clear();
        channels.push_back( mCreator->mIrradianceTex );
        channels.push_back( mCreator->mDepthVarianceTex );
        mIfdIntegrationWorkspace = compositorManager->addWorkspace(
            sceneManager, channels, mCamera, "IrradianceField/IntegrationOnly/Workspace", false, -1, 0,
            &initialLayouts, &initialUavAccess );

        const IrradianceFieldSettings &settings = mCreator->mSettings;
        mConvertToIfdJob->setProperty( "colour_resolution", settings.mIrradianceResolution );
        mConvertToIfdJob->setProperty( "depth_resolution", settings.mDepthProbeResolution );
        mConvertToIfdJob->setProperty( "colour_full_width",
                                       static_cast<int32>( mCreator->mIrradianceTex->getWidth() ) );
        mConvertToIfdJob->setProperty(
            "depth_to_colour_resolution_ratio",
            static_cast<int32>( settings.mDepthProbeResolution / settings.mIrradianceResolution ) );
        mConvertToIfdJob->setProperty(
            "horiz_samples_per_colour_pixel",
            static_cast<int32>( cubemapRes / settings.mIrradianceResolution ) );
        mConvertToIfdJob->setProperty(
            "horiz_samples_per_depth_pixel",
            static_cast<int32>( cubemapRes / settings.mDepthProbeResolution ) );

        mConvertToIfdJob->setThreadsPerGroup( settings.mIrradianceResolution,
                                              settings.mIrradianceResolution, 1u );
        mConvertToIfdJob->setNumThreadGroups( 1u, 1u, 1u );

        mShaderParamsConvertToIfd = &mConvertToIfdJob->getShaderParams( "default" );
        mProbeIdxParam = mShaderParamsConvertToIfd->findParameter( "probeIdx" );
        mProjectionABParam = mShaderParamsConvertToIfd->findParameter( "projectionParams" );
        mNumProbesParam = mShaderParamsConvertToIfd->findParameter( "numProbes" );
    }
    //-------------------------------------------------------------------------
    void IrradianceFieldRaster::destroyWorkspace( void )
    {
        if( !mRenderWorkspace )
            return;

        CompositorManager2 *compositorManager = mCreator->mRoot->getCompositorManager2();

        compositorManager->removeWorkspace( mConvertToIfdWorkspace );
        mConvertToIfdWorkspace = 0;
        compositorManager->removeWorkspace( mRenderWorkspace );
        mRenderWorkspace = 0;

        SceneManager *sceneManager = mCreator->mSceneManager;
        TextureGpuManager *textureManager =
            sceneManager->getDestinationRenderSystem()->getTextureGpuManager();

        textureManager->destroyTexture( mDepthCubemap );
        mDepthCubemap = 0;
        textureManager->destroyTexture( mCubemap );
        mCubemap = 0;

        sceneManager->destroyCamera( mCamera );
        mCamera = 0;
    }
    //-------------------------------------------------------------------------
    Vector3 IrradianceFieldRaster::getProbeCenter( size_t probeIdx ) const
    {
        const IrradianceFieldSettings &settings = mCreator->mSettings;

        Vector3 pos;
        pos.x = probeIdx % settings.mNumProbes[0];
        pos.y =
            ( probeIdx % ( settings.mNumProbes[0] * settings.mNumProbes[1] ) ) / settings.mNumProbes[0];
        pos.z = probeIdx / ( settings.mNumProbes[0] * settings.mNumProbes[1] );
        pos += 0.5f;

        pos /= settings.getNumProbes3f();
        pos *= mCreator->mFieldSize;
        pos += mCreator->mFieldOrigin;

        return pos;
    }
    //-------------------------------------------------------------------------
    void IrradianceFieldRaster::renderProbes( uint32 probesPerFrame )
    {
        SceneManager *sceneManager = mCreator->mSceneManager;
        RenderSystem *renderSystem = sceneManager->getDestinationRenderSystem();

        OGRE_ASSERT_HIGH(
            dynamic_cast<HlmsPbs *>( mCreator->mRoot->getHlmsManager()->getHlms( HLMS_PBS ) ) );
        HlmsPbs *pbs = static_cast<HlmsPbs *>( mCreator->mRoot->getHlmsManager()->getHlms( HLMS_PBS ) );
        IrradianceField *oldIfd = pbs->getIrradianceField();
        pbs->setIrradianceField( 0 );

        sceneManager->updateSceneGraph();

        const uint32 oldVisibilityMask = sceneManager->getVisibilityMask();
        sceneManager->setVisibilityMask( 0xffffffff );

        // numProbesToProcess has already been sanitized by caller
        const size_t numProbesToProcess = probesPerFrame;
        const size_t numProbesProcessed = mCreator->mNumProbesProcessed;
        const size_t maxProbeToProcess = numProbesProcessed + numProbesToProcess;

        bool oldDebugIfdVisibility = false;
        if( mCreator->mDebugIfdProbeVisualizer )
            oldDebugIfdVisibility = mCreator->mDebugIfdProbeVisualizer->getVisible();

        Ogre::Vector2 projectionAB = mCamera->getProjectionParamsAB();
        mProjectionABParam->setManualValue( projectionAB );
        mNumProbesParam->setManualValue( mCreator->mSettings.getNumProbes3f() );

        for( size_t i = numProbesProcessed; i < maxProbeToProcess; ++i )
        {
            const size_t idx = i - numProbesProcessed;

            Vector3 probeCenter = getProbeCenter( i );

            mCamera->setPosition( probeCenter );

            if( idx > 0u && ( idx % 8u ) == 0u )
                renderSystem->_beginFrameOnce();

            // Render to cubemap (also copies depth buffer into its own cubemap)
            mRenderWorkspace->_update();

            // Convert both colour & depth cubemaps to the 2D octahedral maps
            mProbeIdxParam->setManualValue( static_cast<uint32>( i ) );
            mShaderParamsConvertToIfd->setDirty();
            mConvertToIfdWorkspace->_update();

            if( idx > 0u && ( idx % 8u ) == 0u )
            {
                renderSystem->_update();
                renderSystem->_endFrameOnce();
            }
        }

        mIfdIntegrationWorkspace->_beginUpdate( false );
        mIfdIntegrationWorkspace->_update();
        mIfdIntegrationWorkspace->_endUpdate( false );

        TODO_final_memoryBarrier;

        if( mCreator->mDebugIfdProbeVisualizer )
            mCreator->mDebugIfdProbeVisualizer->setVisible( oldDebugIfdVisibility );

        sceneManager->setVisibilityMask( oldVisibilityMask );

        pbs->setIrradianceField( oldIfd );
    }
}  // namespace Ogre
