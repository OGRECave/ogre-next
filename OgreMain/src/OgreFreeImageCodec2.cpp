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

#include "OgreFreeImageCodec2.h"

#include "OgreDataStream.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreString.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 && !defined( _WINDOWS_ )
// This snippet prevents breaking Unity builds in Win32
//(smarty FreeImage defines _WINDOWS_)
#    define VC_EXTRALEAN
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX
#    include <windows.h>
#endif

#include <FreeImage.h>

// freeimage 3.9.1~3.11.0 interoperability fix
#ifndef FREEIMAGE_COLORORDER
// we have freeimage 3.9.1, define these symbols in such way as 3.9.1 really work
//(do not use 3.11.0 definition, as color order was changed between these two versions on Apple systems)
#    define FREEIMAGE_COLORORDER_BGR 0
#    define FREEIMAGE_COLORORDER_RGB 1
#    if defined( FREEIMAGE_BIGENDIAN )
#        define FREEIMAGE_COLORORDER FREEIMAGE_COLORORDER_RGB
#    else
#        define FREEIMAGE_COLORORDER FREEIMAGE_COLORORDER_BGR
#    endif
#endif

#include <sstream>

// We want to be compatible with both FreeImage and FreeImageRe, but first specifies sizes as DWORD and
// second as uint32_t. On Windows both DWORD (unsigned long) and uint32_t are separate 32bit types, so
// pointers to them can not be mixed
#ifndef FISIZE
#    define FISIZE decltype( FIICCPROFILE::size )  // DWORD for FreeImage, uint32_t for FreeImageRe
#endif

