
#ifndef OGRE_HLMS_TEXTURE_BASE_CLASS
    #include "OgreHlmsTextureBaseClass.h"
    #include "OgreDescriptorSetTexture.h"
    #include "OgreDescriptorSetSampler.h"
    #include "OgreTextureGpu.h"
    #include "OgreHlms.h"
    #include "OgreHlmsManager.h"
    #include "OgreRenderSystem.h"
#endif

namespace Ogre
{
    OGRE_HLMS_TEXTURE_BASE_CLASS::OGRE_HLMS_TEXTURE_BASE_CLASS(
            IdString name, Hlms *creator, const HlmsMacroblock *macroblock,
            const HlmsBlendblock *blendblock, const HlmsParamVec &params ) :
        HlmsDatablock( name, creator, macroblock, blendblock, params ),
        mTexturesDescSet( 0 ),
        mSamplersDescSet( 0 )
    {
        memset( mTexIndices, 0, sizeof( mTexIndices ) );
        memset( mTextures, 0, sizeof( mTextures ) );
        memset( mSamplerblocks, 0, sizeof( mSamplerblocks ) );
    }
    //-----------------------------------------------------------------------------------
    OGRE_HLMS_TEXTURE_BASE_CLASS::~OGRE_HLMS_TEXTURE_BASE_CLASS()
    {
        for( size_t i=0; i<OGRE_HLMS_TEXTURE_BASE_MAX_TEX; ++i )
        {
            if( mTextures[i] )
                mTextures[i]->removeListener( this );
        }

        HlmsManager *hlmsManager = mCreator->getHlmsManager();
        if( hlmsManager )
        {
            if( mTexturesDescSet )
            {
                hlmsManager->destroyDescriptorSetTexture( mTexturesDescSet );
                mTexturesDescSet = 0;
            }
            if( mSamplersDescSet )
            {
                hlmsManager->destroyDescriptorSetSampler( mSamplersDescSet );
                mSamplersDescSet = 0;
            }

            for( size_t i=0; i<OGRE_HLMS_TEXTURE_BASE_MAX_TEX; ++i )
            {
                if( mSamplerblocks[i] )
                    hlmsManager->destroySamplerblock( mSamplerblocks[i] );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void OGRE_HLMS_TEXTURE_BASE_CLASS::scheduleConstBufferUpdate( bool updateTextures,
                                                                  bool updateSamplers )
    {
        uint8 flags = ConstBufferPool::DirtyConstBuffer;
        if( updateTextures )
            flags |= ConstBufferPool::DirtyTextures;
        if( updateSamplers )
            flags |= ConstBufferPool::DirtySamplers;

        static_cast<OGRE_HLMS_CREATOR_CLASS*>(mCreator)->scheduleForUpdate( this, flags );
    }
    //-----------------------------------------------------------------------------------
    void OGRE_HLMS_TEXTURE_BASE_CLASS::updateDescriptorSets( bool textureSetDirty, bool samplerSetDirty )
    {
        const RenderSystem *renderSystem = mCreator->getRenderSystem();
        const RenderSystemCapabilities *caps = renderSystem->getCapabilities();
        const bool hasSeparateSamplers = caps->hasCapability( RSC_SEPARATE_SAMPLERS_FROM_TEXTURES );

        bool needsRecalculateHash = false;

        if( textureSetDirty || (samplerSetDirty && !hasSeparateSamplers) )
            needsRecalculateHash |= bakeTextures( hasSeparateSamplers );

        if( samplerSetDirty && hasSeparateSamplers )
            needsRecalculateHash |= bakeSamplers();

        if( needsRecalculateHash )
        {
            calculateHash();
            flushRenderables();
        }
    }
    //-----------------------------------------------------------------------------------
    static bool OrderTextureByPoolThenName( const TextureGpu *_a, const TextureGpu *_b )
    {
        const TexturePool *poolA = _a->getTexturePool();
        const TexturePool *poolB = _b->getTexturePool();

        IdString poolNameA = poolA ? poolA->masterTexture->getName() : IdString();
        IdString poolNameB = poolB ? poolB->masterTexture->getName() : IdString();

        if( poolA != poolB )
            return poolNameA < poolNameB;

        return _a->getName() < _b->getName();
    }
    bool OGRE_HLMS_TEXTURE_BASE_CLASS::bakeTextures( bool hasSeparateSamplers )
    {
        DescriptorSetTexture baseSet;
        DescriptorSetSampler baseSampler;

        for( size_t i=0; i<OGRE_HLMS_TEXTURE_BASE_MAX_TEX; ++i )
        {
            if( mTextures[i] )
            {
                //Keep it sorted to maximize sharing of descriptor sets.
                FastArray<const TextureGpu*>::iterator itor =
                        std::lower_bound( baseSet.mTextures.begin(),
                                          baseSet.mTextures.end(),
                                          mTextures[i], OrderTextureByPoolThenName );

                const size_t idx = itor - baseSet.mTextures.begin();

                //May have changed if the TextureGpuManager updated the Texture.
                mTexIndices[i] = mTextures[i]->getInternalSliceStart();

                if( itor == baseSet.mTextures.end() || (*itor) != mTextures[i] )
                {
                    baseSet.mTextures.insert( itor, mTextures[i] );
                    if( !hasSeparateSamplers )
                    {
                        baseSampler.mSamplers.insert( baseSampler.mSamplers.begin() + idx,
                                                      mSamplerblocks[i] );
                    }
                }
                else if( !hasSeparateSamplers && baseSampler.mSamplers[idx] != mSamplerblocks[i] )
                {
                    assert( idx < baseSampler.mSamplers.size() );

                    //Texture is already there, but we have to duplicate
                    //it because it uses a different samplerblock
                    baseSet.mTextures.insert( itor, mTextures[i] );
                    baseSampler.mSamplers.insert( baseSampler.mSamplers.begin() + idx + 1u,
                                                  mSamplerblocks[i] );
                }
            }
        }

        baseSet.mShaderTypeTexCount[PixelShader] = static_cast<uint16>( baseSet.mTextures.size() );
        baseSampler.mShaderTypeSamplerCount[PixelShader] =
                static_cast<uint16>( baseSampler.mSamplers.size() );

        bool needsRecalculateHash = false;

        HlmsManager *hlmsManager = mCreator->getHlmsManager();
        if( mTexturesDescSet && baseSet.mTextures.empty() )
        {
            hlmsManager->destroyDescriptorSetTexture( mTexturesDescSet );
            mTexturesDescSet = 0;
            needsRecalculateHash = true;
            if( !hasSeparateSamplers )
            {
                hlmsManager->destroyDescriptorSetSampler( mSamplersDescSet );
                mSamplersDescSet = 0;
            }
        }
        else if( !mTexturesDescSet || *mTexturesDescSet != baseSet )
        {
            if( mTexturesDescSet )
            {
                hlmsManager->destroyDescriptorSetTexture( mTexturesDescSet );
                mTexturesDescSet = 0;
            }
            if( !baseSet.mTextures.empty() )
            {
                mTexturesDescSet = hlmsManager->getDescriptorSetTexture( baseSet );
                needsRecalculateHash = true;
            }
            if( !hasSeparateSamplers )
            {
                if( mSamplersDescSet )
                    hlmsManager->destroyDescriptorSetSampler( mSamplersDescSet );
                if( !baseSet.mTextures.empty() )
                    mSamplersDescSet = hlmsManager->getDescriptorSetSampler( baseSampler );
            }
        }

        return needsRecalculateHash;
    }
    //-----------------------------------------------------------------------------------
    static bool OrderBlockById( const BasicBlock *_a, const BasicBlock *_b )
    {
        return _a->mId < _b->mId;
    }
    bool OGRE_HLMS_TEXTURE_BASE_CLASS::bakeSamplers(void)
    {
        assert( mCreator->getRenderSystem()->getCapabilities()->
                hasCapability( RSC_SEPARATE_SAMPLERS_FROM_TEXTURES ) );

        DescriptorSetSampler baseSampler;
        for( size_t i=0; i<OGRE_HLMS_TEXTURE_BASE_MAX_TEX; ++i )
        {
            if( mSamplerblocks[i] )
            {
                //Keep it sorted to maximize sharing of descriptor sets.
                FastArray<const HlmsSamplerblock*>::iterator itor =
                        std::lower_bound( baseSampler.mSamplers.begin(),
                                          baseSampler.mSamplers.end(),
                                          mSamplerblocks[i], OrderBlockById );
                if( itor == baseSampler.mSamplers.end() || (*itor) != mSamplerblocks[i] )
                    itor = baseSampler.mSamplers.insert( itor, mSamplerblocks[i] );
            }
        }

        baseSampler.mShaderTypeSamplerCount[PixelShader] =
                static_cast<uint16>( baseSampler.mSamplers.size() );

        bool needsRecalculateHash = false;

        HlmsManager *hlmsManager = mCreator->getHlmsManager();
        if( mSamplersDescSet && baseSampler.mSamplers.empty() )
        {
            hlmsManager->destroyDescriptorSetSampler( mSamplersDescSet );
            mSamplersDescSet = 0;
            needsRecalculateHash = true;
        }
        else if( !mSamplersDescSet || *mSamplersDescSet != baseSampler )
        {
            if( mSamplersDescSet )
                hlmsManager->destroyDescriptorSetSampler( mSamplersDescSet );
            mSamplersDescSet = hlmsManager->getDescriptorSetSampler( baseSampler );
            needsRecalculateHash = true;
        }

        return needsRecalculateHash;
    }
    //-----------------------------------------------------------------------------------
    void OGRE_HLMS_TEXTURE_BASE_CLASS::setTexture( uint8 texType, TextureGpu *texture,
                                                   const HlmsSamplerblock *refParams )
    {
        assert( texType < OGRE_HLMS_TEXTURE_BASE_MAX_TEX );

        HlmsManager *hlmsManager = mCreator->getHlmsManager();

        //Set the new samplerblock
        HlmsSamplerblock const *samplerblockPtr = 0;
        if( refParams )
        {
            samplerblockPtr = hlmsManager->getSamplerblock( *refParams );
        }
        else if( texture )
        {
            if( !mSamplerblocks[texType] )
            {
                //Adding a texture, but the samplerblock doesn't exist. Create a default one.
                HlmsSamplerblock defaultSamplerblockRef;
                samplerblockPtr = hlmsManager->getSamplerblock( defaultSamplerblockRef );
            }
            else
            {
                //Keeping the current samplerblock. Increase its
                //ref. count because _setTexture will decrease it.
                samplerblockPtr = mSamplerblocks[texType];
                HlmsManager *hlmsManager = mCreator->getHlmsManager();
                hlmsManager->addReference( samplerblockPtr );
            }
        }

        _setTexture( texType, texture, samplerblockPtr );
    }
    //-----------------------------------------------------------------------------------
    void OGRE_HLMS_TEXTURE_BASE_CLASS::_setTexture( uint8 texType, TextureGpu *texture,
                                                    const HlmsSamplerblock *samplerblockPtr )
    {
        assert( texType < OGRE_HLMS_TEXTURE_BASE_MAX_TEX );

        bool textureSetDirty = false;
        bool samplerSetDirty = false;

        if( mTextures[texType] != texture )
        {
            if( (!mTextures[texType] && texture) ||
                (mTextures[texType] && !texture) )
            {
                textureSetDirty = true;
            }

            TexturePool const *prevPool = 0;
            if( mTextures[texType] )
            {
                prevPool = mTextures[texType]->getTexturePool();
                mTextures[texType]->removeListener( this );
            }

            mTextures[texType] = texture;
            mTexIndices[texType] = texture->getInternalSliceStart();

            if( texture )
            {
                if( prevPool != texture->getTexturePool() )
                    textureSetDirty = true;

                texture->addListener( this );
            }

            scheduleConstBufferUpdate( textureSetDirty, samplerSetDirty );
        }

        //Set the new samplerblock
        if( mSamplerblocks[texType] )
        {
            //Decrease ref count of our old block. The new one already has its ref count increased.
            HlmsManager *hlmsManager = mCreator->getHlmsManager();
            hlmsManager->destroySamplerblock( mSamplerblocks[texType] );
        }

        if( mSamplerblocks[texType] != samplerblockPtr )
        {
            mSamplerblocks[texType] = samplerblockPtr;
            samplerSetDirty = true;
        }

        if( textureSetDirty || samplerSetDirty )
            scheduleConstBufferUpdate( textureSetDirty, samplerSetDirty );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* OGRE_HLMS_TEXTURE_BASE_CLASS::getTexture( uint8 texType ) const
    {
        assert( texType < OGRE_HLMS_TEXTURE_BASE_MAX_TEX );
        return mTextures[texType];
    }
    //-----------------------------------------------------------------------------------
    void OGRE_HLMS_TEXTURE_BASE_CLASS::setSamplerblock( uint8 texType, const HlmsSamplerblock &params )
    {
        HlmsManager *hlmsManager = mCreator->getHlmsManager();
        const HlmsSamplerblock *samplerblockPtr = hlmsManager->getSamplerblock( params );
        _setSamplerblock( texType, samplerblockPtr );
    }
    //-----------------------------------------------------------------------------------
    void OGRE_HLMS_TEXTURE_BASE_CLASS::_setSamplerblock( uint8 texType,
                                                         const HlmsSamplerblock *samplerblockPtr )
    {
        if( mSamplerblocks[texType] )
        {
            //Decrease ref count of our old block. The new one already has its ref count increased.
            HlmsManager *hlmsManager = mCreator->getHlmsManager();
            hlmsManager->destroySamplerblock( mSamplerblocks[texType] );
        }

        if( mSamplerblocks[texType] != samplerblockPtr )
        {
            mSamplerblocks[texType] = samplerblockPtr;
            scheduleConstBufferUpdate( false, true );
        }
    }
    //-----------------------------------------------------------------------------------
    const HlmsSamplerblock* OGRE_HLMS_TEXTURE_BASE_CLASS::getSamplerblock( uint8 texType ) const
    {
        assert( texType < OGRE_HLMS_TEXTURE_BASE_MAX_TEX );
        return mSamplerblocks[texType];
    }
    //-----------------------------------------------------------------------------------
    uint8 OGRE_HLMS_TEXTURE_BASE_CLASS::getIndexToDescriptorTexture( uint8 texType )
    {
        assert( texType < OGRE_HLMS_TEXTURE_BASE_MAX_TEX );
        uint8 retVal = OGRE_HLMS_TEXTURE_BASE_MAX_TEX;

        TextureGpu *texture = mTextures[texType];
        if( texture )
        {
            FastArray<const TextureGpu*>::const_iterator itor =
                    std::lower_bound( mTexturesDescSet->mTextures.begin(),
                                      mTexturesDescSet->mTextures.end(),
                                      texture, OrderTextureByPoolThenName );
            if( itor != mTexturesDescSet->mTextures.end() && *itor == texture )
            {
                size_t idx = itor - mTexturesDescSet->mTextures.begin();

                const RenderSystem *renderSystem = mCreator->getRenderSystem();
                const RenderSystemCapabilities *caps = renderSystem->getCapabilities();
                const bool hasSeparateSamplers =
                        caps->hasCapability( RSC_SEPARATE_SAMPLERS_FROM_TEXTURES );

                if( !hasSeparateSamplers )
                {
                    //We may have more than one entry for the same texture
                    //(since they have different samplerblocks).
                    while( mSamplerblocks[texType] != mSamplersDescSet->mSamplers[idx] )
                        ++idx;
                    assert( texture == mTexturesDescSet->mTextures[idx] );
                }

                retVal = static_cast<uint8>( idx );
            }
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    uint8 OGRE_HLMS_TEXTURE_BASE_CLASS::getIndexToDescriptorSampler( uint8 texType )
    {
        assert( texType < OGRE_HLMS_TEXTURE_BASE_MAX_TEX );
        uint8 retVal = OGRE_HLMS_TEXTURE_BASE_MAX_TEX;

        const HlmsSamplerblock *sampler = mSamplerblocks[texType];
        if( sampler )
        {
            assert( mCreator->getRenderSystem()->
                    getCapabilities()->hasCapability( RSC_SEPARATE_SAMPLERS_FROM_TEXTURES ) );

            FastArray<const HlmsSamplerblock*>::const_iterator itor =
                    std::lower_bound( mSamplersDescSet->mSamplers.begin(),
                                      mSamplersDescSet->mSamplers.end(),
                                      sampler, OrderBlockById );
            if( itor != mSamplersDescSet->mSamplers.end() && *itor == sampler )
            {
                const size_t idx = itor - mSamplersDescSet->mSamplers.begin();
                retVal = static_cast<uint8>( idx );
            }
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void OGRE_HLMS_TEXTURE_BASE_CLASS::notifyTextureChanged( TextureGpu *texture,
                                                             TextureGpuListener::Reason reason )
    {
        if( reason == TextureGpuListener::FromStorageToSysRam )
            return; //Does not affect us at all.

        if( mTexturesDescSet )
        {
            //The texture's baked SRV has changed. We always need a new descriptor,
            //and DescriptorSetTexture::!= operator won't see this.
            HlmsManager *hlmsManager = mCreator->getHlmsManager();
            hlmsManager->destroyDescriptorSetTexture( mTexturesDescSet );
            mTexturesDescSet = 0;
        }

        scheduleConstBufferUpdate( true, false );
    }
    //-----------------------------------------------------------------------------------
    void OGRE_HLMS_TEXTURE_BASE_CLASS::loadAllTextures(void)
    {
        for( int i=0; i<OGRE_HLMS_TEXTURE_BASE_MAX_TEX; ++i )
        {
            if( mTextures[i] && mTextures[i]->getNextResidencyStatus() != GpuResidency::Resident )
                mTextures[i]->scheduleTransitionTo( GpuResidency::Resident );
        }
    }
}
