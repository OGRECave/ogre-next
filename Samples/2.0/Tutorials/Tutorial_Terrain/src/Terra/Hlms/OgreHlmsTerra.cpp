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

#include "Terra/Hlms/OgreHlmsTerra.h"

#include "Terra/Hlms/OgreHlmsTerraDatablock.h"

#include "CommandBuffer/OgreCbShaderBuffer.h"
#include "CommandBuffer/OgreCbTexture.h"
#include "CommandBuffer/OgreCommandBuffer.h"
#include "Compositor/OgreCompositorShadowNode.h"
#include "Cubemaps/OgreParallaxCorrectedCubemap.h"
#include "OgreAtmosphereComponent.h"
#include "OgreCamera.h"
#include "OgreForward3D.h"
#include "OgreHighLevelGpuProgram.h"
#include "OgreHighLevelGpuProgramManager.h"
#include "OgreHlmsListener.h"
#include "OgreHlmsManager.h"
#include "OgreIrradianceVolume.h"
#include "OgreLwString.h"
#include "OgreRenderQueue.h"
#include "OgreSceneManager.h"
#include "OgreViewport.h"
#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreVaoManager.h"

#if !OGRE_NO_JSON
#    include "Terra/Hlms/OgreHlmsJsonTerra.h"
#endif

#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
#    include "OgrePlanarReflections.h"
#endif

#include "Terra/Terra.h"

namespace Ogre
{
    const IdString TerraProperty::UseSkirts = IdString( "use_skirts" );
    const IdString TerraProperty::ZUp = IdString( "z_up" );

    const char *TerraProperty::DiffuseMap = "diffuse_map";
    const char *TerraProperty::EnvProbeMap = "envprobe_map";
    const char *TerraProperty::DetailWeightMap = "detail_weight_map";
    const char *TerraProperty::DetailMapN = "detail_map";       // detail_map0-4
    const char *TerraProperty::DetailMapNmN = "detail_map_nm";  // detail_map_nm0-4
    const char *TerraProperty::RoughnessMap = "roughness_map";
    const char *TerraProperty::MetalnessMap = "metalness_map";

    const IdString TerraProperty::DetailTriplanar = IdString( "detail_triplanar" );
    const IdString TerraProperty::DetailTriplanarDiffuse = IdString( "detail_triplanar_diffuse" );
    const IdString TerraProperty::DetailTriplanarNormal = IdString( "detail_triplanar_normal" );
    const IdString TerraProperty::DetailTriplanarRoughness = IdString( "detail_triplanar_roughness" );
    const IdString TerraProperty::DetailTriplanarMetalness = IdString( "detail_triplanar_metalness" );

