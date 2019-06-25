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

#define _OgreHlmsTextureBaseClassExport _OgreHlmsUnlitExport
#define OGRE_HLMS_TEXTURE_BASE_CLASS HlmsUnlitBaseTextureDatablock
#define OGRE_HLMS_TEXTURE_BASE_MAX_TEX NUM_UNLIT_TEXTURE_TYPES
#define OGRE_HLMS_CREATOR_CLASS HlmsUnlit
    #include "OgreHlmsTextureBaseClass.inl"
#undef _OgreHlmsTextureBaseClassExport
#undef OGRE_HLMS_TEXTURE_BASE_CLASS
#undef OGRE_HLMS_TEXTURE_BASE_MAX_TEX
#undef OGRE_HLMS_CREATOR_CLASS

#include "OgreHlmsUnlitDatablock.cpp.inc"

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
        HlmsUnlitBaseTextureDatablock( name, creator, macroblock, blendblock, params ),
        mNumEnabledAnimationMatrices( 0 ),
        mHasColour( false ),
        mR( 1.0f ), mG( 1.0f ), mB( 1.0f ), mA( 1.0f )
    {
        for( size_t i=0; i<NUM_UNLIT_TEXTURE_TYPES; ++i )
        {
            mTextureMatrices[i] = Matrix4::IDENTITY;
            setTextureSwizzle( i, R_MASK, G_MASK, B_MASK, A_MASK );
        }

        memset( mUvSource, 0, sizeof( mUvSource ) );
        memset( mBlendModes, 0, sizeof( mBlendModes ) );

        memset( mEnabledAnimationMatrices, 0, sizeof( mEnabledAnimationMatrices ) );
        memset( mEnablePlanarReflection, 0, sizeof( mEnablePlanarReflection ) );

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

        for( size_t i=0; i<sizeof( c_diffuseMap ) / sizeof( String ); ++i )
        {
            if( Hlms::findParamInVec( params, c_diffuseMap[i], paramVal ) )
            {
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
                            setTexture( i, *itor );
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

        //Use the same hash for everything (the number of materials per buffer is high)
        creator->requestSlot( mNumEnabledAnimationMatrices != 0, this,
                              mNumEnabledAnimationMatrices != 0 );
    }
    //-----------------------------------------------------------------------------------
    HlmsUnlitDatablock::~HlmsUnlitDatablock()
    {
        if( mAssignedPool )
            static_cast<HlmsUnlit*>(mCreator)->releaseSlot( this );
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitDatablock::calculateHash()
    {
        IdString hash;

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
    void HlmsUnlitDatablock::uploadToConstBuffer( char *dstPtr, uint8 dirtyFlags )
    {
        if( dirtyFlags & (ConstBufferPool::DirtyTextures|ConstBufferPool::DirtySamplers) )
        {
            //Must be called first so mTexIndices[i] gets updated before uploading to GPU.
            updateDescriptorSets( (dirtyFlags & ConstBufferPool::DirtyTextures) != 0,
                                  (dirtyFlags & ConstBufferPool::DirtySamplers) != 0 );
        }

        uint16 texIndices[OGRE_NumTexIndices];
        for( size_t i=0; i<OGRE_NumTexIndices; ++i )
            texIndices[i] = mTexIndices[i] & ~ManualTexIndexBit;

        memcpy( dstPtr, &mAlphaTestThreshold, sizeof( float ) );
        dstPtr += 4 * sizeof(float);
        memcpy( dstPtr, &mR, MaterialSizeInGpu - 4 * sizeof(float) - sizeof(mTexIndices) );
        dstPtr += MaterialSizeInGpu - 4 * sizeof(float) - sizeof(mTexIndices);
        memcpy( dstPtr, texIndices, sizeof(texIndices) );
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
    void HlmsUnlitDatablock::setTexture( uint8 texUnit, const String &name,
                                         const HlmsSamplerblock *refParams )
    {
        TextureGpuManager *textureManager = mCreator->getRenderSystem()->getTextureGpuManager();
        TextureGpu *texture = 0;
        if( !name.empty() )
        {
            texture = textureManager->createOrRetrieveTexture(
                          name, GpuPageOutStrategy::Discard,
                          TextureFlags::AutomaticBatching |
                          TextureFlags::PrefersLoadingFromFileAsSRGB,
                          TextureTypes::Type2D,
                          ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
        }
        setTexture( texUnit, texture, refParams );
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
    void HlmsUnlitDatablock::setTextureSwizzle( uint8 texType, uint8 r, uint8 g, uint8 b, uint8 a )
    {
        assert( texType < NUM_UNLIT_TEXTURE_TYPES );
        mTextureSwizzles[texType] = (r << 6u) | ((g & 0x03u) << 4u) | ((b & 0x03u) << 2u) | (a & 0x03u);
        flushRenderables();
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
    void HlmsUnlitDatablock::setEnablePlanarReflection( uint8 textureUnit, bool bEnable )
    {
        assert( textureUnit < NUM_UNLIT_TEXTURE_TYPES );

        if( mEnablePlanarReflection[textureUnit] != bEnable )
        {
            mEnablePlanarReflection[textureUnit] = bEnable;
            flushRenderables();
        }
    }
    //-----------------------------------------------------------------------------------
    bool HlmsUnlitDatablock::getEnablePlanarReflection( uint8 textureUnit ) const
    {
        return mEnablePlanarReflection[textureUnit];
    }
    //-----------------------------------------------------------------------------------
    ColourValue HlmsUnlitDatablock::getDiffuseColour(void) const
    {
        return ColourValue( 0, 0, 0, 0 );
    }
    //-----------------------------------------------------------------------------------
    ColourValue HlmsUnlitDatablock::getEmissiveColour(void) const
    {
        return hasColour() ? getColour() : ColourValue::White;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* HlmsUnlitDatablock::getEmissiveTexture(void) const
    {
        return getTexture( 0 );
    }
}
