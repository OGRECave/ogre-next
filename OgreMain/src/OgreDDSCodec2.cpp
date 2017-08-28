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

#include "OgreRoot.h"
#include "OgreRenderSystem.h"
#include "OgreDDSCodec2.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreBitwise.h"
#include "OgreImage2.h"
#include "OgrePixelFormatGpuUtils.h"

namespace Ogre {
    // Internal DDS structure definitions
#define FOURCC(c0, c1, c2, c3) (c0 | (c1 << 8) | (c2 << 16) | (c3 << 24))
    
#if OGRE_COMPILER == OGRE_COMPILER_MSVC
#pragma pack (push, 1)
#else
#pragma pack (1)
#endif

    // Nested structure
    struct DDSPixelFormat
    {
        uint32 size;
        uint32 flags;
        uint32 fourCC;
        uint32 rgbBits;
        uint32 redMask;
        uint32 greenMask;
        uint32 blueMask;
        uint32 alphaMask;
    };
    
    // Nested structure
    struct DDSCaps
    {
        uint32 caps1;
        uint32 caps2;
        uint32 caps3;
        uint32 caps4;
    };
    // Main header, note preceded by 'DDS '
    struct DDSHeader
    {
        uint32 size;        
        uint32 flags;
        uint32 height;
        uint32 width;
        uint32 sizeOrPitch;
        uint32 depth;
        uint32 mipMapCount;
        uint32 reserved1[11];
        DDSPixelFormat pixelFormat;
        DDSCaps caps;
        uint32 reserved2;
    };

    // Extended header
    struct DDSExtendedHeader
    {
        uint32 dxgiFormat;
        uint32 resourceDimension;
        uint32 miscFlag; // see D3D11_RESOURCE_MISC_FLAG
        uint32 arraySize;
        uint32 reserved;
    };

    // An 8-byte DXT colour block, represents a 4x4 texel area. Used by all DXT formats
    struct DXTColourBlock
    {
        // 2 colour ranges
        uint16 colour_0;
        uint16 colour_1;
        // 16 2-bit indexes, each byte here is one row
        uint8 indexRow[4];
    };
    // An 8-byte DXT explicit alpha block, represents a 4x4 texel area. Used by DXT2/3
    struct DXTExplicitAlphaBlock
    {
        // 16 4-bit values, each 16-bit value is one row
        uint16 alphaRow[4];
    };
    // An 8-byte DXT interpolated alpha block, represents a 4x4 texel area. Used by DXT4/5
    struct DXTInterpolatedAlphaBlock
    {
        // 2 alpha ranges
        uint8 alpha_0;
        uint8 alpha_1;
        // 16 3-bit indexes. Unfortunately 3 bits doesn't map too well to row bytes
        // so just stored raw
        uint8 indexes[6];
    };
    
#if OGRE_COMPILER == OGRE_COMPILER_MSVC
#pragma pack (pop)
#else
#pragma pack ()
#endif

    const uint32 DDS_MAGIC = FOURCC('D', 'D', 'S', ' ');
    const uint32 DDS_PIXELFORMAT_SIZE = 8 * sizeof(uint32);
    const uint32 DDS_CAPS_SIZE = 4 * sizeof(uint32);
    const uint32 DDS_HEADER_SIZE = 19 * sizeof(uint32) + DDS_PIXELFORMAT_SIZE + DDS_CAPS_SIZE;

//    const uint32 DDSD_CAPS = 0x00000001;
//    const uint32 DDSD_HEIGHT = 0x00000002;
//    const uint32 DDSD_WIDTH = 0x00000004;
//    const uint32 DDSD_PIXELFORMAT = 0x00001000;
//    const uint32 DDSD_DEPTH = 0x00800000;
    const uint32 DDPF_ALPHAPIXELS = 0x00000001;
    const uint32 DDPF_FOURCC = 0x00000004;
//    const uint32 DDPF_RGB = 0x00000040;
    //const uint32 DDPF_BUMPLUMINANCE = 0x00040000;
    const uint32 DDPF_BUMPDUDV = 0x00080000;
//    const uint32 DDSCAPS_COMPLEX = 0x00000008;
//    const uint32 DDSCAPS_TEXTURE = 0x00001000;
    const uint32 DDSCAPS_MIPMAP = 0x00400000;
    const uint32 DDSCAPS2_CUBEMAP = 0x00000200;
//    const uint32 DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400;
//    const uint32 DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800;
//    const uint32 DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000;
//    const uint32 DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000;
//    const uint32 DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000;
//    const uint32 DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000;
    const uint32 DDSCAPS2_VOLUME = 0x00200000;

    // Currently unused
//    const uint32 DDSD_PITCH = 0x00000008;
//    const uint32 DDSD_MIPMAPCOUNT = 0x00020000;
//    const uint32 DDSD_LINEARSIZE = 0x00080000;

    // Special FourCC codes
    const uint32 D3DFMT_Q16W16V16U16    = 110;
    const uint32 D3DFMT_R16F            = 111;
    const uint32 D3DFMT_G16R16F         = 112;
    const uint32 D3DFMT_A16B16G16R16F   = 113;
    const uint32 D3DFMT_R32F            = 114;
    const uint32 D3DFMT_G32R32F         = 115;
    const uint32 D3DFMT_A32B32G32R32F   = 116;

