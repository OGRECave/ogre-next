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

#include "OgreHlmsUnlit.h"

#include "CommandBuffer/OgreCbShaderBuffer.h"
#include "CommandBuffer/OgreCbTexture.h"
#include "CommandBuffer/OgreCommandBuffer.h"
#include "Compositor/OgreCompositorShadowNode.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h"
#include "OgreCamera.h"
#include "OgreDescriptorSetTexture.h"
#include "OgreHighLevelGpuProgram.h"
#include "OgreHighLevelGpuProgramManager.h"
#include "OgreHlmsListener.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsUnlitDatablock.h"
#include "OgreLogManager.h"
#include "OgreLwString.h"
#include "OgreProfiler.h"
#include "OgreRenderQueue.h"
#include "OgreRootLayout.h"
#include "OgreSceneManager.h"
#include "OgreTextureGpu.h"
#include "OgreUnlitProperty.h"
#include "OgreViewport.h"
#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreStagingBuffer.h"
#include "Vao/OgreTexBufferPacked.h"
#include "Vao/OgreVaoManager.h"

#if !OGRE_NO_JSON
#    include "OgreHlmsJsonUnlit.h"
#endif

namespace Ogre
{
    extern const String c_unlitBlendModes[];

    HlmsUnlit::HlmsUnlit( Archive *dataFolder, ArchiveVec *libraryFolders ) :
        HlmsUnlit( dataFolder, libraryFolders, HlmsUnlitDatablock::MaterialSizeInGpuAligned )
    {
    }
    HlmsUnlit::HlmsUnlit( Archive *dataFolder, ArchiveVec *libraryFolders, HlmsTypes type,
                          const String &typeName ) :
        HlmsUnlit( dataFolder, libraryFolders, type, typeName,
                   HlmsUnlitDatablock::MaterialSizeInGpuAligned )
    {
    }

