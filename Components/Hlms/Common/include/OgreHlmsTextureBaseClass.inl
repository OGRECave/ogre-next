
#ifndef OGRE_HLMS_TEXTURE_BASE_CLASS
#    include "OgreDescriptorSetSampler.h"
#    include "OgreDescriptorSetTexture.h"
#    include "OgreHlms.h"
#    include "OgreHlmsManager.h"
#    include "OgreHlmsTextureBaseClass.h"
#    include "OgreRenderSystem.h"
#    include "OgreTextureGpu.h"
#    include "OgreTextureGpuManager.h"
#endif

#ifndef OGRE_NumTexIndices
#    define OGRE_NumTexIndices sizeof( mTexIndices ) / sizeof( mTexIndices[0] )
#endif

namespace Ogre
{
    static const uint16 ManualTexIndexBit = 0x8000u;

    OGRE_HLMS_TEXTURE_BASE_CLASS::OGRE_HLMS_TEXTURE_BASE_CLASS( IdString name, Hlms *creator,
                                                                const HlmsMacroblock *macroblock,
                                                                const HlmsBlendblock *blendblock,
                                                                const HlmsParamVec &params ) :
        HlmsDatablock( name, creator, macroblock, blendblock, params ),
        mTexturesDescSet( 0 ),
        mSamplersDescSet( 0 )
    {
        memset( mTexIndices, 0, sizeof( mTexIndices ) );
        memset( mTextures, 0, sizeof( mTextures ) );
        memset( mSamplerblocks, 0, sizeof( mSamplerblocks ) );

        for( size_t i = 0; i < OGRE_HLMS_TEXTURE_BASE_MAX_TEX; ++i )
            mTexLocationInDescSet[i] = OGRE_HLMS_TEXTURE_BASE_MAX_TEX;
    }
    //-----------------------------------------------------------------------------------
    OGRE_HLMS_TEXTURE_BASE_CLASS::~OGRE_HLMS_TEXTURE_BASE_CLASS()
    {
        for( size_t i = 0; i < OGRE_HLMS_TEXTURE_BASE_MAX_TEX; ++i )
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

            for( size_t i = 0; i < OGRE_HLMS_TEXTURE_BASE_MAX_TEX; ++i )
            {
                if( mSamplerblocks[i] )
                    hlmsManager->destroySamplerblock( mSamplerblocks[i] );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void OGRE_HLMS_TEXTURE_BASE_CLASS::cloneImpl( HlmsDatablock *datablock ) const
    {
        OGRE_HLMS_TEXTURE_BASE_CLASS *datablockImpl =
            static_cast<OGRE_HLMS_TEXTURE_BASE_CLASS *>( datablock );

        HlmsManager *hlmsManager = mCreator->getHlmsManager();

        bool hasDirtyTextures = false;
        bool hasDirtySamplers = false;

        for( size_t i = 0; i < OGRE_HLMS_TEXTURE_BASE_MAX_TEX; ++i )
        {
            datablockImpl->mTexIndices[i] = mTexIndices[i];
            datablockImpl->mTextures[i] = mTextures[i];
            if( datablockImpl->mTextures[i] )
            {
                datablockImpl->mTextures[i]->addListener( datablockImpl );
                hasDirtyTextures = true;
            }
            datablockImpl->mSamplerblocks[i] = mSamplerblocks[i];
            if( datablockImpl->mSamplerblocks[i] )
            {
                hlmsManager->addReference( datablockImpl->mSamplerblocks[i] );
                hasDirtySamplers = true;
            }
        }

        if( mTexturesDescSet )
            datablockImpl->mTexturesDescSet = hlmsManager->getDescriptorSetTexture( *mTexturesDescSet );
        if( mSamplersDescSet )
            datablockImpl->mSamplersDescSet = hlmsManager->getDescriptorSetSampler( *mSamplersDescSet );

        datablockImpl->scheduleConstBufferUpdate( hasDirtyTextures, hasDirtySamplers );
    }
    //-----------------------------------------------------------------------------------
    void OGRE_HLMS_TEXTURE_BASE_CLASS::preload() { loadAllTextures(); }
    //-----------------------------------------------------------------------------------
    void OGRE_HLMS_TEXTURE_BASE_CLASS::saveTextures( const String &folderPath,
                                                     set<String>::type &savedTextures, bool saveOitd,
                                                     bool saveOriginal,
                                                     HlmsTextureExportListener *listener )
    {
        for( size_t i = 0; i < OGRE_HLMS_TEXTURE_BASE_MAX_TEX; ++i )
        {
            TextureGpu *texture = mTextures[i];

            if( texture )
            {
                TextureGpuManager *textureManager = texture->getTextureManager();
                textureManager->saveTexture( texture, folderPath, savedTextures, saveOitd, saveOriginal,
                                             listener );
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

        static_cast<OGRE_HLMS_CREATOR_CLASS *>( mCreator )->scheduleForUpdate( this, flags );
    }
    //-----------------------------------------------------------------------------------
    void OGRE_HLMS_TEXTURE_BASE_CLASS::updateDescriptorSets( bool textureSetDirty, bool samplerSetDirty )
    {
        const RenderSystem *renderSystem = mCreator->getRenderSystem();
        const RenderSystemCapabilities *caps = renderSystem->getCapabilities();
        const bool hasSeparateSamplers = caps->hasCapability( RSC_SEPARATE_SAMPLERS_FROM_TEXTURES );

        bool needsRecalculateHash = false;

        if( textureSetDirty || ( samplerSetDirty && !hasSeparateSamplers ) )
            needsRecalculateHash |= bakeTextures( hasSeparateSamplers );

        if( samplerSetDirty && hasSeparateSamplers )
            needsRecalculateHash |= bakeSamplers();

        if( needsRecalculateHash )
            calculateHash();

        // When needsRecalculateHash = false, nothing significant has changed. However
        // this datablock may have been attached to new Items that skipped calculation
        // due to the datablock being dirty, so we have to flush those renderables
        //(the ones that have a null Hlms hash)
        const bool onlyNullHashes = needsRecalculateHash == false;
        flushRenderables( onlyNullHashes );
    }
    //-----------------------------------------------------------------------------------
    bool OGRE_HLMS_TEXTURE_BASE_CLASS::bakeTextures( bool hasSeparateSamplers )
    {
        DescriptorSetTexture baseSet;
        DescriptorSetSampler baseSampler;

        for( size_t i = 0; i < OGRE_HLMS_TEXTURE_BASE_MAX_TEX; ++i )
            mTexLocationInDescSet[i] = OGRE_HLMS_TEXTURE_BASE_MAX_TEX;

        for( size_t i = 0; i < OGRE_HLMS_TEXTURE_BASE_MAX_TEX; ++i )
        {
            if( mTextures[i] )
            {
                // May have changed if the TextureGpuManager updated the Texture.
                if( !( mTexIndices[i] & ManualTexIndexBit ) )
                    mTexIndices[i] = mTextures[i]->getInternalSliceStart();

                // Look if the texture pool has already been added to the desc set so
                // we can share the same spot. In OpenGL, we cannot share it if the
                // samplerblocks are different.
                for( size_t j = 0; j < i; ++j )
                {
                    if( ( mTextures[j] == mTextures[i] ||
                          ( mTextures[j] &&
                            mTextures[j]->getInternalTextureType() ==
                                mTextures[i]->getInternalTextureType() &&
                            mTextures[j]->getTexturePool() &&
                            mTextures[j]->getTexturePool() == mTextures[i]->getTexturePool() ) ) &&
                        ( mSamplerblocks[i] == mSamplerblocks[j] || hasSeparateSamplers ) )
                    {
                        mTexLocationInDescSet[i] = mTexLocationInDescSet[j];
                        break;
                    }
                }

                // It was not. Add it ourself, trying to maintaining
                // the order of OGRE_HLMS_TEXTURE_BASE_MAX_TEX
                if( mTexLocationInDescSet[i] == OGRE_HLMS_TEXTURE_BASE_MAX_TEX )
                {
                    mTexLocationInDescSet[i] = (uint8)baseSet.mTextures.size();
                    baseSet.mTextures.push_back( mTextures[i] );
                    if( !hasSeparateSamplers )
                        baseSampler.mSamplers.push_back( mSamplerblocks[i] );
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
                {
                    hlmsManager->destroyDescriptorSetSampler( mSamplersDescSet );
                    mSamplersDescSet = 0;
                }
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
    bool OGRE_HLMS_TEXTURE_BASE_CLASS::bakeSamplers()
    {
        assert( mCreator->getRenderSystem()->getCapabilities()->hasCapability(
            RSC_SEPARATE_SAMPLERS_FROM_TEXTURES ) );

        DescriptorSetSampler baseSampler;
        for( size_t i = 0; i < OGRE_HLMS_TEXTURE_BASE_MAX_TEX; ++i )
        {
            if( mSamplerblocks[i] )
            {
                // Keep it sorted to maximize sharing of descriptor sets.
                FastArray<const HlmsSamplerblock *>::iterator itor =
                    std::lower_bound( baseSampler.mSamplers.begin(), baseSampler.mSamplers.end(),
                                      mSamplerblocks[i], OrderBlockById );
                if( itor == baseSampler.mSamplers.end() || ( *itor ) != mSamplerblocks[i] )
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
            {
                hlmsManager->destroyDescriptorSetSampler( mSamplersDescSet );
                mSamplersDescSet = 0;
            }
            if( !baseSampler.mSamplers.empty() )
            {
                mSamplersDescSet = hlmsManager->getDescriptorSetSampler( baseSampler );
                needsRecalculateHash = true;
            }
        }

        return needsRecalculateHash;
    }
    //-----------------------------------------------------------------------------------
    void OGRE_HLMS_TEXTURE_BASE_CLASS::setTexture( uint8 texType, TextureGpu *texture,
                                                   const HlmsSamplerblock *refParams, uint16 sliceIdx )
    {
        assert( texType < OGRE_HLMS_TEXTURE_BASE_MAX_TEX );

        HlmsManager *hlmsManager = mCreator->getHlmsManager();

        // Set the new samplerblock
        HlmsSamplerblock const *samplerblockPtr = 0;
        if( refParams )
        {
            samplerblockPtr = hlmsManager->getSamplerblock( *refParams );
        }
        else if( texture )
        {
            if( !mSamplerblocks[texType] )
            {
                // Adding a texture, but the samplerblock doesn't exist. Create a default one.
                HlmsSamplerblock defaultSamplerblockRef;
                samplerblockPtr = hlmsManager->getSamplerblock( defaultSamplerblockRef );
            }
            else
            {
                // Keeping the current samplerblock. Increase its
                // ref. count because _setTexture will decrease it.
                samplerblockPtr = mSamplerblocks[texType];
                hlmsManager->addReference( samplerblockPtr );
            }
        }

        _setTexture( texType, texture, samplerblockPtr, sliceIdx );
    }
    //-----------------------------------------------------------------------------------
    void OGRE_HLMS_TEXTURE_BASE_CLASS::_setTexture( uint8 texType, TextureGpu *texture,
                                                    const HlmsSamplerblock *samplerblockPtr,
                                                    uint16 sliceIdx )
    {
        assert( texType < OGRE_HLMS_TEXTURE_BASE_MAX_TEX );

        bool textureSetDirty = false;
        bool samplerSetDirty = false;

        if( mTextures[texType] != texture )
        {
            if( ( !mTextures[texType] && texture ) || ( mTextures[texType] && !texture ) )
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
            mTexIndices[texType] = 0;

            if( texture )
            {
                if( sliceIdx == std::numeric_limits<uint16>::max() )
                    mTexIndices[texType] = texture->getInternalSliceStart();
                else
                    mTexIndices[texType] = ManualTexIndexBit | sliceIdx;

                if( prevPool != texture->getTexturePool() || !prevPool )
                    textureSetDirty = true;

                texture->addListener( this );
            }

            scheduleConstBufferUpdate( textureSetDirty, samplerSetDirty );
        }

        // Set the new samplerblock
        if( mSamplerblocks[texType] )
        {
            // Decrease ref count of our old block. The new one already has its ref count increased.
            HlmsManager *hlmsManager = mCreator->getHlmsManager();
            hlmsManager->destroySamplerblock( mSamplerblocks[texType] );
        }

        if( mSamplerblocks[texType] != samplerblockPtr )
        {
            mSamplerblocks[texType] = samplerblockPtr;
            samplerSetDirty = true;
        }

        if( textureSetDirty || samplerSetDirty )
        {
            mTexLocationInDescSet[texType] = OGRE_HLMS_TEXTURE_BASE_MAX_TEX;
            scheduleConstBufferUpdate( textureSetDirty, samplerSetDirty );
        }
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *OGRE_HLMS_TEXTURE_BASE_CLASS::getTexture( uint8 texType ) const
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
            // Decrease ref count of our old block. The new one already has its ref count increased.
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
    const HlmsSamplerblock *OGRE_HLMS_TEXTURE_BASE_CLASS::getSamplerblock( uint8 texType ) const
    {
        assert( texType < OGRE_HLMS_TEXTURE_BASE_MAX_TEX );
        return mSamplerblocks[texType];
    }
    //-----------------------------------------------------------------------------------
    uint8 OGRE_HLMS_TEXTURE_BASE_CLASS::getIndexToDescriptorTexture( uint8 texType )
    {
        assert( texType < OGRE_HLMS_TEXTURE_BASE_MAX_TEX );
        assert( ( mTexturesDescSet || !mTextures[texType] ) && "bakeTextures not yet called!" );
        return mTexLocationInDescSet[texType];
    }
    //-----------------------------------------------------------------------------------
    uint8 OGRE_HLMS_TEXTURE_BASE_CLASS::getIndexToDescriptorSampler( uint8 texType )
    {
        assert( texType < OGRE_HLMS_TEXTURE_BASE_MAX_TEX );
        uint8 retVal = OGRE_HLMS_TEXTURE_BASE_MAX_TEX;

        const HlmsSamplerblock *sampler = mSamplerblocks[texType];
        if( sampler )
        {
            assert( mCreator->getRenderSystem()->getCapabilities()->hasCapability(
                RSC_SEPARATE_SAMPLERS_FROM_TEXTURES ) );

            FastArray<const HlmsSamplerblock *>::const_iterator itor =
                std::lower_bound( mSamplersDescSet->mSamplers.begin(), mSamplersDescSet->mSamplers.end(),
                                  sampler, OrderBlockById );
            if( itor != mSamplersDescSet->mSamplers.end() && *itor == sampler )
            {
                const size_t idx = static_cast<size_t>( itor - mSamplersDescSet->mSamplers.begin() );
                retVal = static_cast<uint8>( idx );
            }
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void OGRE_HLMS_TEXTURE_BASE_CLASS::notifyTextureChanged( TextureGpu *texture,
                                                             TextureGpuListener::Reason reason,
                                                             void *extraData )
    {
        if( reason == TextureGpuListener::FromStorageToSysRam )
            return;  // Does not affect us at all.

        if( reason == TextureGpuListener::Deleted )
        {
            for( int i = 0; i < OGRE_HLMS_TEXTURE_BASE_MAX_TEX; ++i )
            {
                if( mTextures[i] == texture )
                    setTexture( (uint8)i, 0, mSamplerblocks[i] );
            }
        }

        if( mTexturesDescSet )
        {
            // The texture's baked SRV has changed. We always need a new descriptor,
            // and DescriptorSetTexture::!= operator won't see this.
            HlmsManager *hlmsManager = mCreator->getHlmsManager();
            hlmsManager->destroyDescriptorSetTexture( mTexturesDescSet );
            mTexturesDescSet = 0;
        }

        scheduleConstBufferUpdate( true, false );
    }
    //-----------------------------------------------------------------------------------
    bool OGRE_HLMS_TEXTURE_BASE_CLASS::shouldStayLoaded( TextureGpu *texture )
    {
        return !mLinkedRenderables.empty();
    }
    //-----------------------------------------------------------------------------------
    void OGRE_HLMS_TEXTURE_BASE_CLASS::loadAllTextures()
    {
        if( !mAllowTextureResidencyChange )
            return;

        for( int i = 0; i < OGRE_HLMS_TEXTURE_BASE_MAX_TEX; ++i )
        {
            if( mTextures[i] )
                mTextures[i]->scheduleTransitionTo( GpuResidency::Resident );
        }
    }
}  // namespace Ogre
