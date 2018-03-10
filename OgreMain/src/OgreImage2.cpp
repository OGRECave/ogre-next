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
#include "OgreImage2.h"
#include "OgreException.h"
#include "OgreImageCodec2.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreColourValue.h"
#include "OgreMath.h"
#include "OgrePixelBox.h"
#include "OgreImageResampler.h"
#include "OgreImageDownsampler.h"
#include "OgreTextureGpuManager.h"
#include "OgreAsyncTextureTicket.h"
#include "OgreResourceGroupManager.h"

namespace Ogre {
    ImageCodec2::~ImageCodec2() {
    }
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
    Image2::Image2( const Image &img ) :
        mBuffer( NULL ),
        mAutoDelete( true )
    {
        // call assignment operator
        *this = img;
    }
    //-----------------------------------------------------------------------------------
    Image2::~Image2()
    {
        freeMemory();
    }
    //-----------------------------------------------------------------------------------
    void Image2::freeMemory()
    {
        //Only delete if this was not a dynamic image (meaning app holds & destroys buffer)
        if( mBuffer && mAutoDelete )
        {
            OGRE_FREE_SIMD( mBuffer, MEMCATEGORY_RESOURCE );
            mBuffer = NULL;
        }
    }
    //-----------------------------------------------------------------------------------
    Image2& Image2::operator = ( const Image2 &img )
    {
        freeMemory();
        mWidth          = img.mWidth;
        mHeight         = img.mHeight;
        mDepthOrSlices  = img.mDepthOrSlices;
        mNumMipmaps     = img.mNumMipmaps;
        mTextureType    = img.mTextureType;
        mPixelFormat    = img.mPixelFormat;
        mAutoDelete     = img.mAutoDelete;
        //Only create/copy when previous data was not dynamic data
        if( mAutoDelete )
        {
            const uint32 rowAlignment = 4u;
            const size_t totalBytes = PixelFormatGpuUtils::calculateSizeBytes( mWidth, mHeight,
                                                                               getDepth(),
                                                                               getNumSlices(),
                                                                               mPixelFormat, mNumMipmaps,
                                                                               rowAlignment );
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
            for( size_t z=0; z<box.getDepthOrSlices(); ++z )
            {
                for( size_t y=0; y<box.height; ++y )
                {
                    uint8 *src = reinterpret_cast<uint8*>( box.at( 0, y, z ) );
                    uint8 *dst = reinterpret_cast<uint8*>( box.at( box.width - 1u, y, z ) );
                    for( size_t x=0; x<(box.width >> 1u); ++x )
                    {
                        std::swap( src, dst );
                        ++src;
                        --dst;
                    }
                }
            }
            break;
        case 2:
            for( size_t z=0; z<box.getDepthOrSlices(); ++z )
            {
                for( size_t y=0; y<box.height; ++y )
                {
                    uint16 *src = reinterpret_cast<uint16*>( box.at( 0, y, z ) );
                    uint16 *dst = reinterpret_cast<uint16*>( box.at( box.width - 1u, y, z ) );
                    for( size_t x=0; x<(box.width >> 1u); ++x )
                    {
                        std::swap( src, dst );
                        ++src;
                        --dst;
                    }
                }
            }
            break;
        case 4:
            for( size_t z=0; z<box.getDepthOrSlices(); ++z )
            {
                for( size_t y=0; y<box.height; ++y )
                {
                    uint32 *src = reinterpret_cast<uint32*>( box.at( 0, y, z ) );
                    uint32 *dst = reinterpret_cast<uint32*>( box.at( box.width - 1u, y, z ) );
                    for( size_t x=0; x<(box.width >> 1u); ++x )
                    {
                        std::swap( src, dst );
                        ++src;
                        --dst;
                    }
                }
            }
            break;
        default:
            for( size_t z=0; z<box.getDepthOrSlices(); ++z )
            {
                for( size_t y=0; y<box.height; ++y )
                {
                    uint8 *src = reinterpret_cast<uint8*>( box.at( 0, y, z ) );
                    uint8 *dst = reinterpret_cast<uint8*>( box.at( box.width - 1u, y, z ) );
                    for( size_t x=0; x<(box.width >> 1u); ++x )
                    {
                        for( size_t i=0; i<box.bytesPerPixel; ++i )
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
    void Image2::flipAroundY(void)
    {
        if( !mBuffer )
        {
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR,
                         "Can not flip an uninitialised texture",
                         "Image2::flipAroundY" );
        }
        if( PixelFormatGpuUtils::isCompressed( mPixelFormat ) )
        {
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR,
                         "Can not flip a compressed texture",
                         "Image2::flipAroundY" );
        }

        for( uint8 i=0; i<mNumMipmaps; ++i )
            flipAroundY( i );
    }
    //-----------------------------------------------------------------------------------
    void Image2::flipAroundX( uint8 mipLevel, void *pTempBuffer )
    {
        TextureBox box = getData( mipLevel );

        for( size_t z=0; z<box.getDepthOrSlices(); ++z )
        {
            for( size_t y=0; y<(box.height >> 1u); ++y )
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
    void Image2::flipAroundX(void)
    {
        if( !mBuffer )
        {
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR,
                         "Can not flip an uninitialised texture",
                         "Image2::flipAroundX" );
        }
        if( PixelFormatGpuUtils::isCompressed( mPixelFormat ) )
        {
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR,
                         "Can not flip a compressed texture",
                         "Image2::flipAroundX" );
        }

        void *pTempBuffer = OGRE_MALLOC_SIMD( getBytesPerRow( 0 ), MEMCATEGORY_RESOURCE );

        for( uint8 i=0; i<mNumMipmaps; ++i )
            flipAroundX( i, pTempBuffer );

        OGRE_FREE_SIMD( pTempBuffer, MEMCATEGORY_RESOURCE );
    }
    //-----------------------------------------------------------------------------------
    void Image2::loadDynamicImage( void *pData, uint32 width, uint32 height, uint32 depthOrSlices,
                                   TextureTypes::TextureTypes textureType, PixelFormatGpu format,
                                   bool autoDelete, uint8 numMipmaps )
    {

        freeMemory();
        assert( numMipmaps > 0 );
        assert( (textureType != TextureTypes::TypeCube || depthOrSlices == 6u) &&
                "depthOrSlices must be 6!" );
        assert( (textureType != TextureTypes::TypeCubeArray || !(depthOrSlices % 6u)) &&
                "depthOrSlices must be multiple of 6!" );
        mWidth          = width;
        mHeight         = height;
        mDepthOrSlices  = depthOrSlices;
        mTextureType    = textureType;
        mPixelFormat    = format;
        mNumMipmaps     = numMipmaps;
        mBuffer         = pData;
        mAutoDelete     = autoDelete;

        const uint8 maxMipCount = PixelFormatGpuUtils::getMaxMipmapCount( width, height, getDepth() );
        assert( mNumMipmaps <= maxMipCount && "Asking more mipmaps than it is actually possible!" );
        mNumMipmaps = std::min( maxMipCount, mNumMipmaps );
    }
    //-----------------------------------------------------------------------------------
    void Image2::convertFromTexture( TextureGpu *texture, uint8 minMip, uint8 maxMip,
                                     bool automaticResolve )
    {
        assert( minMip <= maxMip );

        if( texture->getResidencyStatus() != GpuResidency::Resident &&
            texture->getNextResidencyStatus() != GpuResidency::Resident )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Texture '" + texture->getNameStr() +
                         "' must be resident or becoming resident!!!",
                         "Image2::convertFromTexture" );
        }

        freeMemory();

        texture->waitForData();

        minMip = std::min<uint8>( minMip, texture->getNumMipmaps() - 1u );
        maxMip = std::min<uint8>( maxMip, texture->getNumMipmaps() - 1u );

        mWidth          = std::max( 1u, texture->getWidth() >> minMip );
        mHeight         = std::max( 1u, texture->getHeight() >> minMip );
        mDepthOrSlices  = std::max( 1u, texture->getDepth() >> minMip );
        mDepthOrSlices  = std::max( mDepthOrSlices, texture->getNumSlices() );
        mTextureType    = texture->getTextureType();
        mPixelFormat    = texture->getPixelFormat();
        mNumMipmaps     = maxMip - minMip + 1u;
        mAutoDelete     = true;

        const uint32 rowAlignment = 4u;
        const size_t totalBytes = PixelFormatGpuUtils::calculateSizeBytes( mWidth, mHeight,
                                                                           getDepth(),
                                                                           getNumSlices(),
                                                                           mPixelFormat, mNumMipmaps,
                                                                           rowAlignment );
        mBuffer = OGRE_MALLOC_SIMD( totalBytes, MEMCATEGORY_RESOURCE );

        TextureGpuManager *textureManager = texture->getTextureManager();

        TextureGpu *resolvedTexture = texture;

        if( texture->getMsaa() > 1u && texture->hasMsaaExplicitResolves() && automaticResolve &&
            !texture->isOpenGLRenderWindow() )
        {
            resolvedTexture = textureManager->createTexture( texture->getNameStr() + "/Tmp/__ResolveTex",
                                                             GpuPageOutStrategy::Discard,
                                                             TextureFlags::RenderToTexture,
                                                             texture->getTextureType() );
            resolvedTexture->copyParametersFrom( texture );
            resolvedTexture->setPixelFormat( texture->getInternalPixelFormat() );
            resolvedTexture->setMsaa( 1u );
            resolvedTexture->_transitionTo( GpuResidency::Resident, (uint8*)0 );
            texture->_resolveTo( resolvedTexture );
        }

        for( uint8 mip=minMip; mip<=maxMip; ++mip )
        {
            const uint32 width  = std::max( 1u, texture->getWidth() >> mip );
            const uint32 height = std::max( 1u, texture->getHeight() >> mip );
            const uint32 depth  = std::max( 1u, texture->getDepth() >> mip );
            const uint32 depthOrSlices = std::max( depth, texture->getNumSlices() );

            AsyncTextureTicket *asyncTicket =
                    textureManager->createAsyncTextureTicket( width, height, depthOrSlices,
                                                              texture->getTextureType(),
                                                              texture->getPixelFormat() );
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
                for( size_t i=0; i<asyncTicket->getNumSlices(); ++i )
                {
                    const TextureBox srcBox = asyncTicket->map( i );
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
    void Image2::load( const String& strFileName, const String& group )
    {
        String strExt;

        size_t pos = strFileName.find_last_of( "." );
        if( pos != String::npos && pos < (strFileName.length() - 1u) )
            strExt = strFileName.substr( pos + 1u );

        DataStreamPtr encoded = ResourceGroupManager::getSingleton().openResource( strFileName, group );
        load( encoded, strExt );
    }
    //-----------------------------------------------------------------------------------
    void Image2::save( const String& filename, uint8 mipLevel, uint8 numMipmaps )
    {
        if( !mBuffer )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "No image data loaded",
                         "Image2::save" );
        }
        if( mipLevel + numMipmaps > mNumMipmaps )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Invalid mipmaps specified",
                         "Image2::encode" );
        }

        String strExt;
        size_t pos = filename.find_last_of(".");
        if( pos == String::npos )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Unable to save image file '" + filename + "' - invalid extension.",
                         "Image2::save" );
        }

        while( pos != filename.length() - 1 )
            strExt += filename[++pos];

        Codec * pCodec = Codec::getCodec(strExt);
        if( !pCodec )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Unable to save image file '" + filename + "' - invalid extension.",
                         "Image2::save" );
        }

        ImageCodec2::ImageData2* imgData = OGRE_NEW ImageCodec2::ImageData2();
        imgData->box            = getData( mipLevel );
        imgData->textureType    = mTextureType;
        imgData->format         = mPixelFormat;
        imgData->numMipmaps     = numMipmaps - mipLevel;
        imgData->freeOnDestruction = false;
        // Wrap in CodecDataPtr, this will delete
        Codec::CodecDataPtr codeDataPtr(imgData);
        MemoryDataStreamPtr emptyDataStream;

        pCodec->encodeToFile( emptyDataStream, filename, codeDataPtr );
    }
    //-----------------------------------------------------------------------------------
    DataStreamPtr Image2::encode( const String& formatextension,
                                  uint8 mipLevel, uint8 numMipmaps )
    {
        if( !mBuffer )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "No image data loaded",
                         "Image2::encode" );
        }
        if( mipLevel + numMipmaps >= mNumMipmaps )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Invalid mipmaps specified",
                         "Image2::encode" );
        }

        Codec * pCodec = Codec::getCodec(formatextension);
        if( !pCodec )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Unable to encode image data as '" + formatextension + "' - invalid extension.",
                         "Image2::encode" );
        }

        ImageCodec2::ImageData2* imgData = OGRE_NEW ImageCodec2::ImageData2();
        imgData->box            = getData( mipLevel );
        imgData->textureType    = mTextureType;
        imgData->format         = mPixelFormat;
        imgData->numMipmaps     = numMipmaps - mipLevel;
        imgData->freeOnDestruction = false;
        // Wrap in CodecDataPtr, this will delete
        Codec::CodecDataPtr codeDataPtr(imgData);
        MemoryDataStreamPtr emptyDataStream;

        return pCodec->encode( emptyDataStream, codeDataPtr );
    }
    //-----------------------------------------------------------------------------------
    void Image2::load( DataStreamPtr& stream, const String& type )
    {
        freeMemory();

        Codec *pCodec = 0;
        if( !type.empty() )
        {
            // use named codec
            pCodec = Codec::getCodec(type);
        }
        else
        {
            // derive from magic number
            // read the first 32 bytes or file size, if less
            size_t magicLen = std::min(stream->size(), (size_t)32);
            char magicBuf[32];
            stream->read(magicBuf, magicLen);
            // return to start
            stream->seek(0);
            pCodec = Codec::getCodec(magicBuf, magicLen);

            if( !pCodec )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Unable to load image: Image format is unknown. Unable to identify codec. "
                             "Check it or specify format explicitly.",
                             "Image2::load" );
            }
        }

        Codec::DecodeResult res = pCodec->decode(stream);

        ImageCodec2::ImageData2* pData = static_cast<ImageCodec2::ImageData2*>(res.second.getPointer());

        mWidth          = pData->box.width;
        mHeight         = pData->box.height;
        mDepthOrSlices  = std::max( pData->box.depth, pData->box.numSlices );
        mNumMipmaps     = pData->numMipmaps;
        mTextureType    = pData->textureType;
        mPixelFormat    = pData->format;

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
        stream->seek(0);
        Codec* pCodec = Codec::getCodec( magicBuf, magicLen );

        if( pCodec )
            return pCodec->getType();
        else
            return BLANKSTRING;
    }
    //-----------------------------------------------------------------------------------
    TextureBox Image2::getData( uint8 mipLevel ) const
    {
        assert( mipLevel < mNumMipmaps );

        uint32 width            = mWidth;
        uint32 height           = mHeight;
        uint32 depth            = getDepth();
        const uint32 numSlices  = getNumSlices();
        void *data = PixelFormatGpuUtils::advancePointerToMip( mBuffer, width, height, depth,
                                                               numSlices, mipLevel, mPixelFormat );
        width   = std::max( 1u, width  >> mipLevel );
        height  = std::max( 1u, height >> mipLevel );
        depth   = std::max( 1u, depth  >> mipLevel );

        TextureBox retVal( width, height, depth, numSlices,
                           PixelFormatGpuUtils::getBytesPerPixel( mPixelFormat ),
                           getBytesPerRow( mipLevel ),
                           getBytesPerImage( mipLevel ) );
        retVal.data = data;
        if( PixelFormatGpuUtils::isCompressed( mPixelFormat ) )
            retVal.setCompressedPixelFormat( mPixelFormat );
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    uint32 Image2::getWidth(void) const
    {
        return mWidth;
    }
    //-----------------------------------------------------------------------------------
    uint32 Image2::getHeight(void) const
    {
        return mHeight;
    }
    //-----------------------------------------------------------------------------------
    uint32 Image2::getDepthOrSlices(void) const
    {
        return mDepthOrSlices;
    }
    //-----------------------------------------------------------------------------------
    uint32 Image2::getDepth(void) const
    {
        return (mTextureType != TextureTypes::Type3D) ? 1u : mDepthOrSlices;
    }
    //-----------------------------------------------------------------------------------
    uint32 Image2::getNumSlices(void) const
    {
        return (mTextureType != TextureTypes::Type3D) ? mDepthOrSlices : 1u;
    }
    //-----------------------------------------------------------------------------------
    uint8 Image2::getNumMipmaps(void) const
    {
        return mNumMipmaps;
    }
    //-----------------------------------------------------------------------------------
    TextureTypes::TextureTypes Image2::getTextureType(void) const
    {
        return mTextureType;
    }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu Image2::getPixelFormat(void) const
    {
        return mPixelFormat;
    }
    //-----------------------------------------------------------------------------------
    size_t Image2::getBytesPerRow( uint8 mipLevel ) const
    {
        assert( mipLevel < mNumMipmaps );
        uint32 width = std::max( mWidth >> mipLevel, 1u );
        return PixelFormatGpuUtils::getSizeBytes( width, 1u, 1u, 1u, mPixelFormat, 4u );
    }
    //-----------------------------------------------------------------------------------
    size_t Image2::getBytesPerImage( uint8 mipLevel ) const
    {
        assert( mipLevel < mNumMipmaps );
        uint32 width  = std::max( mWidth >> mipLevel, 1u );
        uint32 height = std::max( mHeight >> mipLevel, 1u );
        return PixelFormatGpuUtils::getSizeBytes( width, height, 1u, 1u, mPixelFormat, 4u );
    }
    //-----------------------------------------------------------------------------------
    size_t Image2::getSizeBytes(void) const
    {
        return PixelFormatGpuUtils::calculateSizeBytes( mWidth, mHeight, getDepth(), getNumSlices(),
                                                        mPixelFormat, mNumMipmaps, 4u );
    }
#if 0
    //-----------------------------------------------------------------------------------
    void Image2::resize( uint32 width, uint32 height, Filter filter )
    {
        // resizing dynamic images is not supported
        assert( mAutoDelete );
        assert( mTextureType == TextureTypes::Type2D && "Texture type not supported" );

        // reassign buffer to temp image, make sure auto-delete is true
        Image2 temp;
        temp.loadDynamicImage( mBuffer, mWidth, mHeight, mDepthOrSlices,
                               mTextureType, mPixelFormat, true, mNumMipmaps );
        // do not delete[] mBuffer!  temp will destroy it

        mNumMipmaps = 1u; // Loses precomputed mipmaps

        // set new dimensions, allocate new buffer
        mWidth = width;
        mHeight = height;
        const uint32 rowAlignment = 4u;
        const size_t totalBytes = PixelFormatGpuUtils::calculateSizeBytes( mWidth, mHeight,
                                                                           getDepth(),
                                                                           getNumSlices(),
                                                                           mPixelFormat, mNumMipmaps,
                                                                           rowAlignment );
        mBuffer = OGRE_MALLOC_SIMD( totalBytes, MEMCATEGORY_RESOURCE );

        // scale the image from temp into our resized buffer
        Image2::scale( temp.getPixelBox(), getPixelBox(), filter );
    }
#endif
    //-----------------------------------------------------------------------------------
    bool Image2::generateMipmaps( bool gammaCorrected, Filter filter )
    {
        // resizing dynamic images is not supported
        assert( mAutoDelete );
        assert( (mTextureType == TextureTypes::Type2D ||
                mTextureType == TextureTypes::TypeCube) && "Texture type not supported" );

        ImageDownsampler2D *downsampler2DFunc       = 0;
        ImageDownsamplerCube *downsamplerCubeFunc   = 0;

        gammaCorrected |= PixelFormatGpuUtils::isSRgb( mPixelFormat );

        switch( mPixelFormat )
        {
        case PFG_R8_UNORM:
        case PFG_R8_UINT:
            if( !gammaCorrected )
            {
                downsampler2DFunc   = downscale2x_X8;
                downsamplerCubeFunc = downscale2x_X8_cube;
            }
            else
            {
                downsampler2DFunc   = downscale2x_sRGB_X8;
                downsamplerCubeFunc = downscale2x_sRGB_X8_cube;
            }
            break;
        case PFG_A8_UNORM:
            if( !gammaCorrected )
            {
                downsampler2DFunc   = downscale2x_A8;
                downsamplerCubeFunc = downscale2x_A8_cube;
            }
            else
            {
                downsampler2DFunc   = downscale2x_sRGB_A8;
                downsamplerCubeFunc = downscale2x_sRGB_A8_cube;
            }
            break;
        case PFG_RG8_UNORM:
        case PFG_RG8_UINT:
            if( !gammaCorrected )
            {
                downsampler2DFunc   = downscale2x_XX88;
                downsamplerCubeFunc = downscale2x_XX88_cube;
            }
            else
            {
                downsampler2DFunc   = downscale2x_sRGB_XX88;
                downsamplerCubeFunc = downscale2x_sRGB_XX88_cube;
            }
            break;
        case PFG_RGBA8_UNORM:
        case PFG_RGBA8_UNORM_SRGB:
        case PFG_RGBA8_UINT:
        case PFG_BGRA8_UNORM:
        case PFG_BGRA8_UNORM_SRGB:
            if( !gammaCorrected )
            {
                downsampler2DFunc   = downscale2x_XXXA8888;
                downsamplerCubeFunc = downscale2x_XXXA8888_cube;
            }
            else
            {
                downsampler2DFunc   = downscale2x_sRGB_XXXA8888;
                downsamplerCubeFunc = downscale2x_sRGB_XXXA8888_cube;
            }
            break;
        case PFG_R8_SNORM:
        case PFG_R8_SINT:
            downsampler2DFunc   = downscale2x_Signed_X8;
            downsamplerCubeFunc = downscale2x_Signed_X8_cube;
            break;
        case PFG_RG8_SNORM: case PFG_RG8_SINT:
            downsampler2DFunc   = downscale2x_Signed_XX88;
            downsamplerCubeFunc = downscale2x_Signed_XX88_cube;
            break;
        case PFG_RGBA8_SNORM: case PFG_RGBA8_SINT:
            downsampler2DFunc   = downscale2x_Signed_XXXA8888;
            downsamplerCubeFunc = downscale2x_Signed_XXXA8888_cube;
            break;
        case PFG_RGBA32_FLOAT:
            downsampler2DFunc   = downscale2x_Float32_XXXA;
            downsamplerCubeFunc = downscale2x_Float32_XXXA_cube;
            break;
        case PFG_RGB32_FLOAT:
            downsampler2DFunc   = downscale2x_Float32_XXX;
            downsamplerCubeFunc = downscale2x_Float32_XXX_cube;
            break;
        case PFG_RG32_FLOAT:
            downsampler2DFunc   = downscale2x_Float32_XX;
            downsamplerCubeFunc = downscale2x_Float32_XX_cube;
            break;
        case PFG_R32_FLOAT:
            downsampler2DFunc   = downscale2x_Float32_X;
            downsamplerCubeFunc = downscale2x_Float32_X_cube;
            break;
        default: //Keep compiler happy
            break;
        }

        if( (mDepthOrSlices == 1u && !downsampler2DFunc) ||
            (mTextureType == TextureTypes::TypeCube && !downsamplerCubeFunc) )
        {
            return false;
        }

        // Allocate new buffer
        uint8 numMipmapsRequired = PixelFormatGpuUtils::getMaxMipmapCount( mWidth, mHeight, getDepth() );
        if( numMipmapsRequired != mNumMipmaps )
        {
            // reassign 'this' buffer to temp image, make sure auto-delete is true
            // do not delete[] mBuffer!  temp will destroy it
            Image2 temp;
            temp.loadDynamicImage( mBuffer, mWidth, mHeight, mDepthOrSlices,
                                   mTextureType, mPixelFormat, true, mNumMipmaps );

            const uint32 rowAlignment = 4u;
            const size_t totalBytes = PixelFormatGpuUtils::calculateSizeBytes( mWidth, mHeight,
                                                                               getDepth(),
                                                                               getNumSlices(),
                                                                               mPixelFormat,
                                                                               numMipmapsRequired,
                                                                               rowAlignment );
            mBuffer = OGRE_MALLOC_SIMD( totalBytes, MEMCATEGORY_RESOURCE );

            TextureBox srcBox = temp.getData( 0 );
            TextureBox dstBox = this->getData( 0 );
            memcpy( dstBox.data, srcBox.data, srcBox.bytesPerImage * srcBox.numSlices );

            mNumMipmaps = numMipmapsRequired;
        }

        uint32 dstWidth  = mWidth;
        uint32 dstHeight = mHeight;

        int filterIdx = 1;

        switch( filter )
        {
        case FILTER_NEAREST:
            filterIdx = 0; break;
        case FILTER_LINEAR:
        case FILTER_BILINEAR:
            filterIdx = 1; break;
        case FILTER_GAUSSIAN:
            filterIdx = 2; break;
        default: // Keep compiler happy
            break;
        }

        const FilterKernel &chosenFilter = c_filterKernels[filterIdx];

        for( uint8 i=1u; i<mNumMipmaps; ++i )
        {
            uint32 srcWidth    = dstWidth;
            uint32 srcHeight   = dstHeight;
            dstWidth   = std::max<uint32>( 1u, dstWidth >> 1u );
            dstHeight  = std::max<uint32>( 1u, dstHeight >> 1u );

            TextureBox box0 = this->getData( i - 1u );
            TextureBox box1 = this->getData( i );

            if( mTextureType == TextureTypes::TypeCube )
            {
                uint8 const *upFaces[6];
                for( size_t j=0; j<6; ++j )
                    upFaces[j] = reinterpret_cast<uint8*>( box0.at( 0, 0, j ) );

                for( size_t j=0; j<6; ++j )
                {
                    uint8 *downFace = reinterpret_cast<uint8*>( box1.at( 0, 0, j ) );
                    (*downsamplerCubeFunc)( reinterpret_cast<uint8*>( downFace ), upFaces,
                                            dstWidth, dstHeight, box1.bytesPerRow,
                                            srcWidth, srcHeight, box0.bytesPerRow,
                                            chosenFilter.kernel,
                                            chosenFilter.kernelStartX, chosenFilter.kernelEndX,
                                            chosenFilter.kernelStartY, chosenFilter.kernelEndY,
                                            j );
                }
            }
            else
            {
                (*downsampler2DFunc)( reinterpret_cast<uint8*>( box1.data ),
                                      reinterpret_cast<uint8*>( box0.data ),
                                      dstWidth, dstHeight, box1.bytesPerRow,
                                      srcWidth, box0.bytesPerRow,
                                      chosenFilter.kernel,
                                      chosenFilter.kernelStartX, chosenFilter.kernelEndX,
                                      chosenFilter.kernelStartY, chosenFilter.kernelEndY );
            }
        }

        return true;
    }
    //-----------------------------------------------------------------------------------
#if 0
    void Image2::scale(const PixelBox &src, const PixelBox &scaled, Filter filter)
    {
        assert(PixelUtil::isAccessible(src.format));
        assert(PixelUtil::isAccessible(scaled.format));
        MemoryDataStreamPtr buf; // For auto-delete
        PixelBox temp;
        switch (filter)
        {
        default:
        case FILTER_NEAREST:
            if(src.format == scaled.format)
            {
                // No intermediate buffer needed
                temp = scaled;
            }
            else
            {
                // Allocate temporary buffer of destination size in source format
                temp = PixelBox(scaled.getWidth(), scaled.getHeight(), scaled.getDepth(), src.format);
                buf.bind(OGRE_NEW MemoryDataStream(temp.getConsecutiveSize()));
                temp.data = buf->getPtr();
            }
            // super-optimized: no conversion
            switch (PixelUtil::getNumElemBytes(src.format))
            {
            case 1: NearestResampler<1>::scale(src, temp); break;
            case 2: NearestResampler<2>::scale(src, temp); break;
            case 3: NearestResampler<3>::scale(src, temp); break;
            case 4: NearestResampler<4>::scale(src, temp); break;
            case 6: NearestResampler<6>::scale(src, temp); break;
            case 8: NearestResampler<8>::scale(src, temp); break;
            case 12: NearestResampler<12>::scale(src, temp); break;
            case 16: NearestResampler<16>::scale(src, temp); break;
            default:
                // never reached
                assert(false);
            }
            if(temp.data != scaled.data)
            {
                // Blit temp buffer
                PixelUtil::bulkPixelConversion(temp, scaled);
            }
            break;

        case FILTER_LINEAR:
        case FILTER_BILINEAR:
            switch (src.format)
            {
            case PF_L8: case PF_A8: case PF_BYTE_LA:
            case PF_R8G8B8: case PF_B8G8R8:
            case PF_R8G8B8A8: case PF_B8G8R8A8:
            case PF_A8B8G8R8: case PF_A8R8G8B8:
            case PF_X8B8G8R8: case PF_X8R8G8B8:
                if(src.format == scaled.format)
                {
                    // No intermediate buffer needed
                    temp = scaled;
                }
                else
                {
                    // Allocate temp buffer of destination size in source format
                    temp = PixelBox(scaled.getWidth(), scaled.getHeight(), scaled.getDepth(), src.format);
                    buf.bind(OGRE_NEW MemoryDataStream(temp.getConsecutiveSize()));
                    temp.data = buf->getPtr();
                }
                // super-optimized: byte-oriented math, no conversion
                switch (PixelUtil::getNumElemBytes(src.format))
                {
                case 1: LinearResampler_Byte<1>::scale(src, temp); break;
                case 2: LinearResampler_Byte<2>::scale(src, temp); break;
                case 3: LinearResampler_Byte<3>::scale(src, temp); break;
                case 4: LinearResampler_Byte<4>::scale(src, temp); break;
                default:
                    // never reached
                    assert(false);
                }
                if(temp.data != scaled.data)
                {
                    // Blit temp buffer
                    PixelUtil::bulkPixelConversion(temp, scaled);
                }
                break;
            case PF_FLOAT32_RGB:
            case PF_FLOAT32_RGBA:
                if (scaled.format == PF_FLOAT32_RGB || scaled.format == PF_FLOAT32_RGBA)
                {
                    // float32 to float32, avoid unpack/repack overhead
                    LinearResampler_Float32::scale(src, scaled);
                    break;
                }
                // else, fall through
            default:
                // non-optimized: floating-point math, performs conversion but always works
                LinearResampler::scale(src, scaled);
            }
            break;
        }
    }
#endif
    //-----------------------------------------------------------------------------------
    void Image2::_setAutoDelete( bool autoDelete )
    {
        mAutoDelete = autoDelete;
    }
    //-----------------------------------------------------------------------------------
    bool Image2::getAutoDelete(void) const
    {
        return mAutoDelete;
    }
}
