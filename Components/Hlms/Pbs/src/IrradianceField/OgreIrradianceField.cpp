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

#include "IrradianceField/OgreIrradianceField.h"
#include "Vct/OgreVctLighting.h"
#include "Vct/OgreVctVoxelizer.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "OgreRoot.h"

#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"
#include "OgreHlmsManager.h"
#include "OgreStringConverter.h"
#include "OgreTextureGpuManager.h"
#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreTexBufferPacked.h"
#include "Vao/OgreVaoManager.h"

#define TODO_handle_leftover

namespace Ogre
{
    IrradianceFieldSettings::IrradianceFieldSettings() :
        mNumRaysPerPixel( 1u ),
        mDepthProbeResolution( 16u ),
        mIrradianceResolution( 8u )
    {
        for( size_t i = 0u; i < 3u; ++i )
            mNumProbes[i] = 32u;
        mNumProbes[1] = 8u;
    }
    //-------------------------------------------------------------------------
    void IrradianceFieldSettings::createSubsamples( void )
    {
        const size_t numRaysPerPixel = mNumRaysPerPixel;
        mSubsamples.resize( numRaysPerPixel );

        if( numRaysPerPixel == 1u )
            mSubsamples[0] = Vector2( 0.5f, 0.5f );
        else if( numRaysPerPixel == 2u )
        {
            mSubsamples[0] = Vector2( 0.75f, 0.75f );
            mSubsamples[1] = Vector2( 0.25f, 0.25f );
        }
        else if( numRaysPerPixel == 3u )
        {
            mSubsamples[0] = Vector2( 0.50f, 0.75f );
            mSubsamples[1] = Vector2( 0.25f, 0.25f );
            mSubsamples[2] = Vector2( 0.75f, 0.25f );
        }
        else
        {
            const float fGridSize = ceilf( sqrtf( numRaysPerPixel ) );
            const float invGridSize = 1.0f / fGridSize;
            const size_t gridSize = static_cast<size_t>( fGridSize );
            const size_t numGridCells = gridSize * gridSize;

            for( size_t i = 0u; i < numGridCells && i < numRaysPerPixel; ++i )
            {
                mSubsamples[i].x = ( i % ( gridSize ) ) * invGridSize;
                mSubsamples[i].y = ( i / ( gridSize ) ) * invGridSize;
            }
        }
    }
    //-------------------------------------------------------------------------
    uint32 IrradianceFieldSettings::getTotalNumProbes( void ) const
    {
        return mNumProbes[0] * mNumProbes[1] * mNumProbes[2];
    }
    //-------------------------------------------------------------------------
    uint32 IrradianceFieldSettings::getDepthProbeFullResolution( void ) const
    {
        const uint32 totalNumProbes = getTotalNumProbes();
        const uint32 sqrtNumProbes = static_cast<uint32>( sqrt( totalNumProbes ) );
        return sqrtNumProbes * mDepthProbeResolution;
    }
    //-------------------------------------------------------------------------
    uint32 IrradianceFieldSettings::getIrradProbeFullResolution( void ) const
    {
        const uint32 totalNumProbes = getTotalNumProbes();
        const uint32 sqrtNumProbes = static_cast<uint32>( sqrt( totalNumProbes ) );
        return sqrtNumProbes * mIrradianceResolution;
    }
    //-------------------------------------------------------------------------
    Vector3 IrradianceFieldSettings::getNumProbes3f( void ) const
    {
        return Vector3( static_cast<Real>( mNumProbes[0] ), static_cast<Real>( mNumProbes[1] ),
                        static_cast<Real>( mNumProbes[2] ) );
    }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    IrradianceField::IrradianceField( Root *root, SceneManager *sceneManager ) :
        IdObject( Id::generateNewId<IrradianceField>() ),
        mNumProbesProcessed( 0u ),
        mFieldOrigin( Vector3::ZERO ),
        mFieldSize( Vector3::ZERO ),
        mVctLighting( 0 ),
        mIrradianceTex( 0 ),
        mDepthVarianceTex( 0 ),
        mGenerationWorkspace( 0 ),
        mGenerationJob( 0 ),
        mIfGenParamsBuffer( 0 ),
        mDirectionsBuffer( 0 ),
        mRoot( root ),
        mSceneManager( sceneManager )
    {
#if OGRE_NO_JSON
        OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                     "To use IrradianceField, Ogre must be build with JSON support "
                     "and you must include the resources bundled at "
                     "Samples/Media/Compute",
                     "IrradianceField::IrradianceField" );
#endif
        VaoManager *vaoManager = mRoot->getRenderSystem()->getVaoManager();
        mIfGenParamsBuffer = vaoManager->createConstBuffer( sizeof( IrradianceFieldGenParams ),
                                                            BT_DYNAMIC_PERSISTENT, 0, false );

