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

#include "Terra/Hlms/OgreHlmsTerra.h"
#include "Terra/Hlms/OgreHlmsTerraDatablock.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsListener.h"
#include "OgreLwString.h"

#if !OGRE_NO_JSON
    #include "Terra/Hlms/OgreHlmsJsonTerra.h"
#endif

#include "OgreViewport.h"
#include "OgreRenderTarget.h"
#include "OgreHighLevelGpuProgramManager.h"
#include "OgreHighLevelGpuProgram.h"
#include "OgreForward3D.h"
#include "Cubemaps/OgreParallaxCorrectedCubemap.h"
#include "OgreIrradianceVolume.h"
#include "OgreCamera.h"

#include "OgreSceneManager.h"
#include "Compositor/OgreCompositorShadowNode.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreConstBufferPacked.h"

#include "CommandBuffer/OgreCommandBuffer.h"
#include "CommandBuffer/OgreCbTexture.h"
#include "CommandBuffer/OgreCbShaderBuffer.h"

#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
    #include "OgrePlanarReflections.h"
#endif

#include "Terra/Terra.h"

namespace Ogre
{
    const IdString TerraProperty::UseSkirts         = IdString( "use_skirts" );

    const char *TerraProperty::DiffuseMap           = "diffuse_map";
    const char *TerraProperty::EnvProbeMap          = "envprobe_map";
    const char *TerraProperty::DetailWeightMap      = "detail_weight_map";
    const char *TerraProperty::DetailMapN           = "detail_map";     //detail_map0-4
    const char *TerraProperty::DetailMapNmN         = "detail_map_nm";  //detail_map_nm0-4
    const char *TerraProperty::RoughnessMap         = "roughness_map";
    const char *TerraProperty::MetalnessMap         = "metalness_map";