    struct TranslateDdsPixelFormatGpu
    {
        PixelFormatGpu  format;
        uint32          rMask;
        uint32          gMask;
        uint32          bMask;
        uint32          aMask;
    };
    static const TranslateDdsPixelFormatGpu cTranslateDdsPixelFormatGpu[] =
    {
        { PFG_R10G10B10A2_UNORM,    0x000003FF, 0x000FFC00, 0x3FF00000, 0xC0000000 },
        { PFG_RGBA8_UNORM,          0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000 },
        { PFG_RGBA8_SNORM,          0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000 },
        { PFG_RG16_UNORM,           0x0000FFFF, 0xFFFF0000, 0x00000000, 0x00000000 },
        { PFG_RG16_SNORM,           0x0000FFFF, 0xFFFF0000, 0x00000000, 0x00000000 },
        { PFG_RG8_UNORM,            0x000000FF, 0x0000FF00, 0x00000000, 0x00000000 },
        { PFG_RG8_SNORM,            0x000000FF, 0x0000FF00, 0x00000000, 0x00000000 },
        { PFG_R16_UNORM,            0x0000FFFF, 0x00000000, 0x00000000, 0x00000000 },
        { PFG_R16_SNORM,            0x0000FFFF, 0x00000000, 0x00000000, 0x00000000 },
        { PFG_R8_UNORM,             0x000000FF, 0x00000000, 0x00000000, 0x00000000 },
        { PFG_R8_SNORM,             0x000000FF, 0x00000000, 0x00000000, 0x00000000 },
        { PFG_A8_UNORM,             0x00000000, 0x00000000, 0x00000000, 0x000000FF },
        { PFG_B5G6R5_UNORM,         0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 },
        { PFG_B5G5R5A1_UNORM,       0x00007C00, 0x000003E0, 0x0000001F, 0x00008000 },
        { PFG_BGRA8_UNORM,          0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000 },
        { PFG_BGRX8_UNORM,          0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 },
        { PFG_B4G4R4A4_UNORM,       0x00000F00, 0x000000F0, 0x0000000F, 0x0000F000 }
    };