    HlmsTerra::HlmsTerra( Archive *dataFolder, ArchiveVec *libraryFolders ) :
        HlmsPbs( dataFolder, libraryFolders ),
        mLastMovableObject( 0 )
    {
        // Override defaults
        mType = HLMS_USER3;
        mTypeName = "Terra";
        mTypeNameStr = "Terra";

        mBytesPerSlot = HlmsTerraDatablock::MaterialSizeInGpuAligned;
        mOptimizationStrategy = LowerGpuOverhead;
        mSetupWorldMatBuf = false;
        mReservedTexBufferSlots = 0u;
        mReservedTexSlots = 3u;  // heightMap, terrainNormals & terrainShadows

        mSkipRequestSlotInChangeRS = true;
    }
    //-----------------------------------------------------------------------------------
    HlmsTerra::~HlmsTerra() { destroyAllBuffers(); }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::_linkTerra( Terra *terra )
    {
        OGRE_ASSERT_LOW( terra->mHlmsTerraIndex == std::numeric_limits<uint32>::max() &&
                         "Terra instance must be unlinked before being linked again!" );

        terra->mHlmsTerraIndex = static_cast<uint32>( mLinkedTerras.size() );
        mLinkedTerras.push_back( terra );
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::_unlinkTerra( Terra *terra )
    {
        if( terra->mHlmsTerraIndex >= mLinkedTerras.size() ||
            terra != *( mLinkedTerras.begin() + terra->mHlmsTerraIndex ) )
        {
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR,
                         "A Terra instance had it's mHlmsTerraIndex out of date!!! "
                         "(or the instance wasn't being tracked by this HlmsTerra)",
                         "HlmsTerra::_unlinkTerra" );
        }

        FastArray<Terra *>::iterator itor = mLinkedTerras.begin() + terra->mHlmsTerraIndex;
        itor = efficientVectorRemove( mLinkedTerras, itor );

        // The Renderable that was at the end got swapped and has now a different index
        if( itor != mLinkedTerras.end() )
            ( *itor )->mHlmsTerraIndex = static_cast<uint32>( itor - mLinkedTerras.begin() );

        terra->mHlmsTerraIndex = std::numeric_limits<uint32>::max();
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::_changeRenderSystem( RenderSystem *newRs )
    {
        HlmsPbs::_changeRenderSystem( newRs );

        if( newRs )
        {
            HlmsDatablockMap::const_iterator itor = mDatablocks.begin();
            HlmsDatablockMap::const_iterator end = mDatablocks.end();

            while( itor != end )
            {
                assert( dynamic_cast<HlmsTerraDatablock *>( itor->second.datablock ) );
                HlmsTerraDatablock *datablock =
                    static_cast<HlmsTerraDatablock *>( itor->second.datablock );

                requestSlot( datablock->mTextureHash, datablock, false );
                ++itor;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::setDetailMapProperties( const size_t tid, HlmsTerraDatablock *datablock,
                                            PiecesMap *inOutPieces )
    {
        int32 minNormalMap = 4;
        bool hasDiffuseMaps = false;
        bool hasNormalMaps = false;
        for( uint8 i = 0; i < 4u; ++i )
        {
            setDetailTextureProperty( tid, TerraProperty::DetailMapN, datablock, TERRA_DETAIL0, i );
            setDetailTextureProperty( tid, TerraProperty::DetailMapNmN, datablock, TERRA_DETAIL0_NM, i );
            setDetailTextureProperty( tid, TerraProperty::RoughnessMap, datablock,
                                      TERRA_DETAIL_ROUGHNESS0, i );
            setDetailTextureProperty( tid, TerraProperty::MetalnessMap, datablock,
                                      TERRA_DETAIL_METALNESS0, i );

            if( datablock->getTexture( TERRA_DETAIL0 + i ) )
                hasDiffuseMaps = true;

            if( datablock->getTexture( TERRA_DETAIL0_NM + i ) )
            {
                minNormalMap = std::min<int32>( minNormalMap, i );
                hasNormalMaps = true;
            }

            if( datablock->mDetailsOffsetScale[i] != Vector4( 0, 0, 1, 1 ) )
                setProperty( tid, *PbsProperty::DetailOffsetsPtrs[i], 1 );
        }

        if( hasDiffuseMaps )
            setProperty( tid, PbsProperty::DetailMapsDiffuse, 4 );

        if( hasNormalMaps )
            setProperty( tid, PbsProperty::DetailMapsNormal, 4 );

        setProperty( tid, PbsProperty::FirstValidDetailMapNm, minNormalMap );
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::setTextureProperty( const size_t tid, const char *propertyName,
                                        HlmsTerraDatablock *datablock, TerraTextureTypes texType )
    {
        uint8 idx = datablock->getIndexToDescriptorTexture( texType );
        if( idx != NUM_TERRA_TEXTURE_TYPES )
        {
            char tmpData[64];
            LwString propName = LwString::FromEmptyPointer( tmpData, sizeof( tmpData ) );

            propName = propertyName;  // diffuse_map

            const size_t basePropSize = propName.size();  // diffuse_map

            // In the template the we subtract the "+1" for the index.
            // We need to increment it now otherwise @property( diffuse_map )
            // can translate to @property( 0 ) which is not what we want.
            setProperty( tid, propertyName, idx + 1 );

            propName.resize( basePropSize );
            propName.a( "_idx" );  // diffuse_map_idx
            setProperty( tid, propName.c_str(), idx );

            if( mHasSeparateSamplers )
            {
                const uint8 samplerIdx = datablock->getIndexToDescriptorSampler( texType );
                propName.resize( basePropSize );
                propName.a( "_sampler" );  // diffuse_map_sampler
                setProperty( tid, propName.c_str(), samplerIdx );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::setDetailTextureProperty( const size_t tid, const char *propertyName,
                                              HlmsTerraDatablock *datablock,
                                              TerraTextureTypes baseTexType, uint8 detailIdx )
    {
        const TerraTextureTypes texType = static_cast<TerraTextureTypes>( baseTexType + detailIdx );
        const uint8 idx = datablock->getIndexToDescriptorTexture( texType );
        if( idx != NUM_TERRA_TEXTURE_TYPES )
        {
            char tmpData[64];
            LwString propName = LwString::FromEmptyPointer( tmpData, sizeof( tmpData ) );

            propName.a( propertyName, detailIdx );  // detail_map0

            const size_t basePropSize = propName.size();  // diffuse_map

            // In the template the we subtract the "+1" for the index.
            // We need to increment it now otherwise @property( diffuse_map )
            // can translate to @property( 0 ) which is not what we want.
            setProperty( tid, propName.c_str(), idx + 1 );

            propName.resize( basePropSize );
            propName.a( "_idx" );  // detail_map0_idx
            setProperty( tid, propName.c_str(), idx );

            if( mHasSeparateSamplers )
            {
                const uint8 samplerIdx = datablock->getIndexToDescriptorSampler( texType );
                propName.resize( basePropSize );
                propName.a( "_sampler" );  // detail_map0_sampler
                setProperty( tid, propName.c_str(), samplerIdx );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::calculateHashFor( Renderable *renderable, uint32 &outHash, uint32 &outCasterHash )
    {
        assert( dynamic_cast<HlmsTerraDatablock *>( renderable->getDatablock() ) );
        HlmsTerraDatablock *datablock = static_cast<HlmsTerraDatablock *>( renderable->getDatablock() );
        if( datablock->getDirtyFlags() & ( DirtyTextures | DirtySamplers ) )
        {
            // Delay hash generation for later, when we have the final (or temporary) descriptor sets.
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
        assert( dynamic_cast<TerrainCell *>( renderable ) &&
                "This Hlms can only be used on a Terra object!" );

        // Disable normal offset bias because the world normals are not available in the
        // vertex shader. We could fetch the normals, but tessellation is constantly changing in
        // Terra, and besides we don't care because Terra doesn't cast shadow maps, thus it
        // has no self occlussion artifacts.
        setProperty( kNoTid, "skip_normal_offset_bias_vs", 1 );

        TerrainCell *terrainCell = static_cast<TerrainCell *>( renderable );
        setProperty( kNoTid, TerraProperty::UseSkirts, terrainCell->getUseSkirts() );
        setProperty( kNoTid, TerraProperty::ZUp, terrainCell->isZUp() );

        assert( dynamic_cast<HlmsTerraDatablock *>( renderable->getDatablock() ) );
        HlmsTerraDatablock *datablock = static_cast<HlmsTerraDatablock *>( renderable->getDatablock() );

        setProperty( kNoTid, PbsProperty::FresnelScalar, 1 );
        setProperty( kNoTid, PbsProperty::FresnelWorkflow, 0 );
        setProperty( kNoTid, PbsProperty::MetallicWorkflow, 1 );

        setProperty( kNoTid, PbsProperty::ReceiveShadows, 1 );

        uint32 brdf = datablock->getBrdf();
        if( ( brdf & TerraBrdf::BRDF_MASK ) == TerraBrdf::Default )
        {
            setProperty( kNoTid, PbsProperty::BrdfDefault, 1 );

            if( !( brdf & TerraBrdf::FLAG_UNCORRELATED ) )
                setProperty( kNoTid, PbsProperty::GgxHeightCorrelated, 1 );
        }
        else if( ( brdf & TerraBrdf::BRDF_MASK ) == TerraBrdf::CookTorrance )
            setProperty( kNoTid, PbsProperty::BrdfCookTorrance, 1 );
        else if( ( brdf & TerraBrdf::BRDF_MASK ) == TerraBrdf::BlinnPhong )
            setProperty( kNoTid, PbsProperty::BrdfBlinnPhong, 1 );

        if( brdf & TerraBrdf::FLAG_HAS_DIFFUSE_FRESNEL )
        {
            setProperty( kNoTid, PbsProperty::FresnelHasDiffuse, 1 );
            if( brdf & TerraBrdf::FLAG_SPERATE_DIFFUSE_FRESNEL )
                setProperty( kNoTid, PbsProperty::FresnelSeparateDiffuse, 1 );
        }

        if( brdf & TerraBrdf::FLAG_LEGACY_MATH )
            setProperty( kNoTid, PbsProperty::LegacyMathBrdf, 1 );
        if( brdf & TerraBrdf::FLAG_FULL_LEGACY )
            setProperty( kNoTid, PbsProperty::RoughnessIsShininess, 1 );

        if( datablock->mTexturesDescSet )
            setDetailMapProperties( kNoTid, datablock, inOutPieces );
        else
            setProperty( kNoTid, PbsProperty::FirstValidDetailMapNm, 4 );

        if( datablock->mSamplersDescSet )
        {
            setProperty( kNoTid, PbsProperty::NumSamplers,
                         (int32)datablock->mSamplersDescSet->mSamplers.size() );
        }

        if( terrainCell->getParentTerra()->getHeightMapTex()->getPixelFormat() == PFG_R16_UINT )
            setProperty( kNoTid, "terra_use_uint", 1 );

        if( datablock->mTexturesDescSet )
        {
            bool envMap = datablock->getTexture( TERRA_REFLECTION ) != 0;
            setProperty( kNoTid, PbsProperty::NumTextures,
                         int32( datablock->mTexturesDescSet->mTextures.size() - envMap ) );

            setTextureProperty( kNoTid, PbsProperty::DiffuseMap, datablock, TERRA_DIFFUSE );
            setTextureProperty( kNoTid, PbsProperty::EnvProbeMap, datablock, TERRA_REFLECTION );
            setTextureProperty( kNoTid, PbsProperty::DetailWeightMap, datablock, TERRA_DETAIL_WEIGHT );

            // Save the name of the cubemap for hazard prevention
            //(don't sample the cubemap and render to it at the same time).
            const TextureGpu *reflectionTexture = datablock->getTexture( TERRA_REFLECTION );
            if( reflectionTexture )
            {
                // Manual reflection texture
                //                if( datablock->getCubemapProbe() )
                //                    setProperty( kNoTid,PbsProperty::UseParallaxCorrectCubemaps, 1 );
                setProperty( kNoTid, PbsProperty::EnvProbeMap,
                             static_cast<int32>( reflectionTexture->getName().getU32Value() ) );
            }
        }

        bool usesNormalMap = false;
        for( uint8 i = TERRA_DETAIL0_NM; i <= TERRA_DETAIL3_NM; ++i )
            usesNormalMap |= datablock->getTexture( i ) != 0;
        setProperty( kNoTid, PbsProperty::NormalMap, usesNormalMap );

        if( usesNormalMap )
        {
            {
                setProperty( kNoTid, PbsProperty::NormalSamplingFormat,
                             static_cast<int32>( PbsProperty::NormalRgSnorm.getU32Value() ) );
                setProperty( kNoTid, PbsProperty::NormalRgSnorm,
                             static_cast<int32>( PbsProperty::NormalRgSnorm.getU32Value() ) );
            }
        }

        if( datablock->getDetailTriplanarDiffuseEnabled() )
        {
            setProperty( kNoTid, TerraProperty::DetailTriplanar, 1 );
            setProperty( kNoTid, TerraProperty::DetailTriplanarDiffuse, 1 );
        }

        if( datablock->getDetailTriplanarNormalEnabled() )
        {
            setProperty( kNoTid, TerraProperty::DetailTriplanar, 1 );
            setProperty( kNoTid, TerraProperty::DetailTriplanarNormal, 1 );
        }

        if( datablock->getDetailTriplanarRoughnessEnabled() )
        {
            setProperty( kNoTid, TerraProperty::DetailTriplanar, 1 );
            setProperty( kNoTid, TerraProperty::DetailTriplanarRoughness, 1 );
        }

        if( datablock->getDetailTriplanarMetalnessEnabled() )
        {
            setProperty( kNoTid, TerraProperty::DetailTriplanar, 1 );
            setProperty( kNoTid, TerraProperty::DetailTriplanarMetalness, 1 );
        }

#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
        if( mPlanarReflections && mPlanarReflections->hasPlanarReflections( renderable ) )
        {
            if( !mPlanarReflections->_isUpdatingRenderablesHlms() )
                mPlanarReflections->_notifyRenderableFlushedHlmsDatablock( renderable );
            else
                setProperty( kNoTid, PbsProperty::UsePlanarReflections, 1 );
        }
#endif
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::calculateHashForPreCaster( Renderable *renderable, PiecesMap *inOutPieces,
                                               const PiecesMap * )
    {
        // Override, since shadow casting is very basic
        mT[kNoTid].setProperties.clear();
        setProperty( kNoTid, "hlms_no_shadowConstantBias_decl", 1 );

        TerrainCell *terrainCell = static_cast<TerrainCell *>( renderable );
        setProperty( kNoTid, TerraProperty::ZUp, terrainCell->isZUp() );
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::notifyPropertiesMergedPreGenerationStep( const size_t tid )
    {
        HlmsPbs::notifyPropertiesMergedPreGenerationStep( tid );

        int32 texSlotsStart = 0;
        if( getProperty( tid, HlmsBaseProp::ForwardPlus ) )
            texSlotsStart = getProperty( tid, "f3dGrid" ) + 1;
        setTextureReg( tid, VertexShader, "heightMap", texSlotsStart + 0 );
        if( !getProperty( tid, HlmsBaseProp::ShadowCaster ) )
        {
            setTextureReg( tid, PixelShader, "terrainNormals", texSlotsStart + 1 );
            setTextureReg( tid, PixelShader, "terrainShadows", texSlotsStart + 2 );
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::analyzeBarriers( BarrierSolver &barrierSolver,
                                     ResourceTransitionArray &resourceTransitions,
                                     Camera *renderingCamera, const bool bCasterPass )
    {
        HlmsPbs::analyzeBarriers( barrierSolver, resourceTransitions, renderingCamera, bCasterPass );

        if( bCasterPass )
            return;

        FastArray<Terra *>::const_iterator itor = mLinkedTerras.begin();
        FastArray<Terra *>::const_iterator endt = mLinkedTerras.end();

        while( itor != endt )
        {
            const Terra *terraObj = static_cast<const Terra *>( *itor );

            if( terraObj->getHeightMapTex()->isRenderToTexture() ||
                terraObj->getHeightMapTex()->isUav() )
            {
                barrierSolver.resolveTransition( resourceTransitions, terraObj->getHeightMapTex(),
                                                 ResourceLayout::Texture, ResourceAccess::Read,
                                                 1u << VertexShader );
            }
            if( terraObj->getNormalMapTex()->isRenderToTexture() ||
                terraObj->getNormalMapTex()->isUav() )
            {
                barrierSolver.resolveTransition( resourceTransitions, terraObj->getNormalMapTex(),
                                                 ResourceLayout::Texture, ResourceAccess::Read,
                                                 1u << PixelShader );
            }
            // Shadow map texture is used in the vertex shader in regular objects.
            // Terra uses it in pixel shader.
            barrierSolver.resolveTransition( resourceTransitions, terraObj->_getShadowMapTex(),
                                             ResourceLayout::Texture, ResourceAccess::Read,
                                             ( 1u << VertexShader ) | ( 1u << PixelShader ) );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsTerra::fillBuffersFor( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                      bool casterPass, uint32 lastCacheHash, uint32 lastTextureHash )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "Trying to use slow-path on a desktop implementation. "
                     "Change the RenderQueue settings.",
                     "HlmsTerra::fillBuffersFor" );
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsTerra::fillBuffersForV1( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                        bool casterPass, uint32 lastCacheHash,
                                        CommandBuffer *commandBuffer )
    {
        return fillBuffersFor( cache, queuedRenderable, casterPass, lastCacheHash, commandBuffer, true );
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsTerra::fillBuffersForV2( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                        bool casterPass, uint32 lastCacheHash,
                                        CommandBuffer *commandBuffer )
    {
        return fillBuffersFor( cache, queuedRenderable, casterPass, lastCacheHash, commandBuffer,
                               false );
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsTerra::fillBuffersFor( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                      bool casterPass, uint32 lastCacheHash,
                                      CommandBuffer *commandBuffer, bool isV1 )
    {
        assert(
            dynamic_cast<const HlmsTerraDatablock *>( queuedRenderable.renderable->getDatablock() ) );
        const HlmsTerraDatablock *datablock =
            static_cast<const HlmsTerraDatablock *>( queuedRenderable.renderable->getDatablock() );

        if( OGRE_EXTRACT_HLMS_TYPE_FROM_CACHE_HASH( lastCacheHash ) != mType )
        {
            // layout(binding = 0) uniform PassBuffer {} pass
            ConstBufferPacked *passBuffer = mPassBuffers[mCurrentPassBuffer - 1];
            *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                VertexShader, 0, passBuffer, 0, (uint32)passBuffer->getTotalSizeBytes() );
            *commandBuffer->addCommand<CbShaderBuffer>() =
                CbShaderBuffer( PixelShader, 0, passBuffer, 0, (uint32)passBuffer->getTotalSizeBytes() );

            uint32 constBufferSlot = 3u;

            if( mUseLightBuffers )
            {
                ConstBufferPacked *light0Buffer = mLight0Buffers[mCurrentPassBuffer - 1];
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    VertexShader, 3, light0Buffer, 0, (uint32)light0Buffer->getTotalSizeBytes() );
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    PixelShader, 3, light0Buffer, 0, (uint32)light0Buffer->getTotalSizeBytes() );

                ConstBufferPacked *light1Buffer = mLight1Buffers[mCurrentPassBuffer - 1];
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    VertexShader, 4, light1Buffer, 0, (uint32)light1Buffer->getTotalSizeBytes() );
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    PixelShader, 4, light1Buffer, 0, (uint32)light1Buffer->getTotalSizeBytes() );

                ConstBufferPacked *light2Buffer = mLight2Buffers[mCurrentPassBuffer - 1];
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    VertexShader, 5, light2Buffer, 0, (uint32)light2Buffer->getTotalSizeBytes() );
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    PixelShader, 5, light2Buffer, 0, (uint32)light2Buffer->getTotalSizeBytes() );

                constBufferSlot = 6u;
            }

            size_t texUnit = mReservedTexBufferSlots;

            if( !casterPass )
            {
                constBufferSlot += mAtmosphere->bindConstBuffers( commandBuffer, constBufferSlot );

                if( mGridBuffer )
                {
                    *commandBuffer->addCommand<CbShaderBuffer>() =
                        CbShaderBuffer( PixelShader, (uint16)texUnit++, mGlobalLightListBuffer, 0, 0 );
                    *commandBuffer->addCommand<CbShaderBuffer>() =
                        CbShaderBuffer( PixelShader, (uint16)texUnit++, mGridBuffer, 0, 0 );
                }

                texUnit += mReservedTexSlots;

                if( !mPrePassTextures->empty() )
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit++, ( *mPrePassTextures )[0], 0 );
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit++, ( *mPrePassTextures )[1], 0 );
                }

                if( mPrePassMsaaDepthTexture )
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit++, mPrePassMsaaDepthTexture, 0 );
                }

                if( mSsrTexture )
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit++, mSsrTexture, 0 );
                }

                if( mIrradianceVolume )
                {
                    TextureGpu *irradianceTex = mIrradianceVolume->getIrradianceVolumeTexture();
                    const HlmsSamplerblock *samplerblock = mIrradianceVolume->getIrradSamplerblock();

                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit, irradianceTex, samplerblock );
                    ++texUnit;
                }

                if( mUsingAreaLightMasks )
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit, mAreaLightMasks, mAreaLightMasksSamplerblock );
                    ++texUnit;
                }

                if( mLtcMatrixTexture )
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit, mLtcMatrixTexture, mAreaLightMasksSamplerblock );
                    ++texUnit;
                }

                for( size_t i = 0; i < 3u; ++i )
                {
                    if( mDecalsTextures[i] && ( i != 2u || !mDecalsDiffuseMergedEmissive ) )
                    {
                        *commandBuffer->addCommand<CbTexture>() =
                            CbTexture( (uint16)texUnit, mDecalsTextures[i], mDecalsSamplerblock );
                        ++texUnit;
                    }
                }

                // We changed HlmsType, rebind the shared textures.
                FastArray<TextureGpu *>::const_iterator itor = mPreparedPass.shadowMaps.begin();
                FastArray<TextureGpu *>::const_iterator end = mPreparedPass.shadowMaps.end();
                while( itor != end )
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit, *itor, mCurrentShadowmapSamplerblock );
                    ++texUnit;
                    ++itor;
                }

                if( mParallaxCorrectedCubemap && !mParallaxCorrectedCubemap->isRendering() )
                {
                    TextureGpu *pccTexture = mParallaxCorrectedCubemap->getBindTexture();
                    const HlmsSamplerblock *samplerblock =
                        mParallaxCorrectedCubemap->getBindTrilinearSamplerblock();
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit, pccTexture, samplerblock );
                    ++texUnit;
                }
            }

            mLastDescTexture = 0;
            mLastDescSampler = 0;
            mLastMovableObject = 0;
            mLastBoundPool = 0;

            // layout(binding = 2) uniform InstanceBuffer {} instance
            if( mCurrentConstBuffer < mConstBuffers.size() &&
                (size_t)( ( mCurrentMappedConstBuffer - mStartMappedConstBuffer ) + 4 ) <=
                    mCurrentConstBufferSize )
            {
                *commandBuffer->addCommand<CbShaderBuffer>() =
                    CbShaderBuffer( VertexShader, 2, mConstBuffers[mCurrentConstBuffer], 0, 0 );
                *commandBuffer->addCommand<CbShaderBuffer>() =
                    CbShaderBuffer( PixelShader, 2, mConstBuffers[mCurrentConstBuffer], 0, 0 );
            }

            // rebindTexBuffer( commandBuffer );