    HlmsUnlit::HlmsUnlit( Archive *dataFolder, ArchiveVec *libraryFolders, uint32 constBufferSize ) :
        HlmsBufferManager( HLMS_UNLIT, "unlit", dataFolder, libraryFolders ),
        ConstBufferPool( constBufferSize, ExtraBufferParams( 64 * NUM_UNLIT_TEXTURE_TYPES ) ),
        mCurrentPassBuffer( 0 ),
        mLastBoundPool( 0 ),
        mHasSeparateSamplers( 0 ),
        mLastDescTexture( 0 ),
        mLastDescSampler( 0 ),
        mConstantBiasScale( 0.1f ),
        mUsingInstancedStereo( false ),
        mDefaultGenerateMipmaps( false ),
        mUsingExponentialShadowMaps( false ),
        mEsmK( 600u ),
        mReservedTexBufferSlots( 2u ),  // Vertex shader consumes 2 slots with its tbuffers.
        mReservedTexSlots( 0u ),
        mTexUnitSlotStart( 2u )
    {
        // Override defaults
        mLightGatheringMode = LightGatherNone;

        // Always use this strategy, even on mobile
        mOptimizationStrategy = LowerCpuOverhead;

        // Always an identity matrix
        mPreparedPass.viewProjMatrix[4] = Matrix4::IDENTITY;

        mParticleSystemConstSlot = 3u;  // It is always 3.
        // It is always 0.
        // (0 is world matrix, we don't need it for particles. 1 is animation matrix)
        mParticleSystemSlot = 0u;
    }
    HlmsUnlit::HlmsUnlit( Archive *dataFolder, ArchiveVec *libraryFolders, HlmsTypes type,
                          const String &typeName, uint32 constBufferSize ) :
        HlmsBufferManager( type, typeName, dataFolder, libraryFolders ),
        ConstBufferPool( constBufferSize, ExtraBufferParams( 64 * NUM_UNLIT_TEXTURE_TYPES ) ),
        mCurrentPassBuffer( 0 ),
        mLastBoundPool( 0 ),
        mLastDescTexture( 0 ),
        mLastDescSampler( 0 ),
        mConstantBiasScale( 0.1f ),
        mUsingInstancedStereo( false ),
        mUsingExponentialShadowMaps( false ),
        mEsmK( 600u ),
        mReservedTexBufferSlots( 2u ),  // Vertex shader consumes 2 slots with its tbuffers.
        mReservedTexSlots( 0u ),
        mTexUnitSlotStart( 2u )
    {
        // Override defaults
        mLightGatheringMode = LightGatherNone;

        // Always use this strategy, even on mobile
        mOptimizationStrategy = LowerCpuOverhead;

        // Always an identity matrix
        mPreparedPass.viewProjMatrix[4] = Matrix4::IDENTITY;

        mParticleSystemConstSlot = 3u;  // It is always 3.
        // It is always 0.
        // (0 is world matrix, we don't need it for particles. 1 is animation matrix)
        mParticleSystemSlot = 0u;
    }
    //-----------------------------------------------------------------------------------
    HlmsUnlit::~HlmsUnlit() { destroyAllBuffers(); }
    //-----------------------------------------------------------------------------------
    void HlmsUnlit::_changeRenderSystem( RenderSystem *newRs )
    {
        if( mVaoManager )
            destroyAllBuffers();

        ConstBufferPool::_changeRenderSystem( newRs );
        HlmsBufferManager::_changeRenderSystem( newRs );

        if( newRs )
        {
            HlmsDatablockMap::const_iterator itor = mDatablocks.begin();
            HlmsDatablockMap::const_iterator end = mDatablocks.end();

            while( itor != end )
            {
                assert( dynamic_cast<HlmsUnlitDatablock *>( itor->second.datablock ) );
                HlmsUnlitDatablock *datablock =
                    static_cast<HlmsUnlitDatablock *>( itor->second.datablock );

                requestSlot( datablock->mNumEnabledAnimationMatrices != 0, datablock,
                             datablock->mNumEnabledAnimationMatrices != 0 );
                ++itor;
            }

            const RenderSystemCapabilities *caps = newRs->getCapabilities();
            mHasSeparateSamplers = caps->hasCapability( RSC_SEPARATE_SAMPLERS_FROM_TEXTURES );
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlit::setupRootLayout( RootLayout &rootLayout, const size_t tid )
    {
        DescBindingRange *descBindingRanges = rootLayout.mDescBindingRanges[0];

        if( getProperty( tid, HlmsBaseProp::ParticleSystem ) == 0 )
            descBindingRanges[DescBindingTypes::ConstBuffer].end = 3u;
        else
            descBindingRanges[DescBindingTypes::ConstBuffer].end = 4u;

        if( getProperty( tid, UnlitProperty::TextureMatrix ) == 0 )
            descBindingRanges[DescBindingTypes::ReadOnlyBuffer].end = 1u;
        else
            descBindingRanges[DescBindingTypes::ReadOnlyBuffer].end = 2u;

        if( getProperty( tid, HlmsBaseProp::BlueNoise ) )
        {
            descBindingRanges[DescBindingTypes::Texture].end =
                (uint16)getProperty( tid, "samplerStateStart" );
            descBindingRanges[DescBindingTypes::Texture].start =
                descBindingRanges[DescBindingTypes::Texture].end - 1u;
        }

        rootLayout.mBaked[1] = true;
        DescBindingRange *bakedRanges = rootLayout.mDescBindingRanges[1];
        bakedRanges[DescBindingTypes::Texture].start = (uint16)getProperty( tid, "samplerStateStart" );
        bakedRanges[DescBindingTypes::Texture].end =
            bakedRanges[DescBindingTypes::Texture].start +
            (uint16)getProperty( tid, UnlitProperty::NumTextures );

        bakedRanges[DescBindingTypes::Sampler].start = bakedRanges[DescBindingTypes::Texture].start;
        bakedRanges[DescBindingTypes::Sampler].end =
            bakedRanges[DescBindingTypes::Sampler].start +
            (uint16)getProperty( tid, UnlitProperty::NumSamplers );

        mListener->setupRootLayout( rootLayout, mT[tid].setProperties, tid );
    }
    //-----------------------------------------------------------------------------------
    const HlmsCache *HlmsUnlit::createShaderCacheEntry( uint32 renderableHash,
                                                        const HlmsCache &passCache, uint32 finalHash,
                                                        const QueuedRenderable &queuedRenderable,
                                                        HlmsCache *reservedStubEntry, const size_t tid )
    {
        OgreProfileExhaustive( "HlmsUnlit::createShaderCacheEntry" );

        const HlmsCache *retVal = Hlms::createShaderCacheEntry(
            renderableHash, passCache, finalHash, queuedRenderable, reservedStubEntry, tid );

        if( mShaderProfile != "glsl" )
        {
            mListener->shaderCacheEntryCreated( mShaderProfile, retVal, passCache, mT[tid].setProperties,
                                                queuedRenderable, tid );
            return retVal;  // D3D embeds the texture slots in the shader.
        }

        // Set samplers.
        /*assert(
         dynamic_cast<const HlmsUnlitDatablock*>( queuedRenderable.renderable->getDatablock() ) );
        const HlmsUnlitDatablock *datablock = static_cast<const HlmsUnlitDatablock*>(
                                                queuedRenderable.renderable->getDatablock() );*/

        GpuProgramParametersSharedPtr vsParams = retVal->pso.vertexShader->getDefaultParameters();
        if( mVaoManager->readOnlyIsTexBuffer() )
        {
            vsParams->setNamedConstant( "worldMatBuf", 0 );

            const int32 texMatrixSlot = getProperty( tid, UnlitProperty::TextureMatrix );
            if( texMatrixSlot )
                vsParams->setNamedConstant( "animationMatrixBuf", texMatrixSlot );
        }

        mListener->shaderCacheEntryCreated( mShaderProfile, retVal, passCache, mT[tid].setProperties,
                                            queuedRenderable, tid );

        mRenderSystem->_setPipelineStateObject( &retVal->pso );

        mRenderSystem->bindGpuProgramParameters( GPT_VERTEX_PROGRAM, vsParams, GPV_ALL );
        if( retVal->pso.pixelShader )
        {
            GpuProgramParametersSharedPtr psParams = retVal->pso.pixelShader->getDefaultParameters();
            mRenderSystem->bindGpuProgramParameters( GPT_FRAGMENT_PROGRAM, psParams, GPV_ALL );
        }

        if( !mRenderSystem->getCapabilities()->hasCapability( RSC_CONST_BUFFER_SLOTS_IN_SHADER ) )
        {
            // Setting it to the vertex shader will set it to the PSO actually.
            retVal->pso.vertexShader->setUniformBlockBinding( "PassBuffer", 0 );
            retVal->pso.vertexShader->setUniformBlockBinding( "MaterialBuf", 1 );
            retVal->pso.vertexShader->setUniformBlockBinding( "InstanceBuffer", 2 );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlit::setTextureProperty( const size_t tid, LwString &propertyName,
                                        HlmsUnlitDatablock *datablock, uint8 texType )
    {
        const size_t basePropSize = propertyName.size();  // diffuse_map0

        const uint8 idx = datablock->getIndexToDescriptorTexture( texType );

        if( idx != NUM_UNLIT_TEXTURE_TYPES )
        {
            setProperty( tid, propertyName.c_str(), 1 );

            propertyName.resize( basePropSize );
            propertyName.a( "_idx" );  // diffuse_map0_idx
            setProperty( tid, propertyName.c_str(), idx );

            const TextureGpu *texture = datablock->getTexture( texType );
            if( texture && texture->getInternalTextureType() == TextureTypes::Type2DArray )
            {
                propertyName.resize( basePropSize );
                propertyName.a( "_array" );  // diffuse_map0_array
                setProperty( tid, propertyName.c_str(), 1 );
            }

            if( mHasSeparateSamplers )
            {
                const uint8 samplerIdx = datablock->getIndexToDescriptorSampler( texType );
                propertyName.resize( basePropSize );
                propertyName.a( "_sampler" );  // diffuse_map0_sampler
                setProperty( tid, propertyName.c_str(), samplerIdx );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlit::calculateHashFor( Renderable *renderable, uint32 &outHash, uint32 &outCasterHash )
    {
        assert( dynamic_cast<HlmsUnlitDatablock *>( renderable->getDatablock() ) );
        HlmsUnlitDatablock *datablock = static_cast<HlmsUnlitDatablock *>( renderable->getDatablock() );
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
    struct UvOutput
    {
        int32 uvSource;
        int32 texUnit;
        bool isAnimated;
    };
    typedef vector<UvOutput>::type UvOutputVec;
    void HlmsUnlit::calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces )
    {
        assert( dynamic_cast<HlmsUnlitDatablock *>( renderable->getDatablock() ) );
        HlmsUnlitDatablock *datablock = static_cast<HlmsUnlitDatablock *>( renderable->getDatablock() );

        setProperty( kNoTid, HlmsBaseProp::Skeleton, 0 );
        setProperty( kNoTid, HlmsBaseProp::Normal, 0 );
        setProperty( kNoTid, HlmsBaseProp::QTangent, 0 );
        setProperty( kNoTid, HlmsBaseProp::Tangent, 0 );
        setProperty( kNoTid, HlmsBaseProp::Tangent4, 0 );
        setProperty( kNoTid, HlmsBaseProp::BonesPerVertex, 0 );

        if( datablock->mTexturesDescSet )
        {
            setProperty( kNoTid, UnlitProperty::NumTextures,
                         static_cast<int32>( datablock->mTexturesDescSet->mTextures.size() ) );

            uint32 currentTextureIdx = 0u;

            FastArray<const TextureGpu *>::const_iterator itor =
                datablock->mTexturesDescSet->mTextures.begin();
            FastArray<const TextureGpu *>::const_iterator end =
                datablock->mTexturesDescSet->mTextures.end();

            while( itor != end )
            {
                if( ( *itor )->getInternalTextureType() == TextureTypes::Type2DArray )
                {
                    char tmpBuffer[32];
                    LwString propName( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
                    propName.a( "is_texture", currentTextureIdx, "_array" );
                    setProperty( kNoTid, propName.c_str(), 1 );
                }

                ++currentTextureIdx;
                ++itor;
            }
        }

        setProperty( kNoTid, UnlitProperty::Diffuse, datablock->mHasColour );

        if( datablock->mSamplersDescSet )
        {
            setProperty( kNoTid, UnlitProperty::NumSamplers,
                         (int32)datablock->mSamplersDescSet->mSamplers.size() );
        }

        bool hasAnimationMatrices = false;
        UvOutputVec uvOutputs;
        bool hasPlanarReflection = false;

        int32 maxUsedTexUnitPlusOne = 0;

        char tmpBuffer[64];
        LwString diffuseMapN( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
        diffuseMapN.a( "diffuse_map" );
        const size_t basePropSize = diffuseMapN.size();

        for( uint8 i = 0; i < NUM_UNLIT_TEXTURE_TYPES; ++i )
        {
            diffuseMapN.resize( basePropSize );
            diffuseMapN.a( i );  // diffuse_map0

            // Set whether the texture is used.
            setTextureProperty( kNoTid, diffuseMapN, datablock, i );

            // Sanity check.
            bool hasTexture = datablock->getTexture( i ) != 0;
            if( hasTexture &&
                getProperty( kNoTid, *HlmsBaseProp::UvCountPtrs[datablock->mUvSource[i]] ) < 2 )
            {
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                             "Renderable must have at least 2 coordinates in UV set #" +
                                 StringConverter::toString( datablock->mUvSource[i] ) +
                                 ". Either change the mesh, or change the UV source settings",
                             "HlmsUnlit::calculateHashForPreCreate" );
            }

            if( hasTexture )
            {
                if( datablock->getEnablePlanarReflection( i ) )
                {
                    diffuseMapN.resize( basePropSize + 1u );
                    diffuseMapN.a( "_reflection" );  // diffuse_map0_reflection
                    IdString diffuseMapNReflection( diffuseMapN.c_str() );
                    setProperty( kNoTid, diffuseMapNReflection, 1 );
                    hasPlanarReflection = true;
                }

                maxUsedTexUnitPlusOne = i + 1;
            }

            // Set the blend mode
            uint8 blendMode = datablock->mBlendModes[i];
            inOutPieces[PixelShader][*UnlitProperty::DiffuseMapPtrs[i].blendModeIndex] =
                "@insertpiece( " + c_unlitBlendModes[blendMode] + ")";

            // Match the texture unit to the UV output.
            if( hasTexture )
            {
                const IdString &uvSourceSwizzleN = *UnlitProperty::DiffuseMapPtrs[i].uvSourceSwizzle;

                if( datablock->mEnabledAnimationMatrices[i] )
                {
                    // Animated outputs need their own entry
                    UvOutput uvOutput;
                    uvOutput.uvSource = datablock->mUvSource[i];
                    uvOutput.texUnit = i;
                    uvOutput.isAnimated = true;
                    hasAnimationMatrices = true;

                    setProperty( kNoTid, *UnlitProperty::DiffuseMapPtrs[i].uvSource,
                                 static_cast<int32>( uvOutputs.size() >> 1u ) );
                    inOutPieces[PixelShader][uvSourceSwizzleN] = uvOutputs.size() % 2 ? "zw" : "xy";

                    uvOutputs.push_back( uvOutput );
                }
                else
                {
                    // Non-animated outputs may share their entry with another non-animated one.
                    UvOutputVec::iterator itor = uvOutputs.begin();
                    UvOutputVec::iterator end = uvOutputs.end();

                    while( itor != end &&
                           ( itor->uvSource != datablock->mUvSource[i] || itor->isAnimated ) )
                    {
                        ++itor;
                    }

                    const ptrdiff_t rawIdx = itor - uvOutputs.begin();
                    int32 idx = static_cast<int32>( rawIdx >> 1u );
                    setProperty( kNoTid, *UnlitProperty::DiffuseMapPtrs[i].uvSource, idx );
                    inOutPieces[PixelShader][uvSourceSwizzleN] = rawIdx % 2 ? "zw" : "xy";

                    if( itor == end )
                    {
                        // Entry didn't exist yet.
                        UvOutput uvOutput;
                        uvOutput.uvSource = datablock->mUvSource[i];
                        uvOutput.texUnit = 0;  // Not used
                        uvOutput.isAnimated = false;

                        uvOutputs.push_back( uvOutput );
                    }
                }

                // Generate the texture swizzle for the pixel shader.
                diffuseMapN.resize( basePropSize + 1u );
                diffuseMapN.a( "_tex_swizzle" );
                IdString diffuseMapNTexSwizzle( diffuseMapN.c_str() );
                String texSwizzle;
                texSwizzle.reserve( 4 );

                for( size_t j = 0; j < 4; ++j )
                {
                    const size_t swizzleMask =
                        ( datablock->mTextureSwizzles[i] >> ( 6u - j * 2u ) ) & 0x03u;
                    if( swizzleMask == HlmsUnlitDatablock::R_MASK )
                        texSwizzle += "x";
                    else if( swizzleMask == HlmsUnlitDatablock::G_MASK )
                        texSwizzle += "y";
                    else if( swizzleMask == HlmsUnlitDatablock::B_MASK )
                        texSwizzle += "z";
                    else if( swizzleMask == HlmsUnlitDatablock::A_MASK )
                        texSwizzle += "w";
                }
                inOutPieces[PixelShader][diffuseMapNTexSwizzle] = texSwizzle;
            }
        }

        if( datablock->mTexturesDescSet )
            setProperty( kNoTid, UnlitProperty::DiffuseMap, maxUsedTexUnitPlusOne );

        if( hasAnimationMatrices )
            setProperty( kNoTid, UnlitProperty::TextureMatrix, 1 );

        if( hasPlanarReflection )
        {
            setProperty( kNoTid, HlmsBaseProp::VPos, 1 );
            setProperty( kNoTid, UnlitProperty::HasPlanarReflections, 1 );
        }

        size_t halfUvOutputs = ( uvOutputs.size() + 1u ) >> 1u;
        setProperty( kNoTid, UnlitProperty::OutUvCount, static_cast<int32>( uvOutputs.size() ) );
        setProperty( kNoTid, UnlitProperty::OutUvHalfCount, static_cast<int32>( halfUvOutputs ) );

        for( size_t i = 0; i < halfUvOutputs; ++i )
        {
            // Decide whether to use vec4 or vec2 in VStoPS_block piece:
            // vec4 uv0; //--> When interpolant contains two uvs in one
            // vec2 uv0; //--> When interpolant contains the last UV (uvOutputs.size() is odd)
            setProperty( kNoTid, "out_uv_half_count" + StringConverter::toString( i ),
                         ( i << 1u ) == ( uvOutputs.size() - 1u ) ? 2 : 4 );
        }

        for( size_t i = 0; i < uvOutputs.size(); ++i )
        {
            String outPrefix = "out_uv" + StringConverter::toString( i );

            setProperty( kNoTid, outPrefix + "_out_uv", int32( i >> 1u ) );
            setProperty( kNoTid, outPrefix + "_texture_matrix", uvOutputs[i].isAnimated );
            setProperty( kNoTid, outPrefix + "_tex_unit", uvOutputs[i].texUnit );
            setProperty( kNoTid, outPrefix + "_source_uv", uvOutputs[i].uvSource );
            inOutPieces[VertexShader][outPrefix + "_swizzle"] = i % 2 ? "zw" : "xy";
        }

        if( mFastShaderBuildHack )
            setProperty( kNoTid, UnlitProperty::MaterialsPerBuffer, static_cast<int>( 2 ) );
        else
            setProperty( kNoTid, UnlitProperty::MaterialsPerBuffer, static_cast<int>( mSlotsPerPool ) );
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlit::calculateHashForPreCaster( Renderable *renderable, PiecesMap *inOutPieces,
                                               const PiecesMap *normalPassPieces )
    {
        // HlmsUnlitDatablock *datablock = static_cast<HlmsUnlitDatablock*>(
        //                                              renderable->getDatablock() );

        HlmsDatablock *datablock = renderable->getDatablock();
        const bool hasAlphaTestOrHash =
            datablock->getAlphaTest() != CMPF_ALWAYS_PASS || datablock->getAlphaHashing();

        if( !hasAlphaTestOrHash )
        {
            HlmsPropertyVec::iterator itor = mT[kNoTid].setProperties.begin();
            HlmsPropertyVec::iterator endt = mT[kNoTid].setProperties.end();

            while( itor != endt )
            {
                if( itor->keyName != UnlitProperty::HwGammaRead &&
                    // itor->keyName != UnlitProperty::UvDiffuse &&
                    itor->keyName != HlmsPsoProp::InputLayoutId &&
                    itor->keyName != HlmsBaseProp::Skeleton &&
                    itor->keyName != HlmsBaseProp::BonesPerVertex &&
                    itor->keyName != HlmsBaseProp::DualParaboloidMapping )
                {
                    itor = mT[kNoTid].setProperties.erase( itor );
                    endt = mT[kNoTid].setProperties.end();
                }
                else
                {
                    ++itor;
                }
            }
        }
        else
        {
            inOutPieces[VertexShader] = normalPassPieces[VertexShader];
            inOutPieces[PixelShader] = normalPassPieces[PixelShader];
        }

        setupSharedBasicProperties( renderable );

        if( mFastShaderBuildHack )
            setProperty( kNoTid, UnlitProperty::MaterialsPerBuffer, static_cast<int>( 2 ) );
        else
            setProperty( kNoTid, UnlitProperty::MaterialsPerBuffer, static_cast<int>( mSlotsPerPool ) );
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlit::notifyPropertiesMergedPreGenerationStep( const size_t tid )
    {
        Hlms::notifyPropertiesMergedPreGenerationStep( tid );

        const int32 samplerStateStart = getProperty( tid, UnlitProperty::SamplerStateStart );
        int32 texUnit = samplerStateStart;

        if( getProperty( tid, HlmsBaseProp::BlueNoise ) )
            setTextureReg( tid, PixelShader, "blueNoiseTex", texUnit - 1 );

        {
            char tmpData[32];
            LwString texName = LwString::FromEmptyPointer( tmpData, sizeof( tmpData ) );
            texName = "textureMaps";
            const size_t baseTexSize = texName.size();

            char tmpBuffer[32];
            LwString isTexArrayProp = LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) );
            isTexArrayProp = "is_texture";
            const size_t baseIsTexArrayProp = isTexArrayProp.size();

            const int32 numTextures = getProperty( tid, UnlitProperty::NumTextures );

            for( int32 i = 0; i < numTextures; ++i )
            {
                texName.resize( baseTexSize );
                isTexArrayProp.resize( baseIsTexArrayProp );

                isTexArrayProp.a( i, "_array" );  // is_texture0_array
                if( getProperty( tid, isTexArrayProp.c_str() ) )
                    texName.a( "Array" );  // textureMapsArray0
                texName.a( i );            // textureMaps0 or textureMapsArray0

                setTextureReg( tid, PixelShader, texName.c_str(), texUnit++ );
            }
        }

        if( getProperty( HlmsBaseProp::ParticleSystem ) )
        {
            setProperty( tid, "particleSystemConstSlot", mParticleSystemConstSlot );
            if( mVaoManager->readOnlyIsTexBuffer() )
                setTextureReg( tid, VertexShader, "particleSystemGpuData", mParticleSystemSlot );
            else
                setProperty( tid, "particleSystemGpuData", mParticleSystemSlot );
        }
    }
    //-----------------------------------------------------------------------------------
    HlmsCache HlmsUnlit::preparePassHash( const CompositorShadowNode *shadowNode, bool casterPass,
                                          bool dualParaboloid, SceneManager *sceneManager )
    {
        OgreProfileExhaustive( "HlmsUnlit::preparePassHash" );

        mT[kNoTid].setProperties.clear();

        // Set the properties and create/retrieve the cache.
        if( casterPass )
        {
            setProperty( kNoTid, HlmsBaseProp::ShadowCaster, 1 );
            if( mUsingExponentialShadowMaps )
                setProperty( kNoTid, UnlitProperty::ExponentialShadowMaps, mEsmK );

            const CompositorPass *pass = sceneManager->getCurrentCompositorPass();
            if( pass )
            {
                const Light *light =
                    shadowNode->getLightAssociatedWith( pass->getDefinition()->mShadowMapIdx );
                if( light->getType() == Light::LT_DIRECTIONAL )
                    setProperty( kNoTid, HlmsBaseProp::ShadowCasterDirectional, 1 );
                else if( light->getType() == Light::LT_POINT )
                    setProperty( kNoTid, HlmsBaseProp::ShadowCasterPoint, 1 );
            }
        }

        const CompositorPass *pass = sceneManager->getCurrentCompositorPass();
        CompositorPassSceneDef const *passSceneDef = 0;

        if( pass && pass->getType() == PASS_SCENE )
        {
            OGRE_ASSERT_HIGH( dynamic_cast<const CompositorPassSceneDef *>( pass->getDefinition() ) );
            passSceneDef = static_cast<const CompositorPassSceneDef *>( pass->getDefinition() );

            if( passSceneDef->mInstancedStereo )
                setProperty( kNoTid, HlmsBaseProp::InstancedStereo, 1 );
        }
        const bool isInstancedStereo = passSceneDef && passSceneDef->mInstancedStereo;

        RenderPassDescriptor *renderPassDesc = mRenderSystem->getCurrentPassDescriptor();
        setProperty( kNoTid, HlmsBaseProp::ShadowUsesDepthTexture,
                     ( renderPassDesc->getNumColourEntries() > 0 ) ? 0 : 1 );
        setProperty( kNoTid, HlmsBaseProp::RenderDepthOnly,
                     ( renderPassDesc->getNumColourEntries() > 0 ) ? 0 : 1 );

        mTexUnitSlotStart =
            uint32( mReservedTexSlots + mReservedTexBufferSlots +
                    mListener->getNumExtraPassTextures( mT[kNoTid].setProperties, casterPass ) );

        if( mHlmsManager->getBlueNoiseTexture() )
        {
            setProperty( kNoTid, HlmsBaseProp::BlueNoise, 1 );
            ++mTexUnitSlotStart;
        }

        setProperty( kNoTid, UnlitProperty::SamplerStateStart, (int32)mTexUnitSlotStart );

        const bool bNeedsViewMatrix = msHasParticleFX2Plugin;
        if( bNeedsViewMatrix )
            setProperty( kNoTid, HlmsBaseProp::ViewMatrix, 1 );

        if( mOptimizationStrategy == LowerGpuOverhead )
            setProperty( kNoTid, UnlitProperty::LowerGpuOverhead, 1 );

        CamerasInProgress cameras = sceneManager->getCamerasInProgress();
        if( cameras.renderingCamera && cameras.renderingCamera->isReflected() )
        {
            int32 numClipDist = std::max( getProperty( kNoTid, HlmsBaseProp::PsoClipDistances ), 1 );
            setProperty( kNoTid, HlmsBaseProp::PsoClipDistances, numClipDist );
            setProperty( kNoTid, HlmsBaseProp::GlobalClipPlanes, 1 );
            // some Android devices(e.g. Mali-G77, Google Pixel 7 Pro) do not support user clip planes
            if( !mRenderSystem->getCapabilities()->hasCapability( RSC_USER_CLIP_PLANES ) )
                setProperty( kNoTid, HlmsBaseProp::EmulateClipDistances, 1 );
        }

        if( pass && pass->getAnyTargetTexture() )
        {
            setProperty( kNoTid, HlmsBaseProp::MsaaSamples,
                         pass->getAnyTargetTexture()->getSampleDescription().getColourSamples() );
        }

        mListener->preparePassHash( shadowNode, casterPass, dualParaboloid, sceneManager, this );

        PassCache passCache;
        passCache.passPso = getPassPsoForScene( sceneManager );
        passCache.properties = mT[kNoTid].setProperties;

        assert( mPassCache.size() <= (size_t)HlmsBits::PassMask &&
                "Too many passes combinations, we'll overflow the bits assigned in the hash!" );
        PassCacheVec::iterator it = std::find( mPassCache.begin(), mPassCache.end(), passCache );
        if( it == mPassCache.end() )
        {
            mPassCache.push_back( passCache );
            it = mPassCache.end() - 1;
        }

        const uint32 hash = uint32( it - mPassCache.begin() ) << HlmsBits::PassShift;

        // Fill the buffers
        HlmsCache retVal( hash, mType, HlmsPso() );
        retVal.setProperties = mT[kNoTid].setProperties;
        retVal.pso.pass = passCache.passPso;

        mUsingInstancedStereo = isInstancedStereo;
        mConstantBiasScale = cameras.renderingCamera->_getConstantBiasScale();
        Matrix4 viewMatrix = cameras.renderingCamera->getViewMatrix( true );

        Matrix4 projectionMatrix = cameras.renderingCamera->getProjectionMatrixWithRSDepth();
        Matrix4 identityProjMat;

        mRenderSystem->_convertProjectionMatrix( Matrix4::IDENTITY, identityProjMat );

        if( renderPassDesc->requiresTextureFlipping() )
        {
            projectionMatrix[1][0] = -projectionMatrix[1][0];
            projectionMatrix[1][1] = -projectionMatrix[1][1];
            projectionMatrix[1][2] = -projectionMatrix[1][2];
            projectionMatrix[1][3] = -projectionMatrix[1][3];

            identityProjMat[1][0] = -identityProjMat[1][0];
            identityProjMat[1][1] = -identityProjMat[1][1];
            identityProjMat[1][2] = -identityProjMat[1][2];
            identityProjMat[1][3] = -identityProjMat[1][3];
        }

#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        identityProjMat =
            identityProjMat *
            Quaternion( cameras.renderingCamera->getOrientationModeAngle(), Vector3::UNIT_Z );
#endif

        if( !isInstancedStereo )
        {
            mPreparedPass.viewProjMatrix[0] = projectionMatrix * viewMatrix;
            mPreparedPass.viewProjMatrix[1] = identityProjMat;
        }
        else
        {
            mPreparedPass.viewProjMatrix[1] = identityProjMat;
            mPreparedPass.viewProjMatrix[3] = identityProjMat;

            Matrix4 vrViewMat[2];
            for( size_t eyeIdx = 0u; eyeIdx < 2u; ++eyeIdx )
            {
                vrViewMat[eyeIdx] = cameras.renderingCamera->getVrViewMatrix( eyeIdx );
                Matrix4 vrProjMat = cameras.renderingCamera->getVrProjectionMatrix( eyeIdx );
                if( renderPassDesc->requiresTextureFlipping() )
                {
                    vrProjMat[1][0] = -vrProjMat[1][0];
                    vrProjMat[1][1] = -vrProjMat[1][1];
                    vrProjMat[1][2] = -vrProjMat[1][2];
                    vrProjMat[1][3] = -vrProjMat[1][3];
                }
                Matrix4 vrViewProjMatrix = vrProjMat * vrViewMat[eyeIdx];
                mPreparedPass.viewProjMatrix[eyeIdx * 2u] = vrViewProjMatrix;
            }
        }

        bool isShadowCastingPointLight =
            casterPass && getProperty( kNoTid, HlmsBaseProp::ShadowCasterPoint ) != 0;

        mT[kNoTid].setProperties.clear();

        // mat4 viewProj[2] + vec4 invWindowSize;
        size_t mapSize = ( 16 + 16 + 4 ) * 4;

        if( isInstancedStereo )
        {
            // mat4 viewProj[2] becomes => mat4 viewProj[4]
            mapSize += ( 16 + 16 ) * 4;
        }

        if( bNeedsViewMatrix )
        {
            // float4x4 view
            mapSize += 16u * 4u;
        }

        const bool isCameraReflected = cameras.renderingCamera->isReflected();
        // mat4 invViewProj
        if( ( isCameraReflected ||
              ( casterPass && ( mUsingExponentialShadowMaps || isShadowCastingPointLight ) ) ) &&
            !isInstancedStereo )
        {
            mapSize += 16u * 4u;
        }

        if( casterPass )
        {
            // vec4 viewZRow
            if( mUsingExponentialShadowMaps )
                mapSize += 4 * 4;
            // vec4 depthRange
            mapSize += ( 2 + 2 ) * 4;
            // vec4 cameraPosWS
            if( isShadowCastingPointLight )
                mapSize += 4 * 4;
        }
        // vec4 clipPlane0
        if( isCameraReflected )
            mapSize += 4 * 4;

        mapSize += mListener->getPassBufferSize( shadowNode, casterPass, dualParaboloid, sceneManager );

        // Arbitrary 2kb (minimum supported by GL is 64kb), should be enough.
        const size_t maxBufferSize = 2 * 1024;
        assert( mapSize <= maxBufferSize );

        if( mCurrentPassBuffer >= mPassBuffers.size() )
        {
            mPassBuffers.push_back(
                mVaoManager->createConstBuffer( maxBufferSize, BT_DYNAMIC_PERSISTENT, 0, false ) );
        }

        ConstBufferPacked *passBuffer = mPassBuffers[mCurrentPassBuffer++];
        float *passBufferPtr = reinterpret_cast<float *>( passBuffer->map( 0, mapSize ) );

#ifndef NDEBUG
        const float *startupPtr = passBufferPtr;
#endif
        //---------------------------------------------------------------------------
        //                          ---- VERTEX SHADER ----
        //---------------------------------------------------------------------------

        // mat4 viewProj[0];
        for( size_t i = 0; i < 16; ++i )
            *passBufferPtr++ = (float)mPreparedPass.viewProjMatrix[0][0][i];

        // mat4 viewProj[1] (identityProj);
        for( size_t i = 0; i < 16; ++i )
            *passBufferPtr++ = (float)mPreparedPass.viewProjMatrix[1][0][i];

        if( isInstancedStereo )
        {
            // mat4 viewProj[2]; (right eye)
            for( size_t i = 0u; i < 16u; ++i )
                *passBufferPtr++ = (float)mPreparedPass.viewProjMatrix[2][0][i];

            // mat4 viewProj[1] (identityProj, right eye);
            for( size_t i = 0u; i < 16u; ++i )
                *passBufferPtr++ = (float)mPreparedPass.viewProjMatrix[3][0][i];
        }

        if( bNeedsViewMatrix )
        {
            for( size_t i = 0u; i < 16u; ++i )
                *passBufferPtr++ = (float)viewMatrix[0][i];
        }

        // vec4 clipPlane0
        if( isCameraReflected )
        {
            const Plane &reflPlane = cameras.renderingCamera->getReflectionPlane();
            *passBufferPtr++ = (float)reflPlane.normal.x;
            *passBufferPtr++ = (float)reflPlane.normal.y;
            *passBufferPtr++ = (float)reflPlane.normal.z;
            *passBufferPtr++ = (float)reflPlane.d;
        }

        if( ( isCameraReflected ||
              ( casterPass && ( mUsingExponentialShadowMaps || isShadowCastingPointLight ) ) ) &&
            !isInstancedStereo )
        {
            // We don't care about the inverse of the identity proj because that's not
            // really compatible with shadows anyway.
            Matrix4 invViewProj = mPreparedPass.viewProjMatrix[0].inverse();
            for( size_t i = 0; i < 16; ++i )
                *passBufferPtr++ = (float)invViewProj[0][i];
        }

        if( casterPass )
        {
            // vec4 viewZRow
            if( mUsingExponentialShadowMaps )
            {
                *passBufferPtr++ = viewMatrix[2][0];
                *passBufferPtr++ = viewMatrix[2][1];
                *passBufferPtr++ = viewMatrix[2][2];
                *passBufferPtr++ = viewMatrix[2][3];
            }

            // vec2 depthRange;
            Real fNear, fFar;
            shadowNode->getMinMaxDepthRange( cameras.renderingCamera, fNear, fFar );
            const Real depthRange = fFar - fNear;
            *passBufferPtr++ = fNear;
            *passBufferPtr++ = 1.0f / depthRange;
            passBufferPtr += 2;

            // vec4 cameraPosWS;
            if( isShadowCastingPointLight )
            {
                const Vector3 &camPos = cameras.renderingCamera->getDerivedPosition();
                *passBufferPtr++ = (float)camPos.x;
                *passBufferPtr++ = (float)camPos.y;
                *passBufferPtr++ = (float)camPos.z;
                *passBufferPtr++ = 1.0f;
            }
        }

        TextureGpu *renderTarget = mRenderSystem->getCurrentRenderViewports()[0].getCurrentTarget();
        // vec4 invWindowSize;
        *passBufferPtr++ = 1.0f / (float)renderTarget->getWidth();
        *passBufferPtr++ = 1.0f / (float)renderTarget->getHeight();
        *passBufferPtr++ = 1.0f;
        *passBufferPtr++ = 1.0f;

        passBufferPtr = mListener->preparePassBuffer( shadowNode, casterPass, dualParaboloid,
                                                      sceneManager, passBufferPtr );

        assert( (size_t)( passBufferPtr - startupPtr ) * 4u == mapSize );

        passBuffer->unmap( UO_KEEP_PERSISTENT );

        // mTexBuffers must hold at least one buffer to prevent out of bound exceptions.
        if( mTexBuffers.empty() )
        {
            size_t bufferSize =
                std::min<size_t>( mTextureBufferDefaultSize, mVaoManager->getReadOnlyBufferMaxSize() );
            ReadOnlyBufferPacked *newBuffer = mVaoManager->createReadOnlyBuffer(
                PFG_RGBA32_FLOAT, bufferSize, BT_DYNAMIC_PERSISTENT, 0, false );
            mTexBuffers.push_back( newBuffer );
        }

        mLastDescTexture = 0;
        mLastDescSampler = 0;
        mLastBoundPool = 0;

        uploadDirtyDatablocks();

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsUnlit::fillBuffersFor( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                      bool casterPass, uint32 lastCacheHash, uint32 lastTextureHash )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "Trying to use slow-path on a desktop implementation. "
                     "Change the RenderQueue settings.",
                     "HlmsUnlit::fillBuffersFor" );
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsUnlit::fillBuffersForV1( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                        bool casterPass, uint32 lastCacheHash,
                                        CommandBuffer *commandBuffer )
    {
        return fillBuffersFor( cache, queuedRenderable, casterPass, lastCacheHash, commandBuffer, true );
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsUnlit::fillBuffersForV2( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                        bool casterPass, uint32 lastCacheHash,
                                        CommandBuffer *commandBuffer )
    {
        return fillBuffersFor( cache, queuedRenderable, casterPass, lastCacheHash, commandBuffer,
                               false );
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsUnlit::fillBuffersFor( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                      bool casterPass, uint32 lastCacheHash,
                                      CommandBuffer *commandBuffer, bool isV1 )
    {
        assert(
            dynamic_cast<const HlmsUnlitDatablock *>( queuedRenderable.renderable->getDatablock() ) );
        const HlmsUnlitDatablock *datablock =
            static_cast<const HlmsUnlitDatablock *>( queuedRenderable.renderable->getDatablock() );

        if( OGRE_EXTRACT_HLMS_TYPE_FROM_CACHE_HASH( lastCacheHash ) != mType )
        {
            // We changed HlmsType, rebind the shared textures.
            mLastDescTexture = 0;
            mLastDescSampler = 0;
            mLastBoundPool = 0;

            // layout(binding = 0) uniform PassBuffer {} pass
            ConstBufferPacked *passBuffer = mPassBuffers[mCurrentPassBuffer - 1];
            *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                VertexShader, 0, passBuffer, 0, (uint32)passBuffer->getTotalSizeBytes() );
            *commandBuffer->addCommand<CbShaderBuffer>() =
                CbShaderBuffer( PixelShader, 0, passBuffer, 0, (uint32)passBuffer->getTotalSizeBytes() );

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

            size_t texUnit = mReservedTexBufferSlots;
            if( mHlmsManager->getBlueNoiseTexture() )
            {
                *commandBuffer->addCommand<CbTexture>() =
                    CbTexture( (uint16)texUnit, mHlmsManager->getBlueNoiseTexture(), 0 );
                ++texUnit;
            }

            rebindTexBuffer( commandBuffer );

            mListener->hlmsTypeChanged( casterPass, commandBuffer, datablock, 0u );
        }

        // Don't bind the material buffer on caster passes (important to keep
        // MDI & auto-instancing running on shadow map passes)
        if( mLastBoundPool != datablock->getAssignedPool() &&
            ( !casterPass || datablock->getAlphaTest() != CMPF_ALWAYS_PASS ||
              datablock->getAlphaHashing() ) )
        {
            // layout(binding = 1) uniform MaterialBuf {} materialArray
            const ConstBufferPool::BufferPool *newPool = datablock->getAssignedPool();
            *commandBuffer->addCommand<CbShaderBuffer>() =
                CbShaderBuffer( VertexShader, 1, newPool->materialBuffer, 0,
                                (uint32)newPool->materialBuffer->getTotalSizeBytes() );
            *commandBuffer->addCommand<CbShaderBuffer>() =
                CbShaderBuffer( PixelShader, 1, newPool->materialBuffer, 0,
                                (uint32)newPool->materialBuffer->getTotalSizeBytes() );
            if( newPool->extraBuffer )
            {
                TexBufferPacked *extraBuffer = static_cast<TexBufferPacked *>( newPool->extraBuffer );
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    VertexShader, 1u, extraBuffer, 0, (uint32)extraBuffer->getTotalSizeBytes() );
            }

            mLastBoundPool = newPool;
        }

        uint32 *RESTRICT_ALIAS currentMappedConstBuffer = mCurrentMappedConstBuffer;
        float *RESTRICT_ALIAS currentMappedTexBuffer = mCurrentMappedTexBuffer;

        const Matrix4 &worldMat = queuedRenderable.movableObject->_getParentNodeFullTransform();

        bool exceedsConstBuffer = (size_t)( ( currentMappedConstBuffer - mStartMappedConstBuffer ) +
                                            4 ) > mCurrentConstBufferSize;

        const size_t minimumTexBufferSize = 16;
        bool exceedsTexBuffer = static_cast<size_t>( currentMappedTexBuffer - mStartMappedTexBuffer ) +
                                    minimumTexBufferSize >=
                                mCurrentTexBufferSize;

        if( exceedsConstBuffer || exceedsTexBuffer )
        {
            currentMappedConstBuffer = mapNextConstBuffer( commandBuffer );

            if( exceedsTexBuffer )
                mapNextTexBuffer( commandBuffer, minimumTexBufferSize * sizeof( float ) );
            else
                rebindTexBuffer( commandBuffer, true, minimumTexBufferSize * sizeof( float ) );

            currentMappedTexBuffer = mCurrentMappedTexBuffer;
        }

        //---------------------------------------------------------------------------
        //                          ---- VERTEX SHADER ----
        //---------------------------------------------------------------------------
        bool useIdentityProjection = queuedRenderable.renderable->getUseIdentityProjection();

        // uint materialIdx[]
        *currentMappedConstBuffer = datablock->getAssignedSlot();
        *reinterpret_cast<float * RESTRICT_ALIAS>( currentMappedConstBuffer + 1 ) =
            datablock->mShadowConstantBias * mConstantBiasScale;
        *( currentMappedConstBuffer + 2 ) = useIdentityProjection;
        currentMappedConstBuffer += 4;

        // mat4 worldViewProj
        Matrix4 tmp =
            mPreparedPass.viewProjMatrix[mUsingInstancedStereo ? 4u : useIdentityProjection] * worldMat;
#if !OGRE_DOUBLE_PRECISION
        memcpy( currentMappedTexBuffer, &tmp, sizeof( Matrix4 ) );
        currentMappedTexBuffer += 16;
#else
        for( int y = 0; y < 4; ++y )
        {
            for( int x = 0; x < 4; ++x )
            {
                *currentMappedTexBuffer++ = tmp[y][x];
            }
        }
#endif

        //---------------------------------------------------------------------------
        //                          ---- PIXEL SHADER ----
        //---------------------------------------------------------------------------

        if( !casterPass || datablock->getAlphaTest() != CMPF_ALWAYS_PASS ||
            datablock->getAlphaHashing() )
        {
            if( datablock->mTexturesDescSet != mLastDescTexture )
            {
                // Bind textures
                size_t texUnit = mTexUnitSlotStart;

                if( datablock->mTexturesDescSet )
                {
                    *commandBuffer->addCommand<CbTextures>() =
                        CbTextures( (uint16)texUnit, std::numeric_limits<uint16>::max(),
                                    datablock->mTexturesDescSet );

                    if( !mHasSeparateSamplers )
                    {
                        *commandBuffer->addCommand<CbSamplers>() =
                            CbSamplers( (uint16)texUnit, datablock->mSamplersDescSet );
                    }

                    texUnit += datablock->mTexturesDescSet->mTextures.size();
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
        mCurrentMappedTexBuffer = currentMappedTexBuffer;

        return uint32( ( ( mCurrentMappedConstBuffer - mStartMappedConstBuffer ) >> 2u ) - 1u );
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlit::destroyAllBuffers()
    {
        HlmsBufferManager::destroyAllBuffers();

        mCurrentPassBuffer = 0;

        {
            ConstBufferPackedVec::const_iterator itor = mPassBuffers.begin();
            ConstBufferPackedVec::const_iterator end = mPassBuffers.end();

            while( itor != end )
            {
                if( ( *itor )->getMappingState() != MS_UNMAPPED )
                    ( *itor )->unmap( UO_UNMAP_ALL );
                mVaoManager->destroyConstBuffer( *itor );
                ++itor;
            }

            mPassBuffers.clear();
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlit::frameEnded()
    {
        HlmsBufferManager::frameEnded();
        mCurrentPassBuffer = 0;
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlit::setShadowSettings( bool useExponentialShadowMaps )
    {
        mUsingExponentialShadowMaps = useExponentialShadowMaps;
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlit::setEsmK( uint16 K )
    {
        assert( K != 0 && "A value of K = 0 is invalid!" );
        mEsmK = K;
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlit::setDefaultGenerateMipmaps( bool bDefaultGenerateMips )
    {
        mDefaultGenerateMipmaps = bDefaultGenerateMips;
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlit::getDefaultPaths( String &outDataFolderPath, StringVector &outLibraryFoldersPaths )
    {
        // We need to know what RenderSystem is currently in use, as the
        // name of the compatible shading language is part of the path
        RenderSystem *renderSystem = Root::getSingleton().getRenderSystem();
        String shaderSyntax = "GLSL";
        if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
            shaderSyntax = "HLSL";
        else if( renderSystem->getName() == "Metal Rendering Subsystem" )
            shaderSyntax = "Metal";

        // Fill the library folder paths with the relevant folders
        outLibraryFoldersPaths.clear();
        outLibraryFoldersPaths.push_back( "Hlms/Common/" + shaderSyntax );
        outLibraryFoldersPaths.push_back( "Hlms/Common/Any" );
        outLibraryFoldersPaths.push_back( "Hlms/Unlit/Any" );

        // Fill the data folder path
        outDataFolderPath = "Hlms/Unlit/" + shaderSyntax;
    }
#if !OGRE_NO_JSON
    //-----------------------------------------------------------------------------------
    void HlmsUnlit::_loadJson( const rapidjson::Value &jsonValue, const HlmsJson::NamedBlocks &blocks,
                               HlmsDatablock *datablock, const String &resourceGroup,
                               HlmsJsonListener *listener,
                               const String &additionalTextureExtension ) const
    {
        HlmsJsonUnlit jsonUnlit( mHlmsManager, mRenderSystem->getTextureGpuManager() );
        jsonUnlit.loadMaterial( jsonValue, blocks, datablock, resourceGroup );
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlit::_saveJson( const HlmsDatablock *datablock, String &outString,
                               HlmsJsonListener *listener,
                               const String &additionalTextureExtension ) const
    {
        HlmsJsonUnlit jsonUnlit( mHlmsManager, mRenderSystem->getTextureGpuManager() );
        jsonUnlit.saveMaterial( datablock, outString );
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlit::_collectSamplerblocks( set<const HlmsSamplerblock *>::type &outSamplerblocks,
                                           const HlmsDatablock *datablock ) const
    {
        HlmsJsonUnlit::collectSamplerblocks( datablock, outSamplerblocks );
    }
#endif
    //-----------------------------------------------------------------------------------
    HlmsDatablock *HlmsUnlit::createDatablockImpl( IdString datablockName,
                                                   const HlmsMacroblock *macroblock,
                                                   const HlmsBlendblock *blendblock,
                                                   const HlmsParamVec &paramVec )
    {
        return OGRE_NEW HlmsUnlitDatablock( datablockName, this, macroblock, blendblock, paramVec );
    }
}  // namespace Ogre
