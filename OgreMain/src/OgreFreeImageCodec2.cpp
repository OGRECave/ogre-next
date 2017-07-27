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

#include "OgreFreeImageCodec2.h"
#include "OgrePixelBox.h"
#include "OgreImage.h"
#include "OgreDataStream.h"
#include "OgreException.h"
#include "OgreLogManager.h"

#include "OgrePixelFormatGpuUtils.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 && !defined(_WINDOWS_)
    //This snippet prevents breaking Unity builds in Win32
    //(smarty FreeImage defines _WINDOWS_)
    #define VC_EXTRALEAN
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#endif

#include <FreeImage.h>

// freeimage 3.9.1~3.11.0 interoperability fix
#ifndef FREEIMAGE_COLORORDER
// we have freeimage 3.9.1, define these symbols in such way as 3.9.1 really work
//(do not use 3.11.0 definition, as color order was changed between these two versions on Apple systems)
#define FREEIMAGE_COLORORDER_BGR    0
#define FREEIMAGE_COLORORDER_RGB    1
#if defined(FREEIMAGE_BIGENDIAN)
#define FREEIMAGE_COLORORDER FREEIMAGE_COLORORDER_RGB
#else
#define FREEIMAGE_COLORORDER FREEIMAGE_COLORORDER_BGR
#endif
#endif

namespace Ogre {

