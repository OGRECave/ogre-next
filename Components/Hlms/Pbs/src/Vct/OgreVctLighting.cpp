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

    VctLighting::VctLighting( IdType id, VctVoxelizer *voxelizer ) :
        IdObject( id ),
        mSamplerblockTrilinear( 0 ),
        mVoxelizer( voxelizer ),
        mLightInjectionJob( 0 ),
        mLightsConstBuffer( 0 ),
        mDefaultLightDistThreshold( 0.5f ),
        mNumLights( 0 ),
        mRayMarchStepSize( 0 ),
        mVoxelCellSize( 0 ),
        mInvVoxelResolution( 0 ),
        mShaderParams( 0 )
    {
        TextureGpuManager *textureManager = voxelizer->getTextureGpuManager();

        RenderSystem *renderSystem = voxelizer->getRenderSystem();
        const bool hasTypedUavs = renderSystem->getCapabilities()->hasCapability( RSC_TYPED_UAV_LOADS );

        uint32 texFlags = TextureFlags::Uav;
        if( !hasTypedUavs )
            texFlags |= TextureFlags::Reinterpretable;

        const TextureGpu *albedoVox = voxelizer->getAlbedoVox();

        const uint32 width  = albedoVox->getWidth();
        const uint32 height = albedoVox->getHeight();
        const uint32 depth  = albedoVox->getDepth();

        const uint32 widthAniso     = std::max( 1u, width >> 1u );
        const uint32 heightAniso    = std::max( 1u, height >> 1u );
        const uint32 depthAniso     = std::max( 1u, depth >> 1u );

        const uint8 numMipsMain  = PixelFormatGpuUtils::getMaxMipmapCount( width, height, depth );
        const uint8 numMipsAniso = PixelFormatGpuUtils::getMaxMipmapCount( widthAniso, heightAniso,
                                                                           depthAniso );

        for( size_t i=0; i<1u; ++i )
        {
            TextureGpu *texture = textureManager->createTexture( "VctLighting" +
                                                                 StringConverter::toString( i ) + "/" +
                                                                 StringConverter::toString( getId() ),
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

        //VctVoxelizer should've already been initialized, thus no need
        //to check if JSON has been built or if the assets were added
        HlmsCompute *hlmsCompute = mVoxelizer->getHlmsManager()->getComputeHlms();
        mLightInjectionJob = hlmsCompute->findComputeJob( "VCT/LightInjection" );

        mShaderParams = &mLightInjectionJob->getShaderParams( "default" );
        mNumLights = mShaderParams->findParameter( "numLights" );
        mRayMarchStepSize = mShaderParams->findParameter( "rayMarchStepSize" );
        mVoxelCellSize = mShaderParams->findParameter( "voxelCellSize" );
        mInvVoxelResolution = mShaderParams->findParameter( "invVoxelResolution" );

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
    void VctLighting::update( SceneManager *sceneManager, float rayMarchStepScale, uint32 _lightMask )
    {
        OGRE_ASSERT_LOW( rayMarchStepScale >= 1.0f );

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
        const uint32 maxNumLights = mLightsConstBuffer->getNumElements() / sizeof(ShaderVctLight);

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
        //float4 voxelCellSize_maxDistance;
        *passBufferPtr++ = 1.0f;
        *passBufferPtr++ = 1.0f;
        *passBufferPtr++ = 1.0f;
        *passBufferPtr++ = 1.414213562f;

        //float4 normalBias_startBias_blendAmbient_blendFade;
        *passBufferPtr++ = 0.01f;
        *passBufferPtr++ = 0.01f;
        *passBufferPtr++ = 0.0f;
        *passBufferPtr++ = 0.0f;

        Matrix4 xform;
        xform.makeTransform( -mVoxelizer->getVoxelOrigin(),
                             1.0f / mVoxelizer->getVoxelSize(),
                             Quaternion::IDENTITY );
        //xform = xform * viewMatrix.inverse();
        xform.concatenateAffine( viewMatrix.inverseAffine() );

        //float4 xform_row0;
        //float4 xform_row1;
        //float4 xform_row2;
        for( size_t i=0; i<12u; ++i )
            *passBufferPtr++ = static_cast<float>( xform[0][i] );
    }
}
