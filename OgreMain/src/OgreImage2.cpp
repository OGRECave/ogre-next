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

#include "OgreImage2.h"

#include "OgreAsyncTextureTicket.h"
#include "OgreColourValue.h"
#include "OgreException.h"
#include "OgreImageCodec2.h"
#include "OgreImageDownsampler.h"
#include "OgreImageResampler.h"
#include "OgreMath.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreProfiler.h"
#include "OgreResourceGroupManager.h"
#include "OgreStagingTexture.h"
#include "OgreTextureGpuManager.h"

namespace Ogre
{
    ImageCodec2::~ImageCodec2() {}
    //-----------------------------------------------------------------------------------
    Image2::Image2() :
        mWidth( 0 ),
        mHeight( 0 ),
        mDepthOrSlices( 0 ),
        mNumMipmaps( 1 ),
        mPixelFormat( PFG_UNKNOWN ),
        mBuffer( NULL ),
        mAutoDelete( true )
    {
    }
    //-----------------------------------------------------------------------------------
    Image2::Image2( const Image2 &img ) : mBuffer( NULL ), mAutoDelete( true )
    {
        // call assignment operator
        *this = img;
    }
    //-----------------------------------------------------------------------------------
    Image2::~Image2() { freeMemory(); }
    //-----------------------------------------------------------------------------------
    void Image2::freeMemory()
    {
        OgreProfileExhaustive( "Image2::freeMemory" );

        // Only delete if this was not a dynamic image (meaning app holds & destroys buffer)
        if( mBuffer && mAutoDelete )
        {
            OGRE_FREE_SIMD( mBuffer, MEMCATEGORY_RESOURCE );
            mBuffer = NULL;
        }
    }
    //-----------------------------------------------------------------------------------
    Image2 &Image2::operator=( const Image2 &img )
    {
        OgreProfileExhaustive( "Image2::operator =" );

        freeMemory();
        mWidth = img.mWidth;
        mHeight = img.mHeight;
        mDepthOrSlices = img.mDepthOrSlices;
        mNumMipmaps = img.mNumMipmaps;
        mTextureType = img.mTextureType;
        mPixelFormat = img.mPixelFormat;
        mAutoDelete = img.mAutoDelete;
        // Only create/copy when previous data was not dynamic data
        if( mAutoDelete )
        {
            const uint32 rowAlignment = 4u;
            const size_t totalBytes = PixelFormatGpuUtils::calculateSizeBytes(
                mWidth, mHeight, getDepth(), getNumSlices(), mPixelFormat, mNumMipmaps, rowAlignment );
            mBuffer = OGRE_MALLOC_SIMD( totalBytes, MEMCATEGORY_RESOURCE );
            memcpy( mBuffer, img.mBuffer, totalBytes );
        }
        else
        {
            mBuffer = img.mBuffer;
        }

        return *this;
    }
    //-----------------------------------------------------------------------------------
    void Image2::flipAroundY( uint8 mipLevel )
    {
        TextureBox box = getData( mipLevel );

        switch( box.bytesPerPixel )
        {
        case 1:
            for( size_t z = 0; z < box.getDepthOrSlices(); ++z )
            {
                for( size_t y = 0; y < box.height; ++y )
                {
                    uint8 *src = reinterpret_cast<uint8 *>( box.at( 0, y, z ) );
                    uint8 *dst = reinterpret_cast<uint8 *>( box.at( box.width - 1u, y, z ) );
                    for( size_t x = 0; x < ( box.width >> 1u ); ++x )
                    {
                        std::swap( src, dst );
                        ++src;
                        --dst;
                    }
                }
            }
            break;
        case 2:
            for( size_t z = 0; z < box.getDepthOrSlices(); ++z )
            {
                for( size_t y = 0; y < box.height; ++y )
                {
                    uint16 *src = reinterpret_cast<uint16 *>( box.at( 0, y, z ) );
                    uint16 *dst = reinterpret_cast<uint16 *>( box.at( box.width - 1u, y, z ) );
                    for( size_t x = 0; x < ( box.width >> 1u ); ++x )
                    {
                        std::swap( src, dst );
                        ++src;
                        --dst;
                    }
                }
            }
            break;
        case 4:
            for( size_t z = 0; z < box.getDepthOrSlices(); ++z )
            {
                for( size_t y = 0; y < box.height; ++y )
                {
                    uint32 *src = reinterpret_cast<uint32 *>( box.at( 0, y, z ) );
                    uint32 *dst = reinterpret_cast<uint32 *>( box.at( box.width - 1u, y, z ) );
                    for( size_t x = 0; x < ( box.width >> 1u ); ++x )
                    {
                        std::swap( src, dst );
                        ++src;
                        --dst;
                    }
                }
            }
            break;
        default:
            for( size_t z = 0; z < box.getDepthOrSlices(); ++z )
            {
                for( size_t y = 0; y < box.height; ++y )
                {
                    uint8 *src = reinterpret_cast<uint8 *>( box.at( 0, y, z ) );
                    uint8 *dst = reinterpret_cast<uint8 *>( box.at( box.width - 1u, y, z ) );
                    for( size_t x = 0; x < ( box.width >> 1u ); ++x )
                    {
                        for( size_t i = 0; i < box.bytesPerPixel; ++i )
                        {
                            std::swap( src, dst );
                            ++src;
                            ++dst;
                        }

                        dst -= box.bytesPerPixel << 1u;
                    }
                }
            }
            break;
        }
    }
    //-----------------------------------------------------------------------------------
    void Image2::flipAroundY()
    {
        OgreProfileExhaustive( "Image2::flipAroundY" );

        if( !mBuffer )
        {
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Can not flip an uninitialised texture",
                         "Image2::flipAroundY" );
        }
        if( PixelFormatGpuUtils::isCompressed( mPixelFormat ) )
        {
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Can not flip a compressed texture",
                         "Image2::flipAroundY" );
        }

        for( uint8 i = 0; i < mNumMipmaps; ++i )
            flipAroundY( i );
    }
    //-----------------------------------------------------------------------------------
    void Image2::flipAroundX( uint8 mipLevel, void *pTempBuffer )
    {
        TextureBox box = getData( mipLevel );

        for( size_t z = 0; z < box.getDepthOrSlices(); ++z )
        {
            for( size_t y = 0; y < ( box.height >> 1u ); ++y )
            {
                void *row0 = box.at( 0, y, z );
                void *rowN = box.at( 0, box.height - y - 1u, z );
                memcpy( pTempBuffer, rowN, box.bytesPerRow );
                memcpy( rowN, row0, box.bytesPerRow );
                memcpy( row0, pTempBuffer, box.bytesPerRow );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void Image2::flipAroundX()
    {
        OgreProfileExhaustive( "Image2::flipAroundX" );

        if( !mBuffer )
        {
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Can not flip an uninitialised texture",
                         "Image2::flipAroundX" );
        }
        if( PixelFormatGpuUtils::isCompressed( mPixelFormat ) )
        {
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Can not flip a compressed texture",
                         "Image2::flipAroundX" );
        }

        void *pTempBuffer = OGRE_MALLOC_SIMD( getBytesPerRow( 0 ), MEMCATEGORY_RESOURCE );

        for( uint8 i = 0; i < mNumMipmaps; ++i )
            flipAroundX( i, pTempBuffer );

        OGRE_FREE_SIMD( pTempBuffer, MEMCATEGORY_RESOURCE );
    }
    //-----------------------------------------------------------------------------------
    void Image2::loadDynamicImage( void *pData, uint32 width, uint32 height, uint32 depthOrSlices,
                                   TextureTypes::TextureTypes textureType, PixelFormatGpu format,
                                   bool autoDelete, uint8 numMipmaps )
    {
        OgreProfileExhaustive( "Image2::loadDynamicImage" );

        freeMemory();
        assert( numMipmaps > 0 );
        assert( ( textureType != TextureTypes::TypeCube || depthOrSlices == 6u ) &&
                "depthOrSlices must be 6!" );
        assert( ( textureType != TextureTypes::TypeCubeArray || !( depthOrSlices % 6u ) ) &&
                "depthOrSlices must be multiple of 6!" );
        mWidth = width;
        mHeight = height;
        mDepthOrSlices = depthOrSlices;
        mTextureType = textureType;
        mPixelFormat = format;
        mNumMipmaps = numMipmaps;
        mBuffer = pData;
        mAutoDelete = autoDelete;

        const uint8 maxMipCount = PixelFormatGpuUtils::getMaxMipmapCount( width, height, getDepth() );
        assert( mNumMipmaps <= maxMipCount && "Asking more mipmaps than it is actually possible!" );
        mNumMipmaps = std::min( maxMipCount, mNumMipmaps );
    }
    //-----------------------------------------------------------------------------------
    void Image2::loadDynamicImage( void *pData, bool autoDelete, const TextureGpu *texture )
    {
        loadDynamicImage( pData, texture->getInternalWidth(), texture->getInternalHeight(),
                          texture->getDepthOrSlices(), texture->getTextureType(),
                          texture->getPixelFormat(), autoDelete, texture->getNumMipmaps() );
    }
    //-----------------------------------------------------------------------------------
    void Image2::loadDynamicImage( void *pData, bool autoDelete, const Image2 *image )
    {
        loadDynamicImage( pData, image->getWidth(), image->getHeight(), image->getDepthOrSlices(),
                          image->getTextureType(), image->getPixelFormat(), autoDelete,
                          image->getNumMipmaps() );
    }
    //-----------------------------------------------------------------------------------
    void Image2::createEmptyImage( uint32 width, uint32 height, uint32 depthOrSlices,
                                   TextureTypes::TextureTypes textureType, PixelFormatGpu format,
                                   uint8 numMipmaps )
    {
        mDepthOrSlices = depthOrSlices;
        mTextureType = textureType;

        const uint32 rowAlignment = 4u;
        const size_t totalBytes = PixelFormatGpuUtils::calculateSizeBytes(
            width, height, getDepth(), getNumSlices(), format, numMipmaps, rowAlignment );

        void *pData = OGRE_MALLOC_SIMD( totalBytes, MEMCATEGORY_RESOURCE );
        loadDynamicImage( pData, width, height, depthOrSlices, textureType, format, true, numMipmaps );
    }
    //-----------------------------------------------------------------------------------
    void Image2::createEmptyImageLike( const TextureGpu *texture )
    {
        createEmptyImage( texture->getInternalWidth(), texture->getInternalHeight(),
                          texture->getDepthOrSlices(), texture->getTextureType(),
                          texture->getPixelFormat(), texture->getNumMipmaps() );
    }
    //-----------------------------------------------------------------------------------
    void Image2::convertFromTexture( TextureGpu *texture, uint8 minMip, uint8 maxMip,
                                     bool automaticResolve )
    {
        assert( minMip <= maxMip );

        OgreProfileExhaustive( "Image2::convertFromTexture" );

        if( texture->getResidencyStatus() != GpuResidency::Resident &&
            texture->getNextResidencyStatus() != GpuResidency::Resident )
        {
            OGRE_EXCEPT(
                Exception::ERR_INVALIDPARAMS,
                "Texture '" + texture->getNameStr() + "' must be resident or becoming resident!!!",
                "Image2::convertFromTexture" );
        }

        freeMemory();

        texture->waitForData();

        minMip = std::min<uint8>( minMip, static_cast<uint8>( texture->getNumMipmaps() - 1u ) );
        maxMip = std::min<uint8>( maxMip, static_cast<uint8>( texture->getNumMipmaps() - 1u ) );

        mWidth = std::max( 1u, texture->getInternalWidth() >> minMip );
        mHeight = std::max( 1u, texture->getInternalHeight() >> minMip );
        mDepthOrSlices = std::max( 1u, texture->getDepth() >> minMip );
        mDepthOrSlices = std::max( mDepthOrSlices, texture->getNumSlices() );
        mTextureType = texture->getTextureType();
        mPixelFormat = texture->getPixelFormat();
        mNumMipmaps = maxMip - minMip + 1u;
        mAutoDelete = true;

        const uint32 rowAlignment = 4u;
        const size_t totalBytes = PixelFormatGpuUtils::calculateSizeBytes(
            mWidth, mHeight, getDepth(), getNumSlices(), mPixelFormat, mNumMipmaps, rowAlignment );
        mBuffer = OGRE_MALLOC_SIMD( totalBytes, MEMCATEGORY_RESOURCE );

        TextureGpuManager *textureManager = texture->getTextureManager();

        TextureGpu *resolvedTexture = texture;

        if( texture->isMultisample() && texture->hasMsaaExplicitResolves() && automaticResolve &&
            !texture->isOpenGLRenderWindow() )
        {
            resolvedTexture = textureManager->createTexture(
                texture->getNameStr() + "/Tmp/__ResolveTex", GpuPageOutStrategy::Discard,
                TextureFlags::RenderToTexture, texture->getTextureType() );
            resolvedTexture->copyParametersFrom( texture );
            resolvedTexture->setPixelFormat( texture->getPixelFormat() );
            resolvedTexture->setSampleDescription( 1u );
            resolvedTexture->_transitionTo( GpuResidency::Resident, (uint8 *)0 );
            texture->_resolveTo( resolvedTexture );
        }

        for( uint8 mip = minMip; mip <= maxMip; ++mip )
        {
            const uint32 width = std::max( 1u, texture->getInternalWidth() >> mip );
            const uint32 height = std::max( 1u, texture->getInternalHeight() >> mip );
            const uint32 depth = std::max( 1u, texture->getDepth() >> mip );
            const uint32 depthOrSlices = std::max( depth, texture->getNumSlices() );

            AsyncTextureTicket *asyncTicket = textureManager->createAsyncTextureTicket(
                width, height, depthOrSlices, texture->getTextureType(), texture->getPixelFormat() );
            asyncTicket->download( resolvedTexture, mip, true );

            TextureBox dstBox = this->getData( mip - minMip );

            if( asyncTicket->canMapMoreThanOneSlice() )
            {
                const TextureBox srcBox = asyncTicket->map( 0 );
                dstBox.copyFrom( srcBox );
                asyncTicket->unmap();
            }
            else
            {
                for( size_t i = 0; i < asyncTicket->getNumSlices(); ++i )
                {
                    const TextureBox srcBox = asyncTicket->map( static_cast<uint32>( i ) );
                    dstBox.copyFrom( srcBox );
                    dstBox.data = dstBox.at( 0, 0, 1u );
                    --dstBox.numSlices;
                    asyncTicket->unmap();
                }
            }

            textureManager->destroyAsyncTextureTicket( asyncTicket );
            asyncTicket = 0;
        }

        if( texture != resolvedTexture )
            textureManager->destroyTexture( resolvedTexture );

        if( texture->isOpenGLRenderWindow() )
            flipAroundX();
    }
    //-----------------------------------------------------------------------------------
    void Image2::copyContentsToMemory( TextureGpu *texture, TextureBox srcBox, TextureBox dstBox,
                                       PixelFormatGpu dstFormat, bool automaticResolve )
    {
        if( texture->getResidencyStatus() != GpuResidency::Resident &&
            texture->getNextResidencyStatus() != GpuResidency::Resident )
        {
            OGRE_EXCEPT(
                Exception::ERR_INVALIDPARAMS,
                "Texture '" + texture->getNameStr() + "' must be resident or becoming resident!!!",
                "Image2::copyContentsToMemory" );
        }

        if( !texture->getEmptyBox( 0 ).fullyContains( srcBox ) || !dstBox.equalSize( srcBox ) )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Invalid box.", "Image2::copyContentsToMemory" );
        }

        texture->waitForData();

        TextureGpuManager *textureManager = texture->getTextureManager();

        TextureGpu *resolvedTexture = texture;

        if( texture->isMultisample() && texture->hasMsaaExplicitResolves() && automaticResolve &&
            !texture->isOpenGLRenderWindow() )
        {
            resolvedTexture = textureManager->createTexture(
                texture->getNameStr() + "/Tmp/__ResolveTex", GpuPageOutStrategy::Discard,
                TextureFlags::RenderToTexture, texture->getTextureType() );
            resolvedTexture->copyParametersFrom( texture );
            resolvedTexture->setPixelFormat( texture->getPixelFormat() );
            resolvedTexture->setSampleDescription( 1u );
            resolvedTexture->_transitionTo( GpuResidency::Resident, (uint8 *)0 );
            texture->_resolveTo( resolvedTexture );
        }

        AsyncTextureTicket *asyncTicket = textureManager->createAsyncTextureTicket(
            srcBox.width, srcBox.height, srcBox.getDepthOrSlices(), texture->getTextureType(),
            texture->getPixelFormat() );
        asyncTicket->download( resolvedTexture, 0, true, &srcBox );

        if( asyncTicket->canMapMoreThanOneSlice() )
        {
            srcBox = asyncTicket->map( 0 );
            PixelFormatGpuUtils::bulkPixelConversion( srcBox, texture->getPixelFormat(), dstBox,
                                                      dstFormat );
            asyncTicket->unmap();
        }
        else
        {
            dstBox.numSlices = 1;
            for( size_t i = 0; i < asyncTicket->getNumSlices(); ++i )
            {
                srcBox = asyncTicket->map( static_cast<uint32>( i ) );
                PixelFormatGpuUtils::bulkPixelConversion( srcBox, texture->getPixelFormat(), dstBox,
                                                          dstFormat );
                dstBox.data = dstBox.at( 0, 0, 1u );
                asyncTicket->unmap();
            }
        }

        textureManager->destroyAsyncTextureTicket( asyncTicket );
        asyncTicket = 0;

        if( texture != resolvedTexture )
            textureManager->destroyTexture( resolvedTexture );
    }
    //-----------------------------------------------------------------------------------
    void Image2::uploadTo( TextureGpu *texture, uint8 minMip, uint8 maxMip, uint32 dstZorSliceStart,
                           uint32 srcDepthOrSlices )
    {
        assert( minMip <= maxMip );
        assert( srcDepthOrSlices <= mDepthOrSlices );

        OgreProfileExhaustive( "Image2::uploadTo" );

        if( srcDepthOrSlices == 0u )
            srcDepthOrSlices = mDepthOrSlices;

        if( texture->getInternalWidth() != mWidth || texture->getInternalHeight() != mHeight ||
            srcDepthOrSlices > texture->getDepthOrSlices() ||
            dstZorSliceStart + srcDepthOrSlices > texture->getDepthOrSlices() ||
            PixelFormatGpuUtils::getFamily( texture->getPixelFormat() ) !=
                PixelFormatGpuUtils::getFamily( mPixelFormat ) ||
            texture->getNumMipmaps() <= maxMip )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Texture and Image must have matching resolution and format!",
                         "Image2::uploadTo" );
        }

        TextureGpuManager *textureManager = texture->getTextureManager();

        // We could grab 1 StagingTexture per loop. But if the StagingTexture is huge,
        // we would waste a lot of memory. So instead we try to reuse them for multiple
        // loop iterations. When we run out, we flush its contents and upload all the
        // unflushed data to the GPU; then grab a new StagingTexture and repeat
        TextureBox unsentBoxes[256];
        TextureBox unsentBoxesCpu[256];
        uint8 numUnsentBoxes = 0;
        StagingTexture *stagingTexture = 0;

        for( size_t i = minMip; i <= maxMip; ++i )
        {
            TextureBox box = getData( static_cast<uint8>( i ) );
            box.depth = mTextureType == TextureTypes::Type3D ? srcDepthOrSlices : box.depth;
            box.numSlices = mTextureType != TextureTypes::Type3D ? srcDepthOrSlices : box.numSlices;
            TextureBox dstBox;
            for( size_t tries = 0; tries < 2 && !dstBox.data; ++tries )
            {
                // Grab a staging texture and start mapping it!
                if( !stagingTexture )
                {
                    stagingTexture = textureManager->getStagingTexture(
                        box.width, box.height, box.depth, box.numSlices, texture->getPixelFormat() );
                    stagingTexture->startMapRegion();
                }

                // This stagingTexture may just have been grabbed,
                // or it may have already been during a previous mipmap iteration
                dstBox = stagingTexture->mapRegion( box.width, box.height, box.depth, box.numSlices,
                                                    texture->getPixelFormat() );

                // Failed to map? Then the staging texture is out of space. Flush it.
                // This loop will then retry for a 2nd time and grab a new staging texture.
                if( !dstBox.data )
                {
                    stagingTexture->stopMapRegion();
                    for( size_t j = 0; j < numUnsentBoxes; ++j )
                    {
                        TextureBox texBox =
                            texture->getEmptyBox( static_cast<uint8>( i - numUnsentBoxes + j ) );
                        texBox.z = mTextureType == TextureTypes::Type3D ? dstZorSliceStart : texBox.z;
                        texBox.sliceStart =
                            mTextureType != TextureTypes::Type3D ? dstZorSliceStart : texBox.sliceStart;
                        texBox.depth = unsentBoxes[j].depth;
                        texBox.numSlices = unsentBoxes[j].numSlices;
                        stagingTexture->upload( unsentBoxes[j], texture,
                                                static_cast<uint8>( i - numUnsentBoxes + j ),
                                                &unsentBoxesCpu[j], &texBox );
                    }
                    numUnsentBoxes = 0;
                    textureManager->removeStagingTexture( stagingTexture );
                    stagingTexture = 0;
                }
            }

            unsentBoxesCpu[numUnsentBoxes] = box;
            unsentBoxes[numUnsentBoxes++] = dstBox;
            dstBox.copyFrom( box );
        }

        // Flush what's left
        stagingTexture->stopMapRegion();
        for( size_t j = 0; j < numUnsentBoxes; ++j )
        {
            const uint8 mipLevel = static_cast<uint8>( maxMip + 1u - numUnsentBoxes + j );
            TextureBox texBox = texture->getEmptyBox( mipLevel );
            texBox.z = mTextureType == TextureTypes::Type3D ? dstZorSliceStart : texBox.z;
            texBox.sliceStart =
                mTextureType != TextureTypes::Type3D ? dstZorSliceStart : texBox.sliceStart;
            texBox.depth = unsentBoxes[j].depth;
            texBox.numSlices = unsentBoxes[j].numSlices;
            stagingTexture->upload( unsentBoxes[j], texture, mipLevel, &unsentBoxesCpu[j], &texBox );
        }
        numUnsentBoxes = 0;
        textureManager->removeStagingTexture( stagingTexture );
        stagingTexture = 0;
    }
    //-----------------------------------------------------------------------------------
    void Image2::load( const String &strFileName, const String &group )
    {
        String strExt;

        size_t pos = strFileName.find_last_of( "." );
        if( pos != String::npos && pos < ( strFileName.length() - 1u ) )
            strExt = strFileName.substr( pos + 1u );

        DataStreamPtr encoded = ResourceGroupManager::getSingleton().openResource( strFileName, group );
        load( encoded, strExt );
    }
    //-----------------------------------------------------------------------------------
    void Image2::save( const String &filename, uint8 mipLevel, uint8 numMipmaps )
    {
        OgreProfileExhaustive( "Image2::save" );

        if( !mBuffer )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "No image data loaded", "Image2::save" );
        }
        if( mipLevel + numMipmaps > mNumMipmaps )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Invalid mipmaps specified", "Image2::encode" );
        }

        String strExt;
        size_t pos = filename.find_last_of( "." );
        if( pos == String::npos )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Unable to save image file '" + filename + "' - invalid extension.",
                         "Image2::save" );
        }

        while( pos != filename.length() - 1 )
            strExt += filename[++pos];

        Codec *pCodec = Codec::getCodec( strExt );
        if( !pCodec )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Unable to save image file '" + filename + "' - invalid extension.",
                         "Image2::save" );
        }

        ImageCodec2::ImageData2 *imgData = OGRE_NEW ImageCodec2::ImageData2();
        imgData->box = getData( mipLevel );
        imgData->textureType = mTextureType;
        imgData->format = mPixelFormat;
        imgData->numMipmaps = numMipmaps - mipLevel;
        imgData->freeOnDestruction = false;
        // Wrap in CodecDataPtr, this will delete
        Codec::CodecDataPtr codeDataPtr( imgData );
        MemoryDataStreamPtr emptyDataStream;

        pCodec->encodeToFile( emptyDataStream, filename, codeDataPtr );
    }
    //-----------------------------------------------------------------------------------
    DataStreamPtr Image2::encode( const String &formatextension, uint8 mipLevel, uint8 numMipmaps )
    {
        OgreProfileExhaustive( "Image2::encode" );

        if( !mBuffer )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "No image data loaded", "Image2::encode" );
        }
        if( mipLevel + numMipmaps > mNumMipmaps )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Invalid mipmaps specified", "Image2::encode" );
        }

        Codec *pCodec = Codec::getCodec( formatextension );
        if( !pCodec )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Unable to encode image data as '" + formatextension + "' - invalid extension.",
                         "Image2::encode" );
        }

        ImageCodec2::ImageData2 *imgData = OGRE_NEW ImageCodec2::ImageData2();
        imgData->box = getData( mipLevel );
        imgData->textureType = mTextureType;
        imgData->format = mPixelFormat;
        imgData->numMipmaps = numMipmaps - mipLevel;
        imgData->freeOnDestruction = false;
        // Wrap in CodecDataPtr, this will delete
        Codec::CodecDataPtr codeDataPtr( imgData );
        MemoryDataStreamPtr emptyDataStream;

        return pCodec->encode( emptyDataStream, codeDataPtr );
    }
    //-----------------------------------------------------------------------------------
    void Image2::load( DataStreamPtr &stream, const String &type )
    {
        OgreProfileExhaustive( "Image2::load" );

        freeMemory();

        Codec *pCodec = 0;
        if( !type.empty() )
        {
            // use named codec
            pCodec = Codec::getCodec( type );
        }
        else
        {
            // derive from magic number
            // read the first 128 bytes or file size, if less
            size_t magicLen = std::min( stream->size(), (size_t)128 );
            char magicBuf[128];
            stream->read( magicBuf, magicLen );
            // return to start
            stream->seek( 0 );
            pCodec = Codec::getCodec( magicBuf, magicLen );

            if( !pCodec )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Unable to load image: Image format is unknown. Unable to identify codec. "
                             "Check it or specify format explicitly.",
                             "Image2::load" );
            }
        }

        load( stream, pCodec );
    }
    //-----------------------------------------------------------------------------------
    void Image2::load2( DataStreamPtr &stream, const String &filename )
    {
        OgreProfileExhaustive( "Image2::load2" );

        freeMemory();

        Codec *pCodec = 0;

        // read the first 128 bytes or file size, if less
        const size_t magicLen = std::min( stream->size(), (size_t)128 );
        char magicBuf[128];
        stream->read( magicBuf, magicLen );
        // return to start
        stream->seek( 0 );

        if( !filename.empty() )
        {
            // Check by extension (much faster)
            String strExt;
            size_t pos = filename.find_last_of( "." );
            if( pos != String::npos && pos + 1u < filename.length() )
                strExt = filename.substr( pos + 1u );
            else
                strExt = filename;

            pCodec = Codec::getCodec( strExt );

            if( pCodec )
            {
                if( pCodec->validateMagicNumber( magicBuf, magicLen ) == Codec::CodecInvalid )
                {
                    // Either the file is invalid, or it was mislabeled (e.g. PNG named file.jpg)
                    // We will fallback to finding the format via its magic number
                    pCodec = 0;
                }
            }
        }

        if( !pCodec )
        {
            // Extension not available or was invalid.
            // Derive from magic number (slower)
            pCodec = Codec::getCodec( magicBuf, magicLen );
        }

        if( !pCodec )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Unable to load image: Image format is unknown. Unable to identify codec. "
                         "Check it or specify format explicitly.",
                         "Image2::load" );
        }

        load( stream, pCodec );
    }
    //-----------------------------------------------------------------------------------
    void Image2::load( DataStreamPtr &stream, Codec *pCodec )
    {
        OgreProfileExhaustive( "Image2::load" );

        freeMemory();

        Codec::DecodeResult res = pCodec->decode( stream );

        ImageCodec2::ImageData2 *pData = static_cast<ImageCodec2::ImageData2 *>( res.second.get() );

        mWidth = pData->box.width;
        mHeight = pData->box.height;
        mDepthOrSlices = std::max( pData->box.depth, pData->box.numSlices );
        mNumMipmaps = pData->numMipmaps;
        mTextureType = pData->textureType;
        mPixelFormat = pData->format;

        // Just use internal buffer of returned memory stream
        mBuffer = pData->box.data;
        // Make sure stream does not delete
        pData->freeOnDestruction = false;
        // make sure we delete
        mAutoDelete = true;
    }
    //-----------------------------------------------------------------------------------
    String Image2::getFileExtFromMagic( DataStreamPtr &stream )
    {
        // read the first 32 bytes or file size, if less
        const size_t magicLen = std::min<size_t>( stream->size(), 32u );
        char magicBuf[32];
        stream->read( magicBuf, magicLen );
        // return to start
        stream->seek( 0 );
        Codec *pCodec = Codec::getCodec( magicBuf, magicLen );

        if( pCodec )
            return pCodec->getType();
        else
            return BLANKSTRING;
    }
    //-----------------------------------------------------------------------------------
    ColourValue Image2::getColourAt( size_t x, size_t y, size_t z, uint8 mipLevel ) const
    {
        TextureBox textureBox = getData( mipLevel );
        return textureBox.getColourAt( x, y, z, mPixelFormat );
    }
    //-----------------------------------------------------------------------------------
    void Image2::setColourAt( const ColourValue &cv, size_t x, size_t y, size_t z, uint8 mipLevel )
    {
        TextureBox textureBox = getData( mipLevel );
        textureBox.setColourAt( cv, x, y, z, mPixelFormat );
    }
    //-----------------------------------------------------------------------------------
    TextureBox Image2::getData( uint8 mipLevel ) const
    {
        assert( mipLevel < mNumMipmaps );

        uint32 width = mWidth;
        uint32 height = mHeight;
        uint32 depth = getDepth();
        const uint32 numSlices = getNumSlices();
        void *data = PixelFormatGpuUtils::advancePointerToMip( mBuffer, width, height, depth, numSlices,
                                                               mipLevel, mPixelFormat );
        width = std::max( 1u, width >> mipLevel );
        height = std::max( 1u, height >> mipLevel );
        depth = std::max( 1u, depth >> mipLevel );

        TextureBox retVal( width, height, depth, numSlices,
                           PixelFormatGpuUtils::getBytesPerPixel( mPixelFormat ),
                           getBytesPerRow( mipLevel ), getBytesPerImage( mipLevel ) );
        retVal.data = data;
        if( PixelFormatGpuUtils::isCompressed( mPixelFormat ) )
            retVal.setCompressedPixelFormat( mPixelFormat );
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    uint32 Image2::getWidth() const { return mWidth; }
    //-----------------------------------------------------------------------------------
    uint32 Image2::getHeight() const { return mHeight; }
    //-----------------------------------------------------------------------------------
    uint32 Image2::getDepthOrSlices() const { return mDepthOrSlices; }
    //-----------------------------------------------------------------------------------
    uint32 Image2::getDepth() const
    {
        return ( mTextureType != TextureTypes::Type3D ) ? 1u : mDepthOrSlices;
    }
    //-----------------------------------------------------------------------------------
    uint32 Image2::getNumSlices() const
    {
        return ( mTextureType != TextureTypes::Type3D ) ? mDepthOrSlices : 1u;
    }
    //-----------------------------------------------------------------------------------
    uint8 Image2::getNumMipmaps() const { return mNumMipmaps; }
    //-----------------------------------------------------------------------------------
    TextureTypes::TextureTypes Image2::getTextureType() const { return mTextureType; }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu Image2::getPixelFormat() const { return mPixelFormat; }
    //-----------------------------------------------------------------------------------
    uint32 Image2::getBytesPerRow( uint8 mipLevel ) const
    {
        assert( mipLevel < mNumMipmaps );
        uint32 width = std::max( mWidth >> mipLevel, 1u );
        return (uint32)PixelFormatGpuUtils::getSizeBytes( width, 1u, 1u, 1u, mPixelFormat, 4u );
    }
    //-----------------------------------------------------------------------------------
    size_t Image2::getBytesPerImage( uint8 mipLevel ) const
    {
        assert( mipLevel < mNumMipmaps );
        uint32 width = std::max( mWidth >> mipLevel, 1u );
        uint32 height = std::max( mHeight >> mipLevel, 1u );
        return PixelFormatGpuUtils::getSizeBytes( width, height, 1u, 1u, mPixelFormat, 4u );
    }
    //-----------------------------------------------------------------------------------
    size_t Image2::getSizeBytes() const
    {
        return PixelFormatGpuUtils::calculateSizeBytes( mWidth, mHeight, getDepth(), getNumSlices(),
                                                        mPixelFormat, mNumMipmaps, 4u );
    }
    //-----------------------------------------------------------------------------------
    void Image2::resize( uint32 width, uint32 height, Filter filter )
    {
        // resizing dynamic images is not supported
        assert( mAutoDelete );
        assert( mTextureType == TextureTypes::Type2D && "Texture type not supported" );

        // reassign buffer to temp image, make sure auto-delete is true
        Image2 temp;
        temp.loadDynamicImage( mBuffer, mWidth, mHeight, mDepthOrSlices, mTextureType, mPixelFormat,
                               true, mNumMipmaps );
        // do not delete[] mBuffer!  temp will destroy it

        mNumMipmaps = 1u;  // Loses precomputed mipmaps

        // set new dimensions, allocate new buffer
        mWidth = width;
        mHeight = height;
        const uint32 rowAlignment = 4u;
        const size_t totalBytes = PixelFormatGpuUtils::calculateSizeBytes(
            mWidth, mHeight, getDepth(), getNumSlices(), mPixelFormat, mNumMipmaps, rowAlignment );
        mBuffer = OGRE_MALLOC_SIMD( totalBytes, MEMCATEGORY_RESOURCE );

        // scale the image from temp into our resized buffer
        TextureBox dst = getData( 0 );
        Image2::scale( temp.getData( 0 ), mPixelFormat, dst, mPixelFormat, filter );
    }
    //-----------------------------------------------------------------------------------
    bool Image2::supportsSwMipmaps( PixelFormatGpu format, uint32 depthOrSlices,
                                    TextureTypes::TextureTypes textureType, Filter filter )
    {
        ImageDownsampler2D *downsampler2DFunc = 0;
        ImageDownsampler3D *downsampler3DFunc = 0;
        ImageDownsamplerCube *downsamplerCubeFunc = 0;
        ImageBlur2D *separableBlur2DFunc = 0;

        bool gammaCorrected = PixelFormatGpuUtils::isSRgb( format );

        bool canGenerateMipmaps = getDownsamplerFunctions( format,                         //
                                                           (void **)&downsampler2DFunc,    //
                                                           (void **)&downsampler3DFunc,    //
                                                           (void **)&downsamplerCubeFunc,  //
                                                           (void **)&separableBlur2DFunc,  //
                                                           gammaCorrected,                 //
                                                           depthOrSlices,                  //
                                                           textureType,                    //
                                                           filter );
        return canGenerateMipmaps;
    }
    //-----------------------------------------------------------------------------------
    bool Image2::getDownsamplerFunctions( PixelFormatGpu format,                   //
                                          void **imageDownsampler2D,               //
                                          void **imageDownsampler3D,               //
                                          void **imageDownsamplerCube,             //
                                          void **imageBlur2D,                      //
                                          bool gammaCorrected,                     //
                                          uint32 depthOrSlices,                    //
                                          TextureTypes::TextureTypes textureType,  //
                                          Filter filter )
    {
        bool retVal = true;

        ImageDownsampler2D *downsampler2DFunc = 0;
        ImageDownsampler3D *downsampler3DFunc = 0;
        ImageDownsamplerCube *downsamplerCubeFunc = 0;
        ImageBlur2D *separableBlur2DFunc = 0;

        gammaCorrected |= PixelFormatGpuUtils::isSRgb( format );

        switch( format )
        {
        case PFG_R8_UNORM:
        case PFG_R8_UINT:
            if( !gammaCorrected )
            {
                downsampler2DFunc = downscale2x_X8;
                downsampler3DFunc = downscale3D2x_X8;
                downsamplerCubeFunc = downscale2x_X8_cube;
                separableBlur2DFunc = separableBlur_X8;
            }
            else
            {
                downsampler2DFunc = downscale2x_sRGB_X8;
                downsampler3DFunc = downscale3D2x_sRGB_X8;
                downsamplerCubeFunc = downscale2x_sRGB_X8_cube;
                separableBlur2DFunc = separableBlur_sRGB_X8;
            }
            break;
        case PFG_A8_UNORM:
            if( !gammaCorrected )
            {
                downsampler2DFunc = downscale2x_A8;
                downsampler3DFunc = downscale3D2x_A8;
                downsamplerCubeFunc = downscale2x_A8_cube;
                separableBlur2DFunc = separableBlur_A8;
            }
            else
            {
                downsampler2DFunc = downscale2x_sRGB_A8;
                downsampler3DFunc = downscale3D2x_sRGB_A8;
                downsamplerCubeFunc = downscale2x_sRGB_A8_cube;
                separableBlur2DFunc = separableBlur_sRGB_A8;
            }
            break;
        case PFG_RG8_UNORM:
        case PFG_RG8_UINT:
            if( !gammaCorrected )
            {
                downsampler2DFunc = downscale2x_XX88;
                downsampler3DFunc = downscale3D2x_XX88;
                downsamplerCubeFunc = downscale2x_XX88_cube;
                separableBlur2DFunc = separableBlur_X8;
            }
            else
            {
                downsampler2DFunc = downscale2x_sRGB_XX88;
                downsampler3DFunc = downscale3D2x_sRGB_XX88;
                downsamplerCubeFunc = downscale2x_sRGB_XX88_cube;
                separableBlur2DFunc = separableBlur_sRGB_X8;
            }
            break;
        case PFG_RGBA8_UNORM:
        case PFG_RGBA8_UNORM_SRGB:
        case PFG_RGBA8_UINT:
        case PFG_BGRA8_UNORM:
        case PFG_BGRA8_UNORM_SRGB:
            if( !gammaCorrected )
            {
                downsampler2DFunc = downscale2x_XXXA8888;
                downsampler3DFunc = downscale3D2x_XXXA8888;
                downsamplerCubeFunc = downscale2x_XXXA8888_cube;
                separableBlur2DFunc = separableBlur_XXXA8888;
            }
            else
            {
                downsampler2DFunc = downscale2x_sRGB_XXXA8888;
                downsampler3DFunc = downscale3D2x_sRGB_XXXA8888;
                downsamplerCubeFunc = downscale2x_sRGB_XXXA8888_cube;
                separableBlur2DFunc = separableBlur_sRGB_XXXA8888;
            }
            break;
        case PFG_R8_SNORM:
        case PFG_R8_SINT:
            downsampler2DFunc = downscale2x_Signed_X8;
            downsampler3DFunc = downscale3D2x_Signed_X8;
            downsamplerCubeFunc = downscale2x_Signed_X8_cube;
            separableBlur2DFunc = separableBlur_X8;
            break;
        case PFG_RG8_SNORM:
        case PFG_RG8_SINT:
            downsampler2DFunc = downscale2x_Signed_XX88;
            downsampler3DFunc = downscale3D2x_Signed_XX88;
            downsamplerCubeFunc = downscale2x_Signed_XX88_cube;
            separableBlur2DFunc = separableBlur_Signed_X8;
            break;
        case PFG_RGBA8_SNORM:
        case PFG_RGBA8_SINT:
            downsampler2DFunc = downscale2x_Signed_XXXA8888;
            downsampler3DFunc = downscale3D2x_Signed_XXXA8888;
            downsamplerCubeFunc = downscale2x_Signed_XXXA8888_cube;
            separableBlur2DFunc = separableBlur_Signed_XXXA8888;
            break;
        case PFG_RGBA32_FLOAT:
            downsampler2DFunc = downscale2x_Float32_XXXA;
            downsampler3DFunc = downscale3D2x_Float32_XXXA;
            downsamplerCubeFunc = downscale2x_Float32_XXXA_cube;
            separableBlur2DFunc = separableBlur_Float32_XXXA;
            break;
        case PFG_RGB32_FLOAT:
            downsampler2DFunc = downscale2x_Float32_XXX;
            downsampler3DFunc = downscale3D2x_Float32_XXX;
            downsamplerCubeFunc = downscale2x_Float32_XXX_cube;
            separableBlur2DFunc = separableBlur_Float32_XXX;
            break;
        case PFG_RG32_FLOAT:
            downsampler2DFunc = downscale2x_Float32_XX;
            downsampler3DFunc = downscale3D2x_Float32_XX;
            downsamplerCubeFunc = downscale2x_Float32_XX_cube;
            separableBlur2DFunc = separableBlur_Float32_XX;
            break;
        case PFG_R32_FLOAT:
            downsampler2DFunc = downscale2x_Float32_X;
            downsampler3DFunc = downscale3D2x_Float32_X;
            downsamplerCubeFunc = downscale2x_Float32_X_cube;
            separableBlur2DFunc = separableBlur_Float32_X;
            break;
        default:  // Keep compiler happy
            break;
        }

        *imageDownsampler2D = (void *)downsampler2DFunc;
        *imageDownsampler3D = (void *)downsampler3DFunc;
        *imageDownsamplerCube = (void *)downsamplerCubeFunc;
        *imageBlur2D = (void *)separableBlur2DFunc;

        if( ( depthOrSlices == 1u && !downsampler2DFunc ) ||
            ( textureType == TextureTypes::TypeCube &&
              ( !downsamplerCubeFunc || filter == FILTER_GAUSSIAN_HIGH ) ) ||
            textureType == TextureTypes::TypeCubeArray ||
            ( textureType == TextureTypes::Type3D &&
              ( !downsampler3DFunc || ( filter != FILTER_BILINEAR && filter != FILTER_LINEAR ) ) ) )
        {
            retVal = false;
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    bool Image2::generateMipmaps( bool gammaCorrected, Filter filter )
    {
        OgreProfileExhaustive( "Image2::generateMipmaps" );

        // resizing dynamic images is not supported
        assert( mAutoDelete );
        assert( ( mTextureType == TextureTypes::Type2D || mTextureType == TextureTypes::TypeCube ||
                  mTextureType == TextureTypes::Type3D ) &&
                "Texture type not supported" );

        ImageDownsampler2D *downsampler2DFunc = 0;
        ImageDownsampler3D *downsampler3DFunc = 0;
        ImageDownsamplerCube *downsamplerCubeFunc = 0;
        ImageBlur2D *separableBlur2DFunc = 0;

        gammaCorrected |= PixelFormatGpuUtils::isSRgb( mPixelFormat );

        bool canGenerateMipmaps = getDownsamplerFunctions(
            mPixelFormat, (void **)&downsampler2DFunc, (void **)&downsampler3DFunc,
            (void **)&downsamplerCubeFunc, (void **)&separableBlur2DFunc, gammaCorrected, mDepthOrSlices,
            mTextureType, filter );

        if( !canGenerateMipmaps )
            return false;

        // Allocate new buffer
        uint8 numMipmapsRequired = PixelFormatGpuUtils::getMaxMipmapCount( mWidth, mHeight, getDepth() );
        Image2 tmpImage0;
        uint8 *tmpBuffer1 = 0;
        if( numMipmapsRequired != mNumMipmaps || filter == FILTER_GAUSSIAN_HIGH )
        {
            // reassign 'this' buffer to temp image, make sure auto-delete is true
            // do not delete[] mBuffer!  temp will destroy it
            tmpImage0.loadDynamicImage( mBuffer, mWidth, mHeight, mDepthOrSlices, mTextureType,
                                        mPixelFormat, true, mNumMipmaps );

            const uint32 rowAlignment = 4u;
            const size_t totalBytes = PixelFormatGpuUtils::calculateSizeBytes(
                mWidth, mHeight, getDepth(), getNumSlices(), mPixelFormat, numMipmapsRequired,
                rowAlignment );
            mBuffer = OGRE_MALLOC_SIMD( totalBytes, MEMCATEGORY_RESOURCE );

            TextureBox srcBox = tmpImage0.getData( 0 );
            TextureBox dstBox = this->getData( 0 );
            memcpy( dstBox.data, srcBox.data, srcBox.bytesPerImage * srcBox.getDepthOrSlices() );

            mNumMipmaps = numMipmapsRequired;

            // Free memory now, it's not needed anymore. If we still
            // need it, it will be freed when function returns
            if( filter != FILTER_GAUSSIAN_HIGH )
                tmpImage0.freeMemory();
            else
            {
                tmpBuffer1 = reinterpret_cast<uint8 *>(
                    OGRE_MALLOC_SIMD( getBytesPerImage( 0 ), MEMCATEGORY_RESOURCE ) );
            }
        }

        uint32 dstWidth = mWidth;
        uint32 dstHeight = mHeight;
        uint32 dstDepth = mDepthOrSlices;

        int filterIdx = 1;

        switch( filter )
        {
        case FILTER_NEAREST:
            filterIdx = 0;
            break;
        case FILTER_LINEAR:
        case FILTER_BILINEAR:
            filterIdx = 1;
            break;
        case FILTER_GAUSSIAN:
            filterIdx = 2;
            break;
        case FILTER_GAUSSIAN_HIGH:
            filterIdx = 1;
            break;
        default:  // Keep compiler happy
            break;
        }

        const FilterKernel &chosenFilter = c_filterKernels[filterIdx];

        for( uint8 i = 1u; i < mNumMipmaps; ++i )
        {
            uint32 srcWidth = dstWidth;
            uint32 srcHeight = dstHeight;
            // uint32 srcDepth    = dstDepth;
            dstWidth = std::max<uint32>( 1u, dstWidth >> 1u );
            dstHeight = std::max<uint32>( 1u, dstHeight >> 1u );
            dstDepth = std::max<uint32>( 1u, dstDepth >> 1u );

            TextureBox box0 = this->getData( i - 1u );
            TextureBox box1 = this->getData( i );

            if( mTextureType == TextureTypes::TypeCube )
            {
                uint8 const *upFaces[6];
                for( size_t j = 0; j < 6; ++j )
                    upFaces[j] = reinterpret_cast<uint8 *>( box0.at( 0, 0, j ) );

                for( size_t j = 0; j < 6; ++j )
                {
                    uint8 *downFace = reinterpret_cast<uint8 *>( box1.at( 0, 0, j ) );
                    ( *downsamplerCubeFunc )(
                        reinterpret_cast<uint8 *>( downFace ), upFaces, static_cast<int32>( dstWidth ),
                        static_cast<int32>( dstHeight ), static_cast<int32>( box1.bytesPerRow ),
                        static_cast<int32>( srcWidth ), static_cast<int32>( srcHeight ),
                        static_cast<int32>( box0.bytesPerRow ), chosenFilter.kernel,
                        chosenFilter.kernelStartX, chosenFilter.kernelEndX, chosenFilter.kernelStartY,
                        chosenFilter.kernelEndY, static_cast<uint8>( j ) );
                }
            }
            else if( mTextureType == TextureTypes::Type3D )
            {
                ( *downsampler3DFunc )(
                    reinterpret_cast<uint8 *>( box1.data ), reinterpret_cast<uint8 *>( box0.data ),
                    static_cast<int32>( dstWidth ), static_cast<int32>( dstHeight ),
                    static_cast<int32>( dstDepth ), static_cast<int32>( box1.bytesPerRow ),
                    static_cast<int32>( box1.bytesPerImage ), static_cast<int32>( srcWidth ),
                    static_cast<int32>( srcHeight ), static_cast<int32>( box0.bytesPerRow ),
                    static_cast<int32>( box0.bytesPerImage ) );
            }
            else
            {
                if( filter != FILTER_GAUSSIAN_HIGH )
                {
                    ( *downsampler2DFunc )(
                        reinterpret_cast<uint8 *>( box1.data ), reinterpret_cast<uint8 *>( box0.data ),
                        static_cast<int32>( dstWidth ), static_cast<int32>( dstHeight ),
                        static_cast<int32>( box1.bytesPerRow ), static_cast<int32>( srcWidth ),
                        static_cast<int32>( box0.bytesPerRow ), chosenFilter.kernel,
                        chosenFilter.kernelStartX, chosenFilter.kernelEndX, chosenFilter.kernelStartY,
                        chosenFilter.kernelEndY );
                }
                else
                {
                    // tmpImage0 should contain one or more mips (from mip 0), and tmpBuffer1 should
                    // be large enough to contain mip 0. This assert should never trigger.
                    assert( tmpImage0.getSizeBytes() >= box0.getSizeBytes() );

                    // Copy box0 to tmpImage0
                    memcpy( tmpImage0.mBuffer, box0.data, box0.getSizeBytes() );

                    // The image right now is in both box0 and tmpImage0. We can't touch box0,
                    // So we blur tmpImage0, and use tmpBuffer1 to store intermediate results
                    const FilterSeparableKernel &separableKernel = c_filterSeparableKernels[0];
                    ( *separableBlur2DFunc )(
                        tmpBuffer1, reinterpret_cast<uint8 *>( tmpImage0.mBuffer ),
                        static_cast<int32>( srcWidth ), static_cast<int32>( srcHeight ),
                        static_cast<int32>( box0.bytesPerRow ), separableKernel.kernel,
                        separableKernel.kernelStart, separableKernel.kernelEnd );
                    // Filter again...
                    ( *separableBlur2DFunc )( tmpBuffer1, reinterpret_cast<uint8 *>( tmpImage0.mBuffer ),
                                              static_cast<int32>( srcWidth ),
                                              static_cast<int32>( srcHeight ),  //
                                              static_cast<int32>( box0.bytesPerRow ),
                                              separableKernel.kernel, separableKernel.kernelStart,
                                              separableKernel.kernelEnd );

                    // Now that tmpImage0 is blurred, bilinear downsample its contents into box1.
                    ( *downsampler2DFunc )(
                        reinterpret_cast<uint8 *>( box1.data ),
                        reinterpret_cast<uint8 *>( tmpImage0.mBuffer ), static_cast<int32>( dstWidth ),
                        static_cast<int32>( dstHeight ), static_cast<int32>( box1.bytesPerRow ),
                        static_cast<int32>( srcWidth ), static_cast<int32>( box0.bytesPerRow ),
                        chosenFilter.kernel, chosenFilter.kernelStartX, chosenFilter.kernelEndX,
                        chosenFilter.kernelStartY, chosenFilter.kernelEndY );
                }
            }
        }

        if( tmpBuffer1 )
        {
            OGRE_FREE_SIMD( tmpBuffer1, MEMCATEGORY_RESOURCE );
            tmpBuffer1 = 0;
        }

        return true;
    }
    //-----------------------------------------------------------------------------------
    void Image2::scale( const TextureBox &src, PixelFormatGpu srcFormat, TextureBox &dst,
                        PixelFormatGpu dstFormat, Filter filter )
    {
        assert( PixelFormatGpuUtils::isAccessible( srcFormat ) );
        assert( PixelFormatGpuUtils::isAccessible( dstFormat ) );
        assert( src.numSlices == dst.numSlices );

        // slices should be scaled individually, without interpolation
        // also our LinearResampler_Byte implementation is optimized for depthOrSlices == 1 case
        if( src.numSlices > 1 )
        {
            TextureBox srcSlice = src, dstSlice = dst;
            srcSlice.numSlices = dstSlice.numSlices = 1;
            for( uint32 sliceIdx = 0; sliceIdx < src.numSlices; ++sliceIdx )
            {
                scale( srcSlice, srcFormat, dstSlice, dstFormat );
                ++srcSlice.sliceStart;
                ++dstSlice.sliceStart;
            }
            return;
        }

        MemoryDataStreamPtr buf;  // For auto-delete
        TextureBox temp;
        switch( filter )
        {
        default:
        case FILTER_NEAREST:
            if( srcFormat == dstFormat )
            {
                // No intermediate buffer needed
                temp = dst;
            }
            else
            {
                // Allocate temporary buffer of destination size in source format
                temp = dst;
                temp.bytesPerPixel = PixelFormatGpuUtils::getBytesPerPixel( srcFormat );
                temp.bytesPerRow =
                    (uint32)PixelFormatGpuUtils::getSizeBytes( dst.width, 1u, 1u, 1u, srcFormat );
                temp.bytesPerImage = temp.bytesPerRow * temp.height;
                buf.reset( OGRE_NEW MemoryDataStream( temp.getSizeBytes() ) );
                temp.data = buf->getPtr();
            }
            // super-optimized: no conversion
            switch( PixelFormatGpuUtils::getBytesPerPixel( srcFormat ) )
            {
            case 1:
                NearestResampler<1>::scale( src, temp );
                break;
            case 2:
                NearestResampler<2>::scale( src, temp );
                break;
            case 3:
                NearestResampler<3>::scale( src, temp );
                break;
            case 4:
                NearestResampler<4>::scale( src, temp );
                break;
            case 6:
                NearestResampler<6>::scale( src, temp );
                break;
            case 8:
                NearestResampler<8>::scale( src, temp );
                break;
            case 12:
                NearestResampler<12>::scale( src, temp );
                break;
            case 16:
                NearestResampler<16>::scale( src, temp );
                break;
            default:
                // never reached
                assert( false );
            }
            if( temp.data != dst.data )
            {
                // Blit temp buffer
                PixelFormatGpuUtils::bulkPixelConversion( temp, srcFormat, dst, dstFormat );
            }
            break;

        case FILTER_LINEAR:
        case FILTER_BILINEAR:
            switch( srcFormat )
            {
            case PFG_RGBA8_UNORM:
            case PFG_RGBA8_UINT:
            case PFG_BGRA8_UNORM:
            case PFG_BGRX8_UNORM:
            case PFG_RGB8_UNORM:
            case PFG_BGR8_UNORM:
            case PFG_RG8_UNORM:
            case PFG_RG8_UINT:
            case PFG_R8_UNORM:
            case PFG_R8_UINT:
            case PFG_A8_UNORM:
                if( srcFormat == dstFormat )
                {
                    // No intermediate buffer needed
                    temp = dst;
                }
                else
                {
                    // Allocate temp buffer of destination size in source format
                    temp = dst;
                    temp.bytesPerPixel = PixelFormatGpuUtils::getBytesPerPixel( srcFormat );
                    temp.bytesPerRow =
                        (uint32)PixelFormatGpuUtils::getSizeBytes( dst.width, 1u, 1u, 1u, srcFormat );
                    temp.bytesPerImage = temp.bytesPerRow * temp.height;
                    buf.reset( OGRE_NEW MemoryDataStream( temp.getSizeBytes() ) );
                    temp.data = buf->getPtr();
                }
                // super-optimized: byte-oriented math, no conversion
                switch( PixelFormatGpuUtils::getBytesPerPixel( srcFormat ) )
                {
                case 1:
                    LinearResampler_Byte<1>::scale( src, srcFormat, temp, srcFormat );
                    break;
                case 2:
                    LinearResampler_Byte<2>::scale( src, srcFormat, temp, srcFormat );
                    break;
                case 3:
                    LinearResampler_Byte<3>::scale( src, srcFormat, temp, srcFormat );
                    break;
                case 4:
                    LinearResampler_Byte<4>::scale( src, srcFormat, temp, srcFormat );
                    break;
                default:
                    // never reached
                    assert( false );
                }
                if( temp.data != dst.data )
                {
                    // Blit temp buffer
                    PixelFormatGpuUtils::bulkPixelConversion( temp, srcFormat, dst, dstFormat );
                }
                break;
            case PFG_RGBA32_FLOAT:
            case PFG_RGB32_FLOAT:
            case PFG_RG32_FLOAT:
            case PFG_R32_FLOAT:
                // float32 to float32, avoid unpack/repack overhead
                LinearResampler_Float32::scale( src, srcFormat, dst, dstFormat );
                break;
            default:
                // non-optimized: floating-point math, performs conversion but always works
                LinearResampler::scale( src, srcFormat, dst, dstFormat );
            }
            break;
        }
    }
    //-----------------------------------------------------------------------------------
    void Image2::_setAutoDelete( bool autoDelete ) { mAutoDelete = autoDelete; }
    //-----------------------------------------------------------------------------------
    bool Image2::getAutoDelete() const { return mAutoDelete; }
}  // namespace Ogre