    FreeImageCodec2::RegisteredCodecList FreeImageCodec2::msCodecList;
    //---------------------------------------------------------------------
    void FreeImageLoadErrorHandler2(FREE_IMAGE_FORMAT fif, const char *message)
    {
        // Callback method as required by FreeImage to report problems
        const char* typeName = FreeImage_GetFormatFromFIF(fif);
        if (typeName)
        {
            LogManager::getSingleton().stream() 
                << "FreeImage error: '" << message << "' when loading format "
                << typeName;
        }
        else
        {
            LogManager::getSingleton().stream() 
                << "FreeImage error: '" << message << "'";
        }

    }
    //---------------------------------------------------------------------
    void FreeImageSaveErrorHandler2(FREE_IMAGE_FORMAT fif, const char *message)
    {
        // Callback method as required by FreeImage to report problems
        OGRE_EXCEPT(Exception::ERR_CANNOT_WRITE_TO_FILE, 
                    message, "FreeImageCodec2::save");
    }
    //---------------------------------------------------------------------
    void FreeImageCodec2::startup(void)
    {
        FreeImage_Initialise(false);

        LogManager::getSingleton().logMessage(
            LML_NORMAL,
            "FreeImage version: " + String(FreeImage_GetVersion()));
        LogManager::getSingleton().logMessage(
            LML_NORMAL,
            FreeImage_GetCopyrightMessage());

        // Register codecs
        StringStream strExt;
        strExt << "Supported formats: ";
        bool first = true;
        for( int i=0; i<FreeImage_GetFIFCount(); ++i )
        {
            // Skip DDS codec since FreeImage does not have the option 
            // to keep DXT data compressed, we'll use our own codec
            if( (FREE_IMAGE_FORMAT)i == FIF_DDS )
                continue;
            
            String exts( FreeImage_GetFIFExtensionList( (FREE_IMAGE_FORMAT)i ) );
            if( !first )
            {
                strExt << ",";
            }
            first = false;
            strExt << exts;
            
            // Pull off individual formats (separated by comma by FI)
            StringVector extsVector = StringUtil::split( exts, "," );
            for( StringVector::iterator v = extsVector.begin(); v != extsVector.end(); ++v )
            {
                // FreeImage 3.13 lists many formats twice: once under their own codec and
                // once under the "RAW" codec, which is listed last. Avoid letting the RAW override
                // the dedicated codec!
                if( !Codec::isCodecRegistered(*v) )
                {
                    ImageCodec2* codec = OGRE_NEW FreeImageCodec2(*v, i);
                    msCodecList.push_back(codec);
                    Codec::registerCodec(codec);
                }
            }
        }

        LogManager::getSingleton().logMessage( LML_NORMAL, strExt.str() );

        // Set error handler
        FreeImage_SetOutputMessage(FreeImageLoadErrorHandler2);
    }
    //---------------------------------------------------------------------
    void FreeImageCodec2::shutdown(void)
    {
        FreeImage_DeInitialise();

        for (RegisteredCodecList::iterator i = msCodecList.begin();
            i != msCodecList.end(); ++i)
        {
            Codec::unregisterCodec(*i);
            OGRE_DELETE *i;
        }
        msCodecList.clear();
    }
    //---------------------------------------------------------------------
    FreeImageCodec2::FreeImageCodec2(const String &type, unsigned int fiType):
        mType(type),
        mFreeImageType(fiType)
    { 
    }
    //---------------------------------------------------------------------
    void FreeImageCodec2::copyData( uint8 *dstData, size_t dstBytesPerRow,
                                    const uint8 *srcData, size_t srcBytesPerRow,
                                    size_t width, size_t height, uint32 bpp,
                                    PixelFormatGpu pixelFormat )
    {
        if( bpp == 24u )
        {
            for( size_t y=0; y<height; ++y )
            {
                uint8 *pDst = reinterpret_cast<uint8*>( dstData + y * dstBytesPerRow );
                uint8 const *pSrc = srcData + (height - y - 1u) * srcBytesPerRow;
                for( size_t x=0; x<width; ++x )
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
        else if( bpp == 32u )
        {
            for( size_t y=0; y<height; ++y )
            {
                uint8 *pDst = reinterpret_cast<uint8*>( dstData + y * dstBytesPerRow );
                uint8 const *pSrc = reinterpret_cast<const uint8*>( srcData + (height - y - 1u) *
                                                                    srcBytesPerRow );
                for( size_t x=0; x<width; ++x )
                {
                    const uint8 b = *pSrc++;
                    const uint8 g = *pSrc++;
                    const uint8 r = *pSrc++;
                    const uint8 a = *pSrc++;

                    *pDst++ = r;
                    *pDst++ = g;
                    *pDst++ = b;
                    *pDst++ = a;
                }
            }
        }
        else if( bpp == 48u )
        {
            for( size_t y=0; y<height; ++y )
            {
                uint16 *pDst = reinterpret_cast<uint16*>( dstData + y * dstBytesPerRow );
                uint16 const *pSrc = reinterpret_cast<const uint16*>( srcData + (height - y - 1u) *
                                                                      srcBytesPerRow );
                for( size_t x=0; x<width; ++x )
                {
                    const uint16 b = *pSrc++;
                    const uint16 g = *pSrc++;
                    const uint16 r = *pSrc++;

                    *pDst++ = r;
                    *pDst++ = g;
                    *pDst++ = b;
                    *pDst++ = 0xFFFF;
                }
            }
        }
        else if( bpp == 64u )
        {
            for( size_t y=0; y<height; ++y )
            {
                uint16 *pDst = reinterpret_cast<uint16*>( dstData + y * dstBytesPerRow );
                uint16 const *pSrc = reinterpret_cast<const uint16*>( srcData + (height - y - 1u) *
                                                                      srcBytesPerRow );
                for( size_t x=0; x<width; ++x )
                {
                    const uint16 b = *pSrc++;
                    const uint16 g = *pSrc++;
                    const uint16 r = *pSrc++;
                    const uint16 a = *pSrc++;

                    *pDst++ = r;
                    *pDst++ = g;
                    *pDst++ = b;
                    *pDst++ = a;
                }
            }
        }
        else
        {
            const size_t bytesToCopyPerRow =
                    width * PixelFormatGpuUtils::getBytesPerPixel( pixelFormat );

            uint8 *pDst = reinterpret_cast<uint8*>( dstData );
            for( size_t y=0; y<height; ++y )
            {
                const uint8 *pSrc = srcData + (height - y - 1u) * srcBytesPerRow;
                memcpy( pDst, pSrc, bytesToCopyPerRow );
                pDst += dstBytesPerRow;
            }
        }
    }
    //---------------------------------------------------------------------
    FIBITMAP* FreeImageCodec2::encodeBitmap( MemoryDataStreamPtr &input, CodecDataPtr &pData ) const
    {
        FIBITMAP* ret = 0;

        ImageData2 *pImgData = static_cast<ImageData2*>( pData.getPointer() );

        // We need to determine the closest format supported by FreeImage.
        FREE_IMAGE_TYPE imageType;
        const PixelFormatGpu origFormat = pImgData->format;
        PixelFormatGpu supportedFormat = pImgData->format;

        switch( origFormat )
        {
        case PFG_RGBA32_FLOAT:
        case PFG_RGBA32_UINT:
        case PFG_RGBA32_SINT:
        case PFG_RGBA16_FLOAT:
        case PFG_RGBA16_UINT:
        case PFG_RGBA16_SNORM:
        case PFG_RGBA16_SINT:
            supportedFormat = PFG_RGBA32_FLOAT;
            imageType = FIT_RGBAF;
            break;
        case PFG_RGB32_FLOAT:
        case PFG_RGB32_UINT:
        case PFG_RGB32_SINT:
        case PFG_RG32_FLOAT:
        case PFG_RG32_UINT:
        case PFG_RG32_SINT:
        case PFG_D32_FLOAT_S8X24_UINT:
        case PFG_R11G11B10_FLOAT:
        case PFG_RG16_FLOAT:
        case PFG_RG16_UINT:
        case PFG_RG16_SNORM:
        case PFG_RG16_SINT:
        case PFG_D24_UNORM_S8_UINT:
            supportedFormat = PFG_RGB32_FLOAT;
            imageType = FIT_RGBF;
            break;
        case PFG_RGBA16_UNORM:
        case PFG_R10G10B10A2_UNORM:
        case PFG_R10G10B10A2_UINT:
        case PFG_RG16_UNORM:
            supportedFormat = PFG_RGBA16_UNORM;
            imageType = FIT_RGBA16;
            break;
        case PFG_RGBA8_UNORM:
        case PFG_RGBA8_UNORM_SRGB:
        case PFG_RGBA8_UINT:
        case PFG_RGBA8_SNORM:
        case PFG_RGBA8_SINT:
            imageType = FIT_BITMAP;
            break;

        case PFG_D32_FLOAT:
        case PFG_R32_FLOAT:
        case PFG_R32_UINT:
        case PFG_R32_SINT:
        case PFG_D24_UNORM:
        case PFG_R16_FLOAT:
            supportedFormat = PFG_R32_FLOAT;
            imageType = FIT_FLOAT;
            break;

        case PFG_RG8_UNORM:
            supportedFormat = PFG_RGBA8_UNORM;
            imageType = FIT_BITMAP;
            break;
        case PFG_RG8_UINT:
            supportedFormat = PFG_RGBA8_UINT;
            imageType = FIT_BITMAP;
            break;
        case PFG_RG8_SNORM:
            supportedFormat = PFG_RGBA8_SNORM;
            imageType = FIT_BITMAP;
            break;
        case PFG_RG8_SINT:
            supportedFormat = PFG_RGBA8_SINT;
            imageType = FIT_BITMAP;
            break;

        case PFG_D16_UNORM:
        case PFG_R16_UNORM:
        case PFG_R16_UINT:
        case PFG_R16_SNORM:
        case PFG_R16_SINT:
            imageType = FIT_UINT16;
            break;

        case PFG_R8_UNORM:
        case PFG_R8_UINT:
        case PFG_R8_SNORM:
        case PFG_R8_SINT:
        case PFG_A8_UNORM:
            imageType = FIT_BITMAP;
            break;

        case PFG_B5G6R5_UNORM:
        case PFG_B5G5R5A1_UNORM:
        case PFG_B4G4R4A4_UNORM:
            supportedFormat = PFG_RGBA8_UNORM;
            imageType = FIT_BITMAP;
            break;

        case PFG_BGRA8_UNORM:
        case PFG_BGRX8_UNORM:
        case PFG_BGRA8_UNORM_SRGB:
        case PFG_BGRX8_UNORM_SRGB:
            imageType = FIT_BITMAP;
            break;
        default:
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Invalid image format",
                         "FreeImageCodec2::encode" );
        };

        const size_t bpp = PixelFormatGpuUtils::getBytesPerPixel( supportedFormat ) << 3u;

        bool conversionRequired = false;

        uint8 const *srcData = reinterpret_cast<const uint8*>( pImgData->box.data );

        uint32 rowAlignment = 4u;
        TextureBox convBox( pImgData->box.width, pImgData->box.height, 1u, 1u,
                            PixelFormatGpuUtils::getBytesPerPixel( supportedFormat ),
                            PixelFormatGpuUtils::getSizeBytes( pImgData->box.width, 1u,
                                                               1u, 1u, supportedFormat, rowAlignment ),
                            PixelFormatGpuUtils::getSizeBytes( pImgData->box.width, pImgData->box.height,
                                                               1u, 1u, supportedFormat, rowAlignment ) );
        if( supportedFormat != pImgData->format )
        {
            conversionRequired = true;
            const size_t requiredBytes = PixelFormatGpuUtils::getSizeBytes( pImgData->box.width,
                                                                            pImgData->box.height,
                                                                            1u, 1u,
                                                                            supportedFormat, 4u );
            // Allocate memory
            convBox.data = OGRE_MALLOC_SIMD( requiredBytes, MEMCATEGORY_GENERAL );
            // perform conversion and reassign source
            PixelFormatGpuUtils::bulkPixelConversion( pImgData->box, pImgData->format,
                                                      convBox, supportedFormat );
            srcData = static_cast<const uint8*>(convBox.data);
        }

        ret = FreeImage_AllocateT( imageType,
                                   static_cast<int>( pImgData->box.width ),
                                   static_cast<int>( pImgData->box.height ),
                                   bpp );

        if( !ret )
        {
            if ( conversionRequired )
                OGRE_FREE( convBox.data, MEMCATEGORY_GENERAL );

            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "FreeImage_AllocateT failed - possibly out of memory. ",
                         __FUNCTION__ );
        }

        if( PixelFormatGpuUtils::getNumberOfComponents( supportedFormat ) == 1u )
        {
            // Must explicitly tell FreeImage that this is greyscale by setting
            // a "grey" palette (otherwise it will save as a normal RGB
            // palettized image).
            FIBITMAP *tmp = FreeImage_ConvertToGreyscale( ret );
            FreeImage_Unload( ret );
            ret = tmp;
        }
        
        const size_t dstBytesPerRow = FreeImage_GetPitch( ret );
        const size_t srcBytesPerRow = pImgData->box.bytesPerRow;
        uint8 *dstData = FreeImage_GetBits( ret );

        // Copy data, invert scanlines and respect FreeImage pitch
        FreeImageCodec2::copyData( dstData, dstBytesPerRow,
                                   reinterpret_cast<const uint8*>( pImgData->box.data ), srcBytesPerRow,
                                   pImgData->box.width, pImgData->box.height, bpp, pImgData->format );

        if( conversionRequired )
        {
            // delete temporary conversion area
            OGRE_FREE_SIMD( convBox.data, MEMCATEGORY_GENERAL );
        }

        // Check BPP. Let FreeImage convert from RGBA to RGB.
        if( !FreeImage_FIFSupportsExportBPP( (FREE_IMAGE_FORMAT)mFreeImageType, (int)bpp ) )
        {
            FIBITMAP *tmp = FreeImage_ConvertTo24Bits( ret );
            FreeImage_Unload( ret );
            ret = tmp;
        }

        return ret;
    }
    //---------------------------------------------------------------------
    DataStreamPtr FreeImageCodec2::encode( MemoryDataStreamPtr &input, Codec::CodecDataPtr &pData ) const
    {        
        FIBITMAP *fiBitmap = encodeBitmap( input, pData );

        // open memory chunk allocated by FreeImage
        FIMEMORY* mem = FreeImage_OpenMemory();
        // write data into memory
        FreeImage_SaveToMemory((FREE_IMAGE_FORMAT)mFreeImageType, fiBitmap, mem);
        // Grab data information
        BYTE* data;
        DWORD size;
        FreeImage_AcquireMemory(mem, &data, &size);
        // Copy data into our own buffer
        // Because we're asking MemoryDataStream to free this, must create in a compatible way
        BYTE* ourData = OGRE_ALLOC_T(BYTE, size, MEMCATEGORY_GENERAL);
        memcpy(ourData, data, size);
        // Wrap data in stream, tell it to free on close 
        DataStreamPtr outstream(OGRE_NEW MemoryDataStream(ourData, size, true));
        // Now free FreeImage memory buffers
        FreeImage_CloseMemory(mem);
        // Unload bitmap
        FreeImage_Unload(fiBitmap);

        return outstream;
    }
    //---------------------------------------------------------------------
    void FreeImageCodec2::encodeToFile(MemoryDataStreamPtr& input,
        const String& outFileName, Codec::CodecDataPtr& pData) const
    {
        FIBITMAP* fiBitmap = encodeBitmap(input, pData);

        FreeImage_Save((FREE_IMAGE_FORMAT)mFreeImageType, fiBitmap, outFileName.c_str());
        FreeImage_Unload(fiBitmap);
    }
    //---------------------------------------------------------------------
    Codec::DecodeResult FreeImageCodec2::decode( DataStreamPtr& input ) const
    {
        // Buffer stream into memory (TODO: override IO functions instead?)
        MemoryDataStream memStream(input, true);

        FIMEMORY* fiMem = FreeImage_OpenMemory( memStream.getPtr(),
                                                static_cast<DWORD>( memStream.size() ) );
        FIBITMAP* fiBitmap = FreeImage_LoadFromMemory( (FREE_IMAGE_FORMAT)mFreeImageType, fiMem );
        if( !fiBitmap )
        {
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR,
                         "Error decoding image",
                         "FreeImageCodec2::decode" );
        }