    HlmsTerra::HlmsTerra( Archive *dataFolder, ArchiveVec *libraryFolders ) :
        HlmsPbs( dataFolder, libraryFolders ),
        mLastMovableObject( 0 ),
        mTerraDescSetSampler( 0 )
    {
        //Override defaults
        mType = HLMS_USER3;
        mTypeName = "Terra";
        mTypeNameStr = "Terra";

        mBytesPerSlot = HlmsTerraDatablock::MaterialSizeInGpuAligned;
        mOptimizationStrategy = LowerGpuOverhead;
        mSetupWorldMatBuf = false;

        //heightMap, terrainNormals & terrainShadows
        mReservedTexSlots = 3u;

        mSkipRequestSlotInChangeRS = true;
    }
    //-----------------------------------------------------------------------------------
    HlmsTerra::~HlmsTerra()
    {
        destroyAllBuffers();
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::_changeRenderSystem( RenderSystem *newRs )
    {
        HlmsPbs::_changeRenderSystem( newRs );

        if( newRs )
        {
            HlmsDatablockMap::const_iterator itor = mDatablocks.begin();
            HlmsDatablockMap::const_iterator end  = mDatablocks.end();

            while( itor != end )
            {
                assert( dynamic_cast<HlmsTerraDatablock*>( itor->second.datablock ) );
                HlmsTerraDatablock *datablock =
                        static_cast<HlmsTerraDatablock*>( itor->second.datablock );

                requestSlot( datablock->mTextureHash, datablock, false );
                ++itor;
            }

            if( !mTerraDescSetSampler )
            {
                //We need one for terrainNormals & terrainShadows. Reuse an existing samplerblock
                DescriptorSetSampler baseSet;
                baseSet.mSamplers.push_back( mAreaLightMasksSamplerblock );
                if( !mHasSeparateSamplers )
                    baseSet.mSamplers.push_back( mAreaLightMasksSamplerblock );
                baseSet.mShaderTypeSamplerCount[PixelShader] = baseSet.mSamplers.size();
                mTerraDescSetSampler = mHlmsManager->getDescriptorSetSampler( baseSet );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::setDetailMapProperties( HlmsTerraDatablock *datablock, PiecesMap *inOutPieces )
    {
        int32 minNormalMap = 4;
        bool hasDiffuseMaps = false;
        bool hasNormalMaps = false;
        for( uint8 i=0; i<4u; ++i )
        {
            setDetailTextureProperty( TerraProperty::DetailMapN,   datablock,
                                      TERRA_DETAIL0, i );
            setDetailTextureProperty( TerraProperty::DetailMapNmN, datablock,
                                      TERRA_DETAIL0_NM, i );
            setDetailTextureProperty( TerraProperty::RoughnessMap, datablock,
                                      TERRA_DETAIL_ROUGHNESS0, i );
            setDetailTextureProperty( TerraProperty::MetalnessMap, datablock,
                                      TERRA_DETAIL_METALNESS0, i );

            if( datablock->getTexture( TERRA_DETAIL0 + i ) )
                hasDiffuseMaps = true;

            if( datablock->getTexture( TERRA_DETAIL0_NM + i ) )
            {
                minNormalMap = std::min<int32>( minNormalMap, i );
                hasNormalMaps = true;
            }

            if( datablock->mDetailsOffsetScale[i] != Vector4( 0, 0, 1, 1 ) )
                setProperty( *PbsProperty::DetailOffsetsPtrs[i], 1 );
        }

        if( hasDiffuseMaps )
            setProperty( PbsProperty::DetailMapsDiffuse, 4 );

        if( hasNormalMaps )
            setProperty( PbsProperty::DetailMapsNormal, 4 );

        setProperty( PbsProperty::FirstValidDetailMapNm, minNormalMap );
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::setTextureProperty( const char *propertyName, HlmsTerraDatablock *datablock,
                                        TerraTextureTypes texType )
    {
        uint8 idx = datablock->getIndexToDescriptorTexture( texType );
        if( idx != NUM_TERRA_TEXTURE_TYPES )
        {
            char tmpData[64];
            LwString propName = LwString::FromEmptyPointer( tmpData, sizeof(tmpData) );

            propName = propertyName; //diffuse_map

            const size_t basePropSize = propName.size(); // diffuse_map

            //In the template the we subtract the "+1" for the index.
            //We need to increment it now otherwise @property( diffuse_map )
            //can translate to @property( 0 ) which is not what we want.
            setProperty( propertyName, idx + 1 );

            propName.resize( basePropSize );
            propName.a( "_idx" ); //diffuse_map_idx
            setProperty( propName.c_str(), idx );

            if( mHasSeparateSamplers )
            {
                const uint8 samplerIdx = datablock->getIndexToDescriptorSampler( texType );
                propName.resize( basePropSize );
                propName.a( "_sampler" );           //diffuse_map_sampler
                setProperty( propName.c_str(), samplerIdx );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::setDetailTextureProperty( const char *propertyName, HlmsTerraDatablock *datablock,
                                              TerraTextureTypes baseTexType, uint8 detailIdx )
    {
        const TerraTextureTypes texType = static_cast<TerraTextureTypes>( baseTexType + detailIdx );
        const uint8 idx = datablock->getIndexToDescriptorTexture( texType );
        if( idx != NUM_TERRA_TEXTURE_TYPES )
        {
            char tmpData[64];
            LwString propName = LwString::FromEmptyPointer( tmpData, sizeof(tmpData) );

            propName.a( propertyName, detailIdx ); //detail_map0

            const size_t basePropSize = propName.size(); // diffuse_map

            //In the template the we subtract the "+1" for the index.
            //We need to increment it now otherwise @property( diffuse_map )
            //can translate to @property( 0 ) which is not what we want.
            setProperty( propName.c_str(), idx + 1 );

            propName.resize( basePropSize );
            propName.a( "_idx" ); //detail_map0_idx
            setProperty( propName.c_str(), idx );

            if( mHasSeparateSamplers )
            {
                const uint8 samplerIdx = datablock->getIndexToDescriptorSampler( texType );
                propName.resize( basePropSize );
                propName.a( "_sampler" );           //detail_map0_sampler
                setProperty( propName.c_str(), samplerIdx );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::calculateHashFor( Renderable *renderable, uint32 &outHash, uint32 &outCasterHash )
    {
        assert( dynamic_cast<HlmsTerraDatablock*>( renderable->getDatablock() ) );
        HlmsTerraDatablock *datablock = static_cast<HlmsTerraDatablock*>( renderable->getDatablock() );
        if( datablock->getDirtyFlags() & (DirtyTextures|DirtySamplers) )
        {
            //Delay hash generation for later, when we have the final (or temporary) descriptor sets.
            outHash = 0;
            outCasterHash = 0;
        }
        else
        {
            Hlms::calculateHashFor( renderable, outHash, outCasterHash );
        }

        datablock->loadAllTextures();
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces )
    {
        assert( dynamic_cast<TerrainCell*>(renderable) &&
                "This Hlms can only be used on a Terra object!" );

        TerrainCell *terrainCell = static_cast<TerrainCell*>(renderable);
        setProperty( TerraProperty::UseSkirts, terrainCell->getUseSkirts() );

        assert( dynamic_cast<HlmsTerraDatablock*>( renderable->getDatablock() ) );
        HlmsTerraDatablock *datablock = static_cast<HlmsTerraDatablock*>( renderable->getDatablock() );

        setProperty( PbsProperty::FresnelScalar, 1 );
        setProperty( PbsProperty::FresnelWorkflow, 0 );
        setProperty( PbsProperty::MetallicWorkflow, 1 );

        setProperty( PbsProperty::ReceiveShadows, 1 );

        uint32 brdf = datablock->getBrdf();
        if( (brdf & TerraBrdf::BRDF_MASK) == TerraBrdf::Default )
        {
            setProperty( PbsProperty::BrdfDefault, 1 );

            if( !(brdf & TerraBrdf::FLAG_UNCORRELATED) )
                setProperty( PbsProperty::GgxHeightCorrelated, 1 );
        }
        else if( (brdf & TerraBrdf::BRDF_MASK) == TerraBrdf::CookTorrance )
            setProperty( PbsProperty::BrdfCookTorrance, 1 );
        else if( (brdf & TerraBrdf::BRDF_MASK) == TerraBrdf::BlinnPhong )
            setProperty( PbsProperty::BrdfBlinnPhong, 1 );

        if( brdf & TerraBrdf::FLAG_SPERATE_DIFFUSE_FRESNEL )
            setProperty( PbsProperty::FresnelSeparateDiffuse, 1 );

        if( brdf & TerraBrdf::FLAG_LEGACY_MATH )
            setProperty( PbsProperty::LegacyMathBrdf, 1 );
        if( brdf & TerraBrdf::FLAG_FULL_LEGACY )
            setProperty( PbsProperty::RoughnessIsShininess, 1 );

        if( datablock->mTexturesDescSet )
            setDetailMapProperties( datablock, inOutPieces );
        else
            setProperty( PbsProperty::FirstValidDetailMapNm, 4 );

        if( datablock->mSamplersDescSet )
            setProperty( PbsProperty::NumSamplers, datablock->mSamplersDescSet->mSamplers.size() );

        if( datablock->mTexturesDescSet )
        {
            bool envMap = datablock->getTexture( TERRA_REFLECTION ) != 0;
            setProperty( PbsProperty::NumTextures,
                         datablock->mTexturesDescSet->mTextures.size() - envMap );

            setTextureProperty( PbsProperty::DiffuseMap,    datablock,  TERRA_DIFFUSE );
            setTextureProperty( PbsProperty::EnvProbeMap,   datablock,  TERRA_REFLECTION );
            setTextureProperty( PbsProperty::DetailWeightMap,datablock, TERRA_DETAIL_WEIGHT );

            //Save the name of the cubemap for hazard prevention
            //(don't sample the cubemap and render to it at the same time).
            const TextureGpu *reflectionTexture = datablock->getTexture( TERRA_REFLECTION );
            if( reflectionTexture )
            {
                //Manual reflection texture
//                if( datablock->getCubemapProbe() )
//                    setProperty( PbsProperty::UseParallaxCorrectCubemaps, 1 );
                setProperty( PbsProperty::EnvProbeMap,
                             static_cast<int32>( reflectionTexture->getName().mHash ) );
            }
        }

        bool usesNormalMap = false;
        for( uint8 i=TERRA_DETAIL0_NM; i<=TERRA_DETAIL3_NM; ++i )
            usesNormalMap |= datablock->getTexture( i ) != 0;
        setProperty( PbsProperty::NormalMap, usesNormalMap );

        if( usesNormalMap )
        {
//            TextureGpu *normalMapTex = datablock->getTexture( TERRA_DETAIL0_NM );
//            if( PixelFormatGpuUtils::isSigned( normalMapTex->getPixelFormat() ) )
            {
                setProperty( PbsProperty::NormalSamplingFormat, PbsProperty::NormalRgSnorm.mHash );
                setProperty( PbsProperty::NormalRgSnorm, PbsProperty::NormalRgSnorm.mHash );
            }
//            else
//            {
//                setProperty( PbsProperty::NormalSamplingFormat, PbsProperty::NormalRgUnorm.mHash );
//                setProperty( PbsProperty::NormalRgUnorm, PbsProperty::NormalRgUnorm.mHash );
//            }
            //Reserved for supporting LA textures in GLES2.
//            else
//            {
//                setProperty( PbsProperty::NormalSamplingFormat, PbsProperty::NormalLa.mHash );
//                setProperty( PbsProperty::NormalLa, PbsProperty::NormalLa.mHash );
//            }
        }

#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
        if( mPlanarReflections && mPlanarReflections->hasPlanarReflections( renderable ) )
        {
            if( !mPlanarReflections->_isUpdatingRenderablesHlms() )
                mPlanarReflections->_notifyRenderableFlushedHlmsDatablock( renderable );
            else
                setProperty( PbsProperty::UsePlanarReflections, 1 );
        }
#endif
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::calculateHashForPreCaster( Renderable *renderable, PiecesMap *inOutPieces )
    {
        //Override, since shadow casting is not supported
        mSetProperties.clear();
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::notifyPropertiesMergedPreGenerationStep(void)
    {
        HlmsPbs::notifyPropertiesMergedPreGenerationStep();

        setTextureReg( VertexShader, "heightMap", 0 );
        setTextureReg( PixelShader, "terrainNormals", 1 );
        setTextureReg( PixelShader, "terrainShadows", 2 );
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsTerra::fillBuffersFor( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                      bool casterPass, uint32 lastCacheHash,
                                      uint32 lastTextureHash )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "Trying to use slow-path on a desktop implementation. "
                     "Change the RenderQueue settings.",
                     "HlmsTerra::fillBuffersFor" );
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsTerra::fillBuffersForV1( const HlmsCache *cache,
                                        const QueuedRenderable &queuedRenderable,
                                        bool casterPass, uint32 lastCacheHash,
                                        CommandBuffer *commandBuffer )
    {
        return fillBuffersFor( cache, queuedRenderable, casterPass,
                               lastCacheHash, commandBuffer, true );
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsTerra::fillBuffersForV2( const HlmsCache *cache,
                                        const QueuedRenderable &queuedRenderable,
                                        bool casterPass, uint32 lastCacheHash,
                                        CommandBuffer *commandBuffer )
    {
        return fillBuffersFor( cache, queuedRenderable, casterPass,
                               lastCacheHash, commandBuffer, false );
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsTerra::fillBuffersFor( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                      bool casterPass, uint32 lastCacheHash,
                                      CommandBuffer *commandBuffer, bool isV1 )
    {
        assert( dynamic_cast<const HlmsTerraDatablock*>( queuedRenderable.renderable->getDatablock() ) );
        const HlmsTerraDatablock *datablock = static_cast<const HlmsTerraDatablock*>(
                                                queuedRenderable.renderable->getDatablock() );

        if( OGRE_EXTRACT_HLMS_TYPE_FROM_CACHE_HASH( lastCacheHash ) != mType )
        {
            //layout(binding = 0) uniform PassBuffer {} pass
            ConstBufferPacked *passBuffer = mPassBuffers[mCurrentPassBuffer-1];
            *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer( VertexShader,
                                                                           0, passBuffer, 0,
                                                                           passBuffer->
                                                                           getTotalSizeBytes() );
            *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer( PixelShader,
                                                                           0, passBuffer, 0,
                                                                           passBuffer->
                                                                           getTotalSizeBytes() );

            if( !casterPass )
            {
                size_t texUnit = mReservedTexSlots;

                *commandBuffer->addCommand<CbSamplers>() =
                        CbSamplers( 1, mTerraDescSetSampler );

                if( mGridBuffer )
                {
                    *commandBuffer->addCommand<CbShaderBuffer>() =
                            CbShaderBuffer( PixelShader, texUnit++, mGridBuffer, 0, 0 );
                    *commandBuffer->addCommand<CbShaderBuffer>() =
                            CbShaderBuffer( PixelShader, texUnit++, mGlobalLightListBuffer, 0, 0 );
                }

                if( !mPrePassTextures->empty() )
                {
                    *commandBuffer->addCommand<CbTexture>() =
                            CbTexture( texUnit++, (*mPrePassTextures)[0], 0 );
                    *commandBuffer->addCommand<CbTexture>() =
                            CbTexture( texUnit++, (*mPrePassTextures)[1], 0 );
                }

                if( mPrePassMsaaDepthTexture )
                {
                    *commandBuffer->addCommand<CbTexture>() =
                            CbTexture( texUnit++, mPrePassMsaaDepthTexture, 0 );
                }

                if( mSsrTexture )
                {
                    *commandBuffer->addCommand<CbTexture>() =
                            CbTexture( texUnit++, mSsrTexture, 0 );
                }

                if( mIrradianceVolume )
                {
                    TextureGpu *irradianceTex = mIrradianceVolume->getIrradianceVolumeTexture();
                    const HlmsSamplerblock *samplerblock = mIrradianceVolume->getIrradSamplerblock();

                    *commandBuffer->addCommand<CbTexture>() = CbTexture( texUnit,
                                                                         irradianceTex,
                                                                         samplerblock );
                    ++texUnit;
                }

                if( mUsingAreaLightMasks )
                {
                    *commandBuffer->addCommand<CbTexture>() = CbTexture( texUnit,
                                                                         mAreaLightMasks,
                                                                         mAreaLightMasksSamplerblock );
                    ++texUnit;
                }

                if( mUsingLtcMatrix )
                {
                    *commandBuffer->addCommand<CbTexture>() = CbTexture( texUnit,
                                                                         mLtcMatrixTexture,
                                                                         mAreaLightMasksSamplerblock );
                    ++texUnit;
                }

                for( size_t i=0; i<3u; ++i )
                {
                    if( mDecalsTextures[i] &&
                        (i != 2u || !mDecalsDiffuseMergedEmissive) )
                    {
                        *commandBuffer->addCommand<CbTexture>() = CbTexture( texUnit,
                                                                             mDecalsTextures[i],
                                                                             mDecalsSamplerblock );
                        ++texUnit;
                    }
                }

                //We changed HlmsType, rebind the shared textures.
                FastArray<TextureGpu*>::const_iterator itor = mPreparedPass.shadowMaps.begin();
                FastArray<TextureGpu*>::const_iterator end  = mPreparedPass.shadowMaps.end();
                while( itor != end )
                {
                    *commandBuffer->addCommand<CbTexture>() = CbTexture( texUnit, *itor,
                                                                         mCurrentShadowmapSamplerblock );
                    ++texUnit;
                    ++itor;
                }

                if( mParallaxCorrectedCubemap && !mParallaxCorrectedCubemap->isRendering() )
                {
                    TextureGpu *pccTexture = mParallaxCorrectedCubemap->getBindTexture();
                    const HlmsSamplerblock *samplerblock =
                            mParallaxCorrectedCubemap->getBindTrilinearSamplerblock();
                    *commandBuffer->addCommand<CbTexture>() = CbTexture( texUnit, pccTexture,
                                                                         samplerblock );
                    ++texUnit;
                }
            }

            mLastDescTexture = 0;
            mLastDescSampler = 0;
            mLastMovableObject = 0;
            mLastBoundPool = 0;

            //layout(binding = 2) uniform InstanceBuffer {} instance
            if( mCurrentConstBuffer < mConstBuffers.size() &&
                (size_t)((mCurrentMappedConstBuffer - mStartMappedConstBuffer) + 4) <=
                    mCurrentConstBufferSize )
            {
                *commandBuffer->addCommand<CbShaderBuffer>() =
                        CbShaderBuffer( VertexShader, 2, mConstBuffers[mCurrentConstBuffer], 0, 0 );
                *commandBuffer->addCommand<CbShaderBuffer>() =
                        CbShaderBuffer( PixelShader, 2, mConstBuffers[mCurrentConstBuffer], 0, 0 );
            }

            //rebindTexBuffer( commandBuffer );

#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
            mLastBoundPlanarReflection = 0u;
#endif
            mListener->hlmsTypeChanged( casterPass, commandBuffer, datablock );
        }

        //Don't bind the material buffer on caster passes (important to keep
        //MDI & auto-instancing running on shadow map passes)
        if( mLastBoundPool != datablock->getAssignedPool() &&
            (!casterPass || datablock->getAlphaTest() != CMPF_ALWAYS_PASS) )
        {
            //layout(binding = 1) uniform MaterialBuf {} materialArray
            const ConstBufferPool::BufferPool *newPool = datablock->getAssignedPool();
            *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer( PixelShader,
                                                                           1, newPool->materialBuffer, 0,
                                                                           newPool->materialBuffer->
                                                                           getTotalSizeBytes() );
            mLastBoundPool = newPool;
        }

        if( mLastMovableObject != queuedRenderable.movableObject )
        {
            //Different Terra? Must change textures then.
            const Terra *terraObj = static_cast<const Terra*>( queuedRenderable.movableObject );
            *commandBuffer->addCommand<CbTextures>() =
                    CbTextures( 0, std::numeric_limits<uint16>::max(),
                                terraObj->getDescriptorSetTexture() );
#if OGRE_DEBUG_MODE
//          Commented: Hack to get a barrier without dealing with the Compositor while debugging.
//            ResourceTransition resourceTransition;
//            resourceTransition.readBarrierBits = ReadBarrier::Uav;
//            resourceTransition.writeBarrierBits = WriteBarrier::Uav;
//            mRenderSystem->_resourceTransitionCreated( &resourceTransition );
//            mRenderSystem->_executeResourceTransition( &resourceTransition );
//            mRenderSystem->_resourceTransitionDestroyed( &resourceTransition );
            TextureGpu *terraShadowText = terraObj->_getShadowMapTex();
            const CompositorTextureVec &compositorTextures = queuedRenderable.movableObject->
                    _getManager()->getCompositorTextures();
            CompositorTextureVec::const_iterator itor = compositorTextures.begin();
            CompositorTextureVec::const_iterator end  = compositorTextures.end();

            while( itor != end && itor->texture != terraShadowText )
                ++itor;

            if( itor == end )
            {
                assert( "Hazard Detected! You should expose this Terra's shadow map texture"
                        " to the compositor pass so Ogre can place the proper Barriers" && false );
            }
#endif

            mLastMovableObject = queuedRenderable.movableObject;
        }

        uint32 * RESTRICT_ALIAS currentMappedConstBuffer    = mCurrentMappedConstBuffer;

        //---------------------------------------------------------------------------
        //                          ---- VERTEX SHADER ----
        //---------------------------------------------------------------------------
        //We need to correct currentMappedConstBuffer to point to the right texture buffer's
        //offset, which may not be in sync if the previous draw had skeletal animation.
        bool exceedsConstBuffer = (size_t)((currentMappedConstBuffer - mStartMappedConstBuffer) + 12)
                                    > mCurrentConstBufferSize;

        if( exceedsConstBuffer )
            currentMappedConstBuffer = mapNextConstBuffer( commandBuffer );

        const TerrainCell *terrainCell = static_cast<const TerrainCell*>( queuedRenderable.renderable );

        terrainCell->uploadToGpu( currentMappedConstBuffer );
        currentMappedConstBuffer += 16u;

        //---------------------------------------------------------------------------
        //                          ---- PIXEL SHADER ----
        //---------------------------------------------------------------------------

        if( !casterPass || datablock->getAlphaTest() != CMPF_ALWAYS_PASS )
        {
#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
            if( mHasPlanarReflections &&
                (queuedRenderable.renderable->mCustomParameter & 0x80) &&
                mLastBoundPlanarReflection != queuedRenderable.renderable->mCustomParameter )
            {
                const uint8 activeActorIdx = queuedRenderable.renderable->mCustomParameter & 0x7F;
                TextureGpu *planarReflTex = mPlanarReflections->getTexture( activeActorIdx );
                *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( mTexUnitSlotStart - 1u, planarReflTex,
                                   mPlanarReflectionsSamplerblock );
                mLastBoundPlanarReflection = queuedRenderable.renderable->mCustomParameter;
            }
#endif
            if( datablock->mTexturesDescSet != mLastDescTexture )
            {
                if( datablock->mTexturesDescSet )
                {
                    //Rebind textures
                    size_t texUnit = mTexUnitSlotStart;

                    *commandBuffer->addCommand<CbTextures>() =
                            CbTextures( texUnit, std::numeric_limits<uint16>::max(),
                                        datablock->mTexturesDescSet );

                    if( !mHasSeparateSamplers )
                    {
                        *commandBuffer->addCommand<CbSamplers>() =
                                CbSamplers( texUnit, datablock->mSamplersDescSet );
                    }
                    //texUnit += datablock->mTexturesDescSet->mTextures.size();
                }

                mLastDescTexture = datablock->mTexturesDescSet;
            }

            if( datablock->mSamplersDescSet != mLastDescSampler && mHasSeparateSamplers )
            {
                if( datablock->mSamplersDescSet )
                {
                    //Bind samplers
                    size_t texUnit = mTexUnitSlotStart;
                    *commandBuffer->addCommand<CbSamplers>() =
                            CbSamplers( texUnit, datablock->mSamplersDescSet );
                    mLastDescSampler = datablock->mSamplersDescSet;
                }
            }
        }

        mCurrentMappedConstBuffer   = currentMappedConstBuffer;

        return ((mCurrentMappedConstBuffer - mStartMappedConstBuffer) >> 4) - 1;
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::getDefaultPaths( String &outDataFolderPath, StringVector &outLibraryFoldersPaths )
    {
        //We need to know what RenderSystem is currently in use, as the
        //name of the compatible shading language is part of the path
        Ogre::RenderSystem *renderSystem = Ogre::Root::getSingleton().getRenderSystem();
        Ogre::String shaderSyntax = "GLSL";
        if (renderSystem->getName() == "Direct3D11 Rendering Subsystem")
            shaderSyntax = "HLSL";
        else if (renderSystem->getName() == "Metal Rendering Subsystem")
            shaderSyntax = "Metal";

        //Fill the library folder paths with the relevant folders
        outLibraryFoldersPaths.clear();
        outLibraryFoldersPaths.push_back( "Hlms/Common/" + shaderSyntax );
        outLibraryFoldersPaths.push_back( "Hlms/Common/Any" );
        outLibraryFoldersPaths.push_back( "Hlms/Pbs/Any" );
        outLibraryFoldersPaths.push_back( "Hlms/Pbs/Any/Main" );
        outLibraryFoldersPaths.push_back( "Hlms/Terra/Any" );

        //Fill the data folder path
        outDataFolderPath = "Hlms/Terra/" + shaderSyntax;
    }
#if !OGRE_NO_JSON
    //-----------------------------------------------------------------------------------
    void HlmsTerra::_loadJson( const rapidjson::Value &jsonValue, const HlmsJson::NamedBlocks &blocks,
                               HlmsDatablock *datablock, const String &resourceGroup,
                               HlmsJsonListener *listener,
                               const String &additionalTextureExtension ) const
    {
        HlmsJsonTerra jsonTerra( mHlmsManager, mRenderSystem->getTextureGpuManager() );
        jsonTerra.loadMaterial( jsonValue, blocks, datablock, resourceGroup );
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::_saveJson( const HlmsDatablock *datablock, String &outString,
                               HlmsJsonListener *listener,
                               const String &additionalTextureExtension ) const
    {
//        HlmsJsonTerra jsonTerra( mHlmsManager );
//        jsonTerra.saveMaterial( datablock, outString );
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::_collectSamplerblocks( set<const HlmsSamplerblock*>::type &outSamplerblocks,
                                         const HlmsDatablock *datablock ) const
    {
        HlmsJsonTerra::collectSamplerblocks( datablock, outSamplerblocks );
    }
#endif
    //-----------------------------------------------------------------------------------
    HlmsDatablock* HlmsTerra::createDatablockImpl( IdString datablockName,
                                                   const HlmsMacroblock *macroblock,
                                                   const HlmsBlendblock *blendblock,
                                                   const HlmsParamVec &paramVec )
    {
        return OGRE_NEW HlmsTerraDatablock( datablockName, this, macroblock, blendblock, paramVec );
    }
}
