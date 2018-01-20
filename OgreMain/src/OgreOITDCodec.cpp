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

#include "OgreOITDCodec.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreBitwise.h"
#include "OgrePixelFormatGpuUtils.h"

#include "OgreStringConverter.h"
#include "OgreDataStream.h"

namespace Ogre
{
#if OGRE_COMPILER == OGRE_COMPILER_MSVC
    #pragma pack (push, 1)
#else
    #pragma pack (1)
#endif
    struct OITDHeader
    {
        uint32      width;
        uint32      height;
        uint32      depthOrSlices;
        uint8       numMipmaps;
        uint8       textureType;    /// See TextureTypes::TextureTypes
        uint16      pixelFormat;    /// See PixelFormatGpu
        uint8       version;

        uint32 getDepth(void) const
        {
            return (textureType != TextureTypes::Type3D) ? 1u : depthOrSlices;
        }
        uint32 getNumSlices(void) const
        {
            return (textureType != TextureTypes::Type3D) ? depthOrSlices : 1u;
        }
    };
#if OGRE_COMPILER == OGRE_COMPILER_MSVC
    #pragma pack (pop)
#else
    #pragma pack ()
#endif

#ifndef FOURCC
    #define FOURCC(c0, c1, c2, c3) (c0 | (c1 << 8u) | (c2 << 16u) | (c3 << 24u))
#endif

    static const uint8 c_OITDVersion = 1u;
    static const uint32 OITD_MAGIC = FOURCC('O', 'I', 'T', 'D');

