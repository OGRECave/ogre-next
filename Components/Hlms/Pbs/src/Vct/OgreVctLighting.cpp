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

#include "Vct/OgreVctLighting.h"
#include "Vct/OgreVctVoxelizer.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"

#include "OgreTextureGpuManager.h"
#include "OgreStringConverter.h"

#include "OgrePixelFormatGpuUtils.h"
#include "OgreRenderSystem.h"
#include "OgreSceneManager.h"

#include "OgreLight.h"
#include "Vao/OgreConstBufferPacked.h"

#include "OgreLwString.h"

#define TODO_memory_barrier

namespace Ogre
{
    struct ShaderVctLight
    {
        //Pre-mul by PI? -No because we lose a ton of precision
        //.w contains lightDistThreshold
        float diffuse[4];
        //For directional lights, pos.xyz contains -dir.xyz and pos.w = 0;
        //For the rest of lights, pos.xyz contains pos.xyz and pos.w = 1;
        float pos[4];
        float uvwPos[4];
    //	float4 dir;
    };

    const uint16 VctLighting::msDistanceThresholdCustomParam = 3876u;

    VctLighting::VctLighting( IdType id, VctVoxelizer *voxelizer, bool bAnisotropic ) :
        IdObject( id ),
        mSamplerblockTrilinear( 0 ),
        mVoxelizer( voxelizer ),
        mLightInjectionJob( 0 ),
        mLightsConstBuffer( 0 ),
        mAnisoGeneratorStep0( 0 ),
        mDefaultLightDistThreshold( 0.5f ),
        mAnisotropic( bAnisotropic ),
        mNumLights( 0 ),
        mRayMarchStepSize( 0 ),
        mVoxelCellSize( 0 ),
        mInvVoxelResolution( 0 ),
        mShaderParams( 0 ),
        mNormalBias( 0.25f )
    {
        memset( mLightVoxel, 0, sizeof(mLightVoxel) );

        createTextures();

        //VctVoxelizer should've already been initialized, thus no need
        //to check if JSON has been built or if the assets were added
        HlmsCompute *hlmsCompute = mVoxelizer->getHlmsManager()->getComputeHlms();
        mLightInjectionJob = hlmsCompute->findComputeJob( "VCT/LightInjection" );

        mShaderParams = &mLightInjectionJob->getShaderParams( "default" );
        mNumLights = mShaderParams->findParameter( "numLights" );
        mRayMarchStepSize = mShaderParams->findParameter( "rayMarchStepSize" );
        mVoxelCellSize = mShaderParams->findParameter( "voxelCellSize" );
        mInvVoxelResolution = mShaderParams->findParameter( "invVoxelResolution" );

        RenderSystem *renderSystem = mVoxelizer->getRenderSystem();
        VaoManager *vaoManager = renderSystem->getVaoManager();
        mLightsConstBuffer = vaoManager->createConstBuffer( sizeof(ShaderVctLight) * 16u,
                                                            BT_DYNAMIC_PERSISTENT, 0, false );

        HlmsManager *hlmsManager = mVoxelizer->getHlmsManager();
        HlmsSamplerblock samplerblock;
        samplerblock.mMipFilter = FO_LINEAR;
        mSamplerblockTrilinear = hlmsManager->getSamplerblock( samplerblock );
    }
    //-------------------------------------------------------------------------
    VctLighting::~VctLighting()
    {
        RenderSystem *renderSystem = mVoxelizer->getRenderSystem();
        VaoManager *vaoManager = renderSystem->getVaoManager();

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

        destroyTextures();
    }
    //-------------------------------------------------------------------------
    void VctLighting::createBarriers(void)
    {
        RenderSystem *renderSystem = mVoxelizer->getRenderSystem();

        if( renderSystem->getCapabilities()->hasCapability( RSC_EXPLICIT_API ) )
        {
            //If calling every frame, left resources in texture mode
            mStartupTrans.oldLayout = ResourceLayout::Texture;
            mStartupTrans.newLayout = ResourceLayout::Uav;
            mStartupTrans.writeBarrierBits = 0;
            mStartupTrans.readBarrierBits  = ReadBarrier::Uav;
            renderSystem->_resourceTransitionCreated( &mStartupTrans );
        }

        //We're done writing to mLightVoxel[0], we will only sample from it as texture
        mPrepareForSamplingTrans.oldLayout = ResourceLayout::Uav;
        mPrepareForSamplingTrans.newLayout = ResourceLayout::Texture;
        mPrepareForSamplingTrans.writeBarrierBits = WriteBarrier::Uav;
        mPrepareForSamplingTrans.readBarrierBits  = ReadBarrier::Texture;
        renderSystem->_resourceTransitionCreated( &mPrepareForSamplingTrans );

        //We're done writing to mLightVoxel[i] MIPS, we will only sample from them as textures
        mAfterAnisoMip0Trans.oldLayout = ResourceLayout::Uav;
        mAfterAnisoMip0Trans.newLayout = ResourceLayout::Texture;
        mAfterAnisoMip0Trans.writeBarrierBits = WriteBarrier::Uav;
        mAfterAnisoMip0Trans.readBarrierBits  = ReadBarrier::Texture;
        renderSystem->_resourceTransitionCreated( &mAfterAnisoMip0Trans );

        //We're done writing to mLightVoxel[i] MIPS, we will only sample from them as textures
        mAfterAnisoMip1Trans.oldLayout = ResourceLayout::Uav;
        mAfterAnisoMip1Trans.newLayout = ResourceLayout::Texture;
        mAfterAnisoMip1Trans.writeBarrierBits = WriteBarrier::Uav;
        mAfterAnisoMip1Trans.readBarrierBits  = ReadBarrier::Texture;
        renderSystem->_resourceTransitionCreated( &mAfterAnisoMip1Trans );
    }
    //-------------------------------------------------------------------------
    void VctLighting::destroyBarriers(void)
    {
        RenderSystem *renderSystem = mVoxelizer->getRenderSystem();
        if( renderSystem->getCapabilities()->hasCapability( RSC_EXPLICIT_API ) )
            renderSystem->_resourceTransitionDestroyed( &mStartupTrans );
        renderSystem->_resourceTransitionDestroyed( &mPrepareForSamplingTrans );
        renderSystem->_resourceTransitionDestroyed( &mAfterAnisoMip0Trans );
        renderSystem->_resourceTransitionDestroyed( &mAfterAnisoMip1Trans );
    }
    //-------------------------------------------------------------------------
    void VctLighting::addLight( ShaderVctLight * RESTRICT_ALIAS vctLight, Light *light,
                                const Vector3 &voxelOrigin, const Vector3 &invVoxelRes )
    {
        const ColourValue diffuseColour = light->getDiffuseColour() * light->getPowerScale();
        for( size_t i=0; i<3u; ++i )
            vctLight->diffuse[i] = static_cast<float>( diffuseColour[i] );

        const Vector4 *lightDistThreshold =
                light->getCustomParameterNoThrow( msDistanceThresholdCustomParam );
        vctLight->diffuse[3] = lightDistThreshold ?
                                   (lightDistThreshold->x * lightDistThreshold->x) :
                                   (mDefaultLightDistThreshold * mDefaultLightDistThreshold);

        Vector4 light4dVec = light->getAs4DVector();
        if( light->getType() != Light::LT_DIRECTIONAL )
            light4dVec -= Vector4( voxelOrigin, 0.0f );

        for( size_t i=0; i<4u; ++i )
            vctLight->pos[i] = static_cast<float>( light4dVec[i] );

        Vector3 uvwPos = light->getParentNode()->_getDerivedPosition();
        uvwPos = (uvwPos - voxelOrigin) * invVoxelRes;
        for( size_t i=0; i<3u; ++i )
            vctLight->uvwPos[i] = static_cast<float>( uvwPos[i] );
        vctLight->uvwPos[3] = 0;
    }
    //-------------------------------------------------------------------------
    void VctLighting::createTextures()
    {
        destroyTextures();

        TextureGpuManager *textureManager = mVoxelizer->getTextureGpuManager();

        RenderSystem *renderSystem = mVoxelizer->getRenderSystem();
        const bool hasTypedUavs = renderSystem->getCapabilities()->hasCapability( RSC_TYPED_UAV_LOADS );

        uint32 texFlags = TextureFlags::Uav;
        if( !hasTypedUavs )
            texFlags |= TextureFlags::Reinterpretable;

        if( !mAnisotropic )
            texFlags |= TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps;

        const TextureGpu *albedoVox = mVoxelizer->getAlbedoVox();

        const uint32 width  = albedoVox->getWidth();
        const uint32 height = albedoVox->getHeight();
        const uint32 depth  = albedoVox->getDepth();

        const uint32 widthAniso     = std::max( 1u, width );
        const uint32 heightAniso    = std::max( 1u, height >> 1u );
        const uint32 depthAniso     = std::max( 1u, depth >> 1u );

        const uint8 numMipsMain  =
                mAnisotropic ? 1u : PixelFormatGpuUtils::getMaxMipmapCount( width, height, depth );
        //numMipsAniso needs one less mip; because the last mip must be 2x1x1, not 1x1x1
        const uint8 numMipsAniso =
                PixelFormatGpuUtils::getMaxMipmapCount( widthAniso, heightAniso, depthAniso ) - 1u;

        const size_t numTextures = mAnisotropic ? 4u : 1u;

        const char *names[] =
        {
            "Main",
            "X_axis",
            "Y_axis",
            "Z_axis"
        };

        for( size_t i=0; i<numTextures; ++i )
        {
            char tmpBuffer[128];
            LwString texName( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );
            texName.a( "VctLighting_", names[i], "/Id", getId() );
            TextureGpu *texture = textureManager->createTexture( texName.c_str(),
                                                                 GpuPageOutStrategy::Discard,
                                                                 texFlags, TextureTypes::Type3D );
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
            texture->setPixelFormat( PFG_RGBA8_UNORM );
            texture->scheduleTransitionTo( GpuResidency::Resident );
            mLightVoxel[i] = texture;
        }

        if( mAnisotropic )
        {
            //Setup the compute shaders for VctLighting::generateAnisotropicMips()
            HlmsCompute *hlmsCompute = mVoxelizer->getHlmsManager()->getComputeHlms();
            mAnisoGeneratorStep0 = hlmsCompute->findComputeJob( "VCT/AnisotropicMipStep0" );

            char tmpBuffer[128];
            LwString jobName( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

            //Step 0
            jobName.clear();
            jobName.a( "VCT/AnisotropicMipStep0/Id", getId() );
            mAnisoGeneratorStep0 = mAnisoGeneratorStep0->clone( jobName.c_str() );

            for( uint8 i=0; i<3u; ++i )
            {
                DescriptorSetUav::TextureSlot uavSlot( DescriptorSetUav::TextureSlot::makeEmpty() );
                uavSlot.access = ResourceAccess::Write;
                uavSlot.texture = mLightVoxel[i+1u];
                mAnisoGeneratorStep0->_setUavTexture( i, uavSlot );
            }

            DescriptorSetTexture2::TextureSlot texSlot( DescriptorSetTexture2::
                                                        TextureSlot::makeEmpty() );
            texSlot.texture = mLightVoxel[0];
            mAnisoGeneratorStep0->setTexture( 0, texSlot );
            texSlot.texture = mVoxelizer->getNormalVox();
            mAnisoGeneratorStep0->setTexture( 1, texSlot );

            ShaderParams *shaderParams = &mAnisoGeneratorStep0->getShaderParams( "default" );
            //higherMipHalfWidth
            ShaderParams::Param *lowerMipResolutionParam = &shaderParams->mParams.back();
            //int32 resolution[4] = { static_cast<int32>( mLightVoxel[1]->getWidth() >> 1u ) };
            lowerMipResolutionParam->setManualValue( static_cast<int32>(
                                                         mLightVoxel[1]->getWidth() >> 1u ) );
            shaderParams->setDirty();

            //Now setup step 1
            //numMipsOnStep1 is subtracted one because mip 0 got processed by step 0
            const uint8 numMipsOnStep1 = mLightVoxel[1]->getNumMipmaps() - 1u;
            mAnisoGeneratorStep1.resize( numMipsOnStep1 );

            HlmsComputeJob *baseJob = hlmsCompute->findComputeJob( "VCT/AnisotropicMipStep1" );

            for( uint8 i=0; i<numMipsOnStep1; ++i )
            {
                jobName.clear();
                jobName.a( "VCT/AnisotropicMipStep1/Id", getId(), "/Mip", i + 1u );
                HlmsComputeJob *mipJob = baseJob->clone( jobName.c_str() );

                for( uint8 axis=0; axis<3u; ++axis )
                {
                    texSlot.texture = mLightVoxel[axis+1u];
                    texSlot.mipmapLevel = i;
                    texSlot.numMipmaps = 1u;
                    mipJob->setTexture( axis, texSlot );

                    DescriptorSetUav::TextureSlot uavSlot( DescriptorSetUav::TextureSlot::makeEmpty() );
                    uavSlot.access = ResourceAccess::Write;
                    uavSlot.texture = mLightVoxel[axis+1u];
                    uavSlot.mipmapLevel = i + 1u;
                    mipJob->_setUavTexture( axis, uavSlot );
                }

                shaderParams = &mipJob->getShaderParams( "default" );
                //higherMipHalfRes_lowerMipHalfWidth
                lowerMipResolutionParam = &shaderParams->mParams.back();
                int32 resolutions[4] =
                {
                    static_cast<int32>( mLightVoxel[1]->getWidth() >> (i + 2u) ),
                    static_cast<int32>( mLightVoxel[1]->getHeight()>> (i + 1u) ),
                    static_cast<int32>( mLightVoxel[1]->getDepth() >> (i + 1u) ),
                    static_cast<int32>( mLightVoxel[1]->getWidth() >> (i + 1u) )
                };
                for( size_t j=0; j<4u; ++j )
                    resolutions[j] = std::max( 1, resolutions[j] );
                lowerMipResolutionParam->setManualValue( resolutions, 4u );
                shaderParams->setDirty();

                mAnisoGeneratorStep1[i] = mipJob;
            }
        }

        createBarriers();
    }
    //-------------------------------------------------------------------------
    void VctLighting::destroyTextures()
    {
        destroyBarriers();

        TextureGpuManager *textureManager = mVoxelizer->getTextureGpuManager();
        for( size_t i=0; i<sizeof(mLightVoxel) / sizeof(mLightVoxel[0]); ++i )
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

        FastArray<HlmsComputeJob*>::const_iterator itor = mAnisoGeneratorStep1.begin();
        FastArray<HlmsComputeJob*>::const_iterator end  = mAnisoGeneratorStep1.end();

        while( itor != end )
        {
            hlmsCompute->destroyComputeJob( (*itor)->getName() );
            ++itor;
        }

        mAnisoGeneratorStep1.clear();
    }
    //-------------------------------------------------------------------------
    void VctLighting::generateAnisotropicMips()
    {
        RenderSystem *renderSystem = mVoxelizer->getRenderSystem();
        HlmsCompute *hlmsCompute = mVoxelizer->getHlmsManager()->getComputeHlms();
        hlmsCompute->dispatch( mAnisoGeneratorStep0, 0, 0 );

        renderSystem->_executeResourceTransition( &mAfterAnisoMip0Trans );

        TODO_memory_barrier;

        FastArray<HlmsComputeJob*>::const_iterator itor = mAnisoGeneratorStep1.begin();
        FastArray<HlmsComputeJob*>::const_iterator end  = mAnisoGeneratorStep1.end();

        while( itor != end )
        {
            hlmsCompute->dispatch( *itor, 0, 0 );
            renderSystem->_executeResourceTransition( &mAfterAnisoMip1Trans );
            ++itor;
        }
    }
    //-------------------------------------------------------------------------
    void VctLighting::update( SceneManager *sceneManager, float rayMarchStepScale, uint32 _lightMask )
    {
        OGRE_ASSERT_LOW( rayMarchStepScale >= 1.0f );

        RenderSystem *renderSystem = mVoxelizer->getRenderSystem();
        if( renderSystem->getCapabilities()->hasCapability( RSC_EXPLICIT_API ) )
            renderSystem->_resourceTransitionCreated( &mStartupTrans );

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
        mLightInjectionJob->_setUavTexture( 0, uavSlot );

        const Vector3 voxelOrigin   = mVoxelizer->getVoxelOrigin();
        const Vector3 invVoxelRes   = 1.0f / mVoxelizer->getVoxelResolution();

        ShaderVctLight * RESTRICT_ALIAS vctLight =
                reinterpret_cast<ShaderVctLight*>(
                    mLightsConstBuffer->map( 0, mLightsConstBuffer->getNumElements() ) );
        uint32 numCollectedLights = 0;
        const uint32 maxNumLights = static_cast<uint32>( mLightsConstBuffer->getNumElements() /
                                                         sizeof(ShaderVctLight) );

        const uint32 lightMask = _lightMask & VisibilityFlags::RESERVED_VISIBILITY_FLAGS;

        ObjectMemoryManager &memoryManager = sceneManager->_getLightMemoryManager();
        const size_t numRenderQueues = memoryManager.getNumRenderQueues();

        for( size_t i=0; i<numRenderQueues; ++i )
        {
            ObjectData objData;
            const size_t totalObjs = memoryManager.getFirstObjectData( objData, i );

            for( size_t j=0; j<totalObjs && numCollectedLights < maxNumLights; j += ARRAY_PACKED_REALS )
            {
                for( size_t k=0; k<ARRAY_PACKED_REALS && numCollectedLights < maxNumLights; ++k )
                {
                    uint32 * RESTRICT_ALIAS visibilityFlags = objData.mVisibilityFlags;

                    if( visibilityFlags[k] & VisibilityFlags::LAYER_VISIBILITY &&
                        visibilityFlags[k] & lightMask )
                    {
                        Light *light = static_cast<Light*>( objData.mOwner[k] );
                        if( light->getType() == Light::LT_DIRECTIONAL ||
                            light->getType() == Light::LT_POINT )
                        {
                            addLight( vctLight, light, voxelOrigin, invVoxelRes );
                            ++vctLight;
                            ++numCollectedLights;
                        }
                    }
                }

                objData.advancePack();
            }
        }

        mLightsConstBuffer->unmap( UO_KEEP_PERSISTENT );

        const Vector3 voxelRes( mLightVoxel[0]->getWidth(), mLightVoxel[0]->getHeight(),
                                mLightVoxel[0]->getDepth() );

        mNumLights->setManualValue( numCollectedLights );
        mRayMarchStepSize->setManualValue( 1.0f / voxelRes );
        mVoxelCellSize->setManualValue( mVoxelizer->getVoxelSize() );
        mInvVoxelResolution->setManualValue( invVoxelRes );
        mShaderParams->setDirty();

        HlmsCompute *hlmsCompute = mVoxelizer->getHlmsManager()->getComputeHlms();
        hlmsCompute->dispatch( mLightInjectionJob, 0, 0 );

        renderSystem->_executeResourceTransition( &mPrepareForSamplingTrans );

        if( mAnisotropic )
            generateAnisotropicMips();
        else
            mLightVoxel[0]->_autogenerateMipmaps();
    }
    //-------------------------------------------------------------------------
    size_t VctLighting::getConstBufferSize(void) const
    {
        return 5u * 4u * sizeof(float);
    }
    //-------------------------------------------------------------------------
    void VctLighting::fillConstBufferData( const Matrix4 &viewMatrix,
                                           float * RESTRICT_ALIAS passBufferPtr ) const
    {
        const uint32 width  = mLightVoxel[0]->getWidth();
        const uint32 height = mLightVoxel[0]->getHeight();
        const uint32 depth  = mLightVoxel[0]->getDepth();

        const float smallestRes = static_cast<float>( std::min( std::min( width, height ), depth ) );
        const float invSmallestRes = 1.0f / smallestRes;

        //float4 startBias_invStartBias_maxDistance_blendAmbient;
        *passBufferPtr++ = invSmallestRes;
        *passBufferPtr++ = smallestRes;
        *passBufferPtr++ = 1.414213562f;
        *passBufferPtr++ = 0.0f;

        //float4 normalBias_blendFade_unused2;
        *passBufferPtr++ = mNormalBias;
        *passBufferPtr++ = 1.0f;
        *passBufferPtr++ = 0.0f;
        *passBufferPtr++ = 0.0f;

        Matrix4 xform;
        xform.makeTransform( -mVoxelizer->getVoxelOrigin() / mVoxelizer->getVoxelSize(),
                             1.0f / mVoxelizer->getVoxelSize(),
                             Quaternion::IDENTITY );
        //xform = xform * viewMatrix.inverse();
        xform = xform.concatenateAffine( viewMatrix.inverseAffine() );

        //float4 xform_row0;
        //float4 xform_row1;
        //float4 xform_row2;
        for( size_t i=0; i<12u; ++i )
            *passBufferPtr++ = static_cast<float>( xform[0][i] );
    }
    //-------------------------------------------------------------------------
    void VctLighting::setAnisotropic( bool bAnisotropic )
    {
        if( mAnisotropic != bAnisotropic )
        {
            mAnisotropic = bAnisotropic;
            createTextures();
        }
    }
}
