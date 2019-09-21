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

#include "OgrePVRTCCodec.h"
#include "OgreImage2.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreBitwise.h"

#define FOURCC(c0, c1, c2, c3) (c0 | (c1 << 8) | (c2 << 16) | (c3 << 24))
#define PVR_TEXTURE_FLAG_TYPE_MASK  0xff

namespace Ogre {
    
#if OGRE_COMPILER == OGRE_COMPILER_MSVC
#pragma pack (push, 1)
#else
#pragma pack (1)
#endif

    const uint32 PVR2_MAGIC = FOURCC('P', 'V', 'R', '!'); 
    const uint32 PVR3_MAGIC = FOURCC('P', 'V', 'R', 3); 

    enum
    {
        kPVRTextureFlagTypePVRTC_2 = 24,
        kPVRTextureFlagTypePVRTC_4
    };

    enum
    {
        kPVRTC1_PF_2BPP_RGB,
        kPVRTC1_PF_2BPP_RGBA,
        kPVRTC1_PF_4BPP_RGB,
        kPVRTC1_PF_4BPP_RGBA,
        kPVRTC2_PF_2BPP,
        kPVRTC2_PF_4BPP
    };

    typedef struct _PVRTCTexHeaderV2
    {
        uint32 headerLength;
        uint32 height;
        uint32 width;
        uint32 numMipmaps;
        uint32 flags;
        uint32 dataLength;
        uint32 bpp;
        uint32 bitmaskRed;
        uint32 bitmaskGreen;
        uint32 bitmaskBlue;
        uint32 bitmaskAlpha;
        uint32 pvrTag;
        uint32 numSurfs;
    } PVRTCTexHeaderV2;

    typedef struct _PVRTCTexHeaderV3
    {
        uint32  version;         //Version of the file header, used to identify it.
        uint32  flags;           //Various format flags.
        uint64  pixelFormat;     //The pixel format, 8cc value storing the 4 channel identifiers and their respective sizes.
        uint32  colourSpace;     //The Colour Space of the texture, currently either linear RGB or sRGB.
        uint32  channelType;     //Variable type that the channel is stored in. Supports signed/unsigned int/short/byte or float for now.
        uint32  height;          //Height of the texture.
        uint32  width;           //Width of the texture.
        uint32  depth;           //Depth of the texture. (Z-slices)
        uint32  numSurfaces;     //Number of members in a Texture Array.
        uint32  numFaces;        //Number of faces in a Cube Map. Maybe be a value other than 6.
        uint32  mipMapCount;     //Number of MIP Maps in the texture - NB: Includes top level.
        uint32  metaDataSize;    //Size of the accompanying meta data.
    } PVRTCTexHeaderV3;

    typedef struct _PVRTCMetaData
    {
        uint32 DevFOURCC;
        uint32 u32Key;
        uint32 u32DataSize;
        uint8* Data;
    } PVRTCMetadata;
    
#if OGRE_COMPILER == OGRE_COMPILER_MSVC
#pragma pack (pop)
#else
#pragma pack ()
#endif

