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

#include "Compositor/OgreCompositorWorkspace.h"

#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"
#include "OgreHlmsManager.h"
#include "OgreStringConverter.h"
#include "OgreTextureGpuManager.h"
#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreTexBufferPacked.h"
#include "Vao/OgreVaoManager.h"

namespace Ogre
{
    IrradianceFieldSettings::IrradianceFieldSettings() :
        mNumRaysPerPixel( 4u ),
        mDepthProbeResolution( 8u ),
        mIrradianceResolution( 16u )
    {
        for( size_t i = 0u; i < 3u; ++i )
            mNumProbes[i] = 32u;
        mNumProbes[1] = 8u;
    }
    //-------------------------------------------------------------------------
    uint32 IrradianceFieldSettings::getTotalNumProbes( void ) const
    {
        return mNumProbes[0] + mNumProbes[1] + mNumProbes[2];
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
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    IrradianceField::IrradianceField( TextureGpuManager *textureManager, HlmsManager *hlmsManager ) :
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
        mTextureManager( textureManager ),
        mHlmsManager( hlmsManager )
    {
#if OGRE_NO_JSON
        OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                     "To use IrradianceField, Ogre must be build with JSON support "
                     "and you must include the resources bundled at "
                     "Samples/Media/Compute",
                     "IrradianceField::IrradianceField" );
#endif
        VaoManager *vaoManager = mTextureManager->getVaoManager();
        mIfGenParamsBuffer = vaoManager->createConstBuffer( sizeof( IrradianceFieldGenParams ),
                                                            BT_DYNAMIC_PERSISTENT, 0, false );

        HlmsCompute *hlmsCompute = hlmsManager->getComputeHlms();
        mGenerationJob = hlmsCompute->findComputeJobNoThrow( "IrradianceField/Gen" );

        if( !mGenerationJob )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "To use VctVoxelizer, you must include the resources bundled at "
                         "Samples/Media/Compute\n"
                         "Could not find IrradianceField/Gen",
                         "IrradianceField::IrradianceField" );
        }
    }
    //-------------------------------------------------------------------------
    IrradianceField::~IrradianceField()
    {
        destroyTextures();
        VaoManager *vaoManager = mTextureManager->getVaoManager();
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
    void IrradianceField::initialize( const IrradianceFieldSettings &settings,
                                      const Vector3 &fieldOrigin, const Vector3 &fieldSize,
                                      VctLighting *vctLighting )
    {
        mSettings = settings;
        mVctLighting = vctLighting;
    }
    //-------------------------------------------------------------------------
    void IrradianceField::createTextures( void )
    {
        destroyTextures();

        mIrradianceTex = mTextureManager->createTexture(
            "IrradianceField" + StringConverter::toString( getId() ), GpuPageOutStrategy::Discard,
            TextureFlags::Uav, TextureTypes::Type2D );
        mIrradianceTex = mTextureManager->createTexture(
            "IrradianceFieldDepth" + StringConverter::toString( getId() ), GpuPageOutStrategy::Discard,
            TextureFlags::Uav, TextureTypes::Type2D );

        const uint32 irradResolution = mSettings.getIrradProbeFullResolution();
        mIrradianceTex->setResolution( irradResolution, irradResolution );
        mIrradianceTex->setPixelFormat( PFG_R10G10B10A2_UNORM );

        const uint32 depthResolution = mSettings.getDepthProbeFullResolution();
        mDepthVarianceTex->setResolution( depthResolution, depthResolution );
        mDepthVarianceTex->setPixelFormat( PFG_RG32_FLOAT );

        const size_t updateDataSize =
            sizeof( float ) * 4u * mSettings.mNumRaysPerPixel * depthResolution * depthResolution;
        float *directionsBuffer =
            reinterpret_cast<float *>( OGRE_MALLOC_SIMD( updateDataSize, MEMCATEGORY_GEOMETRY ) );
        FreeOnDestructor dataPtr( directionsBuffer );
        fillDirections( directionsBuffer );
        VaoManager *vaoManager = mTextureManager->getVaoManager();
        mDirectionsBuffer = vaoManager->createTexBuffer( PFG_RGBA32_FLOAT, updateDataSize, BT_IMMUTABLE,
                                                         directionsBuffer, false );

        mGenerationJob->setConstBuffer( 0, mIfGenParamsBuffer );

        DescriptorSetTexture2::BufferSlot bufferSlot( DescriptorSetTexture2::BufferSlot::makeEmpty() );
        bufferSlot.buffer = mDirectionsBuffer;
        mGenerationJob->setTexBuffer( 0, bufferSlot );

        const bool bIsAnisotropic = mVctLighting->isAnisotropic();
        mGenerationJob->setProperty( "vct_anisotropic", 1 );
        mGenerationJob->setNumTexUnits( 1u + ( bIsAnisotropic ? 1u : 4u ) );

        TextureGpu **vctLightingTextures = mVctLighting->getLightVoxelTextures();
        DescriptorSetTexture2::TextureSlot texSlot( DescriptorSetTexture2::TextureSlot::makeEmpty() );
        for( uint8 i = 0u; i < (bIsAnisotropic ? 4u : 1u); ++i )
        {
            texSlot.texture = vctLightingTextures[i];
            mGenerationJob->setTexture( 1u + i, texSlot, mVctLighting->getBindTrilinearSamplerblock() );
        }
    }
    //-------------------------------------------------------------------------
    void IrradianceField::destroyTextures( void )
    {
        if( mIrradianceTex )
        {
            mTextureManager->destroyTexture( mIrradianceTex );
            mIrradianceTex = 0;
        }
        if( mDepthVarianceTex )
        {
            mTextureManager->destroyTexture( mDepthVarianceTex );
            mDepthVarianceTex = 0;
        }
        if( mDirectionsBuffer )
        {
            VaoManager *vaoManager = mTextureManager->getVaoManager();
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
        mIfGenParams.numProcessedProbes = mNumProbesProcessed;
        *ifGenParams = mIfGenParams;

        probesPerFrame = std::min( totalNumProbes - mNumProbesProcessed, probesPerFrame );
        OGRE_ASSERT_LOW( ( ( probesPerFrame & 0x01u ) == 0u ) && "probesPerFrame must be even!" );

        const uint32 threadsPerGroup = 64u;
        mGenerationJob->setThreadsPerGroup( threadsPerGroup, 1u, 1u );

        const uint32 numRaysPerPixel = mSettings.mNumRaysPerPixel;
        const uint32 depthResolution = mSettings.mDepthProbeResolution;

        const uint32 numRays = probesPerFrame * depthResolution * depthResolution * numRaysPerPixel;

        OGRE_ASSERT_LOW( numRays % threadsPerGroup );

        const uint32 numWorkGroups = numRays / threadsPerGroup;

        // Most GPUs allow up to 65535 thread groups per dimension
        const uint32 numThreadGroupsX = numWorkGroups % 65535u;
        const uint32 numThreadGroupsY = numWorkGroups / 65535u + 1u;
        mGenerationJob->setNumThreadGroups( numThreadGroupsX, numThreadGroupsY, 1u );

        mGenerationWorkspace->_beginUpdate( false );
        mGenerationWorkspace->_update();
        mGenerationWorkspace->_endUpdate( false );

        mNumProbesProcessed += probesPerFrame;

        mIfGenParamsBuffer->unmap( UO_KEEP_PERSISTENT );
    }
}  // namespace Ogre
