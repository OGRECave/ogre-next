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

#include "Terra/Hlms/OgreHlmsTerraDatablock.h"
#include "Terra/Hlms/OgreHlmsTerra.h"
#include "OgreHlmsManager.h"
#include "OgreTextureGpu.h"
#include "OgreTextureGpuManager.h"
#include "OgreRenderSystem.h"
#include "OgreTextureFilters.h"
#include "OgreLogManager.h"

#define _OgreHlmsTextureBaseClassExport
#define OGRE_HLMS_TEXTURE_BASE_CLASS HlmsTerraBaseTextureDatablock
#define OGRE_HLMS_TEXTURE_BASE_MAX_TEX NUM_TERRA_TEXTURE_TYPES
#define OGRE_HLMS_CREATOR_CLASS HlmsTerra
    #include "OgreHlmsTextureBaseClass.inl"
#undef _OgreHlmsTextureBaseClassExport
#undef OGRE_HLMS_TEXTURE_BASE_CLASS
#undef OGRE_HLMS_TEXTURE_BASE_MAX_TEX
#undef OGRE_HLMS_CREATOR_CLASS

#include "OgreHlmsTerraDatablock.cpp.inc"

namespace Ogre
{
    const size_t HlmsTerraDatablock::MaterialSizeInGpu          = 4 * 10 * 4;
    const size_t HlmsTerraDatablock::MaterialSizeInGpuAligned   = alignToNextMultiple(
                                                                    HlmsTerraDatablock::MaterialSizeInGpu,
                                                                    4 * 4 );