namespace Ogre
{
    FreeImageCodec2::RegisteredCodecList FreeImageCodec2::msCodecList;
    //---------------------------------------------------------------------
    void FreeImageLoadErrorHandler2( FREE_IMAGE_FORMAT fif, const char *message )
    {
        // Callback method as required by FreeImage to report problems
        const char *typeName = FreeImage_GetFormatFromFIF( fif );
        if( typeName )
        {
            LogManager::getSingleton().stream()
                << "FreeImage error: '" << message << "' when loading format " << typeName;
        }
        else
        {
            LogManager::getSingleton().stream() << "FreeImage error: '" << message << "'";
        }
    }
    //---------------------------------------------------------------------
    void FreeImageSaveErrorHandler2( FREE_IMAGE_FORMAT fif, const char *message )
    {
        // Callback method as required by FreeImage to report problems
        OGRE_EXCEPT( Exception::ERR_CANNOT_WRITE_TO_FILE, message, "FreeImageCodec2::save" );
    }
    //---------------------------------------------------------------------
    void FreeImageCodec2::startup()
    {
        FreeImage_Initialise( false );

        LogManager::getSingleton().logMessage(
            LML_NORMAL, "FreeImage version: " + String( FreeImage_GetVersion() ) );
        LogManager::getSingleton().logMessage( LML_NORMAL, FreeImage_GetCopyrightMessage() );

        // Register codecs
        StringStream strExt;
        strExt << "Supported formats: ";
        bool first = true;
        for( int i = 0; i < FreeImage_GetFIFCount(); ++i )
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
                if( !Codec::isCodecRegistered( *v ) )
                {
                    ImageCodec2 *codec = OGRE_NEW FreeImageCodec2( *v, static_cast<unsigned int>( i ) );
                    msCodecList.push_back( codec );
                    Codec::registerCodec( codec );
                }
            }
        }

        LogManager::getSingleton().logMessage( LML_NORMAL, strExt.str() );

        // Set error handler
        FreeImage_SetOutputMessage( FreeImageLoadErrorHandler2 );
    }
    //---------------------------------------------------------------------
    void FreeImageCodec2::shutdown()
    {
        FreeImage_DeInitialise();

        for( RegisteredCodecList::iterator i = msCodecList.begin(); i != msCodecList.end(); ++i )
        {
            Codec::unregisterCodec( *i );
            OGRE_DELETE *i;
        }
        msCodecList.clear();
    }
    //---------------------------------------------------------------------
    FreeImageCodec2::FreeImageCodec2( const String &type, unsigned int fiType ) :
        mType( type ),
        mFreeImageType( fiType )
    {
    }
    //---------------------------------------------------------------------
    FIBITMAP *FreeImageCodec2::encodeBitmap( MemoryDataStreamPtr &input, CodecDataPtr &pData ) const
    {
        FIBITMAP *ret = 0;

        ImageData2 *pImgData = static_cast<ImageData2 *>( pData.get() );

        // We need to determine the closest format supported by FreeImage.
        // Components order of 24bit and 32bit FI_BITMAPs is affected by #define FREEIMAGE_COLORORDER,
        // for all other types components order should be RGB
        PixelFormatGpu origFormat = pImgData->format;
        PixelFormatGpu supportedFormat = pImgData->format;
        FREE_IMAGE_TYPE imageType;
        unsigned red_mask = 0u, green_mask = 0u, blue_mask = 0u;

        switch( origFormat )
        {
        case PFG_RGBA32_FLOAT:
        case PFG_RGBA32_UINT:
        case PFG_RGBA32_SINT:
        case PFG_RGBA16_FLOAT:
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
        case PFG_D24_UNORM_S8_UINT:
            supportedFormat = PFG_RGB32_FLOAT;
            imageType = FIT_RGBF;
            break;
        case PFG_RGBA16_UNORM:
        case PFG_RGBA16_UINT:
        case PFG_RGBA16_SNORM:
        case PFG_RGBA16_SINT:
            imageType = FIT_RGBA16;
            break;
        case PFG_R10G10B10A2_UNORM:
        case PFG_R10G10B10A2_UINT:
            supportedFormat = PFG_RGBA16_UNORM;
            imageType = FIT_RGBA16;
            break;
        case PFG_RGB16_UNORM:
            imageType = FIT_RGB16;
            break;
        case PFG_RG16_UNORM:
        case PFG_RG16_UINT:
        case PFG_RG16_SNORM:
        case PFG_RG16_SINT:
            // typeless RG16 => RGB16 conversion (except for SNORM)
            if( origFormat != PFG_RG16_SNORM )
                origFormat = PFG_RG16_UNORM;
            supportedFormat = PFG_RGB16_UNORM;
            imageType = FIT_RGB16;
            break;
        case PFG_RGBA8_UNORM:
        case PFG_RGBA8_UNORM_SRGB:
        case PFG_RGBA8_UINT:
        case PFG_RGBA8_SNORM:
        case PFG_RGBA8_SINT:
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
            // typeless RGBA => BGRA conversion (except for SNORM)
            if( origFormat != PFG_RGBA8_SNORM )
                origFormat = PFG_RGBA8_UNORM;
            supportedFormat = PFG_BGRA8_UNORM;
#else
            // typeless RGBA => RGBA conversion (except for SNORM)
            if( origFormat != PFG_RGBA8_SNORM )
                origFormat = PFG_RGBA8_UNORM;
            supportedFormat = PFG_RGBA8_UNORM;
#endif
            imageType = FIT_BITMAP;
            break;

        case PFG_D32_FLOAT:
        case PFG_R32_FLOAT:
        case PFG_D24_UNORM:
        case PFG_R16_FLOAT:
            supportedFormat = PFG_R32_FLOAT;
            imageType = FIT_FLOAT;
            break;

        case PFG_R32_UINT:
            imageType = FIT_UINT32;
            break;
        case PFG_R32_SINT:
            imageType = FIT_INT32;
            break;

        case PFG_RG8_UNORM:
        case PFG_RG8_UINT:
        case PFG_RG8_SNORM:
        case PFG_RG8_SINT:
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
            // typeless RG => BGR conversion (except for SNORM)
            if( origFormat != PFG_RG8_UNORM )
                origFormat = PFG_RG8_UNORM;
            supportedFormat = PFG_BGR8_UNORM;
#else
            // typeless RG => RGB conversion (except for SNORM)
            if( origFormat != PFG_RG8_UNORM )
                origFormat = PFG_RG8_UNORM;
            supportedFormat = PFG_RGB8_UNORM;
#endif
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
            red_mask = FI16_565_RED_MASK;
            green_mask = FI16_565_GREEN_MASK;
            blue_mask = FI16_565_BLUE_MASK;
            imageType = FIT_BITMAP;
            break;

        case PFG_B5G5R5A1_UNORM:
        case PFG_B4G4R4A4_UNORM:
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
            supportedFormat = PFG_BGRA8_UNORM;
#else
            supportedFormat = PFG_RGBA8_UNORM;
#endif
            imageType = FIT_BITMAP;
            break;

        case PFG_BGRA8_UNORM:
        case PFG_BGRA8_UNORM_SRGB:
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
            // typeless BGRA => BGRA conversion
            origFormat = PFG_BGRA8_UNORM;
            supportedFormat = PFG_BGRA8_UNORM;
#else
            // typeless BGRA => RGBA conversion
            origFormat = PFG_BGRA8_UNORM;
            supportedFormat = PFG_RGBA8_UNORM;
#endif
            imageType = FIT_BITMAP;
            break;
        case PFG_BGRX8_UNORM:
        case PFG_BGRX8_UNORM_SRGB:
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
            // typeless BGRX => BGR conversion
            origFormat = PFG_BGRX8_UNORM;
            supportedFormat = PFG_BGR8_UNORM;
#else
            // typeless BGRX => RGB conversion
            origFormat = PFG_BGRX8_UNORM;
            supportedFormat = PFG_RGB8_UNORM;
#endif
            imageType = FIT_BITMAP;
            break;

        case PFG_RGB8_UNORM:
        case PFG_RGB8_UNORM_SRGB:
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
            // typeless RGB => BGR conversion
            origFormat = PFG_RGB8_UNORM;
            supportedFormat = PFG_BGR8_UNORM;
#else
            // typeless RGB => RGB conversion
            origFormat = PFG_RGB8_UNORM;
            supportedFormat = PFG_RGB8_UNORM;
#endif
            imageType = FIT_BITMAP;
            break;
        case PFG_BGR8_UNORM:
        case PFG_BGR8_UNORM_SRGB:
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
            // typeless BGR => BGR conversion
            origFormat = PFG_BGR8_UNORM;
            supportedFormat = PFG_BGR8_UNORM;
#else
            // typeless BGR => RGB conversion
            origFormat = PFG_BGR8_UNORM;
            supportedFormat = PFG_RGB8_UNORM;
#endif
            imageType = FIT_BITMAP;
            break;

        default:
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Invalid image format",
                         "FreeImageCodec2::encode" );
        };

        // Check BPP
        uint32 bpp = PixelFormatGpuUtils::getBytesPerPixel( supportedFormat ) << 3u;
        if( bpp == 32 && imageType == FIT_BITMAP &&
            ( !FreeImage_FIFSupportsExportBPP( (FREE_IMAGE_FORMAT)mFreeImageType, (int)bpp ) ||
              mFreeImageType ==
                  FIF_JPEG /* in FreeImage 3.18.0 it support 32bpp, but only for FIC_CMYK */ ) &&
            FreeImage_FIFSupportsExportBPP( (FREE_IMAGE_FORMAT)mFreeImageType, 24 ) )
        {
            // drop to 24 bit (lose alpha)
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
            supportedFormat = PFG_BGR8_UNORM;
#else
            supportedFormat = PFG_RGB8_UNORM;
#endif
            bpp = 24;
        }
        else if( bpp == 128 && imageType == FIT_RGBAF &&
                 !FreeImage_FIFSupportsExportType( (FREE_IMAGE_FORMAT)mFreeImageType, imageType ) &&
                 FreeImage_FIFSupportsExportType( (FREE_IMAGE_FORMAT)mFreeImageType, FIT_RGBF ) )
        {
            // drop to 96-bit floating point
            supportedFormat = PFG_RGB32_FLOAT;
            imageType = FIT_RGBF;
        }

        ret = FreeImage_AllocateT( imageType, static_cast<int>( pImgData->box.width ),
                                   static_cast<int>( pImgData->box.height ), (int)bpp, red_mask,
                                   green_mask, blue_mask );
        if( !ret )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "FreeImage_AllocateT failed - possibly out of memory. ", __FUNCTION__ );
        }

        if( PixelFormatGpuUtils::getNumberOfComponents( supportedFormat ) == 1u && bpp == 8 )
        {
            // Must explicitly tell FreeImage that this is greyscale by setting
            // a "grey" palette (otherwise it will save as a normal RGB
            // palettized image).
            FIBITMAP *tmp = FreeImage_ConvertToGreyscale( ret );
            FreeImage_Unload( ret );
            ret = tmp;
        }

        TextureBox destBox( pImgData->box.width, pImgData->box.height, 1u, 1u,
                            PixelFormatGpuUtils::getBytesPerPixel( supportedFormat ),
                            FreeImage_GetPitch( ret ),
                            FreeImage_GetPitch( ret ) * pImgData->box.height );
        destBox.data = FreeImage_GetBits( ret );

        // Convert data inverting scanlines
        PixelFormatGpuUtils::bulkPixelConversion( pImgData->box, origFormat, destBox, supportedFormat,
                                                  true );

        return ret;
    }
    //---------------------------------------------------------------------
    DataStreamPtr FreeImageCodec2::encode( MemoryDataStreamPtr &input, Codec::CodecDataPtr &pData ) const
    {
        FIBITMAP *fiBitmap = encodeBitmap( input, pData );

        // open memory chunk allocated by FreeImage
        FIMEMORY *mem = FreeImage_OpenMemory();
        // write data into memory
        FreeImage_SaveToMemory( (FREE_IMAGE_FORMAT)mFreeImageType, fiBitmap, mem );
        // Grab data information
        uint8_t *data;
        FISIZE size;
        FreeImage_AcquireMemory( mem, &data, &size );
        // Copy data into our own buffer
        // Because we're asking MemoryDataStream to free this, must create in a compatible way
        uint8_t *ourData = OGRE_ALLOC_T( uint8_t, size, MEMCATEGORY_GENERAL );
        memcpy( ourData, data, size );
        // Wrap data in stream, tell it to free on close
        DataStreamPtr outstream( OGRE_NEW MemoryDataStream( ourData, size, true ) );
        // Now free FreeImage memory buffers
        FreeImage_CloseMemory( mem );
        // Unload bitmap
        FreeImage_Unload( fiBitmap );

        return outstream;
    }
    //---------------------------------------------------------------------
    void FreeImageCodec2::encodeToFile( MemoryDataStreamPtr &input, const String &outFileName,
                                        Codec::CodecDataPtr &pData ) const
    {
        FIBITMAP *fiBitmap = encodeBitmap( input, pData );

        FreeImage_Save( (FREE_IMAGE_FORMAT)mFreeImageType, fiBitmap, outFileName.c_str() );
        FreeImage_Unload( fiBitmap );
    }
    //---------------------------------------------------------------------
    Codec::DecodeResult FreeImageCodec2::decode( DataStreamPtr &input ) const
    {
        // Buffer stream into memory (TODO: override IO functions instead?)
        MemoryDataStream memStream( input, true );

        FIMEMORY *fiMem =
            FreeImage_OpenMemory( memStream.getPtr(), static_cast<uint32_t>( memStream.size() ) );
        FIBITMAP *fiBitmap = FreeImage_LoadFromMemory( (FREE_IMAGE_FORMAT)mFreeImageType, fiMem );
        if( !fiBitmap )
        {
            FreeImage_CloseMemory( fiMem );
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Error decoding image",
                         "FreeImageCodec2::decode" );
        }

        // Must derive format first, this may perform conversions
        FREE_IMAGE_TYPE imageType = FreeImage_GetImageType( fiBitmap );
        FREE_IMAGE_COLOR_TYPE colourType = FreeImage_GetColorType( fiBitmap );
        unsigned bpp = FreeImage_GetBPP( fiBitmap );
        PixelFormatGpu origFormat = PFG_UNKNOWN;
        PixelFormatGpu supportedFormat = PFG_UNKNOWN;

        switch( imageType )
        {
        case FIT_UNKNOWN:
        case FIT_COMPLEX:
        case FIT_DOUBLE:
        default:
            FreeImage_CloseMemory( fiMem );
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Unknown or unsupported image format",
                         "FreeImageCodec2::decode" );
            break;
        case FIT_BITMAP:
            // Standard image type
            // Perform any colour conversions for greyscale
            if( colourType == FIC_MINISWHITE || colourType == FIC_MINISBLACK )
            {
                FIBITMAP *newBitmap = FreeImage_ConvertToGreyscale( fiBitmap );
                // free old bitmap and replace
                FreeImage_Unload( fiBitmap );
                fiBitmap = newBitmap;
                // get new formats
                bpp = FreeImage_GetBPP( fiBitmap );
            }
            // Perform any colour conversions for RGB
            else if( bpp < 8 || colourType == FIC_PALETTE || colourType == FIC_CMYK )
            {
                FIBITMAP *newBitmap = NULL;
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
                supportedFormat = PFG_R8_UNORM;
                break;
            case 16:
                // Determine 555 or 565 from green mask
                // cannot be 16-bit greyscale since that's FIT_UINT16
                if( FreeImage_GetGreenMask( fiBitmap ) == FI16_565_GREEN_MASK )
                {
                    supportedFormat = PFG_B5G6R5_UNORM;
                }
                else
                {
                    // FreeImage doesn't support 4444 format so must be 1555
                    supportedFormat = PFG_B5G5R5A1_UNORM;
                }
                break;
            case 24:
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_RGB
                origFormat = PFG_RGB8_UNORM;
                supportedFormat = PFG_RGBA8_UNORM;
#else
                origFormat = PFG_BGR8_UNORM;
                // Do NOT use PFG_BGRA8_UNORM. That format MUST be avoided
                supportedFormat = PFG_RGBA8_UNORM;
#endif
                break;
            case 32:
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_RGB
                origFormat = PFG_RGBA8_UNORM;
                supportedFormat = PFG_RGBA8_UNORM;
#else
                origFormat = PFG_BGRA8_UNORM;
                // Do NOT use PFG_BGRA8_UNORM. That format MUST be avoided
                supportedFormat = PFG_RGBA8_UNORM;
#endif
                break;
            }
            break;
        case FIT_UINT16:
            // 16-bit greyscale
            supportedFormat = PFG_R16_UNORM;
            break;
        case FIT_INT16:
            // 16-bit greyscale
            supportedFormat = PFG_R16_SNORM;
            break;
        case FIT_UINT32:
            supportedFormat = PFG_R32_UINT;
            break;
        case FIT_INT32:
            supportedFormat = PFG_R32_SINT;
            break;
        case FIT_FLOAT:
            // Single-component floating point data
            supportedFormat = PFG_R32_FLOAT;
            break;
        case FIT_RGB16:
            origFormat = PFG_RGB16_UNORM;
            supportedFormat = PFG_RGBA16_UNORM;
            break;
        case FIT_RGBA16:
            supportedFormat = PFG_RGBA16_UNORM;
            break;
        case FIT_RGBF:
            supportedFormat = PFG_RGB32_FLOAT;
            break;
        case FIT_RGBAF:
            supportedFormat = PFG_RGBA32_FLOAT;
            break;
        };

        if( origFormat == PFG_UNKNOWN )
            origFormat = supportedFormat;

        ImageData2 *imgData = OGRE_NEW ImageData2();
        imgData->box.width = FreeImage_GetWidth( fiBitmap );
        imgData->box.height = FreeImage_GetHeight( fiBitmap );
        imgData->box.depth = 1;       // only 2D formats handled by this codec
        imgData->box.numSlices = 1u;  // Always one face, cubemaps are not currently supported
        imgData->numMipmaps = 1;      // no mipmaps in non-DDS
        imgData->textureType = TextureTypes::Type2D;
        imgData->format = supportedFormat;

        const uint32 rowAlignment = 4u;
        imgData->box.bytesPerPixel = PixelFormatGpuUtils::getBytesPerPixel( supportedFormat );
        imgData->box.bytesPerRow = (uint32)PixelFormatGpuUtils::getSizeBytes(
            imgData->box.width, 1u, 1u, 1u, supportedFormat, rowAlignment );
        imgData->box.bytesPerImage = size_t( imgData->box.bytesPerRow ) * size_t( imgData->box.height );
        imgData->box.data = OGRE_MALLOC_SIMD( imgData->box.bytesPerImage, MEMCATEGORY_RESOURCE );

        // Convert data inverting scanlines
        TextureBox srcBox( imgData->box.width, imgData->box.height, 1u, 1u,
                           PixelFormatGpuUtils::getBytesPerPixel( origFormat ),
                           FreeImage_GetPitch( fiBitmap ),
                           FreeImage_GetPitch( fiBitmap ) * imgData->box.height );
        srcBox.data = FreeImage_GetBits( fiBitmap );

        PixelFormatGpuUtils::bulkPixelConversion( srcBox, origFormat, imgData->box, supportedFormat,
                                                  true );

        FreeImage_Unload( fiBitmap );
        FreeImage_CloseMemory( fiMem );

        DecodeResult ret;
        ret.first.reset();
        ret.second = CodecDataPtr( imgData );
        return ret;
    }
    //---------------------------------------------------------------------
    String FreeImageCodec2::getType() const { return mType; }
    //---------------------------------------------------------------------
    String FreeImageCodec2::magicNumberToFileExt( const char *magicNumberPtr, size_t maxbytes ) const
    {
        FIMEMORY *fiMem = FreeImage_OpenMemory( (uint8_t *)const_cast<char *>( magicNumberPtr ),
                                                static_cast<uint32_t>( maxbytes ) );

        FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory( fiMem, (int)maxbytes );
        FreeImage_CloseMemory( fiMem );

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
    //---------------------------------------------------------------------
    FreeImageCodec2::ValidationStatus FreeImageCodec2::validateMagicNumber( const char *magicNumberPtr,
                                                                            size_t maxbytes ) const
    {
        FIMEMORY *fiMem = FreeImage_OpenMemory( (uint8_t *)const_cast<char *>( magicNumberPtr ),
                                                static_cast<uint32_t>( maxbytes ) );

        const auto bValid = FreeImage_ValidateFromMemory( (FREE_IMAGE_FORMAT)mFreeImageType, fiMem );
        FreeImage_CloseMemory( fiMem );

        if( bValid )
        {
            return CodecValid;
        }
        else
        {
            // Unfortunately FreeImage won't tell us when one of its plugin is incapable
            // of determining validation. So we don't really know whether it's invalid or
            // FreeImage is uncapable of validating this format.

            // We took a look at FreeImage's source code and these formats have validation
            // capability. These are popular formats so we return more accurate info
            switch( mFreeImageType )
            {
            case FIF_BMP:
            case FIF_JPEG:
            case FIF_J2K:
            case FIF_PNG:
            case FIF_DDS:
            case FIF_EXR:
            case FIF_PSD:
            case FIF_TARGA:
            case FIF_TIFF:
            case FIF_XPM:
            case FIF_WEBP:
            case FIF_JXR:
                return CodecInvalid;
            }

            return CodecUnknown;
        }
    }
}  // namespace Ogre
