/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#include "Vct/OgreVctLighting.h"

#include "Vct/OgreVctVoxelizerSourceBase.h"
#include "Vct/OgreVoxelVisualizer.h"

#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"
#include "OgreHlmsManager.h"
#include "OgreLight.h"
#include "OgreLwString.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreRenderSystem.h"
#include "OgreSceneManager.h"
#include "OgreShaderPrimitives.h"
#include "OgreStringConverter.h"
#include "OgreTextureGpuManager.h"
#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreVaoManager.h"

namespace Ogre
{
    struct ShaderVctLight
    {
        // Pre-mul by PI? -No because we lose a ton of precision
        //.w contains lightDistThreshold
        float diffuse[4];
        // For directional lights, pos.xyz contains -dir.xyz and pos.w = 0;
        // For the rest of lights, pos.xyz contains pos.xyz and pos.w = 1;
        float pos[4];
        // uvwPos.w contains the light type
        float uvwPos[4];

        // Used by area lights
        // points[0].w contains double sided info
        float points[4][4];
    };

    const uint16 VctLighting::msDistanceThresholdCustomParam = 3876u;

    static const IdString NumVctCascadesProp = "hlms_num_vct_cascades";

    static const size_t c_maxCascades = 8u;

    VctLighting::VctLighting( IdType id, VctVoxelizerSourceBase *voxelizer, bool bAnisotropic ) :
        IdObject( id ),
        mSamplerblockTrilinear( 0 ),
        mVoxelizer( voxelizer ),
        mVoxelizerTexturesChanged( false ),
        mVoxelizerListenersRemoved( false ),
        mLightInjectionJob( 0 ),
        mLightsConstBuffer( 0 ),
        mAnisoGeneratorStep0( 0 ),
        mLightVctBounceInject( 0 ),
        mLightBounce( 0 ),
        mBakingMultiplier( 1.0f ),
        mInvBakingMultiplier( 1.0f ),
        mDefaultLightDistThreshold( 0.5f ),
        mAnisotropic( bAnisotropic ),
        mNumLights( 0 ),
        mRayMarchStepSize( 0 ),
        mVoxelCellSize( 0 ),
        mDirCorrectionRatioThinWallCounter( 0 ),
        mInvVoxelResolution( 0 ),
        mShaderParams( 0 ),
        mBounceVoxelCellSize( 0 ),
        mBounceInvVoxelResolution( 0 ),
        mBounceIterationDampening( 0 ),
        mBounceStartBiasInvBiasCascadeMaxLod( 0 ),
        mBounceFromPreviousProbeToNext( 0 ),
        mBounceShaderParams( 0 ),
        mSpecularSdfQuality( 0.875f ),
        mMultiplier( 1.0f ),
        mDebugVoxelVisualizer( 0 )
    {
        memset( mLightVoxel, 0, sizeof( mLightVoxel ) );
        memset( mUpperHemisphere, 0, sizeof( mUpperHemisphere ) );
        memset( mLowerHemisphere, 0, sizeof( mLowerHemisphere ) );

        OGRE_ASSERT_LOW( mVoxelizer->getAlbedoVox() &&
                         "VctVoxelizer::build must've been called before creating VctLighting!" );

        mVoxelizer->getAlbedoVox()->addListener( this );
        mVoxelizer->getNormalVox()->addListener( this );
        mVoxelizerListenersRemoved = false;

        // VctVoxelizer should've already been initialized, thus no need
        // to check if JSON has been built or if the assets were added
        HlmsCompute *hlmsCompute = mVoxelizer->getHlmsManager()->getComputeHlms();
        mLightInjectionJob = hlmsCompute->findComputeJob( "VCT/LightInjection" );

        mShaderParams = &mLightInjectionJob->getShaderParams( "default" );
        mNumLights = mShaderParams->findParameter( "numLights" );
        mRayMarchStepSize = mShaderParams->findParameter( "rayMarchStepSize_bakingMultiplier" );
        mVoxelCellSize = mShaderParams->findParameter( "voxelCellSize" );
        mDirCorrectionRatioThinWallCounter =
            mShaderParams->findParameter( "dirCorrectionRatio_thinWallCounter" );
        mInvVoxelResolution = mShaderParams->findParameter( "invVoxelResolution" );

        RenderSystem *renderSystem = mVoxelizer->getRenderSystem();
        VaoManager *vaoManager = renderSystem->getVaoManager();
        mLightsConstBuffer = vaoManager->createConstBuffer( sizeof( ShaderVctLight ) * 16u,
                                                            BT_DYNAMIC_PERSISTENT, 0, false );

        HlmsManager *hlmsManager = mVoxelizer->getHlmsManager();
        HlmsSamplerblock samplerblock;
        samplerblock.mMipFilter = FO_LINEAR;
        mSamplerblockTrilinear = hlmsManager->getSamplerblock( samplerblock );

        mLightVctBounceInject = hlmsCompute->findComputeJob( "VCT/LightVctBounceInject" );

        mBounceShaderParams = &mLightVctBounceInject->getShaderParams( "default" );

        mLocalBounceShaderParams.reserve( 7u );

        mBounceVoxelCellSize = addLocalBounceShaderParam( "voxelCellSize" );
        mBounceInvVoxelResolution = addLocalBounceShaderParam( "invVoxelResolution" );
        mBounceIterationDampening = addLocalBounceShaderParam( "iterationDampening" );
        mBounceStartBiasInvBiasCascadeMaxLod =
            addLocalBounceShaderParam( "startBias_invStartBias_cascadeMaxLod" );
        mBounceFromPreviousProbeToNext = addLocalBounceShaderParam( "fromPreviousProbeToNext" );

        createTextures();
    }
    //-------------------------------------------------------------------------
    VctLighting::~VctLighting()
    {
        RenderSystem *renderSystem = mVoxelizer->getRenderSystem();
        VaoManager *vaoManager = renderSystem->getVaoManager();

        setDebugVisualization( false, 0 );

        if( mLightsConstBuffer )
        {
            if( mLightsConstBuffer->getMappingState() != MS_UNMAPPED )
                mLightsConstBuffer->unmap( UO_UNMAP_ALL );
            vaoManager->destroyConstBuffer( mLightsConstBuffer );
            mLightsConstBuffer = 0;
        }

        HlmsManager *hlmsManager = mVoxelizer->getHlmsManager();
        hlmsManager->destroySamplerblock( mSamplerblockTrilinear );
        mSamplerblockTrilinear = 0;

        if( !mVoxelizerListenersRemoved )
        {
            mVoxelizer->getAlbedoVox()->removeListener( this );
            mVoxelizer->getNormalVox()->removeListener( this );
            mVoxelizerListenersRemoved = true;
        }

        destroyTextures();
    }
    //-------------------------------------------------------------------------
    ShaderParams::Param *VctLighting::addLocalBounceShaderParam( const char *name )
    {
        mLocalBounceShaderParams.push_back( ShaderParams::Param() );
        ShaderParams::Param *retVal = &mLocalBounceShaderParams.back();
        retVal->name = name;
        return retVal;
    }
    //-------------------------------------------------------------------------
    void VctLighting::restoreSwappedTextures()
    {
        if( mLightVoxel[0] && mLightBounce )
        {
            char tmpBuffer[128];
            LwString texName( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
            texName.a( "VctLightingBounce/Id", getId() );

            if( mLightBounce->getName() != texName.c_str() )
            {
                std::swap( mLightVoxel[0], mLightBounce );

                DescriptorSetTexture2::TextureSlot texSlot(
                    DescriptorSetTexture2::TextureSlot::makeEmpty() );
                texSlot.texture = mLightVoxel[0];
                mLightVctBounceInject->setTexture( 2, texSlot, mSamplerblockTrilinear );

                if( mAnisoGeneratorStep0 )
                {
                    texSlot.texture = mLightVoxel[0];
                    mAnisoGeneratorStep0->setTexture( 0, texSlot );
                }
            }
        }
    }
    //-------------------------------------------------------------------------
    float VctLighting::addLight( ShaderVctLight *RESTRICT_ALIAS vctLight, Light *light,
                                 const Vector3 &voxelOrigin, const Vector3 &invVoxelSize )
    {
        const ColourValue diffuseColour = light->getDiffuseColour() * light->getPowerScale();
        for( size_t i = 0; i < 3u; ++i )
            vctLight->diffuse[i] = static_cast<float>( diffuseColour[i] );

        const Vector4 *lightDistThreshold =
            light->getCustomParameterNoThrow( msDistanceThresholdCustomParam );
        vctLight->diffuse[3] = lightDistThreshold
                                   ? ( lightDistThreshold->x * lightDistThreshold->x )
                                   : ( mDefaultLightDistThreshold * mDefaultLightDistThreshold );

        Light::LightTypes lightType = light->getType();
        if( lightType == Light::LT_AREA_APPROX )
            lightType = Light::LT_AREA_LTC;

        Vector4 light4dVec = light->getAs4DVector();
        if( lightType != Light::LT_DIRECTIONAL )
            light4dVec -= Vector4( voxelOrigin, 0.0f );

        for( size_t i = 0; i < 4u; ++i )
            vctLight->pos[i] = static_cast<float>( light4dVec[i] );

        Vector3 uvwPos = light->getParentNode()->_getDerivedPosition();
        uvwPos = ( uvwPos - voxelOrigin ) * invVoxelSize;
        for( size_t i = 0; i < 3u; ++i )
            vctLight->uvwPos[i] = static_cast<float>( uvwPos[i] );
        vctLight->uvwPos[3] = static_cast<float>( lightType );

        Vector3 rectPoints[4];

        const Quaternion qRot = light->getParentNode()->_getDerivedOrientation();
        const Vector3 lightDir = qRot.zAxis();

        if( lightType == Light::LT_AREA_LTC )
        {
            const Vector3 lightPos( light4dVec.xyz() );
            const Vector2 rectSize = light->getDerivedRectSize() * 0.5f;
            Vector3 xAxis = qRot.xAxis() * rectSize.x;
            Vector3 yAxis = qRot.yAxis() * rectSize.y;

            rectPoints[0] = lightPos - xAxis - yAxis;
            rectPoints[1] = lightPos + xAxis - yAxis;
            rectPoints[2] = lightPos + xAxis + yAxis;
            rectPoints[3] = lightPos - xAxis + yAxis;
        }
        else
        {
            memset( rectPoints, 0, sizeof( rectPoints ) );

            if( lightType == Light::LT_SPOTLIGHT )
            {
                // float3 spotDirection
                rectPoints[0].x = -lightDir.x;
                rectPoints[0].y = -lightDir.y;
                rectPoints[0].z = -lightDir.z;

                // float3 spotParams
                const Radian innerAngle = light->getSpotlightInnerAngle();
                const Radian outerAngle = light->getSpotlightOuterAngle();
                rectPoints[1].x = 1.0f / ( cosf( innerAngle.valueRadians() * 0.5f ) -
                                           cosf( outerAngle.valueRadians() * 0.5f ) );
                rectPoints[1].y = cosf( outerAngle.valueRadians() * 0.5f );
                rectPoints[1].z = light->getSpotlightFalloff();
            }
        }

        const float isDoubleSided = light->getDoubleSided() ? 1.0f : 0.0f;
        for( size_t i = 0; i < 4u; ++i )
        {
            for( size_t j = 0; j < 3u; ++j )
                vctLight->points[i][j] = rectPoints[i][j];
            vctLight->points[i][3u] = isDoubleSided;
        }

        vctLight->points[1][3u] = static_cast<float>( lightDir.x );
        vctLight->points[2][3u] = static_cast<float>( lightDir.y );
        vctLight->points[3][3u] = static_cast<float>( lightDir.z );

        float maxValue;
        maxValue = std::max( diffuseColour.r, diffuseColour.g );
        maxValue = std::max( maxValue, diffuseColour.b );
        return maxValue;
    }
    //-------------------------------------------------------------------------
    void VctLighting::createTextures()
    {
        const bool allowsMultipleBounces = getAllowMultipleBounces();
        destroyTextures();

        TextureGpuManager *textureManager = mVoxelizer->getTextureGpuManager();

        uint32 texFlags = TextureFlags::Uav | TextureFlags::Reinterpretable;

        const bool bSdfQuality = shouldEnableSpecularSdfQuality();
        if( !mAnisotropic || bSdfQuality )
            texFlags |= TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps;

        const TextureGpu *albedoVox = mVoxelizer->getAlbedoVox();

        const uint32 width = albedoVox->getWidth();
        const uint32 height = albedoVox->getHeight();
        const uint32 depth = albedoVox->getDepth();

        const uint32 widthAniso = std::max( 1u, width );
        const uint32 heightAniso = std::max( 1u, height >> 1u );
        const uint32 depthAniso = std::max( 1u, depth >> 1u );

        const uint8 numMipsMain = ( mAnisotropic && !bSdfQuality )
                                      ? 1u
                                      : PixelFormatGpuUtils::getMaxMipmapCount( width, height, depth );
        // numMipsAniso needs one less mip; because the last mip must be 2x1x1, not 1x1x1
        const uint8 numMipsAniso =
            PixelFormatGpuUtils::getMaxMipmapCount( widthAniso, heightAniso, depthAniso ) - 1u;

        const size_t numTextures = mAnisotropic ? 4u : 1u;

        const char *names[] = {
            "Main",    //
            "X_axis",  //
            "Y_axis",  //
            "Z_axis"   //
        };

        for( size_t i = 0; i < numTextures; ++i )
        {
            char tmpBuffer[128];
            LwString texName( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
            texName.a( "VctLighting_", names[i], "/Id", getId() );
            TextureGpu *texture = textureManager->createTexture(
                texName.c_str(), GpuPageOutStrategy::Discard, texFlags, TextureTypes::Type3D );
            if( i == 0u )
            {
                texture->setResolution( width, height, depth );
                texture->setNumMipmaps( numMipsMain );
            }
            else
            {
                texture->setResolution( widthAniso, heightAniso, depthAniso );
                texture->setNumMipmaps( numMipsAniso );
            }
            texture->setPixelFormat( PFG_RGBA8_UNORM_SRGB );
            texture->scheduleTransitionTo( GpuResidency::Resident );
            mLightVoxel[i] = texture;

            texFlags &= ( uint32 ) ~( TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps );
        }

        if( mAnisotropic )
        {
            // Setup the compute shaders for VctLighting::generateAnisotropicMips()
            HlmsCompute *hlmsCompute = mVoxelizer->getHlmsManager()->getComputeHlms();
            mAnisoGeneratorStep0 = hlmsCompute->findComputeJob( "VCT/AnisotropicMipStep0" );

            char tmpBuffer[128];
            LwString jobName( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

            // Step 0
            jobName.clear();
            jobName.a( "VCT/AnisotropicMipStep0/Id", getId() );
            mAnisoGeneratorStep0 = mAnisoGeneratorStep0->clone( jobName.c_str() );

            for( uint8 i = 0; i < 3u; ++i )
            {
                DescriptorSetUav::TextureSlot uavSlot( DescriptorSetUav::TextureSlot::makeEmpty() );
                uavSlot.access = ResourceAccess::Write;
                uavSlot.texture = mLightVoxel[i + 1u];
                uavSlot.pixelFormat = PFG_RGBA8_UNORM;
                mAnisoGeneratorStep0->_setUavTexture( i, uavSlot );
            }

            DescriptorSetTexture2::TextureSlot texSlot(
                DescriptorSetTexture2::TextureSlot::makeEmpty() );
            texSlot.texture = mLightVoxel[0];
            mAnisoGeneratorStep0->setTexture( 0, texSlot );
            texSlot.texture = mVoxelizer->getNormalVox();
            mAnisoGeneratorStep0->setTexture( 1, texSlot );

            ShaderParams *shaderParams = &mAnisoGeneratorStep0->getShaderParams( "default" );
            // higherMipHalfWidth
            ShaderParams::Param *lowerMipResolutionParam = &shaderParams->mParams.back();
            // int32 resolution[4] = { static_cast<int32>( mLightVoxel[1]->getWidth() >> 1u ) };
            lowerMipResolutionParam->setManualValue(
                static_cast<int32>( mLightVoxel[1]->getWidth() >> 1u ) );
            shaderParams->setDirty();

            // Now setup step 1
            // numMipsOnStep1 is subtracted one because mip 0 got processed by step 0
            const uint8 numMipsOnStep1 = mLightVoxel[1]->getNumMipmaps() - 1u;
            mAnisoGeneratorStep1.resize( numMipsOnStep1 );

            HlmsComputeJob *baseJob = hlmsCompute->findComputeJob( "VCT/AnisotropicMipStep1" );

            for( uint8 i = 0; i < numMipsOnStep1; ++i )
            {
                jobName.clear();
                jobName.a( "VCT/AnisotropicMipStep1/Id", getId(), "/Mip", i + 1u );
                HlmsComputeJob *mipJob = baseJob->clone( jobName.c_str() );

                for( uint8 axis = 0; axis < 3u; ++axis )
                {
                    texSlot.texture = mLightVoxel[axis + 1u];
                    texSlot.mipmapLevel = i;
                    texSlot.numMipmaps = 1u;
                    mipJob->setTexture( axis, texSlot );

                    DescriptorSetUav::TextureSlot uavSlot( DescriptorSetUav::TextureSlot::makeEmpty() );
                    uavSlot.access = ResourceAccess::Write;
                    uavSlot.texture = mLightVoxel[axis + 1u];
                    uavSlot.mipmapLevel = i + 1u;
                    uavSlot.pixelFormat = PFG_RGBA8_UNORM;
                    mipJob->_setUavTexture( axis, uavSlot );
                }

                shaderParams = &mipJob->getShaderParams( "default" );
                // higherMipHalfRes_lowerMipHalfWidth
                lowerMipResolutionParam = &shaderParams->mParams.back();
                int32 resolutions[4] = { //
                                         static_cast<int32>( mLightVoxel[1]->getWidth() >> ( i + 2u ) ),
                                         static_cast<int32>( mLightVoxel[1]->getHeight() >> ( i + 1u ) ),
                                         static_cast<int32>( mLightVoxel[1]->getDepth() >> ( i + 1u ) ),
                                         static_cast<int32>( mLightVoxel[1]->getWidth() >> ( i + 1u ) )
                };
                for( size_t j = 0; j < 4u; ++j )
                    resolutions[j] = std::max( 1, resolutions[j] );
                lowerMipResolutionParam->setManualValue( resolutions, 4u );
                shaderParams->setDirty();

                mAnisoGeneratorStep1[i] = mipJob;
            }
        }

        mLightInjectionJob->setProperty( "correct_area_light_shadows",
                                         albedoVox->getNumMipmaps() > 1u ? 1 : 0 );

        setAllowMultipleBounces( allowsMultipleBounces );

        if( mDebugVoxelVisualizer )
        {
            mDebugVoxelVisualizer->setTrackingVoxel( mLightVoxel[0], mLightVoxel[0], true );
            mDebugVoxelVisualizer->setVisible( true );
        }
    }
    //-------------------------------------------------------------------------
    void VctLighting::destroyTextures()
    {
        restoreSwappedTextures();

        TextureGpuManager *textureManager = mVoxelizer->getTextureGpuManager();
        for( size_t i = 0; i < sizeof( mLightVoxel ) / sizeof( mLightVoxel[0] ); ++i )
        {
            if( mLightVoxel[i] )
            {
                textureManager->destroyTexture( mLightVoxel[i] );
                mLightVoxel[i] = 0;
            }
        }

        HlmsCompute *hlmsCompute = mVoxelizer->getHlmsManager()->getComputeHlms();

        if( mAnisoGeneratorStep0 )
        {
            hlmsCompute->destroyComputeJob( mAnisoGeneratorStep0->getName() );
            mAnisoGeneratorStep0 = 0;
        }

        FastArray<HlmsComputeJob *>::const_iterator itor = mAnisoGeneratorStep1.begin();
        FastArray<HlmsComputeJob *>::const_iterator end = mAnisoGeneratorStep1.end();

        while( itor != end )
        {
            hlmsCompute->destroyComputeJob( ( *itor )->getName() );
            ++itor;
        }

        mAnisoGeneratorStep1.clear();
        setAllowMultipleBounces( false );

        if( mDebugVoxelVisualizer )
            mDebugVoxelVisualizer->setVisible( false );
    }
    //-------------------------------------------------------------------------
    void VctLighting::checkTextures()
    {
        if( mVoxelizerTexturesChanged )
            createTextures();

        if( mVoxelizerListenersRemoved )
        {
            mVoxelizer->getAlbedoVox()->addListener( this );
            mVoxelizer->getNormalVox()->addListener( this );
            mVoxelizerListenersRemoved = false;
        }
    }
    //-------------------------------------------------------------------------
    void VctLighting::setupBounceTextures()
    {
        const size_t numExtraCascades = mExtraCascades.size();

        uint8 numNeededTexUnits;
        if( mAnisotropic )
            numNeededTexUnits = 6u + 4u * static_cast<uint8>( numExtraCascades );
        else
            numNeededTexUnits = 3u + static_cast<uint8>( numExtraCascades );

        HlmsManager *hlmsManager = mVoxelizer->getHlmsManager();
        const RenderSystemCapabilities *caps = hlmsManager->getRenderSystem()->getCapabilities();
        const bool bSetSampler = !caps->hasCapability( RSC_SEPARATE_SAMPLERS_FROM_TEXTURES );

        if( mLightVctBounceInject->getNumTexUnits() != numNeededTexUnits )
        {
            mLightVctBounceInject->setNumTexUnits( numNeededTexUnits );
            if( !bSetSampler )
                mLightVctBounceInject->setNumSamplerUnits( 3u );
        }

        setupGlslTextureUnits();

        DescriptorSetTexture2::TextureSlot texSlot( DescriptorSetTexture2::TextureSlot::makeEmpty() );
        texSlot.texture = mVoxelizer->getAlbedoVox();
        mLightVctBounceInject->setTexture( 0, texSlot );
        texSlot.texture = mVoxelizer->getNormalVox();
        mLightVctBounceInject->setTexture( 1, texSlot );
        texSlot.texture = mLightVoxel[0];
        mLightVctBounceInject->setTexture( 2, texSlot, mSamplerblockTrilinear );

        uint8 texSlotIdx = 3u;

        for( size_t cascadeIdx = 0u; cascadeIdx < numExtraCascades; ++cascadeIdx )
        {
            texSlot.texture = mExtraCascades[cascadeIdx]->mLightVoxel[0];
            mLightVctBounceInject->setTexture( texSlotIdx++, texSlot, 0, false );
            if( bSetSampler )
            {
                // Only OpenGL needs this sampler set
                hlmsManager->addReference( mSamplerblockTrilinear );
                mLightVctBounceInject->_setSamplerblock( texSlotIdx - 1u, mSamplerblockTrilinear );
            }
        }

        if( mAnisotropic )
        {
            for( uint8 i = 0u; i < 3u; ++i )
            {
                texSlot.texture = mLightVoxel[i + 1u];
                mLightVctBounceInject->setTexture( texSlotIdx++, texSlot, 0, false );
                if( bSetSampler )
                {
                    // Only OpenGL needs this sampler set
                    hlmsManager->addReference( mSamplerblockTrilinear );
                    mLightVctBounceInject->_setSamplerblock( texSlotIdx - 1u, mSamplerblockTrilinear );
                }

                for( size_t cascadeIdx = 0u; cascadeIdx < numExtraCascades; ++cascadeIdx )
                {
                    texSlot.texture = mExtraCascades[cascadeIdx]->mLightVoxel[i + 1u];
                    mLightVctBounceInject->setTexture( texSlotIdx++, texSlot, 0, false );
                    if( bSetSampler )
                    {
                        // Only OpenGL needs this sampler set
                        hlmsManager->addReference( mSamplerblockTrilinear );
                        mLightVctBounceInject->_setSamplerblock( texSlotIdx - 1u,
                                                                 mSamplerblockTrilinear );
                    }
                }
            }
        }

        DescriptorSetUav::TextureSlot uavSlot( DescriptorSetUav::TextureSlot::makeEmpty() );
        uavSlot.access = ResourceAccess::Write;
        uavSlot.texture = mLightBounce;
        uavSlot.pixelFormat = PFG_RGBA8_UNORM;
        mLightVctBounceInject->_setUavTexture( 0, uavSlot );
    }
    //-------------------------------------------------------------------------
    void VctLighting::setupGlslTextureUnits()
    {
        const size_t numExtraCascades = mExtraCascades.size();
        size_t numNeededTexUnits;
        if( mAnisotropic )
            numNeededTexUnits = 6u + 4u * numExtraCascades;
        else
            numNeededTexUnits = 3u + numExtraCascades;

        // This code assumes there's 2 textures at the beginning that always stays the same
        // the rest of them are dynamically generated.
        //
        // We also need to check if another VctLighting instance set a different number of cascades
        ShaderParams &glslShaderParams = mLightVctBounceInject->getShaderParams( "glsl" );
        if( glslShaderParams.mParams.size() != numNeededTexUnits ||
            glslShaderParams.mParams[3].mp.dataSizeBytes !=
                ( numExtraCascades + 1u ) * sizeof( uint32 ) )
        {
            glslShaderParams.mParams.resize( 2u );

            ShaderParams::Param param;
            int32 texSlotIdx = 2u;

            const char *names[4] = { "vctProbes", "vctProbeX", "vctProbeY", "vctProbeZ" };

            const uint32 numTextureVariables = mAnisotropic ? 4u : 1u;

            for( size_t i = 0u; i < numTextureVariables; ++i )
            {
                param.name = names[i];
                int32 textureUnitsTmp[16];
                for( size_t cascadeIdx = 0u; cascadeIdx < numExtraCascades + 1u; ++cascadeIdx )
                    textureUnitsTmp[cascadeIdx] = texSlotIdx++;
                param.setManualValue( textureUnitsTmp, static_cast<uint32>( numExtraCascades + 1u ) );
                glslShaderParams.mParams.push_back( param );
            }

            glslShaderParams.setDirty();
        }
    }
    //-------------------------------------------------------------------------
    void VctLighting::generateAnisotropicMips()
    {
        RenderSystem *renderSystem = mVoxelizer->getRenderSystem();
        renderSystem->debugAnnotationPush( "VctLighting Anisotropic Mips" );

        HlmsCompute *hlmsCompute = mVoxelizer->getHlmsManager()->getComputeHlms();

        mAnisoGeneratorStep0->analyzeBarriers( mResourceTransitions );
        renderSystem->executeResourceTransition( mResourceTransitions );
        hlmsCompute->dispatch( mAnisoGeneratorStep0, 0, 0 );

        FastArray<HlmsComputeJob *>::const_iterator itor = mAnisoGeneratorStep1.begin();
        FastArray<HlmsComputeJob *>::const_iterator endt = mAnisoGeneratorStep1.end();

        while( itor != endt )
        {
            ( *itor )->analyzeBarriers( mResourceTransitions );
            renderSystem->executeResourceTransition( mResourceTransitions );
            hlmsCompute->dispatch( *itor, 0, 0 );
            ++itor;
        }
        renderSystem->debugAnnotationPop();
    }
    //-------------------------------------------------------------------------
    void VctLighting::runBounce( uint32 bounceIteration )
    {
        RenderSystem *renderSystem = mVoxelizer->getRenderSystem();
        renderSystem->debugAnnotationPush( "VctLighting Bounce" );

        mBounceVoxelCellSize->setManualValue( mVoxelizer->getVoxelCellSize() );
        mBounceInvVoxelResolution->setManualValue( 1.0f / mVoxelizer->getVoxelResolution() );
        // mBounceIterationDampening->setManualValue( 1.0f /
        //                                           ( Math::PI * ( bounceIteration * 0.5f + 1.0f ) ) );
        mBounceIterationDampening->setManualValue( 1.0f / (float)Math::PI );

        const size_t numCascades = mExtraCascades.size() + 1u;

        OGRE_ASSERT_LOW( numCascades < c_maxCascades && "VctLighting: Up to 16 cascades are supported" );

        float4 startBias_invStartBias_cascadeMaxLod[c_maxCascades];
        float4 fromPreviousProbeToNext[c_maxCascades][2];

        for( size_t i = 0u; i < numCascades; ++i )
        {
            const VctLighting *cascade;
            if( i == 0u )
                cascade = this;
            else
                cascade = mExtraCascades[i - 1u];

            const TextureGpu *lightVoxelTexture = cascade->mLightVoxel[0];

            const uint32 width = lightVoxelTexture->getWidth();
            const uint32 height = lightVoxelTexture->getHeight();
            const uint32 depth = lightVoxelTexture->getDepth();

            const float smallestRes = static_cast<float>( std::min( std::min( width, height ), depth ) );
            const float invSmallestRes = 1.0f / smallestRes;

            startBias_invStartBias_cascadeMaxLod[i].x = invSmallestRes;  // startBias
            startBias_invStartBias_cascadeMaxLod[i].y = smallestRes;     // invStartBias

            uint8 cascadeNumMipmaps = 0u;

            if( cascade->mLightVoxel[1] )
            {
                // Anisotropic has the number of mipmaps calculated
                cascadeNumMipmaps = cascade->mLightVoxel[1]->getNumMipmaps();
            }
            else
            {
                cascadeNumMipmaps = cascade->mLightVoxel[0]->getNumMipmaps();
            }

            if( i == numCascades - 1u )
            {
                startBias_invStartBias_cascadeMaxLod[i].z = 256.0f;  // cascadeMaxLod
            }
            else
            {
                const VctLighting *nextCascade = mExtraCascades[i];

                const Vector3 cascadeVoxelCellSize = cascade->mVoxelizer->getVoxelCellSize();
                const Vector3 nextCascadeVoxelCellSize = nextCascade->mVoxelizer->getVoxelCellSize();

                const Vector3 currToNextFactor = nextCascadeVoxelCellSize / cascadeVoxelCellSize;
                const float maxFactor =
                    std::max( currToNextFactor.x, std::max( currToNextFactor.y, currToNextFactor.z ) );

                // cascadeMaxLod
                startBias_invStartBias_cascadeMaxLod[i].z =
                    std::min<float>( Math::Log2( maxFactor ), cascadeNumMipmaps );
            }

            if( i > 0u )
            {
                const VctLighting *prevCascade;
                if( i == 1u )
                    prevCascade = this;
                else
                    prevCascade = mExtraCascades[i - 2u];

                const float cascadeFinalMultiplier = cascade->mMultiplier / this->mMultiplier;

                const Vector3 cascadeVoxelSize = cascade->mVoxelizer->getVoxelSize();
                fromPreviousProbeToNext[i - 1u][0] = Vector4(
                    prevCascade->mVoxelizer->getVoxelSize() / cascadeVoxelSize, cascadeFinalMultiplier );
                fromPreviousProbeToNext[i - 1u][1] =
                    Vector4( ( prevCascade->mVoxelizer->getVoxelOrigin() -
                               cascade->mVoxelizer->getVoxelOrigin() ) /
                                 cascadeVoxelSize,
                             1.0f / cascadeNumMipmaps );
            }
        }

        {
            const int32 numCascadesI32 = static_cast<int32>( numCascades );
            if( mLightVctBounceInject->getProperty( NumVctCascadesProp ) != numCascadesI32 )
                mLightVctBounceInject->setProperty( NumVctCascadesProp, numCascadesI32 );
        }

        mBounceStartBiasInvBiasCascadeMaxLod->setManualValue( &startBias_invStartBias_cascadeMaxLod[0].x,
                                                              static_cast<uint32>( numCascades * 4u ) );
        if( !mExtraCascades.empty() )
        {
            mBounceFromPreviousProbeToNext->setManualValueEx(
                &fromPreviousProbeToNext[0]->x, static_cast<uint32>( ( numCascades - 1u ) * 2u * 4u ) );
        }
        else
        {
            mBounceFromPreviousProbeToNext->setManualValue( 0.0f );
            mBounceFromPreviousProbeToNext->isDirty = false;
        }
        mBounceShaderParams->mParams.swap( mLocalBounceShaderParams );
        mBounceShaderParams->setDirty();

        HlmsCompute *hlmsCompute = mVoxelizer->getHlmsManager()->getComputeHlms();
        mLightVctBounceInject->analyzeBarriers( mResourceTransitions );
        renderSystem->executeResourceTransition( mResourceTransitions );
        hlmsCompute->dispatch( mLightVctBounceInject, 0, 0 );

        std::swap( mLightVoxel[0], mLightBounce );

        DescriptorSetTexture2::TextureSlot texSlot( DescriptorSetTexture2::TextureSlot::makeEmpty() );
        texSlot.texture = mLightVoxel[0];
        mLightVctBounceInject->setTexture( 2, texSlot, mSamplerblockTrilinear );

        DescriptorSetUav::TextureSlot uavSlot( DescriptorSetUav::TextureSlot::makeEmpty() );
        uavSlot.access = ResourceAccess::Write;
        uavSlot.texture = mLightBounce;
        uavSlot.pixelFormat = PFG_RGBA8_UNORM;
        mLightVctBounceInject->_setUavTexture( 0, uavSlot );

        if( mAnisotropic )
        {
            texSlot.texture = mLightVoxel[0];
            mAnisoGeneratorStep0->setTexture( 0, texSlot );

            generateAnisotropicMips();
        }

        if( mLightVoxel[0]->getNumMipmaps() > 1u )
        {
            renderSystem->debugAnnotationPush( "VctLighting::runBounce regular mipmaps" );
            mLightVoxel[0]->_autogenerateMipmaps();
            renderSystem->endCopyEncoder();
            renderSystem->debugAnnotationPop();
        }

        mBounceShaderParams->mParams.swap( mLocalBounceShaderParams );

        renderSystem->debugAnnotationPop();
    }
    //-------------------------------------------------------------------------
    void VctLighting::reserveExtraCascades( size_t numExtraCascades )
    {
        mExtraCascades.reserve( numExtraCascades );
    }
    //-------------------------------------------------------------------------
    void VctLighting::addCascade( VctLighting *cascade ) { mExtraCascades.push_back( cascade ); }
    //-------------------------------------------------------------------------
    void VctLighting::setAllowMultipleBounces( bool bAllowMultipleBounces )
    {
        if( getAllowMultipleBounces() == bAllowMultipleBounces )
            return;

        TextureGpuManager *textureManager = mVoxelizer->getTextureGpuManager();
        if( bAllowMultipleBounces )
        {
            uint32 texFlags = TextureFlags::Uav | TextureFlags::Reinterpretable;
            if( !mAnisotropic || shouldEnableSpecularSdfQuality() )
                texFlags |= TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps;

            char tmpBuffer[128];
            LwString texName( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
            texName.a( "VctLightingBounce/Id", getId() );
            TextureGpu *texture = textureManager->createTexture(
                texName.c_str(), GpuPageOutStrategy::Discard, texFlags, TextureTypes::Type3D );

            texture->setResolution( mLightVoxel[0]->getWidth(), mLightVoxel[0]->getHeight(),
                                    mLightVoxel[0]->getDepth() );
            texture->setNumMipmaps( mLightVoxel[0]->getNumMipmaps() );
            texture->setPixelFormat( PFG_RGBA8_UNORM_SRGB );
            texture->scheduleTransitionTo( GpuResidency::Resident );
            mLightBounce = texture;
        }
        else
        {
            restoreSwappedTextures();
            textureManager->destroyTexture( mLightBounce );
            mLightBounce = 0;
        }

        if( mLightVctBounceInject )
        {
            if( mAnisotropic )
            {
                mLightVctBounceInject->setProperty( "vct_anisotropic", 1 );
                mLightVctBounceInject->setNumTexUnits( 6u );
            }
            else
            {
                mLightVctBounceInject->setProperty( "vct_anisotropic", 0 );
                mLightVctBounceInject->setNumTexUnits( 3u );
            }
        }

        if( bAllowMultipleBounces )
            setupBounceTextures();
        else
            setupGlslTextureUnits();
    }
    //-------------------------------------------------------------------------
    bool VctLighting::getAllowMultipleBounces() const { return mLightBounce != 0; }
    //-------------------------------------------------------------------------
    void VctLighting::setBakingMultiplier( float bakingMult ) { mBakingMultiplier = bakingMult; }
    //-------------------------------------------------------------------------
    void VctLighting::update( SceneManager *sceneManager, uint32 numBounces, float thinWallCounter,
                              bool autoMultiplier, float rayMarchStepScale, uint32 _lightMask )
    {
        OGRE_ASSERT_LOW( rayMarchStepScale >= 1.0f );

        checkTextures();

        RenderSystem *renderSystem = mVoxelizer->getRenderSystem();

        renderSystem->debugAnnotationPush( "VctLighting Update" );

        mLightInjectionJob->setConstBuffer( 0, mLightsConstBuffer );

        DescriptorSetTexture2::TextureSlot texSlot( DescriptorSetTexture2::TextureSlot::makeEmpty() );
        texSlot.texture = mVoxelizer->getAlbedoVox();
        mLightInjectionJob->setTexture( 0, texSlot );
        texSlot.texture = mVoxelizer->getNormalVox();
        mLightInjectionJob->setTexture( 1, texSlot );
        texSlot.texture = mVoxelizer->getEmissiveVox();
        mLightInjectionJob->setTexture( 2, texSlot );

        DescriptorSetUav::TextureSlot uavSlot( DescriptorSetUav::TextureSlot::makeEmpty() );
        uavSlot.access = ResourceAccess::Write;
        uavSlot.texture = mLightVoxel[0];
        uavSlot.pixelFormat = PFG_RGBA8_UNORM;
        mLightInjectionJob->_setUavTexture( 0, uavSlot );

        float autoMultiplierValue = 0.0f;

        const Vector3 voxelOrigin = mVoxelizer->getVoxelOrigin();
        const Vector3 invVoxelRes = 1.0f / mVoxelizer->getVoxelResolution();
        const Vector3 invVoxelSize = 1.0f / mVoxelizer->getVoxelSize();

        ShaderVctLight *RESTRICT_ALIAS vctLight = reinterpret_cast<ShaderVctLight *>(
            mLightsConstBuffer->map( 0, mLightsConstBuffer->getNumElements() ) );
        uint32 numCollectedLights = 0;
        const uint32 maxNumLights =
            static_cast<uint32>( mLightsConstBuffer->getNumElements() / sizeof( ShaderVctLight ) );

        const uint32 lightMask = _lightMask & VisibilityFlags::RESERVED_VISIBILITY_FLAGS;

        ObjectMemoryManager &memoryManager = sceneManager->_getLightMemoryManager();
        const size_t numRenderQueues = memoryManager.getNumRenderQueues();

        for( size_t i = 0; i < numRenderQueues; ++i )
        {
            ObjectData objData;
            const size_t totalObjs = memoryManager.getFirstObjectData( objData, i );

            for( size_t j = 0; j < totalObjs && numCollectedLights < maxNumLights;
                 j += ARRAY_PACKED_REALS )
            {
                for( size_t k = 0; k < ARRAY_PACKED_REALS && numCollectedLights < maxNumLights; ++k )
                {
                    uint32 *RESTRICT_ALIAS visibilityFlags = objData.mVisibilityFlags;

                    if( visibilityFlags[k] & VisibilityFlags::LAYER_VISIBILITY &&
                        visibilityFlags[k] & lightMask )
                    {
                        Light *light = static_cast<Light *>( objData.mOwner[k] );
                        if( light->getType() == Light::LT_DIRECTIONAL ||
                            light->getType() == Light::LT_POINT ||
                            light->getType() == Light::LT_SPOTLIGHT ||
                            light->getType() == Light::LT_AREA_APPROX ||
                            light->getType() == Light::LT_AREA_LTC )
                        {
                            const float maxVal = addLight( vctLight, light, voxelOrigin, invVoxelSize );
                            autoMultiplierValue = std::max( autoMultiplierValue, maxVal );
                            ++vctLight;
                            ++numCollectedLights;
                        }
                    }
                }

                objData.advancePack();
            }
        }

        mLightsConstBuffer->unmap( UO_KEEP_PERSISTENT );

        autoMultiplierValue /= Math::PI;
        autoMultiplierValue = 1.0f / autoMultiplierValue;
        if( !autoMultiplier )
            autoMultiplierValue = mBakingMultiplier;
        mInvBakingMultiplier = 1.0f / autoMultiplierValue;

        const Vector3 voxelRes( Real( mLightVoxel[0]->getWidth() ), Real( mLightVoxel[0]->getHeight() ),
                                Real( mLightVoxel[0]->getDepth() ) );
        const Vector3 voxelCellSize( mVoxelizer->getVoxelCellSize() );

        Vector3 dirCorrection( 1.0f / voxelCellSize );
        dirCorrection /= std::max( std::max( fabsf( dirCorrection.x ), fabsf( dirCorrection.y ) ),
                                   fabsf( dirCorrection.z ) );

        mNumLights->setManualValue( numCollectedLights );
        mRayMarchStepSize->setManualValue(
            Vector4( rayMarchStepScale / voxelRes, autoMultiplierValue ) );
        mVoxelCellSize->setManualValue( voxelCellSize );
        mDirCorrectionRatioThinWallCounter->setManualValue( Vector4( dirCorrection, thinWallCounter ) );
        mInvVoxelResolution->setManualValue( invVoxelRes );
        mShaderParams->setDirty();

        renderSystem->endCopyEncoder();

        HlmsCompute *hlmsCompute = mVoxelizer->getHlmsManager()->getComputeHlms();
        mLightInjectionJob->analyzeBarriers( mResourceTransitions );
        renderSystem->executeResourceTransition( mResourceTransitions );
        hlmsCompute->dispatch( mLightInjectionJob, 0, 0 );

        if( mAnisotropic )
            generateAnisotropicMips();

        if( mLightVoxel[0]->getNumMipmaps() > 1u )
        {
            renderSystem->debugAnnotationPush( "VctLighting::update regular mipmaps" );
            mLightVoxel[0]->_autogenerateMipmaps();
            renderSystem->endCopyEncoder();
            renderSystem->debugAnnotationPop();
        }

        if( numBounces > 0u )
        {
            if( !getAllowMultipleBounces() )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "numBounces must be 0, else call setAllowMultipleBounces first!",
                             "VctLighting::update" );
            }
            for( uint32 i = 0u; i < numBounces; ++i )
                runBounce( i );
        }

        if( mDebugVoxelVisualizer )
            mDebugVoxelVisualizer->setTrackingVoxel( mLightVoxel[0], mLightVoxel[0], true );

        renderSystem->debugAnnotationPop();
    }
    //-------------------------------------------------------------------------
    bool VctLighting::needsAmbientHemisphere() const
    {
        return memcmp( mUpperHemisphere, mLowerHemisphere, sizeof( mUpperHemisphere ) ) != 0;
    }
    //-------------------------------------------------------------------------
    void VctLighting::resetTexturesFromBuildRelative()
    {
        if( mDebugVoxelVisualizer )
        {
            Node *visNode = mDebugVoxelVisualizer->getParentNode();
            visNode->setPosition( mVoxelizer->getVoxelOrigin() );
            visNode->setScale( mVoxelizer->getVoxelCellSize() );

            // The visualizer is static so force-update its transform manually
            visNode->_getFullTransformUpdated();
            mDebugVoxelVisualizer->getWorldAabbUpdated();
        }

        if( mVoxelizerTexturesChanged )
        {
            checkTextures();
            return;
        }

        if( getAllowMultipleBounces() )
            setupBounceTextures();

        if( mAnisotropic )
        {
            DescriptorSetTexture2::TextureSlot texSlot(
                DescriptorSetTexture2::TextureSlot::makeEmpty() );
            texSlot.texture = mVoxelizer->getNormalVox();
            mAnisoGeneratorStep0->setTexture( 1, texSlot );
        }
    }
    //-------------------------------------------------------------------------
    size_t VctLighting::getConstBufferSize() const
    {
        size_t retVal = 10u * 4u * sizeof( float );
        retVal += ( 4u + 4u * 2u ) * sizeof( float ) * mExtraCascades.size();
        return retVal;
    }
    //-------------------------------------------------------------------------
    void VctLighting::fillConstBufferData( const Matrix4 &viewMatrix,
                                           float *RESTRICT_ALIAS passBufferPtr ) const
    {
        const uint32 width = mLightVoxel[0]->getWidth();
        const uint32 height = mLightVoxel[0]->getHeight();
        const uint32 depth = mLightVoxel[0]->getDepth();

        const float smallestRes = static_cast<float>( std::min( std::min( width, height ), depth ) );

        const float maxMipmapCount = static_cast<float>(
            PixelFormatGpuUtils::getMaxMipmapCount( static_cast<uint32>( smallestRes ) ) );

        const float mipDiff = ( maxMipmapCount - 8.0f ) * 0.5f;

        const float finalMultiplier = mInvBakingMultiplier * mMultiplier;
        const float invFinalMultiplier = 1.0f / finalMultiplier;

        const size_t numCascades = mExtraCascades.size() + 1u;

        // float4 vctInvResolution_cascadeMaxLod;
        for( size_t i = 0u; i < numCascades; ++i )
        {
            const VctLighting *cascade;
            if( i == 0u )
                cascade = this;
            else
                cascade = mExtraCascades[i - 1u];

            const TextureGpu *cascadeLightVoxel = cascade->mLightVoxel[0];
            const uint32 widthCascade = cascadeLightVoxel->getWidth();
            const uint32 heightCascade = cascadeLightVoxel->getHeight();
            const uint32 depthCascade = cascadeLightVoxel->getDepth();

            uint8 cascadeNumMipmaps = 0u;

            if( cascade->mLightVoxel[1] )
            {
                // Anisotropic has the number of mipmaps calculated
                cascadeNumMipmaps = cascade->mLightVoxel[1]->getNumMipmaps();
            }
            else
            {
                cascadeNumMipmaps = cascadeLightVoxel->getNumMipmaps();
            }

            *passBufferPtr++ = 1.0f / static_cast<float>( widthCascade );
            *passBufferPtr++ = 1.0f / static_cast<float>( heightCascade );
            *passBufferPtr++ = 1.0f / static_cast<float>( depthCascade );
            if( i == numCascades - 1u )
            {
                *passBufferPtr++ = 256.0f;  // cascadeMaxLod
            }
            else
            {
                const VctLighting *nextCascade = mExtraCascades[i];

                const Vector3 cascadeVoxelCellSize = cascade->mVoxelizer->getVoxelCellSize();
                const Vector3 nextCascadeVoxelCellSize = nextCascade->mVoxelizer->getVoxelCellSize();

                const Vector3 currToNextFactor = nextCascadeVoxelCellSize / cascadeVoxelCellSize;
                const float maxFactor =
                    std::max( currToNextFactor.x, std::max( currToNextFactor.y, currToNextFactor.z ) );

                *passBufferPtr++ =
                    std::min<float>( Math::Log2( maxFactor ), cascadeNumMipmaps );  // cascadeMaxLod
            }
        }

        // float4 fromPreviousProbeToNext[numCascades - 1u][2]
        for( size_t i = 1u; i < numCascades; ++i )
        {
            const VctLighting *cascade = mExtraCascades[i - 1u];
            const VctLighting *prevCascade;
            if( i == 1u )
                prevCascade = this;
            else
                prevCascade = mExtraCascades[i - 2u];

            const Vector3 cascadeVoxelSize = cascade->mVoxelizer->getVoxelSize();
            const Vector3 vScale = prevCascade->mVoxelizer->getVoxelSize() / cascadeVoxelSize;
            const Vector3 vPos =
                ( prevCascade->mVoxelizer->getVoxelOrigin() - cascade->mVoxelizer->getVoxelOrigin() ) /
                cascadeVoxelSize;

            const float cascadeFinalMultiplier =
                cascade->mInvBakingMultiplier * cascade->mMultiplier / finalMultiplier;

            float cascadeNumMipmaps = 0u;

            if( cascade->mLightVoxel[1] )
            {
                // Anisotropic has the number of mipmaps calculated
                cascadeNumMipmaps = static_cast<float>( cascade->mLightVoxel[1]->getNumMipmaps() );
            }
            else
            {
                const TextureGpu *cascadeLightVoxel = cascade->mLightVoxel[0];
                cascadeNumMipmaps = static_cast<float>( cascadeLightVoxel->getNumMipmaps() );
            }

            *passBufferPtr++ = static_cast<float>( vScale.x );
            *passBufferPtr++ = static_cast<float>( vScale.y );
            *passBufferPtr++ = static_cast<float>( vScale.z );
            *passBufferPtr++ = cascadeFinalMultiplier;

            *passBufferPtr++ = static_cast<float>( vPos.x );
            *passBufferPtr++ = static_cast<float>( vPos.y );
            *passBufferPtr++ = static_cast<float>( vPos.z );
            // HACK: This is so hacky it hurts: cascadeNumMipmaps^3 empirically looks reasonably
            // good for brightness. We need a better way to equalize specular. Specular
            // brightness equalization depends on:
            //      - Roughness (as it affects lighting)
            //      - Cell Size Volume
            *passBufferPtr++ = 1.0f / ( cascadeNumMipmaps * cascadeNumMipmaps * cascadeNumMipmaps );
        }

        // float specSdfMaxMip;
        // float specularSdfFactor;
        // float blendFade;
        // float multiplier;
        *passBufferPtr++ = 7.0f + mipDiff;
        // Where did 0.1875f & 0.3125f come from? Empirically obtained.
        // At 128x128x128, values in range [24; 40] gave good results.
        // Below 24, quality became unnacceptable.
        // Past 40, performance only went down without visible changes.
        // Thus 24 / 128 and 40 / 128 = 0.1875f and 0.3125f
        *passBufferPtr++ = Math::lerp( 0.1875f, 0.3125f, mSpecularSdfQuality ) * smallestRes;
        *passBufferPtr++ = 1.0f;
        *passBufferPtr++ = finalMultiplier;

        // float4 ambientUpperHemi
        *passBufferPtr++ = mUpperHemisphere[0] * invFinalMultiplier;
        *passBufferPtr++ = mUpperHemisphere[1] * invFinalMultiplier;
        *passBufferPtr++ = mUpperHemisphere[2] * invFinalMultiplier;
        *passBufferPtr++ = 0.0f;

        // float4 ambientLowerHemi
        *passBufferPtr++ = mLowerHemisphere[0] * invFinalMultiplier;
        *passBufferPtr++ = mLowerHemisphere[1] * invFinalMultiplier;
        *passBufferPtr++ = mLowerHemisphere[2] * invFinalMultiplier;
        *passBufferPtr++ = 0.0f;

        Matrix4 xform, invXForm;
        xform.makeTransform( -mVoxelizer->getVoxelOrigin() / mVoxelizer->getVoxelSize(),
                             1.0f / mVoxelizer->getVoxelSize(), Quaternion::IDENTITY );
        // xform = xform * viewMatrix.inverse();
        xform = xform.concatenateAffine( viewMatrix.inverseAffine() );
        invXForm = xform.inverseAffine();

        // float4 xform_row0;
        // float4 xform_row1;
        // float4 xform_row2;
        for( size_t i = 0; i < 12u; ++i )
            *passBufferPtr++ = static_cast<float>( xform[0][i] );

        // float4 invXform_row0;
        // float4 invXform_row1;
        // float4 invXform_row2;
        for( size_t i = 0; i < 12u; ++i )
            *passBufferPtr++ = static_cast<float>( invXForm[0][i] );
    }
    //-------------------------------------------------------------------------
    bool VctLighting::shouldEnableSpecularSdfQuality() const
    {
        return mVoxelizer->getAlbedoVox()->getWidth() > 32u &&
               mVoxelizer->getAlbedoVox()->getHeight() > 32u &&
               mVoxelizer->getAlbedoVox()->getDepth() > 32u;
    }
    //-------------------------------------------------------------------------
    void VctLighting::setDebugVisualization( bool bShow, SceneManager *sceneManager )
    {
        if( bShow == getDebugVisualizationMode() )
            return;

        if( !bShow )
        {
            SceneNode *sceneNode = mDebugVoxelVisualizer->getParentSceneNode();
            sceneNode->getParentSceneNode()->removeAndDestroyChild( sceneNode );
            OGRE_DELETE mDebugVoxelVisualizer;
            mDebugVoxelVisualizer = 0;
        }
        else
        {
            SceneNode *rootNode = sceneManager->getRootSceneNode( SCENE_STATIC );
            SceneNode *visNode = rootNode->createChildSceneNode( SCENE_STATIC );

            mDebugVoxelVisualizer = OGRE_NEW VoxelVisualizer(
                Ogre::Id::generateNewId<Ogre::MovableObject>(),
                &sceneManager->_getEntityMemoryManager( SCENE_STATIC ), sceneManager, 0u );

            mDebugVoxelVisualizer->setTrackingVoxel( mLightVoxel[0], mLightVoxel[0], true );

            visNode->setPosition( mVoxelizer->getVoxelOrigin() );
            visNode->setScale( mVoxelizer->getVoxelCellSize() );
            visNode->attachObject( mDebugVoxelVisualizer );
        }
    }
    //-------------------------------------------------------------------------
    bool VctLighting::getDebugVisualizationMode() const { return mDebugVoxelVisualizer != 0; }
    //-------------------------------------------------------------------------
    void VctLighting::setAnisotropic( bool bAnisotropic )
    {
        if( mAnisotropic != bAnisotropic )
        {
            mAnisotropic = bAnisotropic;
            createTextures();
        }
    }
    //-------------------------------------------------------------------------
    void VctLighting::setAmbient( const ColourValue &upperHemisphere,
                                  const ColourValue &lowerHemisphere )
    {
        for( size_t i = 0; i < 3u; ++i )
        {
            mUpperHemisphere[i] = static_cast<float>( upperHemisphere[i] );
            mLowerHemisphere[i] = static_cast<float>( lowerHemisphere[i] );
        }
    }
    //-------------------------------------------------------------------------
    TextureGpu **VctLighting::getLightVoxelTextures( const size_t cascadeIdx )
    {
        if( cascadeIdx == 0u )
            return mLightVoxel;
        else
            return mExtraCascades[cascadeIdx - 1u]->mLightVoxel;
    }
    //-------------------------------------------------------------------------
    void VctLighting::notifyTextureChanged( TextureGpu *texture, TextureGpuListener::Reason reason,
                                            void *extraData )
    {
        if( reason == TextureGpuListener::LostResidency || reason == TextureGpuListener::Deleted )
            mVoxelizerTexturesChanged = true;

        if( reason == TextureGpuListener::Deleted )
        {
            texture->removeListener( this );
            mVoxelizerListenersRemoved = true;
        }
    }
}  // namespace Ogre