    //---------------------------------------------------------------------
    PVRTCCodec* PVRTCCodec::msInstance = 0;
    //---------------------------------------------------------------------
    void PVRTCCodec::startup(void)
    {
        if (!msInstance)
        {
            LogManager::getSingleton().logMessage(
                LML_NORMAL,
                "PVRTC codec registering");

            msInstance = OGRE_NEW PVRTCCodec();
            Codec::registerCodec(msInstance);
        }
    }
    //---------------------------------------------------------------------
    void PVRTCCodec::shutdown(void)
    {
        if(msInstance)
        {
            Codec::unregisterCodec(msInstance);
            OGRE_DELETE msInstance;
            msInstance = 0;
        }
    }
    //---------------------------------------------------------------------
    PVRTCCodec::PVRTCCodec():
        mType("pvr")
    { 
    }
    //---------------------------------------------------------------------
    DataStreamPtr PVRTCCodec::encode(MemoryDataStreamPtr& input, Codec::CodecDataPtr& pData) const
    {        
        OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED,
                    "PVRTC encoding not supported",
                    "PVRTCCodec::encode" ) ;
    }
    //---------------------------------------------------------------------
    void PVRTCCodec::encodeToFile(MemoryDataStreamPtr& input,
        const String& outFileName, Codec::CodecDataPtr& pData) const
    {
        OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED,
                    "PVRTC encoding not supported",
                    "PVRTCCodec::encodeToFile" ) ;
    }
    //---------------------------------------------------------------------
    Codec::DecodeResult PVRTCCodec::decode(DataStreamPtr& stream) const
    {
        // Assume its a pvr 2 header
        PVRTCTexHeaderV2 headerV2;
        stream->read(&headerV2, sizeof(PVRTCTexHeaderV2));
        stream->seek(0);

        if (PVR2_MAGIC == headerV2.pvrTag)
        {           
            return decodeV2(stream);
        }

        // Try it as pvr 3 header
        PVRTCTexHeaderV3 headerV3;
        stream->read(&headerV3, sizeof(PVRTCTexHeaderV3));
        stream->seek(0);

        if (PVR3_MAGIC == headerV3.version)
        {
            return decodeV3(stream);
        }

        
        OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                        "This is not a PVR2 / PVR3 file!", "PVRTCCodec::decode");
    }
    //---------------------------------------------------------------------    
    Codec::DecodeResult PVRTCCodec::decodeV2(DataStreamPtr& stream) const
    {
        PVRTCTexHeaderV2 header;
        uint32 flags = 0, formatFlags = 0;
        size_t numFaces = 1; // Assume one face until we know otherwise

        ImageData2 *imgData = OGRE_NEW ImageData2();

        // Read the PVRTC header
        stream->read(&header, sizeof(PVRTCTexHeaderV2));

        // Get format flags
        flags = header.flags;
        flipEndian(&flags, sizeof(uint32));
        formatFlags = flags & PVR_TEXTURE_FLAG_TYPE_MASK;

        uint32 bitmaskAlpha = header.bitmaskAlpha;
        flipEndian(&bitmaskAlpha, sizeof(uint32));

        if (formatFlags == kPVRTextureFlagTypePVRTC_4 || formatFlags == kPVRTextureFlagTypePVRTC_2)
        {
            if (formatFlags == kPVRTextureFlagTypePVRTC_4)
            {
                imgData->format = bitmaskAlpha ? PFG_PVRTC_RGBA4 : PFG_PVRTC_RGB4;
            }
            else if (formatFlags == kPVRTextureFlagTypePVRTC_2)
            {
                imgData->format = bitmaskAlpha ? PFG_PVRTC_RGBA2 : PFG_PVRTC_RGB2;
            }

            imgData->box.depth = 1u;
            imgData->box.numSlices = static_cast<uint32>( numFaces );
            imgData->box.width = header.width;
            imgData->box.height = header.height;
            imgData->numMipmaps = static_cast<uint8>( header.numMipmaps );

            // PVRTC is a compressed format
            imgData->box.setCompressedPixelFormat( imgData->format );
        }

        // Calculate total size from number of mipmaps, faces and size
        const uint32 rowAlignment = 4u;
        const size_t requiredBytes = PixelFormatGpuUtils::calculateSizeBytes( imgData->box.width,
                                                                              imgData->box.height,
                                                                              imgData->box.depth,
                                                                              imgData->box.numSlices,
                                                                              imgData->format,
                                                                              imgData->numMipmaps,
                                                                              rowAlignment );
        // Bind output buffer
        imgData->box.data = OGRE_MALLOC_SIMD( requiredBytes, MEMCATEGORY_RESOURCE );

        // Now deal with the data
        stream->read( imgData->box.data, requiredBytes );

        DecodeResult ret;
        ret.first.reset();
        ret.second = CodecDataPtr( imgData );

        return ret;
    }
    //---------------------------------------------------------------------    
    Codec::DecodeResult PVRTCCodec::decodeV3(DataStreamPtr& stream) const
    {
        PVRTCTexHeaderV3 header;
        PVRTCMetadata metadata;
        uint32 flags = 0;
        size_t numFaces = 1; // Assume one face until we know otherwise

        ImageData2 *imgData = OGRE_NEW ImageData2();

        // Read the PVRTC header
        stream->read(&header, sizeof(PVRTCTexHeaderV3));

        // Read the PVRTC metadata
        if(header.metaDataSize)
        {
            stream->read(&metadata, sizeof(PVRTCMetadata));
        }

        // Identify the pixel format
        switch (header.pixelFormat)
        {
            case kPVRTC1_PF_2BPP_RGB:
                imgData->format = PFG_PVRTC_RGB2;
                break;
            case kPVRTC1_PF_2BPP_RGBA:
                imgData->format = PFG_PVRTC_RGBA2;
                break;
            case kPVRTC1_PF_4BPP_RGB:
                imgData->format = PFG_PVRTC_RGB4;
                break;
            case kPVRTC1_PF_4BPP_RGBA:
                imgData->format = PFG_PVRTC_RGBA4;
                break;
            case kPVRTC2_PF_2BPP:
                imgData->format = PFG_PVRTC2_2BPP;
                break;
            case kPVRTC2_PF_4BPP:
                imgData->format = PFG_PVRTC2_4BPP;
                break;
        }

        // Get format flags
        flags = header.flags;
        flipEndian(&flags, sizeof(uint32));

        imgData->box.depth = header.depth;
        imgData->box.numSlices = static_cast<uint32>( header.numSurfaces );
        imgData->box.width = header.width;
        imgData->box.height = header.height;
        imgData->numMipmaps = static_cast<uint8>( header.mipMapCount );

        // PVRTC is a compressed format
        imgData->box.setCompressedPixelFormat( imgData->format );

        if( !( header.numFaces % 6u ) )
        {
            imgData->textureType =
                header.numFaces == 6u ? TextureTypes::TypeCube : TextureTypes::TypeCubeArray;
            imgData->box.numSlices = header.numFaces;
        }

        if( header.depth > 1 )
            imgData->textureType = TextureTypes::Type3D;

        // Calculate total size from number of mipmaps, faces and size
        const uint32 rowAlignment = 4u;
        const size_t requiredBytes = PixelFormatGpuUtils::calculateSizeBytes( imgData->box.width,
                                                                              imgData->box.height,
                                                                              imgData->box.depth,
                                                                              imgData->box.numSlices,
                                                                              imgData->format,
                                                                              imgData->numMipmaps,
                                                                              rowAlignment );
        // Bind output buffer
        imgData->box.data = OGRE_MALLOC_SIMD( requiredBytes, MEMCATEGORY_RESOURCE );

        // Now deal with the data
        void *destPtr = imgData->box.data;
        
        uint32 width = imgData->box.width;
        uint32 height = imgData->box.height;
        uint32 depth = imgData->box.depth;

        // All mips for a surface, then each face
        for( size_t mip = 0; mip <= imgData->numMipmaps; ++mip )
        {
            //for( size_t surface = 0; surface < header.numSurfaces; ++surface )
            size_t depthOrSlices = imgData->box.getDepthOrSlices();
            if( imgData->textureType == TextureTypes::Type3D )
                depthOrSlices = depth;  // account for mips

            for( size_t surface = 0; surface < depthOrSlices; ++surface )
            {
                // Ogre does not support 3D arrays or weird (incompatible) combinaations)
                //for( size_t i = 0; i < numFaces; ++i )
                {
                    // Load directly
                    const size_t pvrSize = PixelFormatGpuUtils::calculateSizeBytes(
                        width, height, depth, 1u, imgData->format, 1u, rowAlignment );
                    stream->read( destPtr, pvrSize );
                    destPtr = static_cast<void *>( static_cast<uchar *>( destPtr ) + pvrSize );
                }
            }

            // Next mip
            // clang-format off
            width   = std::max( 1u, width  >> 1u );
            height  = std::max( 1u, height >> 1u );
            depth   = std::max( 1u, depth  >> 1u );
            // clang-format on
        }

        DecodeResult ret;
        ret.first.reset();
        ret.second = CodecDataPtr( imgData );

        return ret;
    }
    //---------------------------------------------------------------------    
    String PVRTCCodec::getType() const 
    {
        return mType;
    }
    //---------------------------------------------------------------------    
    void PVRTCCodec::flipEndian(void * pData, size_t size, size_t count)
    {
#if OGRE_ENDIAN == OGRE_ENDIAN_BIG
		Bitwise::bswapChunks(pData, size, count);
#endif
    }
    //---------------------------------------------------------------------    
    void PVRTCCodec::flipEndian(void * pData, size_t size)
    {
#if OGRE_ENDIAN == OGRE_ENDIAN_BIG
        Bitwise::bswapBuffer(pData, size);
#endif
    }
    //---------------------------------------------------------------------
    String PVRTCCodec::magicNumberToFileExt(const char *magicNumberPtr, size_t maxbytes) const
    {
        if (maxbytes >= sizeof(uint32))
        {
            uint32 fileType;
            memcpy(&fileType, magicNumberPtr, sizeof(uint32));
			flipEndian(&fileType, sizeof(uint32));

            if (PVR3_MAGIC == fileType || PVR2_MAGIC == fileType)
            {
                return String("pvr");
            }
        }

        return BLANKSTRING;
    }
}