    //-----------------------------------------------------------------------------------
    HlmsTerraDatablock::HlmsTerraDatablock( IdString name, HlmsTerra *creator,
                                        const HlmsMacroblock *macroblock,
                                        const HlmsBlendblock *blendblock,
                                        const HlmsParamVec &params ) :
        HlmsTerraBaseTextureDatablock( name, creator, macroblock, blendblock, params ),
        mkDr( 0.318309886f ), mkDg( 0.318309886f ), mkDb( 0.318309886f ), //Max Diffuse = 1 / PI
        _padding0( 1 ),
        mBrdf( TerraBrdf::Default )
    {
        mRoughness[0] = mRoughness[1] = 1.0f;
        mRoughness[2] = mRoughness[3] = 1.0f;
        mMetalness[0] = mMetalness[1] = 1.0f;
        mMetalness[2] = mMetalness[3] = 1.0f;

        for( size_t i=0; i<4; ++i )
            mDetailsOffsetScale[i] = Vector4( 0, 0, 1, 1 );

        creator->requestSlot( /*mTextureHash*/0, this, false );
        calculateHash();
    }
    //-----------------------------------------------------------------------------------
    HlmsTerraDatablock::~HlmsTerraDatablock()
    {
        if( mAssignedPool )
            static_cast<HlmsTerra*>(mCreator)->releaseSlot( this );

        HlmsManager *hlmsManager = mCreator->getHlmsManager();
        if( hlmsManager )
        {
            for( size_t i=0; i<NUM_TERRA_TEXTURE_TYPES; ++i )
            {
                if( mSamplerblocks[i] )
                    hlmsManager->destroySamplerblock( mSamplerblocks[i] );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerraDatablock::calculateHash()
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

        if( static_cast<HlmsTerra*>(mCreator)->getOptimizationStrategy() == HlmsTerra::LowerGpuOverhead )
        {
            const size_t poolIdx = static_cast<HlmsTerra*>(mCreator)->getPoolIndex( this );
            const uint32 finalHash = (hash.mHash & 0xFFFFFE00) | (poolIdx & 0x000001FF);
            mTextureHash = finalHash;
        }
        else
        {
            const size_t poolIdx = static_cast<HlmsTerra*>(mCreator)->getPoolIndex( this );
            const uint32 finalHash = (hash.mHash & 0xFFFFFFF0) | (poolIdx & 0x0000000F);
            mTextureHash = finalHash;
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerraDatablock::scheduleConstBufferUpdate(void)
    {
        static_cast<HlmsTerra*>(mCreator)->scheduleForUpdate( this );
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerraDatablock::uploadToConstBuffer( char *dstPtr, uint8 dirtyFlags )
    {
        memcpy( dstPtr, &mkDr, MaterialSizeInGpu );

        if( dirtyFlags & (ConstBufferPool::DirtyTextures|ConstBufferPool::DirtySamplers) )
        {
            //Must be called first so mTexIndices[i] gets updated before uploading to GPU.
            updateDescriptorSets( (dirtyFlags & ConstBufferPool::DirtyTextures) != 0,
                                  (dirtyFlags & ConstBufferPool::DirtySamplers) != 0 );
        }

        uint16 texIndices[OGRE_NumTexIndices];
        for( size_t i=0; i<OGRE_NumTexIndices; ++i )
            texIndices[i] = mTexIndices[i] & ~ManualTexIndexBit;

        memcpy( dstPtr, &mkDr, MaterialSizeInGpu - sizeof(mTexIndices) );
        dstPtr += MaterialSizeInGpu - sizeof(mTexIndices);
        memcpy( dstPtr, texIndices, sizeof(texIndices) );
        dstPtr += sizeof(texIndices);
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerraDatablock::setDiffuse( const Vector3 &diffuseColour )
    {
        const float invPI = 0.318309886f;
        mkDr = diffuseColour.x * invPI;
        mkDg = diffuseColour.y * invPI;
        mkDb = diffuseColour.z * invPI;
        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    Vector3 HlmsTerraDatablock::getDiffuse(void) const
    {
        return Vector3( mkDr, mkDg, mkDb ) * Ogre::Math::PI;
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerraDatablock::setRoughness( uint8 detailMapIdx, float roughness )
    {
        mRoughness[detailMapIdx] = roughness;
        if( mRoughness[detailMapIdx] <= 1e-6f )
        {
            LogManager::getSingleton().logMessage( "WARNING: TERRA Datablock '" +
                        mName.getFriendlyText() + "' Very low roughness values can "
                                                  "cause NaNs in the pixel shader!" );
        }
        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    float HlmsTerraDatablock::getRoughness( uint8 detailMapIdx ) const
    {
        return mRoughness[detailMapIdx];
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerraDatablock::setMetalness( uint8 detailMapIdx, float metalness )
    {
        mMetalness[detailMapIdx] = metalness;
        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    float HlmsTerraDatablock::getMetalness( uint8 detailMapIdx ) const
    {
        return mMetalness[detailMapIdx];
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerraDatablock::setDetailMapOffsetScale( uint8 detailMap, const Vector4 &offsetScale )
    {
        assert( detailMap < 8 );
        bool wasDisabled = mDetailsOffsetScale[detailMap] == Vector4( 0, 0, 1, 1 );

        mDetailsOffsetScale[detailMap] = offsetScale;

        if( wasDisabled != (mDetailsOffsetScale[detailMap] == Vector4( 0, 0, 1, 1 )) )
        {
            flushRenderables();
        }

        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    const Vector4& HlmsTerraDatablock::getDetailMapOffsetScale( uint8 detailMap ) const
    {
        assert( detailMap < 8 );
        return mDetailsOffsetScale[detailMap];
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerraDatablock::setAlphaTestThreshold( float threshold )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "Alpha testing not supported on Terra Hlms",
                     "HlmsTerraDatablock::setAlphaTestThreshold" );

        HlmsDatablock::setAlphaTestThreshold( threshold );
        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    void HlmsTerraDatablock::setBrdf( TerraBrdf::TerraBrdf brdf )
    {
        if( mBrdf != brdf )
        {
            mBrdf = brdf;
            flushRenderables();
        }
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsTerraDatablock::getBrdf(void) const
    {
        return mBrdf;
    }
    //-----------------------------------------------------------------------------------
    bool HlmsTerraDatablock::suggestUsingSRGB( TerraTextureTypes type ) const
    {
        if( type == TERRA_DETAIL_WEIGHT ||
            (type >= TERRA_DETAIL_METALNESS0 && type <= TERRA_DETAIL_METALNESS3) ||
            (type >= TERRA_DETAIL_ROUGHNESS0 && type <= TERRA_DETAIL_ROUGHNESS3) ||
            (type >= TERRA_DETAIL0_NM && type <= TERRA_DETAIL3_NM) )
        {
            return false;
        }

        return true;
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsTerraDatablock::suggestFiltersForType( TerraTextureTypes type ) const
    {
        switch( type )
        {
        case TERRA_DETAIL0_NM:
        case TERRA_DETAIL1_NM:
        case TERRA_DETAIL2_NM:
        case TERRA_DETAIL3_NM:
            return TextureFilter::TypePrepareForNormalMapping;
        case TERRA_DETAIL_ROUGHNESS0:
        case TERRA_DETAIL_ROUGHNESS1:
        case TERRA_DETAIL_ROUGHNESS2:
        case TERRA_DETAIL_ROUGHNESS3:
        case TERRA_DETAIL_METALNESS0:
        case TERRA_DETAIL_METALNESS1:
        case TERRA_DETAIL_METALNESS2:
        case TERRA_DETAIL_METALNESS3:
            return TextureFilter::TypeLeaveChannelR;
        default:
            return 0;
        }

        return 0;
    }
    //-----------------------------------------------------------------------------------
    /*HlmsTextureManager::TextureMapType HlmsTerraDatablock::suggestMapTypeBasedOnTextureType(
                                                                        TerraTextureTypes type )
    {
        HlmsTextureManager::TextureMapType retVal;
        switch( type )
        {
        default:
        case TERRA_DIFFUSE:
            retVal = HlmsTextureManager::TEXTURE_TYPE_DIFFUSE;
            break;
        case TERRA_DETAIL_WEIGHT:
            retVal = HlmsTextureManager::TEXTURE_TYPE_NON_COLOR_DATA;
            break;
        case TERRA_DETAIL0:
        case TERRA_DETAIL1:
        case TERRA_DETAIL2:
        case TERRA_DETAIL3:
#ifdef OGRE_TEXTURE_ATLAS
            retVal = HlmsTextureManager::TEXTURE_TYPE_DETAIL;
#else
            retVal = HlmsTextureManager::TEXTURE_TYPE_DIFFUSE;
#endif
            break;

        case TERRA_DETAIL0_NM:
        case TERRA_DETAIL1_NM:
        case TERRA_DETAIL2_NM:
        case TERRA_DETAIL3_NM:
#ifdef OGRE_TEXTURE_ATLAS
            retVal = HlmsTextureManager::TEXTURE_TYPE_DETAIL_NORMAL_MAP;
#else
            retVal = HlmsTextureManager::TEXTURE_TYPE_NORMALS;
#endif
            break;

        case TERRA_DETAIL_ROUGHNESS0:
        case TERRA_DETAIL_ROUGHNESS1:
        case TERRA_DETAIL_ROUGHNESS2:
        case TERRA_DETAIL_ROUGHNESS3:
        case TERRA_DETAIL_METALNESS0:
        case TERRA_DETAIL_METALNESS1:
        case TERRA_DETAIL_METALNESS2:
        case TERRA_DETAIL_METALNESS3:
            retVal = HlmsTextureManager::TEXTURE_TYPE_MONOCHROME;
            break;

        case TERRA_REFLECTION:
            retVal = HlmsTextureManager::TEXTURE_TYPE_ENV_MAP;
            break;
        }

        return retVal;
    }*/
}
