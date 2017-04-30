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

#include "OgreStableHeaders.h"

#include "OgreHlmsUnlitDatablock.h"
#include "OgreHlmsUnlit.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsTextureManager.h"
#include "OgreTexture.h"
#include "OgreTextureGpu.h"
#include "OgreTextureGpuManager.h"
#include "OgreRenderSystem.h"
#include "OgreLogManager.h"

namespace Ogre
{
    extern const String c_unlitBlendModes[];
    const String c_unlitBlendModes[] =
    {
        "NormalNonPremul", "NormalPremul", "Add", "Subtract", "Multiply",
        "Multiply2x", "Screen", "Overlay", "Lighten", "Darken", "GrainExtract",
        "GrainMerge", "Difference"
    };

    const String c_diffuseMap[NUM_UNLIT_TEXTURE_TYPES] =
    {
        "diffuse_map",
        "diffuse_map1",
        "diffuse_map2",
        "diffuse_map3",
        "diffuse_map4",
        "diffuse_map5",
        "diffuse_map6",
        "diffuse_map7",
        "diffuse_map8",
        "diffuse_map9",
        "diffuse_map10",
        "diffuse_map11",
        "diffuse_map12",
        "diffuse_map13",
        "diffuse_map14",
        "diffuse_map15"
    };