    //---------------------------------------------------------------------
    DDSCodec2* DDSCodec2::msInstance = 0;
    //---------------------------------------------------------------------
    void DDSCodec2::startup(void)
    {
        if (!msInstance)
        {
            LogManager::getSingleton().logMessage( LML_NORMAL, "DDS codec registering");

            msInstance = OGRE_NEW DDSCodec2();
            Codec::registerCodec(msInstance);
        }
    }
    //---------------------------------------------------------------------
    void DDSCodec2::shutdown(void)
    {
        if( msInstance )
        {
            Codec::unregisterCodec(msInstance);
            OGRE_DELETE msInstance;
            msInstance = 0;
        }
    }
    //---------------------------------------------------------------------
    DDSCodec2::DDSCodec2():
        mType("dds")
    { 
    }
    //---------------------------------------------------------------------
    DataStreamPtr DDSCodec2::encode(MemoryDataStreamPtr& input, Codec::CodecDataPtr& pData) const
    {        
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "DDS encoding not supported",
                     "DDSCodec2::encode" );
    }
    //---------------------------------------------------------------------
    void DDSCodec2::encodeToFile( MemoryDataStreamPtr &input, const String& outFileName,
                                  Codec::CodecDataPtr &pData ) const
    {
#if TODO_OGRE_2_2
        // Unwrap codecDataPtr - data is cleaned by calling function
        DDSCodec2 *imgData = static_cast<DDSCodec2*>( pData.getPointer() );

        // Check size for cube map faces
        bool isCubeMap = (imgData->size ==
            Image::calculateSize(imgData->num_mipmaps, 6, imgData->width, 
            imgData->height, imgData->depth, imgData->format));

        // Establish texture attributes
        bool isVolume = (imgData->depth > 1);       
        bool isFloat32r = (imgData->format == PF_FLOAT32_R);
        bool isFloat16 = (imgData->format == PF_FLOAT16_RGBA);
        bool isFloat32 = (imgData->format == PF_FLOAT32_RGBA);
        bool notImplemented = false;
        String notImplementedString = "";

        // Check for all the 'not implemented' conditions
        if ((isVolume == true)&&(imgData->width != imgData->height))
        {
            // Square textures only
            notImplemented = true;
            notImplementedString += " non square textures";
        }

        uint32 size = 1;
        while (size < imgData->width)
        {
            size <<= 1;
        }
        if (size != imgData->width)
        {
            // Power two textures only
            notImplemented = true;
            notImplementedString += " non power two textures";
        }

        switch(imgData->format)
        {
        case PF_A8R8G8B8:
        case PF_X8R8G8B8:
        case PF_R8G8B8:
        case PF_A8B8G8R8:
        case PF_X8B8G8R8:
        case PF_B8G8R8:
        case PF_FLOAT32_R:
        case PF_FLOAT16_RGBA:
        case PF_FLOAT32_RGBA:
            break;
        default:
            // No crazy FOURCC or 565 et al. file formats at this stage
            notImplemented = true;
            notImplementedString = " unsupported pixel format";
            break;
        }       



        // Except if any 'not implemented' conditions were met
        if (notImplemented)
        {
            OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED,
                "DDS encoding for" + notImplementedString + " not supported",
                "DDSCodec2::encodeToFile" ) ;
        }
        else
        {
            // Build header and write to disk

            // Variables for some DDS header flags
            bool hasAlpha = false;
            uint32 ddsHeaderFlags = 0;
            uint32 ddsHeaderRgbBits = 0;
            uint32 ddsHeaderSizeOrPitch = 0;
            uint32 ddsHeaderCaps1 = 0;
            uint32 ddsHeaderCaps2 = 0;
            uint32 ddsMagic = DDS_MAGIC;

            // Initalise the header flags
            ddsHeaderFlags = (isVolume) ? DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_DEPTH|DDSD_PIXELFORMAT :
                DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;  

            bool flipRgbMasks = false;

            // Initalise the rgbBits flags
            switch(imgData->format)
            {
            case PF_A8B8G8R8:
                flipRgbMasks = true;
            case PF_A8R8G8B8:
                ddsHeaderRgbBits = 8 * 4;
                hasAlpha = true;
                break;
            case PF_X8B8G8R8:
                flipRgbMasks = true;
            case PF_X8R8G8B8:
                ddsHeaderRgbBits = 8 * 4;
                break;
            case PF_B8G8R8:
            case PF_R8G8B8:
                ddsHeaderRgbBits = 8 * 3;
                break;
            case PF_FLOAT32_R:
                ddsHeaderRgbBits = 32;
                break;
            case PF_FLOAT16_RGBA:
                ddsHeaderRgbBits = 16 * 4;
                hasAlpha = true;
                break;
            case PF_FLOAT32_RGBA:
                ddsHeaderRgbBits = 32 * 4;
                hasAlpha = true;
                break;
            default:
                ddsHeaderRgbBits = 0;
                break;
            }

            // Initalise the SizeOrPitch flags (power two textures for now)
            ddsHeaderSizeOrPitch = static_cast<uint32>(ddsHeaderRgbBits * imgData->width);

            // Initalise the caps flags
            ddsHeaderCaps1 = (isVolume||isCubeMap) ? DDSCAPS_COMPLEX|DDSCAPS_TEXTURE : DDSCAPS_TEXTURE;
            if (isVolume)
            {
                ddsHeaderCaps2 = DDSCAPS2_VOLUME;
            }
            else if (isCubeMap)
            {
                ddsHeaderCaps2 = DDSCAPS2_CUBEMAP|
                    DDSCAPS2_CUBEMAP_POSITIVEX|DDSCAPS2_CUBEMAP_NEGATIVEX|
                    DDSCAPS2_CUBEMAP_POSITIVEY|DDSCAPS2_CUBEMAP_NEGATIVEY|
                    DDSCAPS2_CUBEMAP_POSITIVEZ|DDSCAPS2_CUBEMAP_NEGATIVEZ;
            }

            if( imgData->num_mipmaps > 0 )
                ddsHeaderCaps1 |= DDSCAPS_MIPMAP;

            // Populate the DDS header information
            DDSHeader ddsHeader;
            ddsHeader.size = DDS_HEADER_SIZE;
            ddsHeader.flags = ddsHeaderFlags;       
            ddsHeader.width = (uint32)imgData->width;
            ddsHeader.height = (uint32)imgData->height;
            ddsHeader.depth = (uint32)(isVolume ? imgData->depth : 0);
            ddsHeader.depth = (uint32)(isCubeMap ? 6 : ddsHeader.depth);
            ddsHeader.mipMapCount = imgData->num_mipmaps + 1;
            ddsHeader.sizeOrPitch = ddsHeaderSizeOrPitch;
            for (uint32 reserved1=0; reserved1<11; reserved1++) // XXX nasty constant 11
            {
                ddsHeader.reserved1[reserved1] = 0;
            }
            ddsHeader.reserved2 = 0;

            ddsHeader.pixelFormat.size = DDS_PIXELFORMAT_SIZE;
            ddsHeader.pixelFormat.flags = (hasAlpha) ? DDPF_RGB|DDPF_ALPHAPIXELS : DDPF_RGB;
            ddsHeader.pixelFormat.flags = (isFloat32r || isFloat16 || isFloat32) ? DDPF_FOURCC : ddsHeader.pixelFormat.flags;
            if (isFloat32r) {
                ddsHeader.pixelFormat.fourCC = D3DFMT_R32F;
            }
            else if (isFloat16) {
                ddsHeader.pixelFormat.fourCC = D3DFMT_A16B16G16R16F;
            }
            else if (isFloat32) {
                ddsHeader.pixelFormat.fourCC = D3DFMT_A32B32G32R32F;
            }
            else {
                ddsHeader.pixelFormat.fourCC = 0;
            }
            ddsHeader.pixelFormat.rgbBits = ddsHeaderRgbBits;

            ddsHeader.pixelFormat.alphaMask = (hasAlpha)   ? 0xFF000000 : 0x00000000;
            ddsHeader.pixelFormat.alphaMask = (isFloat32r) ? 0x00000000 : ddsHeader.pixelFormat.alphaMask;
            ddsHeader.pixelFormat.redMask   = (isFloat32r) ? 0xFFFFFFFF :0x00FF0000;
            ddsHeader.pixelFormat.greenMask = (isFloat32r) ? 0x00000000 :0x0000FF00;
            ddsHeader.pixelFormat.blueMask  = (isFloat32r) ? 0x00000000 :0x000000FF;

            if( flipRgbMasks )
                std::swap( ddsHeader.pixelFormat.redMask, ddsHeader.pixelFormat.blueMask );

            ddsHeader.caps.caps1 = ddsHeaderCaps1;
            ddsHeader.caps.caps2 = ddsHeaderCaps2;
//          ddsHeader.caps.reserved[0] = 0;
//          ddsHeader.caps.reserved[1] = 0;

            // Swap endian
            flipEndian(&ddsMagic, sizeof(uint32));
            flipEndian(&ddsHeader, 4, sizeof(DDSHeader) / 4);

            char *tmpData = 0;
            char const *dataPtr = (char const *)input->getPtr();

            if( imgData->format == PF_B8G8R8 )
            {
                PixelBox src( imgData->size / 3, 1, 1, PF_B8G8R8, input->getPtr() );
                tmpData = new char[imgData->size];
                PixelBox dst( imgData->size / 3, 1, 1, PF_R8G8B8, tmpData );

                PixelUtil::bulkPixelConversion( src, dst );

                dataPtr = tmpData;
            }

            try
            {
                // Write the file
                std::ofstream of;
                of.open(outFileName.c_str(), std::ios_base::binary|std::ios_base::out);
                of.write((const char *)&ddsMagic, sizeof(uint32));
                of.write((const char *)&ddsHeader, DDS_HEADER_SIZE);
                // XXX flipEndian on each pixel chunk written unless isFloat32r ?
                of.write(dataPtr, (uint32)imgData->size);
                of.close();
            }
            catch(...)
            {
                delete [] tmpData;
            }
        }
