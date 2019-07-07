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

#include "OgreHlmsComputeJob.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsCompute.h"

#include "OgreRenderSystem.h"

#include "Vao/OgreTexBufferPacked.h"
#include "Vao/OgreUavBufferPacked.h"

#include "OgreTextureGpu.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreLwString.h"

#include "OgreLogManager.h"

namespace Ogre
{
    static const IdString c_textureTypesProps[8] =
    {
        "TextureTypes_Unknown",
        "TextureTypes_Type1D",
        "TextureTypes_Type1DArray",
        "TextureTypes_Type2D",
        "TextureTypes_Type2DArray",
        "TextureTypes_TypeCube",
        "TextureTypes_TypeCubeArray",
        "TextureTypes_Type3D"
    };

    //-----------------------------------------------------------------------------------
    HlmsComputeJob::HlmsComputeJob( IdString name, Hlms *creator,
                                    const String &sourceFilename,
                                    const StringVector &includedPieceFiles ) :
        mCreator( creator ),
        mName( name ),
        mSourceFilename( sourceFilename ),
        mIncludedPieceFiles( includedPieceFiles ),
        mThreadGroupsBasedOnTexture( ThreadGroupsBasedOnNothing ),
        mThreadGroupsBasedOnTexSlot( 0 ),
        mTexturesDescSet( 0 ),
        mSamplersDescSet( 0 ),
        mUavsDescSet( 0 ),
        mInformHlmsOfTextureData( false ),
        mMaxTexUnitReached( 0 ),
        mMaxUavUnitReached( 0 ),
        mPsoCacheHash( -1 )
    {
        memset( mThreadsPerGroup, 0, sizeof( mThreadsPerGroup ) );
        memset( mNumThreadGroups, 0, sizeof( mNumThreadGroups ) );

        mThreadGroupsBasedDivisor[0] = 1;
        mThreadGroupsBasedDivisor[1] = 1;
        mThreadGroupsBasedDivisor[2] = 1;
    }
    //-----------------------------------------------------------------------------------
    HlmsComputeJob::~HlmsComputeJob()
    {
        destroyDescriptorSamplers();
        destroyDescriptorTextures();
        destroyDescriptorUavs();

        removeListenerFromTextures( mUavSlots, 0, mUavSlots.size() );
        removeListenerFromTextures( mTexSlots, 0, mTexSlots.size() );

        HlmsManager *hlmsManager = mCreator->getHlmsManager();
        FastArray<const HlmsSamplerblock*>::const_iterator itor = mSamplerSlots.begin();
        FastArray<const HlmsSamplerblock*>::const_iterator end  = mSamplerSlots.end();

        while( itor != end )
        {
            if( *itor )
                hlmsManager->destroySamplerblock( *itor );
            ++itor;
        }

        mSamplerSlots.clear();
    }
    //-----------------------------------------------------------------------------------
    template <typename T>
    void HlmsComputeJob::removeListenerFromTextures( T &container, size_t first, size_t lastPlusOne )
    {
        typename T::const_iterator itor = container.begin() + first;
        typename T::const_iterator end  = container.begin() + lastPlusOne;
        while( itor != end )
        {
            if( itor->isTexture() && itor->getTexture().texture )
                itor->getTexture().texture->removeListener( this );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::destroyDescriptorSamplers(void)
    {
        if( mSamplersDescSet )
        {
            HlmsManager *hlmsManager = mCreator->getHlmsManager();
            hlmsManager->destroyDescriptorSetSampler( mSamplersDescSet );
            mSamplersDescSet = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::destroyDescriptorTextures(void)
    {
        if( mTexturesDescSet )
        {
            HlmsManager *hlmsManager = mCreator->getHlmsManager();
            hlmsManager->destroyDescriptorSetTexture2( mTexturesDescSet );
            mTexturesDescSet = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::destroyDescriptorUavs(void)
    {
        if( mUavsDescSet )
        {
            HlmsManager *hlmsManager = mCreator->getHlmsManager();
            hlmsManager->destroyDescriptorSetUav( mUavsDescSet );
            mUavsDescSet = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::setTextureProperties( const TextureGpu *texture, PixelFormatGpu pixelFormat,
                                               ResourceAccess::ResourceAccess access,
                                               LwString &propName,
                                               const PixelFormatToShaderType *toShaderType )
    {
        if( pixelFormat == PFG_UNKNOWN )
            pixelFormat = texture->getPixelFormat();

        const size_t texturePropSize = propName.size();

        propName.a( "_width" );                 //texture0_width
        setProperty( propName.c_str(), texture->getWidth() );
        propName.resize( texturePropSize );

        propName.a( "_height" );                //texture0_height
        setProperty( propName.c_str(), texture->getHeight() );
        propName.resize( texturePropSize );

        propName.a( "_depth" );                 //texture0_depth
        setProperty( propName.c_str(), texture->getDepthOrSlices() );
        propName.resize( texturePropSize );

        propName.a( "_mipmaps" );               //texture0_mipmaps
        setProperty( propName.c_str(), texture->getNumMipmaps() );
        propName.resize( texturePropSize );

        propName.a( "_msaa" );                  //texture0_msaa
        setProperty( propName.c_str(), texture->getMsaa() > 1u ? 1 : 0 );
        propName.resize( texturePropSize );

        propName.a( "_msaa_samples" );          //texture0_msaa_samples
        setProperty( propName.c_str(), texture->getMsaa() );
        propName.resize( texturePropSize );

        propName.a( "_texture_type" );          //_texture_type
        setProperty( propName.c_str(),
                     c_textureTypesProps[texture->getTextureType()].mHash );
        propName.resize( texturePropSize );

        propName.a( "_pf_type" );               //uav0_pf_type
        const char *typeName = toShaderType->getPixelFormatType( pixelFormat );
        if( typeName )
            setPiece( propName.c_str(), typeName );
        propName.resize( texturePropSize );

        propName.a( "_data_type" );             //uav0_data_type
        const char *dataType = toShaderType->getDataType( pixelFormat,
                                                          texture->getTextureType(),
                                                          texture->getMsaa() > 1u,
                                                          access );
        if( typeName )
            setPiece( propName.c_str(), dataType );
        propName.resize( texturePropSize );

        if( PixelFormatGpuUtils::isInteger( pixelFormat ) )
        {
            propName.a( "_is_integer" );        //uav0_is_integer
            setProperty( propName.c_str(), 1 );
            propName.resize( texturePropSize );
        }
        if( PixelFormatGpuUtils::isSigned( pixelFormat ) )
        {
            propName.a( "_is_signed" );         //uav0_is_signed
            setProperty( propName.c_str(), 1 );
            propName.resize( texturePropSize );
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::clearAutoProperties( const char *propTexture, uint8 maxTexUnitReached )
    {
        assert( propTexture == ComputeProperty::Texture || propTexture == ComputeProperty::Uav );
        char tmpData[64];
        LwString propName = LwString::FromEmptyPointer( tmpData, sizeof(tmpData) );
        propName = propTexture; //It's either ComputeProperty::Texture or ComputeProperty::Uav

        const size_t texturePropNameSize = propName.size();

        //Remove everything from any previous run.
        for( uint8 i=0; i<maxTexUnitReached; ++i )
        {
            propName.resize( texturePropNameSize );
            propName.a( i );                        //texture0 or uav0

            const size_t texturePropSize = propName.size();

            propName.a( "_width" );                 //texture0_width
            removeProperty( propName.c_str() );
            propName.resize( texturePropSize );

            propName.a( "_height" );                //texture0_height
            removeProperty( propName.c_str() );
            propName.resize( texturePropSize );

            propName.a( "_depth" );                 //texture0_depth
            removeProperty( propName.c_str() );
            propName.resize( texturePropSize );

            propName.a( "_mipmaps" );               //texture0_mipmaps
            removeProperty( propName.c_str() );
            propName.resize( texturePropSize );

            propName.a( "_msaa" );                  //texture0_msaa
            removeProperty( propName.c_str() );
            propName.resize( texturePropSize );

            propName.a( "_msaa_samples" );          //texture0_msaa_samples
            removeProperty( propName.c_str() );
            propName.resize( texturePropSize );

            propName.a( "_texture_type" );          //texture0_texture_type
            removeProperty( propName.c_str() );
            propName.resize( texturePropSize );

            propName.a( "_is_buffer" );             //texture0_is_buffer
            removeProperty( propName.c_str() );
            propName.resize( texturePropSize );

            propName.a( "_pf_type" );           //uav0_pf_type
            removePiece( propName.c_str() );
            propName.resize( texturePropSize );

            //Note we're comparing pointers, not string comparison!
            if( propTexture == ComputeProperty::Uav )
            {
                propName.a( "_width_with_lod" );    //uav0_width_with_lod
                removeProperty( propName.c_str() );
                propName.resize( texturePropSize );

                propName.a( "_height_with_lod" );   //uav0_height_with_lod
                removeProperty( propName.c_str() );
                propName.resize( texturePropSize );

                propName.a( "_is_integer" );        //uav0_is_integer
                removeProperty( propName.c_str() );
                propName.resize( texturePropSize );

                propName.a( "_is_signed" );         //uav0_is_signed
                removeProperty( propName.c_str() );
                propName.resize( texturePropSize );

                propName.a( "_data_type" );         //uav0_data_type
                removePiece( propName.c_str() );
                propName.resize( texturePropSize );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    String HlmsComputeJob::getNameStr(void) const
    {
        String retVal;
        HlmsCompute *compute = static_cast<HlmsCompute*>( mCreator );
        const String *nameStr = compute->getJobNameStr( mName );

        if( nameStr )
            retVal = *nameStr;
        else
            retVal = mName.getFriendlyText();

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::_updateAutoProperties(void)
    {
        setProperty( ComputeProperty::ThreadsPerGroupX, mThreadsPerGroup[0] );
        setProperty( ComputeProperty::ThreadsPerGroupY, mThreadsPerGroup[1] );
        setProperty( ComputeProperty::ThreadsPerGroupZ, mThreadsPerGroup[2] );
        setProperty( ComputeProperty::NumThreadGroupsX, mNumThreadGroups[0] );
        setProperty( ComputeProperty::NumThreadGroupsY, mNumThreadGroups[1] );
        setProperty( ComputeProperty::NumThreadGroupsZ, mNumThreadGroups[2] );

        RenderSystem *renderSystem = mCreator->getRenderSystem();
        const bool typedUavs = renderSystem->getCapabilities()->hasCapability( RSC_TYPED_UAV_LOADS );
        setProperty( ComputeProperty::TypedUavLoad, typedUavs ? 1 : 0 );

        removeProperty( ComputeProperty::NumTextureSlots );
        removeProperty( ComputeProperty::MaxTextureSlot );
        removeProperty( ComputeProperty::NumUavSlots );
        removeProperty( ComputeProperty::MaxUavSlot );

        for( size_t i=0; i<sizeof(c_textureTypesProps) / sizeof(c_textureTypesProps[0]); ++i )
            removeProperty( c_textureTypesProps[i] );

        clearAutoProperties( ComputeProperty::Texture, mMaxTexUnitReached );
        clearAutoProperties( ComputeProperty::Uav, mMaxUavUnitReached );

        mMaxTexUnitReached = 0;
        mMaxUavUnitReached = 0;

        if( mInformHlmsOfTextureData )
        {
            for( size_t i=0; i<sizeof(c_textureTypesProps) / sizeof(c_textureTypesProps[0]); ++i )
                setProperty( c_textureTypesProps[i], c_textureTypesProps[i].mHash );

            const PixelFormatToShaderType *toShaderType = renderSystem->getPixelFormatToShaderType();

            char tmpData[64];
            LwString propName = LwString::FromEmptyPointer( tmpData, sizeof(tmpData) );

            //Deal with textures
            {
                //Inform number of UAVs
                setProperty( ComputeProperty::NumTextureSlots,
                             static_cast<int32>( mTexSlots.size() ) );

                propName = ComputeProperty::Texture;
                const size_t texturePropNameSize = propName.size();

                DescriptorSetTexSlotArray::const_iterator begin= mTexSlots.begin();
                DescriptorSetTexSlotArray::const_iterator itor = mTexSlots.begin();
                DescriptorSetTexSlotArray::const_iterator end  = mTexSlots.end();

                while( itor != end )
                {
                    const size_t slotIdx = itor - begin;
                    propName.resize( texturePropNameSize );
                    propName.a( static_cast<uint32>(slotIdx) ); //texture0
                    const size_t texturePropSize = propName.size();

                    if( !itor->empty() )
                    {
                        setProperty( propName.c_str(), 1 );

                        if( itor->isTexture() )
                        {
                            setTextureProperties( itor->getTexture().texture,
                                                  itor->getTexture().pixelFormat,
                                                  ResourceAccess::Undefined,
                                                  propName, toShaderType );
                        }
                        else if( itor->isBuffer() )
                        {
                            propName.a( "_is_buffer" );             //texture0_is_buffer
                            setProperty( propName.c_str(), 1 );
                            propName.resize( texturePropSize );
                        }
                    }

                    ++itor;
                }

                mMaxTexUnitReached = mTexSlots.size();
            }

            //Deal with UAVs
            {
                //Inform number of UAVs
                setProperty( ComputeProperty::NumUavSlots,
                             static_cast<int32>( mUavSlots.size() ) );

                propName = ComputeProperty::Uav;
                const size_t texturePropNameSize = propName.size();

                DescriptorSetUavSlotArray::const_iterator begin= mUavSlots.begin();
                DescriptorSetUavSlotArray::const_iterator itor = mUavSlots.begin();
                DescriptorSetUavSlotArray::const_iterator end  = mUavSlots.end();

                while( itor != end )
                {
                    const size_t slotIdx = itor - begin;
                    propName.resize( texturePropNameSize );
                    propName.a( static_cast<uint32>(slotIdx) ); //uav0
                    const size_t texturePropSize = propName.size();

                    if( !itor->empty() )
                        setProperty( propName.c_str(), 1 );

                    if( itor->isTexture() && itor->getTexture().texture )
                    {
                        const DescriptorSetUav::TextureSlot &texSlot = itor->getTexture();
                        setTextureProperties( texSlot.texture, texSlot.pixelFormat, texSlot.access,
                                              propName, toShaderType );

                        const TextureGpu *texture = texSlot.texture;

                        uint32 mipLevel = std::min<uint32>( texSlot.mipmapLevel,
                                                            texture->getNumMipmaps() - 1u );

                        propName.a( "_width_with_lod" );    //uav0_width_with_lod
                        setProperty( propName.c_str(), std::max( texture->getWidth() >>
                                                                 (uint32)mipLevel, 1u ) );
                        propName.resize( texturePropSize );

                        propName.a( "_height_with_lod" );   //uav0_height_with_lod
                        setProperty( propName.c_str(), std::max( texture->getHeight() >>
                                                                 (uint32)mipLevel, 1u ) );
                        propName.resize( texturePropSize );
                    }
                    else if( itor->isBuffer() && itor->getBuffer().buffer )
                    {
                        propName.a( "_is_buffer" );             //uav0_is_buffer
                        setProperty( propName.c_str(), 1 );
                        propName.resize( texturePropSize );
                    }

                    ++itor;
                }

                mMaxUavUnitReached = mUavSlots.size();
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::setInformHlmsOfTextureData( bool bInformHlms )
    {
        mInformHlmsOfTextureData = bInformHlms;
        mPsoCacheHash = -1;
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::setThreadsPerGroup( uint32 threadsPerGroupX, uint32 threadsPerGroupY, uint32 threadsPerGroupZ )
    {
        if( mThreadsPerGroup[0] != threadsPerGroupX ||
            mThreadsPerGroup[1] != threadsPerGroupY ||
            mThreadsPerGroup[2] != threadsPerGroupZ )
        {
            mThreadsPerGroup[0] = threadsPerGroupX;
            mThreadsPerGroup[1] = threadsPerGroupY;
            mThreadsPerGroup[2] = threadsPerGroupZ;
            mPsoCacheHash = -1;
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::setNumThreadGroups( uint32 numThreadGroupsX, uint32 numThreadGroupsY, uint32 numThreadGroupsZ )
    {
        if( mNumThreadGroups[0] != numThreadGroupsX ||
            mNumThreadGroups[1] != numThreadGroupsY ||
            mNumThreadGroups[2] != numThreadGroupsZ )
        {
            mNumThreadGroups[0] = numThreadGroupsX;
            mNumThreadGroups[1] = numThreadGroupsY;
            mNumThreadGroups[2] = numThreadGroupsZ;
            mPsoCacheHash = -1;
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::setNumThreadGroupsBasedOn( ThreadGroupsBasedOn source, uint8 texSlot,
                                                    uint8 divisorX, uint8 divisorY, uint8 divisorZ )
    {
        mThreadGroupsBasedOnTexture = source;
        mThreadGroupsBasedOnTexSlot = texSlot;

        mThreadGroupsBasedDivisor[0] = divisorX;
        mThreadGroupsBasedDivisor[1] = divisorY;
        mThreadGroupsBasedDivisor[2] = divisorZ;
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::_calculateNumThreadGroupsBasedOnSetting()
    {
        bool hasChanged = false;

        if( mThreadGroupsBasedOnTexture != ThreadGroupsBasedOnNothing )
        {
            const size_t maxSlots = mThreadGroupsBasedOnTexture == ThreadGroupsBasedOnTexture ?
                                        mTexSlots.size() : mUavSlots.size();
            TextureGpu const *tex = 0;
            uint8 mipLevel = 0;
            if( mThreadGroupsBasedOnTexSlot < maxSlots )
            {
                if( mThreadGroupsBasedOnTexture == ThreadGroupsBasedOnTexture )
                {
                    tex = mTexSlots[mThreadGroupsBasedOnTexSlot].getTexture().texture;
                    mipLevel = mTexSlots[mThreadGroupsBasedOnTexSlot].getTexture().mipmapLevel;
                }
                else if( mUavSlots[mThreadGroupsBasedOnTexSlot].isTexture() )
                {
                    tex = mUavSlots[mThreadGroupsBasedOnTexSlot].getTexture().texture;
                    mipLevel = mUavSlots[mThreadGroupsBasedOnTexSlot].getTexture().mipmapLevel;
                }
            }

            if( tex )
            {
                uint32 resolution[3];
                resolution[0] = tex->getWidth() >> mipLevel;
                resolution[1] = tex->getHeight() >> mipLevel;
                resolution[2] = tex->getDepthOrSlices() >> mipLevel;

                for( int i=0; i<3; ++i )
                {
                    resolution[i] = (resolution[i] + mThreadGroupsBasedDivisor[i] - 1u) /
                                    mThreadGroupsBasedDivisor[i];

                    uint32 numThreadGroups = (resolution[i] + mThreadsPerGroup[i] - 1u) /
                                             mThreadsPerGroup[i];
                    if( mNumThreadGroups[i] != numThreadGroups )
                    {
                        mNumThreadGroups[i] = numThreadGroups;
                        hasChanged = true;
                    }
                }
            }
            else
            {
                LogManager::getSingleton().logMessage(
                            "WARNING: No texture/uav bound to compute job '" + mName.getFriendlyText() +
                            "' at slot " + StringConverter::toString(mThreadGroupsBasedOnTexSlot) +
                            " while calculating number of thread groups based on texture");
            }
        }

        if( hasChanged )
            mPsoCacheHash = -1;

        if( !mUavsDescSet && !mUavSlots.empty() )
        {
            //UAV desc set is dirty. Time to calculate it again.
            HlmsManager *hlmsManager = mCreator->getHlmsManager();

            DescriptorSetUav baseParams;
            baseParams.mUavs = mUavSlots;
            mUavsDescSet = hlmsManager->getDescriptorSetUav( baseParams );
        }

        if( !mTexturesDescSet && !mTexSlots.empty() )
        {
            //Texture desc set is dirty. Time to calculate it again.
            HlmsManager *hlmsManager = mCreator->getHlmsManager();

            DescriptorSetTexture2 baseParams;
            baseParams.mTextures = mTexSlots;
            baseParams.mShaderTypeTexCount[0] = mTexSlots.size();
            mTexturesDescSet = hlmsManager->getDescriptorSetTexture2( baseParams );
        }

        if( !mSamplersDescSet && !mSamplerSlots.empty() )
        {
            //Sampler desc set is dirty. Time to calculate it again.
            HlmsManager *hlmsManager = mCreator->getHlmsManager();

            DescriptorSetSampler baseParams;
            baseParams.mSamplers = mSamplerSlots;
            baseParams.mShaderTypeSamplerCount[0] = mSamplerSlots.size();
            mSamplersDescSet = hlmsManager->getDescriptorSetSampler( baseParams );
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::setProperty( IdString key, int32 value )
    {
        HlmsProperty p( key, value );
        HlmsPropertyVec::iterator it = std::lower_bound( mSetProperties.begin(), mSetProperties.end(),
                                                         p, OrderPropertyByIdString );
        if( it == mSetProperties.end() || it->keyName != p.keyName )
            mSetProperties.insert( it, p );
        else
            *it = p;

        mPsoCacheHash = -1;
    }
    //-----------------------------------------------------------------------------------
    int32 HlmsComputeJob::getProperty( IdString key, int32 defaultVal ) const
    {
        HlmsProperty p( key, 0 );
        HlmsPropertyVec::const_iterator it = std::lower_bound( mSetProperties.begin(),
                                                               mSetProperties.end(),
                                                               p, OrderPropertyByIdString );
        if( it != mSetProperties.end() && it->keyName == p.keyName )
            defaultVal = it->value;

        return defaultVal;
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::removeProperty( IdString key )
    {
        HlmsProperty p( key, 0 );
        HlmsPropertyVec::iterator it = std::lower_bound( mSetProperties.begin(), mSetProperties.end(),
                                                         p, OrderPropertyByIdString );
        if( it != mSetProperties.end() && it->keyName == p.keyName )
            mSetProperties.erase( it );
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::setPiece( IdString pieceName, const String &pieceContent )
    {
        mPieces[pieceName] = pieceContent;

        int32 contentHash = 0;
        MurmurHash3_x86_32( pieceContent.c_str(), pieceContent.size(), IdString::Seed, &contentHash );
        setProperty( pieceName, contentHash );
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::removePiece( IdString pieceName )
    {
        PiecesMap::iterator it = mPieces.find( pieceName );
        if( it != mPieces.end() )
        {
            removeProperty( pieceName );
            mPieces.erase( it );
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::setConstBuffer( uint8 slotIdx, ConstBufferPacked *constBuffer )
    {
        ConstBufferSlotVec::iterator itor = std::lower_bound( mConstBuffers.begin(),
                                                              mConstBuffers.end(), slotIdx,
                                                              ConstBufferSlot() );

        if( !constBuffer )
        {
            if( itor != mConstBuffers.end() && itor->slotIdx == slotIdx )
                mConstBuffers.erase( itor );
        }
        else
        {
            if( itor == mConstBuffers.end() || itor->slotIdx != slotIdx )
                itor = mConstBuffers.insert( itor, ConstBufferSlot() );

            itor->slotIdx = slotIdx;
            itor->buffer = constBuffer;
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::createShaderParams( IdString key )
    {
        if( mShaderParams.find( key ) == mShaderParams.end() )
            mShaderParams[key] = ShaderParams();
    }
    //-----------------------------------------------------------------------------------
    ShaderParams& HlmsComputeJob::getShaderParams( IdString key )
    {
        ShaderParams *retVal = 0;

        map<IdString, ShaderParams>::type::iterator itor = mShaderParams.find( key );
        if( itor == mShaderParams.end() )
        {
            createShaderParams( key );
            itor = mShaderParams.find( key );
        }

        retVal = &itor->second;

        return *retVal;
    }
    //-----------------------------------------------------------------------------------
    ShaderParams* HlmsComputeJob::_getShaderParams( IdString key )
    {
        ShaderParams *retVal = 0;

        map<IdString, ShaderParams>::type::iterator itor = mShaderParams.find( key );
        if( itor != mShaderParams.end() )
            retVal = &itor->second;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::setNumTexUnits( uint8 numSlots )
    {
        destroyDescriptorSamplers();
        destroyDescriptorTextures();

        if( numSlots < mSamplerSlots.size() )
        {
            HlmsManager *hlmsManager = mCreator->getHlmsManager();
            FastArray<const HlmsSamplerblock*>::const_iterator itor = mSamplerSlots.begin() + numSlots;
            FastArray<const HlmsSamplerblock*>::const_iterator end  = mSamplerSlots.end();

            while( itor != end )
            {
                if( *itor )
                    hlmsManager->destroySamplerblock( *itor );
                ++itor;
            }

            removeListenerFromTextures( mTexSlots, numSlots, mTexSlots.size() );
        }

        mSamplerSlots.resize( numSlots );
        mTexSlots.resize( numSlots );
        if( mInformHlmsOfTextureData )
            mPsoCacheHash = -1;
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::removeTexUnit( uint8 slotIdx )
    {
        destroyDescriptorSamplers();
        destroyDescriptorTextures();

        if( mSamplerSlots[slotIdx] )
        {
            HlmsManager *hlmsManager = mCreator->getHlmsManager();
            hlmsManager->destroySamplerblock( mSamplerSlots[slotIdx] );
        }
        removeListenerFromTextures( mTexSlots, slotIdx, slotIdx + 1u );

        mSamplerSlots.erase( mSamplerSlots.begin() + slotIdx );
        mTexSlots.erase( mTexSlots.begin() + slotIdx );
        if( mInformHlmsOfTextureData )
            mPsoCacheHash = -1;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* HlmsComputeJob::getTexture( uint8 slotIdx ) const
    {
        TextureGpu *retVal = 0;

        if( mTexSlots[slotIdx].isTexture() )
            retVal = mTexSlots[slotIdx].getTexture().texture;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::setNumUavUnits( uint8 numSlots )
    {
        destroyDescriptorUavs();

        if( numSlots < mUavSlots.size() )
            removeListenerFromTextures( mUavSlots, numSlots, mUavSlots.size() );

        mUavSlots.resize( numSlots );
        if( mInformHlmsOfTextureData )
            mPsoCacheHash = -1;
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::removeUavUnit( uint8 slotIdx )
    {
        destroyDescriptorUavs();
        removeListenerFromTextures( mUavSlots, slotIdx, slotIdx + 1u );
        mUavSlots.erase( mUavSlots.begin() + slotIdx );
        if( mInformHlmsOfTextureData )
            mPsoCacheHash = -1;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* HlmsComputeJob::getUavTexture( uint8 slotIdx ) const
    {
        return mUavSlots[slotIdx].getTexture().texture;
    }
    //-----------------------------------------------------------------------------------
    UavBufferPacked* HlmsComputeJob::getUavBuffer( uint8 slotIdx ) const
    {
        return mUavSlots[slotIdx].getBuffer().buffer;
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::setTexBuffer( uint8 slotIdx, const DescriptorSetTexture2::BufferSlot &newSlot )
    {
        assert( slotIdx < mTexSlots.size() );

        DescriptorSetTexture2::Slot &slot = mTexSlots[slotIdx];
        if( slot.slotType != DescriptorSetTexture2::SlotTypeBuffer ||
            slot.getBuffer() != newSlot )
        {
            if( mInformHlmsOfTextureData &&
                slot.slotType == DescriptorSetTexture2::SlotTypeTexture &&
                slot.getTexture().texture )
            {
                mPsoCacheHash = -1;
            }

            slot.slotType = DescriptorSetTexture2::SlotTypeBuffer;
            DescriptorSetTexture2::BufferSlot &bufferSlot = slot.getBuffer();
            bufferSlot = newSlot;
            destroyDescriptorTextures();    //Descriptor is dirty

            //Remove sampler
            if( mSamplerSlots[slotIdx] )
            {
                destroyDescriptorSamplers();    //Sampler descriptors are also dirty

                HlmsManager *hlmsManager = mCreator->getHlmsManager();
                hlmsManager->destroySamplerblock( mSamplerSlots[slotIdx] );
                mSamplerSlots[slotIdx] = 0;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::setTexture( uint8 slotIdx, const DescriptorSetTexture2::TextureSlot &newSlot,
                                     const HlmsSamplerblock *refParams )
    {
        assert( slotIdx < mTexSlots.size() );

        HlmsManager *hlmsManager = mCreator->getHlmsManager();

        DescriptorSetTexture2::Slot &slot = mTexSlots[slotIdx];
        if( slot.slotType != DescriptorSetTexture2::SlotTypeTexture ||
            slot.getTexture() != newSlot )
        {
            slot.slotType = DescriptorSetTexture2::SlotTypeTexture;
            DescriptorSetTexture2::TextureSlot &texSlot = slot.getTexture();

            if( mInformHlmsOfTextureData &&
                texSlot.texture != newSlot.texture &&
                (!texSlot.texture || !newSlot.texture ||
                 !texSlot.texture->hasEquivalentParameters( newSlot.texture )) )
            {
                mPsoCacheHash = -1;
            }

            if( texSlot.texture )
                texSlot.texture->removeListener( this );
            if( newSlot.texture )
                newSlot.texture->addListener( this );

            texSlot = newSlot;
            destroyDescriptorTextures();    //Descriptor is dirty
        }

        //Set explicit sampler, or create a default one if needed.
        if( refParams || (!mSamplerSlots[slotIdx] && newSlot.texture) )
        {
            const HlmsSamplerblock *oldSamplerblock = mSamplerSlots[slotIdx];
            if( refParams )
                mSamplerSlots[slotIdx] = hlmsManager->getSamplerblock( *refParams );
            else
                mSamplerSlots[slotIdx] = hlmsManager->getSamplerblock( HlmsSamplerblock() );

            if( oldSamplerblock != mSamplerSlots[slotIdx] )
                destroyDescriptorSamplers();    //Sampler descriptors are also dirty

            if( oldSamplerblock )
                hlmsManager->destroySamplerblock( oldSamplerblock );
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::setSamplerblock( uint8 slotIdx, const HlmsSamplerblock &refParams )
    {
        assert( slotIdx < mSamplerSlots.size() );

        HlmsManager *hlmsManager = mCreator->getHlmsManager();

        const HlmsSamplerblock *oldSamplerblock = mSamplerSlots[slotIdx];
        mSamplerSlots[slotIdx] = hlmsManager->getSamplerblock( refParams );

        if( oldSamplerblock != mSamplerSlots[slotIdx] )
            destroyDescriptorSamplers();    //Sampler descriptors are dirty

        if( oldSamplerblock )
            hlmsManager->destroySamplerblock( oldSamplerblock );
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::_setSamplerblock( uint8 slotIdx, const HlmsSamplerblock *refParams )
    {
        assert( slotIdx < mSamplerSlots.size() );

        HlmsManager *hlmsManager = mCreator->getHlmsManager();

        const HlmsSamplerblock *oldSamplerblock = mSamplerSlots[slotIdx];
        mSamplerSlots[slotIdx] = refParams;

        if( oldSamplerblock != mSamplerSlots[slotIdx] )
            destroyDescriptorSamplers();    //Sampler descriptors are dirty

        if( oldSamplerblock )
            hlmsManager->destroySamplerblock( oldSamplerblock );
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::_setUavBuffer( uint8 slotIdx, const DescriptorSetUav::BufferSlot &newSlot )
    {
        assert( slotIdx < mUavSlots.size() );

        DescriptorSetUav::Slot &slot = mUavSlots[slotIdx];
        if( slot.slotType != DescriptorSetUav::SlotTypeBuffer ||
            slot.getBuffer() != newSlot )
        {
            if( mInformHlmsOfTextureData &&
                slot.slotType == DescriptorSetUav::SlotTypeTexture &&
                slot.getTexture().texture )
            {
                mPsoCacheHash = -1;
            }

            slot.slotType = DescriptorSetUav::SlotTypeBuffer;
            DescriptorSetUav::BufferSlot &bufferSlot = slot.getBuffer();

            bufferSlot = newSlot;
            destroyDescriptorUavs();    //Descriptor is dirty
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::_setUavTexture( uint8 slotIdx, const DescriptorSetUav::TextureSlot &newSlot )
    {
        assert( slotIdx < mUavSlots.size() );

        DescriptorSetUav::Slot &slot = mUavSlots[slotIdx];
        if( slot.slotType != DescriptorSetUav::SlotTypeTexture ||
            slot.getTexture() != newSlot )
        {
            slot.slotType = DescriptorSetUav::SlotTypeTexture;
            DescriptorSetUav::TextureSlot &texSlot = slot.getTexture();

            if( mInformHlmsOfTextureData &&
                texSlot.texture != newSlot.texture &&
                (!texSlot.texture || !newSlot.texture ||
                 !texSlot.texture->hasEquivalentParameters( newSlot.texture )) )
            {
                mPsoCacheHash = -1;
            }

            if( texSlot.texture )
                texSlot.texture->removeListener( this );
            if( newSlot.texture )
                newSlot.texture->addListener( this );

            texSlot = newSlot;
            destroyDescriptorUavs();    //Descriptor is dirty
        }
    }
    //-----------------------------------------------------------------------------------
    HlmsComputeJob* HlmsComputeJob::clone( const String &cloneName )
    {
        HlmsCompute *compute = static_cast<HlmsCompute*>( mCreator );
        HlmsComputeJob *newJob = compute->createComputeJob( cloneName, cloneName,
                                                            this->mSourceFilename,
                                                            this->mIncludedPieceFiles );

        this->cloneTo( newJob );

        return newJob;
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::cloneTo( HlmsComputeJob *dstJob )
    {
        IdString originalName = dstJob->mName;
        *dstJob = *this;
        dstJob->mName = originalName;

        HlmsManager *hlmsManager = mCreator->getHlmsManager();

        if( mTexturesDescSet )
            dstJob->mTexturesDescSet = hlmsManager->getDescriptorSetTexture2( *this->mTexturesDescSet );
        if( mSamplersDescSet )
            dstJob->mSamplersDescSet = hlmsManager->getDescriptorSetSampler( *this->mSamplersDescSet );
        if( mUavsDescSet )
            dstJob->mUavsDescSet = hlmsManager->getDescriptorSetUav( *this->mUavsDescSet );

        FastArray<const HlmsSamplerblock*>::const_iterator itor = dstJob->mSamplerSlots.begin();
        FastArray<const HlmsSamplerblock*>::const_iterator end  = dstJob->mSamplerSlots.end();

        while( itor != end )
        {
            if( *itor )
                hlmsManager->addReference( *itor );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsComputeJob::notifyTextureChanged( TextureGpu *texture, TextureGpuListener::Reason reason,
                                               void *extraData )
    {
        if( reason == TextureGpuListener::Deleted )
        {
            if( texture->isTexture() )
            {
                DescriptorSetTexSlotArray::const_iterator itor = mTexSlots.begin();
                DescriptorSetTexSlotArray::const_iterator end  = mTexSlots.end();

                while( itor != end )
                {
                    if( itor->isTexture() && itor->getTexture().texture == texture )
                    {
                        DescriptorSetTexture2::TextureSlot emptySlot = itor->getTexture();
                        emptySlot.texture = 0;
                        setTexture( itor - mTexSlots.begin(), emptySlot );
                    }
                    ++itor;
                }
            }

            if( texture->isUav() )
            {
                DescriptorSetUavSlotArray::const_iterator itor = mUavSlots.begin();
                DescriptorSetUavSlotArray::const_iterator end  = mUavSlots.end();

                while( itor != end )
                {
                    if( itor->isTexture() && itor->getTexture().texture == texture )
                    {
                        DescriptorSetUav::TextureSlot emptySlot = itor->getTexture();
                        emptySlot.texture = 0;
                        _setUavTexture( itor - mUavSlots.begin(), emptySlot );
                    }
                    ++itor;
                }
            }
        }

        if( texture->isTexture() )
            destroyDescriptorTextures();
        if( texture->isUav() )
            destroyDescriptorUavs();
    }
}
