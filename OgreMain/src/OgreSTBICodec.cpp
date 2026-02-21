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

#include "OgreSTBICodec.h"

#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreDataStream.h"
#include "OgreString.h"

#if __OGRE_HAVE_NEON
#define STBI_NEON
#endif

#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stbi/stb_image.h"

#include <sstream>

namespace Ogre {

    STBIImageCodec::RegisteredCodecList STBIImageCodec::msCodecList;
    //---------------------------------------------------------------------
    void STBIImageCodec::startup(void)
    {
        stbi_convert_iphone_png_to_rgb(1);
        stbi_set_unpremultiply_on_load(1);

        LogManager::getSingleton().logMessage(LML_NORMAL, "stb_image - v2.27 - public domain JPEG/PNG reader");
        
        // Register codecs
        String exts = "jpeg,jpg,png,bmp,psd,tga,gif,pic,ppm,pgm";
        StringVector extsVector = StringUtil::split(exts, ",");
        for (StringVector::iterator v = extsVector.begin(); v != extsVector.end(); ++v)
        {
            ImageCodec2* codec = OGRE_NEW STBIImageCodec(*v);
            msCodecList.push_back(codec);
            Codec::registerCodec(codec);
        }
        
        StringStream strExt;
        strExt << "Supported formats: " << exts;
        
        LogManager::getSingleton().logMessage(
            LML_NORMAL,
            strExt.str());
    }
    //---------------------------------------------------------------------
    void STBIImageCodec::shutdown(void)
    {
        for (RegisteredCodecList::iterator i = msCodecList.begin();
            i != msCodecList.end(); ++i)
        {
            Codec::unregisterCodec(*i);
            OGRE_DELETE *i;
        }
        msCodecList.clear();
    }
    //---------------------------------------------------------------------
    STBIImageCodec::STBIImageCodec(const String &type):
        mType(type)
    { 
    }
    //---------------------------------------------------------------------
    DataStreamPtr STBIImageCodec::encode(MemoryDataStreamPtr& input, Codec::CodecDataPtr& pData) const
    {        
        OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED,
                    "STBI encoding not supported",
                    "STBIImageCodec::encode" ) ;

        return DataStreamPtr();
    }
    //---------------------------------------------------------------------
    void STBIImageCodec::encodeToFile(MemoryDataStreamPtr& input,
        const String& outFileName, Codec::CodecDataPtr& pData) const
    {
        OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED,
                    "STBI encoding not supported",
                    "STBIImageCodec::encodeToFile" ) ;
    }
    //---------------------------------------------------------------------
    Codec::DecodeResult STBIImageCodec::decode(DataStreamPtr& input) const
    {
        // Buffer stream into memory (TODO: override IO functions instead?)
        MemoryDataStream memStream(input, true);

        int width, height, components;
        stbi_uc *pixelData = stbi_load_from_memory(
            memStream.getPtr(), static_cast<int>( memStream.size() ), &width, &height, &components, 0 );

        if (!pixelData)
        {
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, 
                "Error decoding image: " + String(stbi_failure_reason()),
                "STBIImageCodec::decode");
        }

        ImageData2* imgData = OGRE_NEW ImageData2();

        imgData->box.depth = 1u; // only 2D formats handled by this codec
        imgData->box.numSlices = 1u;
        imgData->box.width = static_cast<uint32>( width );
        imgData->box.height = static_cast<uint32>( height );
        imgData->numMipmaps = 1u; // no mipmaps in non-DDS
        imgData->textureType = TextureTypes::Type2D;

        switch( components )
        {
            case 1:
                imgData->format = PFG_R8_UNORM;
                break;
            case 2:
                imgData->format = PFG_RG8_UNORM;
                break;
            case 3:
                imgData->format = PFG_RGBA8_UNORM;
                break;
            case 4:
                imgData->format = PFG_RGBA8_UNORM;
                break;
            default:
                stbi_image_free(pixelData);
                OGRE_DELETE imgData;
                OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
                            "Unknown or unsupported image format",
                            "STBIImageCodec::decode");
                break;
        }

        const uint32 rowAlignment = 4u;
        imgData->box.bytesPerPixel = PixelFormatGpuUtils::getBytesPerPixel( imgData->format );
        imgData->box.bytesPerRow = PixelFormatGpuUtils::getSizeBytes( imgData->box.width,
                                                                      1u, 1u, 1u,
                                                                      imgData->format,
                                                                      rowAlignment );
        imgData->box.bytesPerImage = imgData->box.bytesPerRow * imgData->box.height;

        imgData->box.data = OGRE_MALLOC_SIMD( imgData->box.bytesPerImage, MEMCATEGORY_RESOURCE );

        if( components != 3 )
            memcpy( imgData->box.data, pixelData, imgData->box.bytesPerImage );
        else
        {
            for( size_t y = 0; y < (size_t)height; ++y )
            {
                uint8 *pDst = reinterpret_cast<uint8 *>( imgData->box.at( 0u, y, 0u ) );
                uint8 const *pSrc = pixelData + y * width * components;
                for( size_t x = 0; x < (size_t)width; ++x )
                {
                    const uint8 b = *pSrc++;
                    const uint8 g = *pSrc++;
                    const uint8 r = *pSrc++;

                    *pDst++ = r;
                    *pDst++ = g;
                    *pDst++ = b;
                    *pDst++ = 0xFF;
                }
            }
        }
        stbi_image_free(pixelData);

        DecodeResult ret;
        ret.first.reset();
        ret.second = CodecDataPtr(imgData);
        return ret;
    }
    //---------------------------------------------------------------------    
    String STBIImageCodec::getType() const
    {
        return mType;
    }
    //---------------------------------------------------------------------
    static void logUnsupportedMagic(const char *magicNumberPtr, size_t maxbytes)
    {
        StringStream strMagic;
        bool lastHex = false;
        for (size_t i = 0; i < maxbytes; ++i)
        {
            uint8_t ch = magicNumberPtr[i];
            if (std::isprint(ch))
            {
                if (lastHex)
                    strMagic << " ";
                strMagic << ch;
                lastHex = false;
            }
            else
            {
                char hex[16];
                sprintf(hex, "0x%02x", ch);
                strMagic << " " << hex;
                lastHex = true;
            }
        }
        // In Gazebo, these logs appear by default in ~/.gz/rendering/ogre2.log.
        LogManager::getSingleton().logMessage( std::string( "Unsupported magic in STBIImageCodec::magicNumberPtr(" ) +
                                               std::to_string( maxbytes ) + "): " + strMagic.str());
    }
    //---------------------------------------------------------------------
    String STBIImageCodec::magicNumberToFileExt(const char *magicNumberPtr, size_t maxbytes) const
    {
        static const uint8_t png_sig[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
        static const uint8_t jpg_sig[] = { 0xff, 0xd8, 0xff };

        if( maxbytes >= sizeof( png_sig ) && memcmp( magicNumberPtr, &png_sig, sizeof( png_sig ) ) == 0 )
            return "png";

        if( maxbytes >= sizeof( jpg_sig ) && memcmp( magicNumberPtr, &jpg_sig, sizeof( jpg_sig ) ) == 0 )
            return "jpg";

        logUnsupportedMagic(magicNumberPtr, maxbytes);

        return BLANKSTRING;
    }
}