        HlmsCompute *hlmsCompute = mRoot->getHlmsManager()->getComputeHlms();
        mGenerationJob = hlmsCompute->findComputeJobNoThrow( "IrradianceField/Gen" );

        if( !mGenerationJob )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "To use IrradianceField, you must include the resources bundled at "
                         "Samples/Media/Compute\n"
                         "Could not find IrradianceField/Gen",
                         "IrradianceField::IrradianceField" );
        }
    }
    //-------------------------------------------------------------------------
    IrradianceField::~IrradianceField()
    {
        destroyTextures();
        VaoManager *vaoManager = mRoot->getRenderSystem()->getVaoManager();
        if( mIfGenParamsBuffer->getMappingState() != MS_UNMAPPED )
            mIfGenParamsBuffer->unmap( UO_UNMAP_ALL );
        vaoManager->destroyConstBuffer( mIfGenParamsBuffer );
        mIfGenParamsBuffer = 0;
    }
    //-------------------------------------------------------------------------
    void IrradianceField::fillDirections( float *RESTRICT_ALIAS outBuffer )
    {
        float *RESTRICT_ALIAS updateData = reinterpret_cast<float * RESTRICT_ALIAS>( outBuffer );
        const float *RESTRICT_ALIAS updateDataStart = updateData;

        const Vector2 *subsamples = &mSettings.getSubsamples()[0];

        const size_t numRaysPerPixel = mSettings.mNumRaysPerPixel;
        const size_t depthProbeRes = mSettings.mDepthProbeResolution;

        for( size_t y = 0u; y < depthProbeRes; ++y )
        {
            for( size_t x = 0u; x < depthProbeRes; ++x )
            {
                for( size_t rayIdx = 0u; rayIdx < numRaysPerPixel; ++rayIdx )
                {
                    Vector2 uvOct = Vector2( x, y ) + subsamples[rayIdx];
                    uvOct /= static_cast<float>( depthProbeRes );

                    Vector3 directionVector = Math::octahedronMappingDecode( uvOct );

                    *updateData++ = static_cast<float>( directionVector.x );
                    *updateData++ = static_cast<float>( directionVector.y );
                    *updateData++ = static_cast<float>( directionVector.z );
                    *updateData++ = 0.0f;
                }
            }
        }

        OGRE_ASSERT_LOW( ( size_t )( updateData - updateDataStart ) <=
                         ( depthProbeRes * depthProbeRes * numRaysPerPixel * 4u ) );
    }
    //-------------------------------------------------------------------------
    void IrradianceField::setIrradianceFieldGenParams()
    {
        const uint32 numRaysPerPixel = mSettings.mNumRaysPerPixel;
        const uint32 depthProbeRes = mSettings.mDepthProbeResolution;
        const uint32 irradProbeRes = mSettings.mIrradianceResolution;

        const uint32 numRaysPerProbe = depthProbeRes * depthProbeRes * numRaysPerPixel;

        mIfGenParams.invNumRaysPerPixel = 1.0f / numRaysPerPixel;
        mIfGenParams.numRaysPerPixel = numRaysPerPixel;
        mIfGenParams.numRaysPerIrradiancePixel = 2u * depthProbeRes * numRaysPerPixel / irradProbeRes;
        mIfGenParams.invNumRaysPerIrradiancePixel = 1.0f / mIfGenParams.numRaysPerIrradiancePixel;

        const TextureGpu *vctLightingTex = mVctLighting->getLightVoxelTextures()[0];
        const float smallestRes = static_cast<float>(
            std::min( std::min( vctLightingTex->getWidth(), vctLightingTex->getHeight() ),
                      vctLightingTex->getDepth() ) );
        const float invSmallestRes = 1.0f / smallestRes;

        mIfGenParams.coneAngleTan = Math::Tan( Math::TWO_PI / static_cast<float>( numRaysPerProbe ) );
        mIfGenParams.numProcessedProbes = 0u;
        mIfGenParams.vctStartBias = invSmallestRes;
        mIfGenParams.vctInvStartBias = smallestRes;

        mIfGenParams.numProbes_threadsPerRow.x = mSettings.mNumProbes[0];
        mIfGenParams.numProbes_threadsPerRow.y = mSettings.mNumProbes[1];
        mIfGenParams.numProbes_threadsPerRow.z = mSettings.mNumProbes[2];
        mIfGenParams.numProbes_threadsPerRow.w = 0u;

        const VctVoxelizer *voxelizer = mVctLighting->getVoxelizer();
        Matrix4 irrProbeToVctTransform;
        irrProbeToVctTransform.makeTransform(
            ( voxelizer->getVoxelOrigin() - mFieldOrigin ) / voxelizer->getVoxelSize(),
            ( mFieldSize / voxelizer->getVoxelSize() ) / mSettings.getNumProbes3f(),
            Quaternion::IDENTITY );
        mIfGenParams.irrProbeToVctTransform = irrProbeToVctTransform;

        mGenerationJob->setProperty( "num_rays_per_probe", static_cast<int32>( numRaysPerProbe ) );
        mGenerationJob->setProperty( "num_rays_per_depth_pixel", static_cast<int32>( numRaysPerPixel ) );
        mGenerationJob->setProperty( "depth_resolution", static_cast<int32>( depthProbeRes ) );
        mGenerationJob->setProperty( "num_depth_pixels_per_probe",
                                     static_cast<int32>( depthProbeRes * depthProbeRes ) );
        mGenerationJob->setProperty( "depth_full_resolution",
                                     static_cast<int32>( mDepthVarianceTex->getWidth() ) );
        mGenerationJob->setProperty( "depth_to_colour_resolution_ratio",
                                     static_cast<int32>( depthProbeRes / irradProbeRes ) );
    }
    //-------------------------------------------------------------------------
    void IrradianceField::initialize( const IrradianceFieldSettings &settings,
                                      const Vector3 &fieldOrigin, const Vector3 &fieldSize,
                                      VctLighting *vctLighting )
    {
        mSettings = settings;
        mSettings.createSubsamples();
        mVctLighting = vctLighting;
        mFieldOrigin = fieldOrigin;
        mFieldSize = fieldSize;
        createTextures();
        setIrradianceFieldGenParams();
    }
    //-------------------------------------------------------------------------
    void IrradianceField::createTextures( void )
    {
        destroyTextures();

        TextureGpuManager *textureManager = mRoot->getRenderSystem()->getTextureGpuManager();

        mIrradianceTex = textureManager->createTexture(
            "IrradianceField" + StringConverter::toString( getId() ), GpuPageOutStrategy::Discard,
            TextureFlags::Uav, TextureTypes::Type2D );
        mDepthVarianceTex = textureManager->createTexture(
            "IrradianceFieldDepth" + StringConverter::toString( getId() ), GpuPageOutStrategy::Discard,
            TextureFlags::Uav, TextureTypes::Type2D );

        const uint32 irradResolution = mSettings.getIrradProbeFullResolution();
        mIrradianceTex->setResolution( irradResolution, irradResolution );
        mIrradianceTex->setPixelFormat( PFG_R10G10B10A2_UNORM );

        const uint32 depthResolution = mSettings.getDepthProbeFullResolution();
        mDepthVarianceTex->setResolution( depthResolution, depthResolution );
        mDepthVarianceTex->setPixelFormat( PFG_RG32_FLOAT );

        mIrradianceTex->scheduleTransitionTo( GpuResidency::Resident );
        mDepthVarianceTex->scheduleTransitionTo( GpuResidency::Resident );

        const size_t updateDataSize =
            sizeof( float ) * 4u * mSettings.mNumRaysPerPixel * depthResolution * depthResolution;
        float *directionsBuffer =
            reinterpret_cast<float *>( OGRE_MALLOC_SIMD( updateDataSize, MEMCATEGORY_GEOMETRY ) );
        FreeOnDestructor dataPtr( directionsBuffer );
        fillDirections( directionsBuffer );
        VaoManager *vaoManager = textureManager->getVaoManager();
        mDirectionsBuffer = vaoManager->createTexBuffer( PFG_RGBA32_FLOAT, updateDataSize, BT_IMMUTABLE,
                                                         directionsBuffer, false );

        mGenerationJob->setConstBuffer( 0, mIfGenParamsBuffer );

        const bool bIsAnisotropic = mVctLighting->isAnisotropic();
        mGenerationJob->setProperty( "vct_anisotropic", 1 );
        mGenerationJob->setNumTexUnits( 1u + ( bIsAnisotropic ? 4u : 1u ) );

        DescriptorSetTexture2::BufferSlot bufferSlot( DescriptorSetTexture2::BufferSlot::makeEmpty() );
        bufferSlot.buffer = mDirectionsBuffer;
        mGenerationJob->setTexBuffer( 0, bufferSlot );

        TextureGpu **vctLightingTextures = mVctLighting->getLightVoxelTextures();
        DescriptorSetTexture2::TextureSlot texSlot( DescriptorSetTexture2::TextureSlot::makeEmpty() );
        for( uint8 i = 0u; i < ( bIsAnisotropic ? 4u : 1u ); ++i )
        {
            texSlot.texture = vctLightingTextures[i];
            mGenerationJob->setTexture( 1u + i, texSlot, mVctLighting->getBindTrilinearSamplerblock() );
        }

        CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
        CompositorChannelVec channels;
        channels.push_back( mIrradianceTex );
        channels.push_back( mDepthVarianceTex );
        mGenerationWorkspace = compositorManager->addWorkspace( mSceneManager, channels, 0,
                                                                "IrradianceField/Gen/Workspace", false );
    }
    //-------------------------------------------------------------------------
    void IrradianceField::destroyTextures( void )
    {
        TextureGpuManager *textureManager = mRoot->getRenderSystem()->getTextureGpuManager();

        if( mGenerationWorkspace )
        {
            CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            compositorManager->removeWorkspace( mGenerationWorkspace );
            mGenerationWorkspace = 0;
        }
        if( mIrradianceTex )
        {
            textureManager->destroyTexture( mIrradianceTex );
            mIrradianceTex = 0;
        }
        if( mDepthVarianceTex )
        {
            textureManager->destroyTexture( mDepthVarianceTex );
            mDepthVarianceTex = 0;
        }
        if( mDirectionsBuffer )
        {
            VaoManager *vaoManager = textureManager->getVaoManager();
            vaoManager->destroyTexBuffer( mDirectionsBuffer );
            mDirectionsBuffer = 0;
        }
    }
    //-------------------------------------------------------------------------
    void IrradianceField::update( uint32 probesPerFrame )
    {
        const uint32 totalNumProbes = mSettings.getTotalNumProbes();
        if( mNumProbesProcessed >= totalNumProbes )
            return;

        IrradianceFieldGenParams *ifGenParams = reinterpret_cast<IrradianceFieldGenParams *>(
            mIfGenParamsBuffer->map( 0, mIfGenParamsBuffer->getNumElements() ) );

        probesPerFrame = totalNumProbes;
        probesPerFrame = std::min( totalNumProbes - mNumProbesProcessed, probesPerFrame );
        OGRE_ASSERT_LOW( ( ( probesPerFrame & 0x01u ) == 0u ) && "probesPerFrame must be even!" );

        const uint32 threadsPerGroup = 64u;
        mGenerationJob->setThreadsPerGroup( threadsPerGroup, 1u, 1u );

        const uint32 numRaysPerPixel = mSettings.mNumRaysPerPixel;
        const uint32 depthResolution = mSettings.mDepthProbeResolution;

        const uint32 numRays = probesPerFrame * depthResolution * depthResolution * numRaysPerPixel;

        OGRE_ASSERT_LOW( ( numRays % threadsPerGroup ) == 0u );

        const uint32 numWorkGroups = numRays / threadsPerGroup;

        // There's a leftover the first dispatch is not currently handling,
        // i.e. numThreadGroupsX * numThreadGroupsY * threadsPerGroup != numRays
        TODO_handle_leftover;
        // Most GPUs allow up to 65535 thread groups per dimension
        const uint32 numThreadGroupsY = numWorkGroups / 65535u + 1u;
        const uint32 numThreadGroupsX = numWorkGroups / numThreadGroupsY;
        mGenerationJob->setNumThreadGroups( numThreadGroupsX, numThreadGroupsY, 1u );

        mIfGenParams.numProcessedProbes = mNumProbesProcessed;
        mIfGenParams.numProbes_threadsPerRow.w = numThreadGroupsX * threadsPerGroup;
        *ifGenParams = mIfGenParams;

        mGenerationWorkspace->_beginUpdate( false );
        mGenerationWorkspace->_update();
        mGenerationWorkspace->_endUpdate( false );

        mNumProbesProcessed += probesPerFrame;

        mIfGenParamsBuffer->unmap( UO_KEEP_PERSISTENT );
    }
}  // namespace Ogre