    //---------------------------------------------------------------------
    OITDCodec* OITDCodec::msInstance = 0;
    //---------------------------------------------------------------------
    void OITDCodec::startup(void)
    {
        if (!msInstance)
        {
            LogManager::getSingleton().logMessage( LML_NORMAL, "OITD codec registering");

            msInstance = OGRE_NEW OITDCodec();
            Codec::registerCodec(msInstance);
        }
    }
    //---------------------------------------------------------------------
    void OITDCodec::shutdown(void)
    {
        if( msInstance )
        {
            Codec::unregisterCodec(msInstance);
            OGRE_DELETE msInstance;
            msInstance = 0;
        }
    }
    //---------------------------------------------------------------------
    OITDCodec::OITDCodec():
        mType("oitd")
    {
    }
    //---------------------------------------------------------------------
    DataStreamPtr OITDCodec::encode( MemoryDataStreamPtr& input, Codec::CodecDataPtr& pData ) const
    {
        ImageData2 *pImgData = static_cast<ImageData2*>( pData.getPointer() );

        OITDHeader oitdHeader;
        oitdHeader.width            = pImgData->box.width;
        oitdHeader.height           = pImgData->box.height;
        oitdHeader.depthOrSlices    = std::max( pImgData->box.depth, pImgData->box.numSlices );
        oitdHeader.numMipmaps       = pImgData->numMipmaps;
        oitdHeader.textureType      = pImgData->textureType;
        oitdHeader.pixelFormat      = pImgData->format;
        oitdHeader.version          = c_OITDVersion;

        uint32 oitdMagic = OITD_MAGIC;

        // Swap endian
        flipEndian( &oitdMagic, sizeof(uint32) );
        flipEndian( &oitdHeader, 4u, sizeof(OITDHeader) / 4u );

        const uint32 rowAlignment = 4u;
        const size_t requiredBytes = PixelFormatGpuUtils::calculateSizeBytes( pImgData->box.width,
                                                                              pImgData->box.height,
                                                                              pImgData->box.depth,
                                                                              pImgData->box.numSlices,
                                                                              pImgData->format,
                                                                              pImgData->numMipmaps,
                                                                              rowAlignment );

        const size_t totalSize = sizeof(uint32) + sizeof(OITDHeader) + requiredBytes;

        uint8 *ourData = OGRE_ALLOC_T( uint8, totalSize, MEMCATEGORY_GENERAL );

        {
            uint8 *tmpData = ourData;
            memcpy( tmpData, (const void*)&oitdMagic, sizeof(uint32) );
            tmpData += sizeof(uint32);
            memcpy( tmpData, (const void*)&oitdHeader, sizeof(OITDHeader) );
            tmpData += sizeof(OITDHeader);
            memcpy( tmpData, pImgData->box.data, requiredBytes );
            tmpData += requiredBytes;
        }

        // Wrap data in stream, tell it to free on close
        DataStreamPtr outstream( OGRE_NEW MemoryDataStream( ourData, totalSize, true ) );

        return outstream;
    }
    //---------------------------------------------------------------------
    void OITDCodec::encodeToFile( MemoryDataStreamPtr &input, const String& outFileName,
                                  Codec::CodecDataPtr &pData ) const
    {
        ImageData2 *pImgData = static_cast<ImageData2*>( pData.getPointer() );

        OITDHeader oitdHeader;
        oitdHeader.width            = pImgData->box.width;
        oitdHeader.height           = pImgData->box.height;
        oitdHeader.depthOrSlices    = std::max( pImgData->box.depth, pImgData->box.numSlices );
        oitdHeader.numMipmaps       = pImgData->numMipmaps;
        oitdHeader.textureType      = pImgData->textureType;
        oitdHeader.pixelFormat      = pImgData->format;
        oitdHeader.version          = c_OITDVersion;

        uint32 oitdMagic = OITD_MAGIC;

        // Swap endian
        flipEndian( &oitdMagic, sizeof(uint32) );
        flipEndian( &oitdHeader, 4u, sizeof(OITDHeader) / 4u );

        const uint32 rowAlignment = 4u;
        const size_t requiredBytes = PixelFormatGpuUtils::calculateSizeBytes( pImgData->box.width,
                                                                              pImgData->box.height,
                                                                              pImgData->box.depth,
                                                                              pImgData->box.numSlices,
                                                                              pImgData->format,
                                                                              pImgData->numMipmaps,
                                                                              rowAlignment );
        // Write the file
        std::ofstream outFile;
        outFile.open( outFileName.c_str(), std::ios_base::binary|std::ios_base::out );
        outFile.write( (const char*)&oitdMagic, sizeof(uint32) );
        outFile.write( (const char*)&oitdHeader, sizeof(OITDHeader) );
        outFile.write( (const char*)pImgData->box.data, requiredBytes );
        outFile.close();
    }
    //---------------------------------------------------------------------
    Codec::DecodeResult OITDCodec::decode( DataStreamPtr& stream ) const
    {
        // Read 4 character code
        uint32 fileType;
        stream->read( &fileType, sizeof(uint32) );
        flipEndian( &fileType, sizeof(uint32) );

        if( OITD_MAGIC != fileType )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "This is not a OITD file!",
                         "OITDCodec::decode" );
        }

        // Read header in full
        OITDHeader header;
        stream->read( &header, sizeof(OITDHeader) );

        // Endian flip if required, all 32-bit values
        flipEndian( &header, 4u, sizeof(OITDHeader) / 4u );

        if( header.version != c_OITDVersion )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "This OITD file format is version " +
                         StringConverter::toString( header.version ) +
                         "but we only support version " + StringConverter::toString( c_OITDVersion ),
                         "OITDCodec::decode" );
        }
        if( header.pixelFormat >= PFG_COUNT )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "OITD pixel format not recognized! This file is not valid.",
                         "OITDCodec::decode" );
        }

        ImageData2 *imgData = OGRE_NEW ImageData2();

        imgData->box.width      = header.width;
        imgData->box.height     = header.height;
        imgData->box.depth      = header.getDepth();
        imgData->box.numSlices  = header.getNumSlices();
        imgData->textureType    = static_cast<TextureTypes::TextureTypes>( header.textureType );
        imgData->format         = static_cast<PixelFormatGpu>( header.pixelFormat );
        imgData->numMipmaps     = header.numMipmaps;

        const uint32 rowAlignment = 4u;
        imgData->box.bytesPerPixel  = PixelFormatGpuUtils::getBytesPerPixel( imgData->format );
        imgData->box.bytesPerRow    = PixelFormatGpuUtils::getSizeBytes( imgData->box.width,
                                                                         1u, 1u, 1u,
                                                                         imgData->format,
                                                                         rowAlignment );
        imgData->box.bytesPerImage  = PixelFormatGpuUtils::getSizeBytes( imgData->box.width,
                                                                         imgData->box.height,
                                                                         1u, 1u,
                                                                         imgData->format,
                                                                         rowAlignment );
        const size_t requiredBytes = PixelFormatGpuUtils::calculateSizeBytes( imgData->box.width,
                                                                              imgData->box.height,
                                                                              imgData->box.depth,
                                                                              imgData->box.numSlices,
                                                                              imgData->format,
                                                                              imgData->numMipmaps,
                                                                              rowAlignment );

        // Bind output buffer
        imgData->box.data = OGRE_MALLOC_SIMD( requiredBytes, MEMCATEGORY_RESOURCE );

        stream->read( imgData->box.data, requiredBytes );

        DecodeResult ret;
        ret.first.reset();
        ret.second = CodecDataPtr( imgData );

        return ret;
    }
    //---------------------------------------------------------------------
    String OITDCodec::getType() const
    {
        return mType;
    }
    //---------------------------------------------------------------------
    void OITDCodec::flipEndian( void *pData, size_t size, size_t count )
    {
#if OGRE_ENDIAN == OGRE_ENDIAN_BIG
        Bitwise::bswapChunks(pData, size, count);
#endif
    }
    //---------------------------------------------------------------------
    void OITDCodec::flipEndian( void *pData, size_t size )
    {
#if OGRE_ENDIAN == OGRE_ENDIAN_BIG
        Bitwise::bswapBuffer(pData, size);
#endif
    }
    //---------------------------------------------------------------------
    String OITDCodec::magicNumberToFileExt( const char *magicNumberPtr, size_t maxbytes ) const
    {
        if (maxbytes >= sizeof(uint32))
        {
            uint32 fileType;
            memcpy( &fileType, magicNumberPtr, sizeof(uint32) );
            flipEndian( &fileType, sizeof(uint32) );

            if( OITD_MAGIC == fileType )
            {
                return String("oitd");
            }
        }

        return BLANKSTRING;
    }
}