        ImageData2* imgData = OGRE_NEW ImageData2();

        imgData->box.depth = 1; // only 2D formats handled by this codec
        imgData->box.width  = FreeImage_GetWidth( fiBitmap );
        imgData->box.height = FreeImage_GetHeight( fiBitmap );
        imgData->numMipmaps = 1; // no mipmaps in non-DDS
        imgData->textureType = TextureTypes::Type2D;

        // Must derive format first, this may perform conversions
        
        FREE_IMAGE_TYPE imageType = FreeImage_GetImageType( fiBitmap );
        FREE_IMAGE_COLOR_TYPE colourType = FreeImage_GetColorType( fiBitmap );
        unsigned bpp = FreeImage_GetBPP( fiBitmap );

        switch(imageType)
        {
        case FIT_UNKNOWN:
        case FIT_COMPLEX:
        case FIT_UINT32:
        case FIT_INT32:
        case FIT_DOUBLE:
        default:
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Unknown or unsupported image format",
                         "FreeImageCodec2::decode" );
            break;
        case FIT_BITMAP:
            // Standard image type
            // Perform any colour conversions for greyscale
            if( colourType == FIC_MINISWHITE || colourType == FIC_MINISBLACK )
            {
                FIBITMAP* newBitmap = FreeImage_ConvertToGreyscale( fiBitmap );
                // free old bitmap and replace
                FreeImage_Unload( fiBitmap );
                fiBitmap = newBitmap;
                // get new formats
                bpp = FreeImage_GetBPP( fiBitmap );
            }
            // Perform any colour conversions for RGB
            else if( bpp < 8 || colourType == FIC_PALETTE || colourType == FIC_CMYK )
            {
                FIBITMAP* newBitmap =  NULL;    
                if( FreeImage_IsTransparent( fiBitmap ) )
                {
                    // convert to 32 bit to preserve the transparency 
                    // (the alpha byte will be 0 if pixel is transparent)
                    newBitmap = FreeImage_ConvertTo32Bits( fiBitmap );
                }
                else
                {
                    // no transparency - only 3 bytes are needed
                    newBitmap = FreeImage_ConvertTo24Bits( fiBitmap );
                }

                // free old bitmap and replace
                FreeImage_Unload( fiBitmap );
                fiBitmap = newBitmap;
                // get new formats
                bpp = FreeImage_GetBPP( fiBitmap );
            }