    const size_t HlmsUnlitDatablock::MaterialSizeInGpu          = 8 * 4 + NUM_UNLIT_TEXTURE_TYPES * 2;
    const size_t HlmsUnlitDatablock::MaterialSizeInGpuAligned   = alignToNextMultiple(
                                                                   HlmsUnlitDatablock::MaterialSizeInGpu,
                                                                   4 * 4 );
    const uint8 HlmsUnlitDatablock::R_MASK  = 0;
    const uint8 HlmsUnlitDatablock::G_MASK  = 1;
    const uint8 HlmsUnlitDatablock::B_MASK  = 2;
    const uint8 HlmsUnlitDatablock::A_MASK  = 3;
    //-----------------------------------------------------------------------------------
    HlmsUnlitDatablock::HlmsUnlitDatablock( IdString name, HlmsUnlit *creator,
                                            const HlmsMacroblock *macroblock,
                                            const HlmsBlendblock *blendblock,
                                            const HlmsParamVec &params ) :
        HlmsDatablock( name, creator, macroblock, blendblock, params ),
        mNumEnabledAnimationMatrices( 0 ),
        mHasColour( false ),
        mR( 1.0f ), mG( 1.0f ), mB( 1.0f ), mA( 1.0f ),
        mTexturesDescSet( 0 ),
        mSamplersDescSet( 0 )
    {
        for( size_t i=0; i<NUM_UNLIT_TEXTURE_TYPES; ++i )
        {
            mTextureMatrices[i] = Matrix4::IDENTITY;
            setTextureSwizzle( i, R_MASK, G_MASK, B_MASK, A_MASK );
        }

        memset( mUvSource, 0, sizeof( mUvSource ) );
        memset( mBlendModes, 0, sizeof( mBlendModes ) );

        memset( mTexIndices, 0, sizeof( mTexIndices ) );
        memset( mTextures, 0, sizeof( mTextures ) );
        memset( mSamplerblocks, 0, sizeof( mSamplerblocks ) );

        memset( mEnabledAnimationMatrices, 0, sizeof( mEnabledAnimationMatrices ) );

        for( size_t i=0; i<NUM_UNLIT_TEXTURE_TYPES; ++i )
            mTexToBakedTextureIdx[i] = NUM_UNLIT_TEXTURE_TYPES;

        String paramVal;

        if( Hlms::findParamInVec( params, "diffuse", paramVal ) )
        {
            mHasColour = true;

            if( !paramVal.empty() )
            {
                ColourValue val = StringConverter::parseColourValue( paramVal );
                mR = val.r;
                mG = val.g;
                mB = val.b;
                mA = val.a;
            }
        }

        HlmsManager *hlmsManager = mCreator->getHlmsManager();
        HlmsTextureManager *hlmsTextureManager = hlmsManager->getTextureManager();

        UnlitBakedTexture textures[NUM_UNLIT_TEXTURE_TYPES];

        const HlmsSamplerblock *defaultSamplerblock = hlmsManager->getSamplerblock( HlmsSamplerblock() );

        for( size_t i=0; i<sizeof( c_diffuseMap ) / sizeof( String ); ++i )
        {
            if( Hlms::findParamInVec( params, c_diffuseMap[i], paramVal ) )
            {
                mTexIndices[i] = 0;
                textures[i].texture = hlmsTextureManager->getBlankTexture().texture;
                mSamplerblocks[i] = defaultSamplerblock;
                hlmsManager->addReference( mSamplerblocks[i] );

                StringVector vec = StringUtil::split( paramVal );

                StringVector::const_iterator itor = vec.begin();
                StringVector::const_iterator end  = vec.end();

                while( itor != end )
                {
                    uint val = StringConverter::parseUnsignedInt( *itor, ~0 );

                    if( val != (uint)(~0) )
                    {
                        //It's a number, must be an UV Set
                        setTextureUvSource( i, val );
                    }
                    else if( !itor->empty() )
                    {
                        //Is it a blend mode?
                        const String *it = std::find( c_unlitBlendModes, c_unlitBlendModes +
                                                      sizeof(c_unlitBlendModes) / sizeof( String ),
                                                      *itor );

                        if( it == c_unlitBlendModes + sizeof(c_unlitBlendModes) / sizeof( String ) )
                        {
                            //Not blend mode, try loading a texture
                            textures[i].texture = setTexture( *itor, i );
                        }
                        else
                        {
                            //It's a blend mode
                            mBlendModes[i] = (it - c_unlitBlendModes);
                        }
                    }

                    ++itor;
                }
            }
        }

        //Remove the reference
        hlmsManager->destroySamplerblock( defaultSamplerblock );

        for( size_t i=0; i<NUM_UNLIT_TEXTURE_TYPES; ++i )
            textures[i].samplerBlock = mSamplerblocks[i];

        if( Hlms::findParamInVec( params, "animate", paramVal ) )
        {
            size_t pos = paramVal.find_first_of( ' ' );
            while( pos != String::npos )
            {
                uint val = StringConverter::parseUnsignedInt( paramVal.substr( pos, 1 ), ~0 );

                if( val >= NUM_UNLIT_TEXTURE_TYPES )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 mName.getFriendlyText() +
                                 ": animate parameters must be in range [0; " +
                                 StringConverter::toString( NUM_UNLIT_TEXTURE_TYPES ) + ")",
                                 "HlmsUnlitDatablock::HlmsUnlitDatablock" );
                }

                if( mEnabledAnimationMatrices[val] )
                {
                    LogManager::getSingleton().logMessage( "WARNING: specified same texture unit twice "
                            "in material '" + mName.getFriendlyText() +
                            "'; parameter 'animate'. Are you sure this is correct?", LML_CRITICAL );
                }
                else
                {
                    ++mNumEnabledAnimationMatrices;
                }

                mEnabledAnimationMatrices[val] = true;

                pos = paramVal.find_first_of( ' ' );
            }
        }

        bakeTextures( textures );

        calculateHash();

        //Use the same hash for everything (the number of materials per buffer is high)
        creator->requestSlot( mNumEnabledAnimationMatrices != 0, this,
                              mNumEnabledAnimationMatrices != 0 );
    }
    //-----------------------------------------------------------------------------------
    HlmsUnlitDatablock::~HlmsUnlitDatablock()
    {
        if( mAssignedPool )
            static_cast<HlmsUnlit*>(mCreator)->releaseSlot( this );

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

            for( size_t i=0; i<NUM_UNLIT_TEXTURE_TYPES; ++i )
            {
                if( mSamplerblocks[i] )
                    hlmsManager->destroySamplerblock( mSamplerblocks[i] );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::calculateHash()
    {
        IdString hash;

        {
            UnlitBakedTextureArray::const_iterator itor = mBakedTextures.begin();
            UnlitBakedTextureArray::const_iterator end  = mBakedTextures.end();

            while( itor != end )
            {
                hash += IdString( itor->texture->getName() );
                hash += IdString( itor->samplerBlock->mId );

                ++itor;
            }
        }

        if( mTexturesDescSet )
        {
            FastArray<const TextureGpu*>::const_iterator itor = mTexturesDescSet->mTextures.begin();
            FastArray<const TextureGpu*>::const_iterator end  = mTexturesDescSet->mTextures.end();
            while( itor != end )
            {
                hash += (*itor)->getName();
                ++itor;
            }
        }
        if( mSamplersDescSet )
        {
            FastArray<const HlmsSamplerblock*>::const_iterator itor= mSamplersDescSet->mSamplers.begin();
            FastArray<const HlmsSamplerblock*>::const_iterator end = mSamplersDescSet->mSamplers.end();
            while( itor != end )
            {
                hash += IdString( (*itor)->mId );
                ++itor;
            }
        }

        if( mTextureHash != hash.mHash )
        {
            mTextureHash = hash.mHash;
            //static_cast<HlmsUnlit*>(mCreator)->requestSlot( mTextureHash, this );
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::scheduleConstBufferUpdate(void)
    {
        static_cast<HlmsUnlit*>(mCreator)->scheduleForUpdate( this );
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::uploadToConstBuffer( char *dstPtr )
    {
        memcpy( dstPtr, &mAlphaTestThreshold, sizeof( float ) );
        dstPtr += 4 * sizeof(float);
        memcpy( dstPtr, &mR, MaterialSizeInGpu - 4 * sizeof(float) );
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::uploadToExtraBuffer( char *dstPtr )
    {
#if !OGRE_DOUBLE_PRECISION
        memcpy( dstPtr, mTextureMatrices, sizeof( mTextureMatrices ) );
#else
        float * RESTRICT_ALIAS dstFloat = reinterpret_cast<float * RESTRICT_ALIAS>( dstPtr );

        for( size_t i=0; i<NUM_UNLIT_TEXTURE_TYPES * 4; ++i )
            *dstFloat++ = (float)mTextureMatrices[0][0][i];
#endif
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::updateDescriptorSets( bool textureSetDirty, bool samplerSetDirty )
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
            calculateHash();
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::decompileBakedTextures( UnlitBakedTexture outTextures[NUM_UNLIT_TEXTURE_TYPES] )
    {
        //Decompile the baked textures to know which texture is assigned to each type.
        for( size_t i=0; i<NUM_UNLIT_TEXTURE_TYPES; ++i )
        {
            uint8 idx = mTexToBakedTextureIdx[i];

            if( idx < NUM_UNLIT_TEXTURE_TYPES )
            {
                outTextures[i] = UnlitBakedTexture( mBakedTextures[idx].texture, mSamplerblocks[i] );
            }
            else
            {
                //The texture may be null, but the samplerblock information may still be there.
                outTextures[i] = UnlitBakedTexture( TexturePtr(), mSamplerblocks[i] );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::bakeTextures( const UnlitBakedTexture textures[NUM_UNLIT_TEXTURE_TYPES] )
    {
        //The shader might need to be recompiled (mTexToBakedTextureIdx changed).
        //We'll need to flush.
        //Most likely mTexIndices also changed, so we need to update the const buffers as well
        mBakedTextures.clear();

        for( size_t i=0; i<NUM_UNLIT_TEXTURE_TYPES; ++i )
        {
            if( !textures[i].texture.isNull() )
            {
                UnlitBakedTextureArray::const_iterator itor = std::find( mBakedTextures.begin(),
                                                                         mBakedTextures.end(),
                                                                         textures[i] );

                if( itor == mBakedTextures.end() )
                {
                    mTexToBakedTextureIdx[i] = mBakedTextures.size();
                    mBakedTextures.push_back( textures[i] );
                }
                else
                {
                    mTexToBakedTextureIdx[i] = itor - mBakedTextures.begin();
                }
            }
            else
            {
                mTexToBakedTextureIdx[i] = NUM_UNLIT_TEXTURE_TYPES;
            }
        }

        calculateHash();
        flushRenderables();
        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    bool OrderTextureByPoolAndName( const TextureGpu *_a, const TextureGpu *_b )
    {
        const TexturePool *poolA = _a->getTexturePool();
        const TexturePool *poolB = _b->getTexturePool();

        IdString poolNameA = poolA ? poolA->masterTexture->getName() : IdString();
        IdString poolNameB = poolB ? poolB->masterTexture->getName() : IdString();

        if( poolA != poolB )
            return poolNameA < poolNameB;

        return _a->getName() < _b->getName();
    }
    bool HlmsUnlitDatablock::bakeTextures( bool hasSeparateSamplers )
    {
        DescriptorSetTexture baseSet;
        DescriptorSetSampler baseSampler;
        for( size_t i=0; i<NUM_UNLIT_TEXTURE_TYPES; ++i )
        {
            if( mTextures[i] )
            {
                //Keep it sorted to maximize sharing of descriptor sets.
                FastArray<const TextureGpu*>::iterator itor =
                        std::lower_bound( baseSet.mTextures.begin(),
                                          baseSet.mTextures.end(),
                                          mTextures[i], OrderTextureByPoolAndName );

                const size_t idx = itor - baseSet.mTextures.begin();

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
            flushRenderables();
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
                hlmsManager->destroyDescriptorSetTexture( mTexturesDescSet );
            mTexturesDescSet = hlmsManager->getDescriptorSetTexture( baseSet );
            flushRenderables();
            needsRecalculateHash = true;
            if( !hasSeparateSamplers )
            {
                if( mSamplersDescSet )
                    hlmsManager->destroyDescriptorSetSampler( mSamplersDescSet );
                mSamplersDescSet = hlmsManager->getDescriptorSetSampler( baseSampler );
            }
        }

        return needsRecalculateHash;
    }
    //-----------------------------------------------------------------------------------
    bool OrderBlockById( const BasicBlock *_a, const BasicBlock *_b )
    {
        return _a->mId < _b->mId;
    }
    bool HlmsUnlitDatablock::bakeSamplers(void)
    {
        assert( mCreator->getRenderSystem()->
                getCapabilities()->hasCapability( RSC_SEPARATE_SAMPLERS_FROM_TEXTURES ) );

        DescriptorSetSampler baseSampler;
        for( size_t i=0; i<NUM_UNLIT_TEXTURE_TYPES; ++i )
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
    TexturePtr HlmsUnlitDatablock::setTexture( const String &name, uint8 texUnit )
    {
        HlmsManager *hlmsManager = mCreator->getHlmsManager();
        HlmsTextureManager *hlmsTextureManager = hlmsManager->getTextureManager();
        HlmsTextureManager::TextureLocation texLocation = hlmsTextureManager->
                                                    createOrRetrieveTexture(
                                                        name,
                                                        HlmsTextureManager::TEXTURE_TYPE_DIFFUSE );

        mTexIndices[texUnit] = texLocation.xIdx;

        return texLocation.texture;
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::setUseColour( bool useColour )
    {
        if( mHasColour != useColour )
        {
            mHasColour = useColour;
            flushRenderables();
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::setColour( const ColourValue &diffuse )
    {
        assert( diffuse == ColourValue::White ||
                (mHasColour &&
                 "Setting colour to a Datablock created w/out diffuse flag will be ignored") );
        mR = diffuse.r;
        mG = diffuse.g;
        mB = diffuse.b;
        mA = diffuse.a;

        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::setTexture( uint8 texType, uint16 arrayIndex,
                                       const TexturePtr &newTexture, const HlmsSamplerblock *refParams )
    {
        assert( texType < NUM_UNLIT_TEXTURE_TYPES );

        UnlitBakedTexture textures[NUM_UNLIT_TEXTURE_TYPES];

        //Decompile the baked textures to know which texture is assigned to each type.
        decompileBakedTextures( textures );

        //Set the new samplerblock
        if( refParams )
        {
            HlmsManager *hlmsManager = mCreator->getHlmsManager();
            const HlmsSamplerblock *oldSamplerblock = mSamplerblocks[texType];
            mSamplerblocks[texType] = hlmsManager->getSamplerblock( *refParams );

            if( oldSamplerblock )
                hlmsManager->destroySamplerblock( oldSamplerblock );
        }
        else if( !newTexture.isNull() && !mSamplerblocks[texType] )
        {
            //Adding a texture, but the samplerblock doesn't exist. Create a default one.
            HlmsSamplerblock samplerBlockRef;
            HlmsManager *hlmsManager = mCreator->getHlmsManager();
            mSamplerblocks[texType] = hlmsManager->getSamplerblock( samplerBlockRef );
        }

        UnlitBakedTexture oldTex = textures[texType];

        //Set the texture and make the samplerblock changes to take effect
        textures[texType].texture = newTexture;
        textures[texType].samplerBlock = mSamplerblocks[texType];
        mTexIndices[texType] = arrayIndex;

        if( oldTex == textures[texType] )
        {
            //Only the array index changed. Just update our constant buffer.
            scheduleConstBufferUpdate();
        }
        else
        {
            bakeTextures( textures );
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::setTexture( uint8 texType, const TextureGpu *texture,
                                         const HlmsSamplerblock *refParams, bool bUpdateDescriptorSets )
    {
        assert( texType < NUM_UNLIT_TEXTURE_TYPES );

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
                prevPool = mTextures[texType]->getTexturePool();

            mTextures[texType] = texture;

            if( texture && prevPool != texture->getTexturePool() )
                textureSetDirty = true;

            scheduleConstBufferUpdate();
        }

        //Set the new samplerblock
        if( refParams )
        {
            HlmsManager *hlmsManager = mCreator->getHlmsManager();
            const HlmsSamplerblock *oldSamplerblock = mSamplerblocks[texType];
            mSamplerblocks[texType] = hlmsManager->getSamplerblock( *refParams );

            if( oldSamplerblock != mSamplerblocks[texType] )
                samplerSetDirty = true;

            if( oldSamplerblock )
                hlmsManager->destroySamplerblock( oldSamplerblock );
        }
        else if( texture && !mSamplerblocks[texType] )
        {
            //Adding a texture, but the samplerblock doesn't exist. Create a default one.
            HlmsSamplerblock samplerBlockRef;
            HlmsManager *hlmsManager = mCreator->getHlmsManager();
            mSamplerblocks[texType] = hlmsManager->getSamplerblock( samplerBlockRef );
            samplerSetDirty = true;
        }

        if( bUpdateDescriptorSets )
            updateDescriptorSets( textureSetDirty, samplerSetDirty );
    }
    //-----------------------------------------------------------------------------------
    const TextureGpu* HlmsUnlitDatablock::getTexture( uint8 texType ) const
    {
        assert( texType < NUM_UNLIT_TEXTURE_TYPES );
        return mTextures[texType];
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::setTextureSwizzle( uint8 texType, uint8 r, uint8 g, uint8 b, uint8 a )
    {
        assert( texType < NUM_UNLIT_TEXTURE_TYPES );
        mTextureSwizzles[texType] = (r << 6u) | ((g & 0x03u) << 4u) | ((b & 0x03u) << 2u) | (a & 0x03u);
        flushRenderables();
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::setSamplerblock( uint8 texType, const HlmsSamplerblock &params )
    {
        const HlmsSamplerblock *oldSamplerblock = mSamplerblocks[texType];
        HlmsManager *hlmsManager = mCreator->getHlmsManager();
        mSamplerblocks[texType] = hlmsManager->getSamplerblock( params );

        if( oldSamplerblock != mSamplerblocks[texType] )
        {
            UnlitBakedTexture textures[NUM_UNLIT_TEXTURE_TYPES];
            decompileBakedTextures( textures );
            bakeTextures( textures );

            updateDescriptorSets( false, true );
        }

        if( oldSamplerblock )
            hlmsManager->destroySamplerblock( oldSamplerblock );
    }
    //-----------------------------------------------------------------------------------
    const HlmsSamplerblock* HlmsUnlitDatablock::getSamplerblock( uint8 texType ) const
    {
        assert( texType < NUM_UNLIT_TEXTURE_TYPES );
        return mSamplerblocks[texType];
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::setTextureUvSource( uint8 sourceType, uint8 uvSet )
    {
        if( uvSet >= 8 )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "UV set must be in rage in range [0; 8)",
                         "HlmsUnlitDatablock::setTextureUvSource" );
        }

        if( sourceType >= NUM_UNLIT_TEXTURE_TYPES )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Invalid sourceType",
                         "HlmsUnlitDatablock::setTextureUvSource" );
        }

        if( mUvSource[sourceType] != uvSet )
        {
            mUvSource[sourceType] = uvSet;
            flushRenderables();
        }
    }
    //-----------------------------------------------------------------------------------
    uint8 HlmsUnlitDatablock::getTextureUvSource( uint8 sourceType ) const
    {
        assert( sourceType < 8 );
        return mUvSource[sourceType];
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::setBlendMode( uint8 texType, UnlitBlendModes blendMode )
    {
        if( texType >= NUM_UNLIT_TEXTURE_TYPES )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Invalid blendMode",
                         "HlmsUnlitDatablock::setBlendMode" );
        }

        if( mBlendModes[texType] != blendMode )
        {
            mBlendModes[texType] = blendMode;
            flushRenderables();
        }
    }
    //-----------------------------------------------------------------------------------
    UnlitBlendModes HlmsUnlitDatablock::getBlendMode( uint8 texType ) const
    {
        return static_cast<UnlitBlendModes>( mBlendModes[texType] );
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::setEnableAnimationMatrix( uint8 textureUnit, bool bEnable )
    {
        assert( textureUnit < NUM_UNLIT_TEXTURE_TYPES );

        if( mEnabledAnimationMatrices[textureUnit] != bEnable )
        {
            mEnabledAnimationMatrices[textureUnit] = bEnable;
            mNumEnabledAnimationMatrices += (bEnable * 2) - 1; //bEnable ? +1 : -1

            if( !mNumEnabledAnimationMatrices || (mNumEnabledAnimationMatrices == 1 && bEnable) )
            {
                static_cast<HlmsUnlit*>(mCreator)->requestSlot( mNumEnabledAnimationMatrices != 0, this,
                                                                mNumEnabledAnimationMatrices != 0 );
            }

            scheduleConstBufferUpdate();

            flushRenderables();
        }
    }
    //-----------------------------------------------------------------------------------
    bool HlmsUnlitDatablock::getEnableAnimationMatrix( uint8 textureUnit ) const
    {
        return mEnabledAnimationMatrices[textureUnit];
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::setAnimationMatrix( uint8 textureUnit, const Matrix4 &matrix )
    {
        assert( textureUnit < NUM_UNLIT_TEXTURE_TYPES );

        mTextureMatrices[textureUnit] = matrix;
        if( mEnabledAnimationMatrices[textureUnit] )
            scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    const Matrix4& HlmsUnlitDatablock::getAnimationMatrix( uint8 textureUnit ) const
    {
        return mTextureMatrices[textureUnit];
    }
    //-----------------------------------------------------------------------------------
    uint8 HlmsUnlitDatablock::getBakedTextureIdx( uint8 texType ) const
    {
        assert( texType < NUM_UNLIT_TEXTURE_TYPES );
        return mTexToBakedTextureIdx[texType];
    }
    //-----------------------------------------------------------------------------------
    uint8 HlmsUnlitDatablock::getIndexToDescriptorTexture( uint8 texType )
    {
        assert( texType < NUM_UNLIT_TEXTURE_TYPES );
        uint8 retVal = NUM_UNLIT_TEXTURE_TYPES;

        const TextureGpu *texture = mTextures[texType];
        if( texture )
        {
            FastArray<const TextureGpu*>::const_iterator itor =
                    std::lower_bound( mTexturesDescSet->mTextures.begin(),
                                      mTexturesDescSet->mTextures.end(),
                                      texture, OrderTextureByPoolAndName );
            if( itor != mTexturesDescSet->mTextures.end() && *itor == texture )
            {
                size_t idx = itor - mTexturesDescSet->mTextures.begin();

                const RenderSystem *renderSystem = mCreator->getRenderSystem();
                const RenderSystemCapabilities *caps = renderSystem->getCapabilities();
                const bool hasSeparateSamplers =
                        caps->hasCapability( RSC_SEPARATE_SAMPLERS_FROM_TEXTURES );

                if( !hasSeparateSamplers )
                {
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
    uint8 HlmsUnlitDatablock::getIndexToDescriptorSampler( uint8 texType )
    {
        assert( texType < NUM_UNLIT_TEXTURE_TYPES );
        uint8 retVal = NUM_UNLIT_TEXTURE_TYPES;

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
}