#endif
    }
    //---------------------------------------------------------------------
    PixelFormatGpu DDSCodec2::convertDXToOgreFormat( uint32 dxfmt ) const
    {
        switch (dxfmt)
        {
        case 72: //DXGI_FORMAT_BC1_UNORM_SRGB
            return PFG_BC1_UNORM_SRGB;
        case 75: //DXGI_FORMAT_BC2_UNORM_SRGB
            return PFG_BC2_UNORM_SRGB;
        case 78: // DXGI_FORMAT_BC3_UNORM_SRGB
            return PFG_BC3_UNORM_SRGB;
        case 80: // DXGI_FORMAT_BC4_UNORM
            return PFG_BC4_UNORM;
        case 81: // DXGI_FORMAT_BC4_SNORM
            return PFG_BC4_SNORM;
        case 83: // DXGI_FORMAT_BC5_UNORM
            return PFG_BC5_UNORM;
        case 84: // DXGI_FORMAT_BC5_SNORM
            return PFG_BC5_SNORM;
        case 95: // DXGI_FORMAT_BC6H_UF16
            return PFG_BC6H_UF16;
        case 96: // DXGI_FORMAT_BC6H_SF16
            return PFG_BC6H_SF16;
        case 98: // DXGI_FORMAT_BC7_UNORM
            return PFG_BC7_UNORM;
        case 99: // DXGI_FORMAT_BC7_UNORM_SRGB
            return PFG_BC7_UNORM_SRGB;
        default:
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Unsupported DirectX format found in DDS file",
                         "DDSCodec2::convertDXToOgreFormat" );
        }
    }
    //---------------------------------------------------------------------
    PixelFormatGpu DDSCodec2::convertFourCCFormat( uint32 fourcc ) const
    {
        // convert dxt pixel format
        switch( fourcc )
        {
        case FOURCC('D','X','T','1'):
            return PFG_BC1_UNORM;
        case FOURCC('D','X','T','2'):
            return PFG_BC2_UNORM;
        case FOURCC('D','X','T','3'):
            return PFG_BC2_UNORM;
        case FOURCC('D','X','T','4'):
            return PFG_BC3_UNORM;
        case FOURCC('D','X','T','5'):
            return PFG_BC3_UNORM;
        case FOURCC('A','T','I','1'):
        case FOURCC('B','C','4','U'):
            return PFG_BC4_UNORM;
        case FOURCC('B','C','4','S'):
            return PFG_BC4_SNORM;
        case FOURCC('A','T','I','2'):
        case FOURCC('B','C','5','U'):
            return PFG_BC5_UNORM;
        case FOURCC('B','C','5','S'):
            return PFG_BC5_SNORM;
        case D3DFMT_Q16W16V16U16:
            return PFG_RGBA16_SNORM;
        case D3DFMT_R16F:
            return PFG_R16_FLOAT;
        case D3DFMT_G16R16F:
            return PFG_RG16_FLOAT;
        case D3DFMT_A16B16G16R16F:
            return PFG_RGBA16_FLOAT;
        case D3DFMT_R32F:
            return PFG_R32_FLOAT;
        case D3DFMT_G32R32F:
            return PFG_RG32_FLOAT;
        case D3DFMT_A32B32G32R32F:
            return PFG_RGBA32_FLOAT;
        // We could support 3Dc here, but only ATI cards support it, not nVidia
        default:
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Unsupported FourCC format found in DDS file",
                         "DDSCodec2::convertFourCCFormat" );
        };
    }
    //---------------------------------------------------------------------
    PixelFormatGpu DDSCodec2::convertPixelFormat( uint32 rgbBits, uint32 rMask,
                                                  uint32 gMask, uint32 bMask,
                                                  uint32 aMask, bool isSigned ) const
    {
        const size_t numFormats = sizeof(cTranslateDdsPixelFormatGpu) /
                                  sizeof(cTranslateDdsPixelFormatGpu[0]);
        // General search through pixel formats
        for( size_t i=0; i<numFormats; ++i )
        {
            PixelFormatGpu pf = cTranslateDdsPixelFormatGpu[i].format;

            const size_t bytesPerPixel = PixelFormatGpuUtils::getBytesPerPixel( pf );
            const size_t bitsPerPixel = bytesPerPixel << 3u;
            if( (bitsPerPixel == rgbBits || (bitsPerPixel == 32u && rgbBits == 24u)) &&
                PixelFormatGpuUtils::isSigned( pf ) == isSigned )
            {
                if( cTranslateDdsPixelFormatGpu[i].rMask == rMask &&
                    cTranslateDdsPixelFormatGpu[i].gMask == gMask &&
                    cTranslateDdsPixelFormatGpu[i].bMask == bMask &&
                    cTranslateDdsPixelFormatGpu[i].aMask == aMask )
                {
                    return pf;
                }
            }
        }

        OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Cannot determine pixel format",
                     "DDSCodec2::convertPixelFormat" );
    }
    //---------------------------------------------------------------------
    void DDSCodec2::unpackDXTColour( PixelFormatGpu pf, const DXTColourBlock& block,
                                     ColourValue* pCol ) const
    {
        // Note - we assume all values have already been endian swapped

        // Colour lookup table
        ColourValue derivedColours[4];

        if( (pf == PFG_BC1_UNORM || pf == PFG_BC1_UNORM_SRGB) && block.colour_0 <= block.colour_1)
        {
            // 1-bit alpha
            PixelFormatGpuUtils::unpackColour( &(derivedColours[0]), PFG_B5G6R5_UNORM,
                                               &(block.colour_0) );
            PixelFormatGpuUtils::unpackColour( &(derivedColours[1]), PFG_B5G6R5_UNORM,
                                               &(block.colour_1) );
            // one intermediate colour, half way between the other two
            derivedColours[2] = (derivedColours[0] + derivedColours[1]) / 2;
            // transparent colour
            derivedColours[3] = ColourValue::ZERO;
        }
        else
        {
            PixelFormatGpuUtils::unpackColour( &(derivedColours[0]), PFG_B5G6R5_UNORM,
                                               &(block.colour_0) );
            PixelFormatGpuUtils::unpackColour( &(derivedColours[1]), PFG_B5G6R5_UNORM,
                                               &(block.colour_1) );
            // first interpolated colour, 1/3 of the way along
            derivedColours[2] = (2 * derivedColours[0] + derivedColours[1]) / 3;
            // second interpolated colour, 2/3 of the way along
            derivedColours[3] = (derivedColours[0] + 2 * derivedColours[1]) / 3;
        }

        // Process 4x4 block of texels
        for( size_t row = 0; row<4u; ++row )
        {
            for( size_t x = 0; x<4u; ++x )
            {
                // LSB come first
                uint8 colIdx = static_cast<uint8>(block.indexRow[row] >> (x << 1u) & 0x3);
                if( pf == PFG_BC1_UNORM || pf == PFG_BC1_UNORM_SRGB )
                {
                    // Overwrite entire colour
                    pCol[(row * 4u) + x] = derivedColours[colIdx];
                }
                else
                {
                    // alpha has already been read (alpha precedes colour)
                    ColourValue &col = pCol[(row * 4u) + x];
                    col.r = derivedColours[colIdx].r;
                    col.g = derivedColours[colIdx].g;
                    col.b = derivedColours[colIdx].b;
                }
            }
        }
    }
    //---------------------------------------------------------------------
    void DDSCodec2::unpackDXTAlpha( const DXTExplicitAlphaBlock& block, ColourValue* pCol) const
    {
        // Note - we assume all values have already been endian swapped
        
        // This is an explicit alpha block, 4 bits per pixel, LSB first
        for( size_t row = 0; row < 4u; ++row )
        {
            for( size_t x = 0; x < 4u; ++x )
            {
                // Shift and mask off to 4 bits
                uint8 val = static_cast<uint8>(block.alphaRow[row] >> (x << 2u) & 0xF);
                // Convert to [0,1]
                pCol->a = (Real)val / (Real)0xF;
                pCol++;
            }
        }
    }
    //---------------------------------------------------------------------
    void DDSCodec2::unpackDXTAlpha( const DXTInterpolatedAlphaBlock& block, ColourValue* pCol) const
    {
        // Adaptive 3-bit alpha part
        float derivedAlphas[8];

        // Explicit extremes
        derivedAlphas[0] = ((float) block.alpha_0) * (1.0f / 255.0f);
        derivedAlphas[1] = ((float) block.alpha_1) * (1.0f / 255.0f);

        if(block.alpha_0 > block.alpha_1)
        {
            // 6 interpolated alpha values.
            // full range including extremes at [0] and [7]
            // we want to fill in [1] through [6] at weights ranging
            // from 1/7 to 6/7
            for( size_t i=1u; i<7u; ++i )
            {
                derivedAlphas[i + 1] = (derivedAlphas[0] * (7 - i) +
                                        derivedAlphas[1] * i) * (1.0f / 7.0f);
            }
        }
        else
        {
            // 4 interpolated alpha values.
            // full range including extremes at [0] and [5]
            // we want to fill in [1] through [4] at weights ranging
            // from 1/5 to 4/5
            for( size_t i=1u; i<5u; ++i )
            {
                derivedAlphas[i + 1] = (derivedAlphas[0] * (5 - i) +
                                        derivedAlphas[1] * i) * (1.0f / 5.0f);
            }

            derivedAlphas[6] = 0.0f;
            derivedAlphas[7] = 1.0f;
        }

        // Ok, now we've built the reference values, process the indexes
        uint32 dw = block.indexes[0] | (block.indexes[1] << 8u) | (block.indexes[2] << 16u);

        for( size_t i=0; i<8; ++i, dw >>= 3u )
            pCol[i].a = derivedAlphas[dw & 0x7];

        dw = block.indexes[3] | (block.indexes[4] << 8u) | (block.indexes[5] << 16u);

        for( size_t i=8; i<16; ++i, dw >>= 3u )
            pCol[i].a = derivedAlphas[dw & 0x7];
    }
    //---------------------------------------------------------------------
    Codec::DecodeResult DDSCodec2::decode(DataStreamPtr& stream) const
    {
        // Read 4 character code
        uint32 fileType;
        stream->read(&fileType, sizeof(uint32));
        flipEndian(&fileType, sizeof(uint32));
        
        if( FOURCC('D', 'D', 'S', ' ') != fileType )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "This is not a DDS file!",
                         "DDSCodec2::decode" );
        }
        
        // Read header in full
        DDSHeader header;
        stream->read( &header, sizeof(DDSHeader) );

        // Endian flip if required, all 32-bit values
        flipEndian( &header, 4u, sizeof(DDSHeader) / 4u );

        // Check some sizes
        if( header.size != DDS_HEADER_SIZE )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "DDS header size mismatch!", "DDSCodec2::decode" );
        }
        if( header.pixelFormat.size != DDS_PIXELFORMAT_SIZE )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "DDS header size mismatch!", "DDSCodec2::decode" );
        }

        ImageData2 *imgData = OGRE_NEW ImageData2();

        imgData->box.depth = 1u; // (deal with volume later)
        imgData->box.width  = header.width;
        imgData->box.height = header.height;
        imgData->box.numSlices = 1u;
        imgData->textureType = TextureTypes::Type2D;

        if( header.caps.caps1 & DDSCAPS_MIPMAP )
            imgData->numMipmaps = static_cast<uint8>( header.mipMapCount );

        bool decompressDXT = false;
        // Figure out basic image type
        if( header.caps.caps2 & DDSCAPS2_CUBEMAP )
        {
            imgData->textureType = TextureTypes::TypeCube;
            imgData->box.numSlices = 6u;
        }
        else if( header.caps.caps2 & DDSCAPS2_VOLUME )
        {
            imgData->textureType = TextureTypes::Type3D;
            imgData->box.depth = header.depth;
        }

        // Pixel format
        PixelFormatGpu sourceFormat = PFG_UNKNOWN;

        if( header.pixelFormat.flags & DDPF_FOURCC )
        {
            // Check if we have an DX10 style extended header and read it. This is necessary for B6H and B7 formats
            if( header.pixelFormat.fourCC == FOURCC('D', 'X', '1', '0') )
            {
                DDSExtendedHeader extHeader;
                stream->read( &extHeader, sizeof(DDSExtendedHeader) );

                // Endian flip if required, all 32-bit values
                flipEndian( &header, sizeof(DDSExtendedHeader) );
                sourceFormat = convertDXToOgreFormat( extHeader.dxgiFormat );
            }
            else
            {
                sourceFormat = convertFourCCFormat( header.pixelFormat.fourCC );
            }
        }
        else
        {
            sourceFormat = convertPixelFormat( header.pixelFormat.rgbBits,
                                               header.pixelFormat.redMask,
                                               header.pixelFormat.greenMask,
                                               header.pixelFormat.blueMask,
                                               header.pixelFormat.flags & DDPF_ALPHAPIXELS ?
                                                   header.pixelFormat.alphaMask : 0,
                                               (header.pixelFormat.flags & DDPF_BUMPDUDV) != 0 );
        }

        if( PixelFormatGpuUtils::isCompressed( sourceFormat ) )
        {
            RenderSystem *renderSystem = Root::getSingleton().getRenderSystem();
            if( !renderSystem ||
                !renderSystem->getCapabilities()->hasCapability(RSC_TEXTURE_COMPRESSION_DXT) )
            {
                // We'll need to decompress
                decompressDXT = true;
                // Convert format
                switch( sourceFormat )
                {
                case PFG_BC1_UNORM:
                case PFG_BC1_UNORM_SRGB:
                    // source can be either 565 or 5551 depending on whether alpha present
                    // unfortunately you have to read a block to figure out which
                    // Note that we upgrade to 32-bit pixel formats here, even 
                    // though the source is 16-bit; this is because the interpolated
                    // values will benefit from the 32-bit results, and the source
                    // from which the 16-bit samples are calculated may have been
                    // 32-bit so can benefit from this.
                    DXTColourBlock block;
                    stream->read(&block, sizeof(DXTColourBlock));
                    flipEndian(&(block.colour_0), sizeof(uint16));
                    flipEndian(&(block.colour_1), sizeof(uint16));
                    // skip back since we'll need to read this again
                    stream->skip(0 - (long)sizeof(DXTColourBlock));
                    // colour_0 <= colour_1 means transparency in DXT1
                    if (block.colour_0 <= block.colour_1)
                    {
                        //With 1-bit alpha.
                        imgData->format = sourceFormat == PFG_BC1_UNORM ? PFG_RGBA8_UNORM :
                                                                          PFG_RGBA8_UNORM_SRGB;
                    }
                    else
                    {
                        //Without alpha, use the same format.
                        imgData->format = sourceFormat == PFG_BC1_UNORM ? PFG_RGBA8_UNORM :
                                                                          PFG_RGBA8_UNORM_SRGB;
                    }
                    break;
                case PFG_BC2_UNORM:
                case PFG_BC2_UNORM_SRGB:
                case PFG_BC3_UNORM:
                case PFG_BC3_UNORM_SRGB:
                case PFG_BC4_UNORM:
                case PFG_BC4_SNORM:
                case PFG_BC5_UNORM:
                case PFG_BC5_SNORM:
                    // full alpha present, formats vary only in encoding 
                    imgData->format = PixelFormatGpuUtils::isSRgb( sourceFormat ) ?
                                          PFG_RGBA8_UNORM_SRGB : PFG_RGBA8_UNORM;
                    break;
                default:
                    // all other cases need no special format handling
                    break;
                }
            }
            else
            {
                // Use original format
                imgData->format = sourceFormat;
            }
        }
        else // not compressed
        {
            // Don't test against DDPF_RGB since greyscale DDS doesn't set this
            // just derive any other kind of format
            imgData->format = sourceFormat;
        }

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

        Image2 image;
        image.loadDynamicImage( imgData->box.data, imgData->box.width, imgData->box.height,
                                imgData->box.getDepthOrSlices(), imgData->textureType, imgData->format,
                                false, imgData->numMipmaps );

        uint8 *rgb24TmpRow = 0;
        if( header.pixelFormat.rgbBits == 24u )
        {
            rgb24TmpRow = reinterpret_cast<uint8*>(
                              OGRE_MALLOC_SIMD( imgData->box.width * imgData->box.height * 3u,
                                                MEMCATEGORY_RESOURCE ) );
        }

        // All mips for a face, then each face
        const size_t numSlices = imgData->box.numSlices;
        for( size_t i=0; i<numSlices; ++i )
        {
            uint32 width    = imgData->box.width;
            uint32 height   = imgData->box.height;
            uint32 depth    = imgData->box.depth;

            for( size_t mip=0; mip < imgData->numMipmaps; ++mip )
            {
                TextureBox dstBox = image.getData( mip );
                void *destPtr = dstBox.at( 0, 0, i );
                size_t srcBytesPerRow = PixelFormatGpuUtils::getSizeBytes( width, 1u, 1u, 1u,
                                                                           sourceFormat, 1u );
                size_t dstBytesPerRow = dstBox.bytesPerRow;
                
                if( PixelFormatGpuUtils::isCompressed( sourceFormat ) )
                {
                    // Compressed data
                    if( decompressDXT )
                    {
                        DXTColourBlock col;
                        DXTInterpolatedAlphaBlock iAlpha;
                        DXTExplicitAlphaBlock eAlpha;
                        // 4x4 block of decompressed colour
                        ColourValue tempColours[16];
                        //size_t destBpp = PixelUtil::getNumElemBytes(imgData->format);
                        //All uncompressed formats are PFG_RGBA8_UNORM(_SRGB) so
                        //safely assume it's 4 bytes per pixel
                        const size_t dstBytesPerPixel = 4u;

                        // slices are done individually
                        for( size_t z=0; z<depth; ++z )
                        {
                            size_t remainingHeight = height;

                            // 4x4 blocks in x/y
                            for( size_t y=0; y<height; y += 4u )
                            {
                                size_t sy = std::min<size_t>( remainingHeight, 4u );
                                remainingHeight -= sy;

                                size_t remainingWidth = width;

                                for( size_t x=0; x<width; x += 4u )
                                {
                                    size_t sx = std::min<size_t>( remainingWidth, 4u );
                                    size_t destPitchMinus4 = dstBytesPerRow - dstBytesPerPixel * sx;

                                    remainingWidth -= sx;

                                    if( sourceFormat == PFG_BC2_UNORM ||
                                        sourceFormat == PFG_BC2_UNORM_SRGB )
                                    {
                                        // explicit alpha
                                        stream->read( &eAlpha, sizeof(DXTExplicitAlphaBlock) );
                                        flipEndian( eAlpha.alphaRow, sizeof(uint16), 4u );
                                        unpackDXTAlpha( eAlpha, tempColours );
                                    }
                                    else if( sourceFormat == PFG_BC3_UNORM ||
                                             sourceFormat == PFG_BC3_UNORM_SRGB )
                                    {
                                        // interpolated alpha
                                        stream->read( &iAlpha, sizeof(DXTInterpolatedAlphaBlock) );
                                        flipEndian( &(iAlpha.alpha_0), sizeof(uint16) );
                                        flipEndian( &(iAlpha.alpha_1), sizeof(uint16) );
                                        unpackDXTAlpha( iAlpha, tempColours );
                                    }
                                    // always read colour
                                    stream->read( &col, sizeof(DXTColourBlock) );
                                    flipEndian( &(col.colour_0), sizeof(uint16) );
                                    flipEndian( &(col.colour_1), sizeof(uint16) );
                                    unpackDXTColour( sourceFormat, col, tempColours );

                                    // write 4x4 block to uncompressed version
                                    for( size_t by = 0; by < sy; ++by )
                                    {
                                        for( size_t bx = 0; bx < sx; ++bx )
                                        {
                                            PixelFormatGpuUtils::packColour( tempColours[by*4+bx],
                                                                             imgData->format, destPtr );
                                            destPtr = static_cast<void*>(
                                                static_cast<uchar*>(destPtr) + dstBytesPerPixel );
                                        }
                                        // advance to next row
                                        destPtr = static_cast<void*>(
                                            static_cast<uchar*>(destPtr) + destPitchMinus4 );
                                    }
                                    // next block. Our dest pointer is 4 lines down
                                    // from where it started
                                    if( x + 4u >= width )
                                    {
                                        // Jump back to the start of the line
                                        destPtr = static_cast<void*>(
                                            static_cast<uchar*>(destPtr) - destPitchMinus4 );
                                    }
                                    else
                                    {
                                        // Jump back up 4 rows and 4 pixels to the
                                        // right to be at the next block to the right
                                        destPtr = static_cast<void*>(
                                            static_cast<uchar*>(destPtr) -
                                                      dstBytesPerRow * sy + dstBytesPerPixel * sx );
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        // load directly
                        // DDS format lies! sizeOrPitch is not always set for DXT!!
                        size_t dxtSize = PixelFormatGpuUtils::getSizeBytes( width, height, depth,
                                                                            1u, imgData->format, 1u );
                        stream->read( destPtr, dxtSize );

                        size_t dstBytes = PixelFormatGpuUtils::getSizeBytes( width, height, depth,
                                                                             1u, imgData->format,
                                                                             rowAlignment );
                        destPtr = static_cast<void*>(static_cast<uchar*>(destPtr) + dstBytes);
                    }
                }
                else
                {
                    if( header.pixelFormat.rgbBits != 24u )
                    {
                        for( size_t z=0; z<depth; ++z )
                        {
                            for( size_t y=0; y<height; ++y )
                            {
                                stream->read( destPtr, srcBytesPerRow );
                                destPtr = static_cast<void*>( static_cast<uchar*>(destPtr) +
                                                              dstBytesPerRow );
                            }
                        }
                    }
                    else
                    {
                        //24-bits is a special case, since we don't support it.
                        //We need to upgrade it to 32
                        srcBytesPerRow = width * 3u;

                        for( size_t z=0; z<depth; ++z )
                        {
                            for( size_t y=0; y<height; ++y )
                            {
                                stream->read( rgb24TmpRow, srcBytesPerRow );

                                uint8 * RESTRICT_ALIAS dstRgba32 =
                                        static_cast<uint8 * RESTRICT_ALIAS>(destPtr);
                                uint8 * RESTRICT_ALIAS srcRgba24 =
                                        static_cast<uint8 * RESTRICT_ALIAS>(rgb24TmpRow);

                                for( size_t x=0; x<width; ++x )
                                {
                                    const uint8 b = *srcRgba24++;
                                    const uint8 g = *srcRgba24++;
                                    const uint8 r = *srcRgba24++;
#if OGRE_ENDIAN == OGRE_ENDIAN_LITTLE
                                    *dstRgba32++ = b;
                                    *dstRgba32++ = g;
                                    *dstRgba32++ = r;
                                    *dstRgba32++ = 255u;
#else
                                    *dstRgba32++ = 255u;
                                    *dstRgba32++ = r;
                                    *dstRgba32++ = g;
                                    *dstRgba32++ = b;
#endif
                                }
                                destPtr = static_cast<void*>( static_cast<uchar*>(destPtr) +
                                                              dstBytesPerRow );
                            }
                        }
                    }
                }

                /// Next mip
                width   = std::max( 1u, width >> 1u );
                height  = std::max( 1u, height >> 1u );
                depth   = std::max( 1u, depth >> 1u );
            }
        }

        if( rgb24TmpRow )
        {
            OGRE_FREE_SIMD( rgb24TmpRow, MEMCATEGORY_RESOURCE );
            rgb24TmpRow = 0;
        }

        DecodeResult ret;
        ret.first.reset();
        ret.second = CodecDataPtr( imgData );

        return ret;
    }
    //---------------------------------------------------------------------    
    String DDSCodec2::getType() const
    {
        return mType;
    }
    //---------------------------------------------------------------------    
    void DDSCodec2::flipEndian(void * pData, size_t size, size_t count)
    {
#if OGRE_ENDIAN == OGRE_ENDIAN_BIG
		Bitwise::bswapChunks(pData, size, count);
#endif
    }
    //---------------------------------------------------------------------    
    void DDSCodec2::flipEndian(void * pData, size_t size)
    {
#if OGRE_ENDIAN == OGRE_ENDIAN_BIG
        Bitwise::bswapBuffer(pData, size);
#endif
    }
    //---------------------------------------------------------------------
    String DDSCodec2::magicNumberToFileExt( const char *magicNumberPtr, size_t maxbytes ) const
    {
        if (maxbytes >= sizeof(uint32))
        {
            uint32 fileType;
            memcpy(&fileType, magicNumberPtr, sizeof(uint32));
            flipEndian(&fileType, sizeof(uint32));

            if( DDS_MAGIC == fileType )
            {
                return String("dds");
            }
        }

        return BLANKSTRING;
    }    
}