            // by this stage, 8-bit is greyscale, 16/24/32 bit are RGB[A]
            switch( bpp )
            {
            case 8:
                imgData->format = PFG_R8_UNORM;
                break;
            case 16:
                // Determine 555 or 565 from green mask
                // cannot be 16-bit greyscale since that's FIT_UINT16
                if(FreeImage_GetGreenMask(fiBitmap) == FI16_565_GREEN_MASK)
                {
                    imgData->format = PFG_B5G6R5_UNORM;
                }
                else
                {
                    // FreeImage doesn't support 4444 format so must be 1555
                    imgData->format = PFG_B5G5R5A1_UNORM;
                }
                break;
            case 24:
                // FreeImage differs per platform
                //     PF_BYTE_BGR[A] for little endian (== PF_ARGB native)
                //     PF_BYTE_RGB[A] for big endian (== PF_RGBA native)
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_RGB
                imgData->format = PFG_BGRX8_UNORM;
#else
                imgData->format = PFG_RGBA8_UNORM;
#endif
                break;
            case 32:
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_RGB
                imgData->format = PFG_BGRA8_UNORM;
#else
                imgData->format = PFG_RGBA8_UNORM;
#endif
                break;
                
                
            };
            break;
        case FIT_UINT16:
            // 16-bit greyscale
            imgData->format = PFG_R16_UNORM;
            break;
        case FIT_INT16:
            // 16-bit greyscale
            imgData->format = PFG_R16_SNORM;
            break;
        case FIT_FLOAT:
            // Single-component floating point data
            imgData->format = PFG_R32_FLOAT;
            break;
        case FIT_RGB16:
            imgData->format = PFG_RGBA16_UNORM;
            break;
        case FIT_RGBA16:
            imgData->format = PFG_RGBA16_UNORM;
            break;
        case FIT_RGBF:
            imgData->format = PFG_RGB32_FLOAT;
            break;
        case FIT_RGBAF:
            imgData->format = PFG_RGBA32_FLOAT;
            break;  
        };

        const uint32 rowAlignment = 4u;
        imgData->box.bytesPerPixel = PixelFormatGpuUtils::getBytesPerPixel( imgData->format );
        imgData->box.bytesPerRow = PixelFormatGpuUtils::getSizeBytes( imgData->box.width,
                                                                      1u, 1u, 1u,
                                                                      imgData->format,
                                                                      rowAlignment );
        imgData->box.bytesPerImage = imgData->box.bytesPerRow * imgData->box.height;

        const unsigned char *srcData    = FreeImage_GetBits( fiBitmap );
        const size_t srcBytesPerRow     = FreeImage_GetPitch( fiBitmap );
        const size_t dstBytesPerRow     = imgData->box.bytesPerRow;

        // Final data - invert image.
        imgData->box.data = OGRE_MALLOC_SIMD( imgData->box.bytesPerImage, MEMCATEGORY_RESOURCE );

        FreeImageCodec2::copyData( reinterpret_cast<uint8*>( imgData->box.data ), dstBytesPerRow,
                                   srcData, srcBytesPerRow,
                                   imgData->box.width, imgData->box.height, bpp, imgData->format );

        FreeImage_Unload(fiBitmap);
        FreeImage_CloseMemory(fiMem);

        DecodeResult ret;
        ret.first.reset();
        ret.second = CodecDataPtr(imgData);
        return ret;
    }
    //---------------------------------------------------------------------    
    String FreeImageCodec2::getType() const
    {
        return mType;
    }
    //---------------------------------------------------------------------
    String FreeImageCodec2::magicNumberToFileExt(const char *magicNumberPtr, size_t maxbytes) const
    {
        FIMEMORY* fiMem = 
                    FreeImage_OpenMemory( (BYTE*)const_cast<char*>(magicNumberPtr),
                                          static_cast<DWORD>(maxbytes) );

        FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory( fiMem, (int)maxbytes );
        FreeImage_CloseMemory(fiMem);

        if( fif != FIF_UNKNOWN )
        {
            String ext( FreeImage_GetFormatFromFIF( fif ) );
            StringUtil::toLowerCase( ext );
            return ext;
        }
        else
        {
            return BLANKSTRING;
        }
    }
}