#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
            mLastBoundPlanarReflection = 0u;
            if( mHasPlanarReflections )
                ++texUnit;  // We do not bind this texture now, but its slot is reserved.
#endif
            mListener->hlmsTypeChanged( casterPass, commandBuffer, datablock, 0u );
        }

        // Don't bind the material buffer on caster passes (important to keep
        // MDI & auto-instancing running on shadow map passes)
        if( mLastBoundPool != datablock->getAssignedPool() )
        {
            // layout(binding = 1) uniform MaterialBuf {} materialArray
            const ConstBufferPool::BufferPool *newPool = datablock->getAssignedPool();
            *commandBuffer->addCommand<CbShaderBuffer>() =
                CbShaderBuffer( VertexShader, 1, newPool->materialBuffer, 0,
                                (uint32)newPool->materialBuffer->getTotalSizeBytes() );
            *commandBuffer->addCommand<CbShaderBuffer>() =
                CbShaderBuffer( PixelShader, 1, newPool->materialBuffer, 0,
                                (uint32)newPool->materialBuffer->getTotalSizeBytes() );
            mLastBoundPool = newPool;
        }

        if( mLastMovableObject != queuedRenderable.movableObject )
        {
            // Different Terra? Must change textures then.
            const Terra *terraObj = static_cast<const Terra *>( queuedRenderable.movableObject );
            *commandBuffer->addCommand<CbTexture>() =
                CbTexture( mTexBufUnitSlotEnd + 0u, terraObj->getHeightMapTex() );
            if( !casterPass )
            {
                // Do not bind these textures during caster pass:
                //  1. They're not actually used/needed
                //  2. We haven't transitioned them to Texture yet

                // We need one for terrainNormals & terrainShadows. Reuse an existing samplerblock
                *commandBuffer->addCommand<CbTexture>() = CbTexture(
                    mTexBufUnitSlotEnd + 1u, terraObj->getNormalMapTex(), mAreaLightMasksSamplerblock );
                *commandBuffer->addCommand<CbTexture>() = CbTexture(
                    mTexBufUnitSlotEnd + 2u, terraObj->_getShadowMapTex(), mAreaLightMasksSamplerblock );
            }
            mLastMovableObject = queuedRenderable.movableObject;
        }

        uint32 *RESTRICT_ALIAS currentMappedConstBuffer = mCurrentMappedConstBuffer;

        //---------------------------------------------------------------------------
        //                          ---- VERTEX SHADER ----
        //---------------------------------------------------------------------------
        // We need to correct currentMappedConstBuffer to point to the right texture buffer's
        // offset, which may not be in sync if the previous draw had skeletal animation.
        bool exceedsConstBuffer = (size_t)( ( currentMappedConstBuffer - mStartMappedConstBuffer ) +
                                            12 ) > mCurrentConstBufferSize;

        if( exceedsConstBuffer )
            currentMappedConstBuffer = mapNextConstBuffer( commandBuffer );

        const TerrainCell *terrainCell = static_cast<const TerrainCell *>( queuedRenderable.renderable );

        terrainCell->uploadToGpu( currentMappedConstBuffer );
        currentMappedConstBuffer += 16u;

        //---------------------------------------------------------------------------
        //                          ---- PIXEL SHADER ----
        //---------------------------------------------------------------------------

        if( !casterPass || datablock->getAlphaTest() != CMPF_ALWAYS_PASS )
        {
#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
            if( mHasPlanarReflections && ( queuedRenderable.renderable->mCustomParameter & 0x80 ) &&
                mLastBoundPlanarReflection != queuedRenderable.renderable->mCustomParameter )
            {
                const uint8 activeActorIdx = queuedRenderable.renderable->mCustomParameter & 0x7F;
                TextureGpu *planarReflTex = mPlanarReflections->getTexture( activeActorIdx );
                *commandBuffer->addCommand<CbTexture>() = CbTexture(
                    uint16( mTexUnitSlotStart - 1u ), planarReflTex, mPlanarReflectionsSamplerblock );
                mLastBoundPlanarReflection = queuedRenderable.renderable->mCustomParameter;
            }
#endif
            if( datablock->mTexturesDescSet != mLastDescTexture )
            {
                if( datablock->mTexturesDescSet )
                {
                    // Rebind textures
                    size_t texUnit = mTexUnitSlotStart;

                    *commandBuffer->addCommand<CbTextures>() =
                        CbTextures( (uint16)texUnit, std::numeric_limits<uint16>::max(),
                                    datablock->mTexturesDescSet );

                    if( !mHasSeparateSamplers )
                    {
                        *commandBuffer->addCommand<CbSamplers>() =
                            CbSamplers( (uint16)texUnit, datablock->mSamplersDescSet );
                    }
                    // texUnit += datablock->mTexturesDescSet->mTextures.size();
                }

                mLastDescTexture = datablock->mTexturesDescSet;
            }

            if( datablock->mSamplersDescSet != mLastDescSampler && mHasSeparateSamplers )
            {
                if( datablock->mSamplersDescSet )
                {
                    // Bind samplers
                    size_t texUnit = mTexUnitSlotStart;
                    *commandBuffer->addCommand<CbSamplers>() =
                        CbSamplers( (uint16)texUnit, datablock->mSamplersDescSet );
                    mLastDescSampler = datablock->mSamplersDescSet;
                }
            }
        }

        mCurrentMappedConstBuffer = currentMappedConstBuffer;

        return uint32( ( ( mCurrentMappedConstBuffer - mStartMappedConstBuffer ) >> 4u ) - 1u );
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerra::getDefaultPaths( String &outDataFolderPath, StringVector &outLibraryFoldersPaths )
    {
        // We need to know what RenderSystem is currently in use, as the
        // name of the compatible shading language is part of the path
        Ogre::RenderSystem *renderSystem = Ogre::Root::getSingleton().getRenderSystem();
        Ogre::String shaderSyntax = "GLSL";
        if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
            shaderSyntax = "HLSL";
        else if( renderSystem->getName() == "Metal Rendering Subsystem" )
            shaderSyntax = "Metal";

        // Fill the library folder paths with the relevant folders
        outLibraryFoldersPaths.clear();
        outLibraryFoldersPaths.push_back( "Hlms/Common/" + shaderSyntax );
        outLibraryFoldersPaths.push_back( "Hlms/Common/Any" );
        outLibraryFoldersPaths.push_back( "Hlms/Pbs/Any" );
#ifdef OGRE_BUILD_COMPONENT_ATMOSPHERE
        outLibraryFoldersPaths.push_back( "Hlms/Pbs/Any/Atmosphere" );
#endif
        outLibraryFoldersPaths.push_back( "Hlms/Pbs/Any/Main" );
        outLibraryFoldersPaths.push_back( "Hlms/Pbs/" + shaderSyntax );
        outLibraryFoldersPaths.push_back( "Hlms/Terra/Any" );

        // Fill the data folder path
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
    void HlmsTerra::_collectSamplerblocks( set<const HlmsSamplerblock *>::type &outSamplerblocks,
                                           const HlmsDatablock *datablock ) const
    {
        HlmsJsonTerra::collectSamplerblocks( datablock, outSamplerblocks );
    }
#endif
    //-----------------------------------------------------------------------------------
    HlmsDatablock *HlmsTerra::createDatablockImpl( IdString datablockName,
                                                   const HlmsMacroblock *macroblock,
                                                   const HlmsBlendblock *blendblock,
                                                   const HlmsParamVec &paramVec )
    {
        return OGRE_NEW HlmsTerraDatablock( datablockName, this, macroblock, blendblock, paramVec );
    }
}  // namespace Ogre
