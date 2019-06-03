/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2017 Torus Knot Software Ltd

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

#include "OgreMetalPixelFormatToShaderType.h"
#include "OgreMetalPixelFormatToShaderType.inl"

#include "OgrePixelFormatGpuUtils.h"
#include "OgreTextureGpu.h"

namespace Ogre
{
    static const char c_pixelFormatTypes[PixelFormatDataTypes::NumPixelFormatDataTypes][8] =
    {
        { "float" },
        { "half" },

        { "int" },
        { "unit" },

        { "short" },
        { "ushort" },

        { "char" },
        { "uchar" }
    };

    //-------------------------------------------------------------------------
    PixelFormatDataTypes::PixelFormatDataTypes MetalPixelFormatToShaderType::getPixelFormatDataType(
            PixelFormatGpu pixelFormat )
    {
        switch( pixelFormat )
        {
        case PFG_RGBA32_FLOAT:
        case PFG_RGB32_FLOAT:
        case PFG_RGBA16_UNORM:
        case PFG_RG32_FLOAT:
        case PFG_D32_FLOAT_S8X24_UINT:
        case PFG_R10G10B10A2_UNORM:
        case PFG_R11G11B10_FLOAT:
        case PFG_RGBA8_UNORM:
        case PFG_RGBA8_UNORM_SRGB:
        case PFG_RG16_UNORM:
        case PFG_D32_FLOAT:
        case PFG_R32_FLOAT:
        case PFG_D24_UNORM:
        case PFG_D24_UNORM_S8_UINT:
        case PFG_RG8_UNORM:
        case PFG_D16_UNORM:
        case PFG_R16_UNORM:
        case PFG_R8_UNORM:
        case PFG_A8_UNORM:
        case PFG_R1_UNORM:
        case PFG_R9G9B9E5_SHAREDEXP:
        case PFG_R8G8_B8G8_UNORM:
        case PFG_G8R8_G8B8_UNORM:
        case PFG_BC1_UNORM:
        case PFG_BC1_UNORM_SRGB:
        case PFG_BC2_UNORM:
        case PFG_BC2_UNORM_SRGB:
        case PFG_BC3_UNORM:
        case PFG_BC3_UNORM_SRGB:
        case PFG_BC4_UNORM:
        case PFG_BC4_SNORM:
        case PFG_BC5_UNORM:
        case PFG_BC5_SNORM:
        case PFG_B5G6R5_UNORM:
        case PFG_B5G5R5A1_UNORM:
        case PFG_BGRA8_UNORM:
        case PFG_BGRX8_UNORM:
        case PFG_R10G10B10_XR_BIAS_A2_UNORM:
        case PFG_BGRA8_UNORM_SRGB:
        case PFG_BGRX8_UNORM_SRGB:
        case PFG_BC6H_UF16:
        case PFG_BC6H_SF16:
        case PFG_BC7_UNORM:
        case PFG_BC7_UNORM_SRGB:
        case PFG_B4G4R4A4_UNORM:
        case PFG_RGBA16_SNORM:
        case PFG_RGBA8_SNORM:
        case PFG_RG16_SNORM:
        case PFG_RG8_SNORM:
        case PFG_R16_SNORM:
        case PFG_R8_SNORM:
        case PFG_PVRTC_RGB2:
        case PFG_PVRTC_RGB2_SRGB:
        case PFG_PVRTC_RGBA2:
        case PFG_PVRTC_RGBA2_SRGB:
        case PFG_PVRTC_RGB4:
        case PFG_PVRTC_RGB4_SRGB:
        case PFG_PVRTC_RGBA4:
        case PFG_PVRTC_RGBA4_SRGB:
        case PFG_PVRTC2_2BPP:
        case PFG_PVRTC2_2BPP_SRGB:
        case PFG_PVRTC2_4BPP:
        case PFG_PVRTC2_4BPP_SRGB:
        case PFG_ETC1_RGB8_UNORM:
        case PFG_ETC2_RGB8_UNORM:
        case PFG_ETC2_RGB8_UNORM_SRGB:
        case PFG_ETC2_RGBA8_UNORM:
        case PFG_ETC2_RGBA8_UNORM_SRGB:
        case PFG_ETC2_RGB8A1_UNORM:
        case PFG_ETC2_RGB8A1_UNORM_SRGB:
        case PFG_EAC_R11_UNORM:
        case PFG_EAC_R11_SNORM:
        case PFG_EAC_R11G11_UNORM:
        case PFG_EAC_R11G11_SNORM:
        case PFG_ATC_RGB:
        case PFG_ATC_RGBA_EXPLICIT_ALPHA:
        case PFG_ATC_RGBA_INTERPOLATED_ALPHA:
        case PFG_ASTC_RGBA_UNORM_4X4_LDR:   case PFG_ASTC_RGBA_UNORM_4X4_sRGB:
        case PFG_ASTC_RGBA_UNORM_5X4_LDR:   case PFG_ASTC_RGBA_UNORM_5X4_sRGB:
        case PFG_ASTC_RGBA_UNORM_5X5_LDR:   case PFG_ASTC_RGBA_UNORM_5X5_sRGB:
        case PFG_ASTC_RGBA_UNORM_6X5_LDR:   case PFG_ASTC_RGBA_UNORM_6X5_sRGB:
        case PFG_ASTC_RGBA_UNORM_6X6_LDR:   case PFG_ASTC_RGBA_UNORM_6X6_sRGB:
        case PFG_ASTC_RGBA_UNORM_8X5_LDR:   case PFG_ASTC_RGBA_UNORM_8X5_sRGB:
        case PFG_ASTC_RGBA_UNORM_8X6_LDR:   case PFG_ASTC_RGBA_UNORM_8X6_sRGB:
        case PFG_ASTC_RGBA_UNORM_8X8_LDR:   case PFG_ASTC_RGBA_UNORM_8X8_sRGB:
        case PFG_ASTC_RGBA_UNORM_10X5_LDR:  case PFG_ASTC_RGBA_UNORM_10X5_sRGB:
        case PFG_ASTC_RGBA_UNORM_10X6_LDR:  case PFG_ASTC_RGBA_UNORM_10X6_sRGB:
        case PFG_ASTC_RGBA_UNORM_10X8_LDR:  case PFG_ASTC_RGBA_UNORM_10X8_sRGB:
        case PFG_ASTC_RGBA_UNORM_10X10_LDR: case PFG_ASTC_RGBA_UNORM_10X10_sRGB:
        case PFG_ASTC_RGBA_UNORM_12X10_LDR: case PFG_ASTC_RGBA_UNORM_12X10_sRGB:
        case PFG_ASTC_RGBA_UNORM_12X12_LDR: case PFG_ASTC_RGBA_UNORM_12X12_sRGB:
            return PixelFormatDataTypes::Float;

        case PFG_RGBA16_FLOAT:
        case PFG_RG16_FLOAT:
        case PFG_R16_FLOAT:
            return PixelFormatDataTypes::Half;

        case PFG_RGBA32_UINT:
        case PFG_RGB32_UINT:
        case PFG_RG32_UINT:
        case PFG_R32_UINT:
            return PixelFormatDataTypes::Uint;

        case PFG_RGBA32_SINT:
        case PFG_RGB32_SINT:
        case PFG_RG32_SINT:
        case PFG_R32_SINT:
            return PixelFormatDataTypes::Int;

        case PFG_R10G10B10A2_UINT:
        case PFG_RG16_UINT:
        case PFG_R16_UINT:
            return PixelFormatDataTypes::Ushort;

        case PFG_RGBA16_SINT:
        case PFG_RG16_SINT:
        case PFG_R16_SINT:
            return PixelFormatDataTypes::Short;

        case PFG_RGBA8_UINT:
        case PFG_RG8_UINT:
        case PFG_R8_UINT:
            return PixelFormatDataTypes::Uchar;

        case PFG_RGBA8_SINT:
        case PFG_RG8_SINT:
        case PFG_R8_SINT:
            return PixelFormatDataTypes::Char;

        default:
            return PixelFormatDataTypes::NumPixelFormatDataTypes;
        }

        return PixelFormatDataTypes::NumPixelFormatDataTypes;
    }
    //-------------------------------------------------------------------------
    const char* MetalPixelFormatToShaderType::getPixelFormatType( PixelFormatGpu pixelFormat ) const
    {
        PixelFormatDataTypes::PixelFormatDataTypes pfDataType = getPixelFormatDataType( pixelFormat );

        if( pfDataType == PixelFormatDataTypes::NumPixelFormatDataTypes )
            return 0;

        return c_pixelFormatTypes[pfDataType];
    }
    //-------------------------------------------------------------------------
    const char* MetalPixelFormatToShaderType::getDataType( PixelFormatGpu pixelFormat,
                                                           uint32 textureType,
                                                           bool isMsaa,
                                                           ResourceAccess::ResourceAccess access ) const
    {
        if( textureType == TextureTypes::Unknown )
            textureType = TextureTypes::Type2D;

        PixelFormatDataTypes::PixelFormatDataTypes pfDataType = getPixelFormatDataType( pixelFormat );
        if( pfDataType == PixelFormatDataTypes::NumPixelFormatDataTypes )
            return 0;

        size_t accessIdx = static_cast<size_t>( access );

        if( !PixelFormatGpuUtils::isDepth( pixelFormat ) )
        {
            if( isMsaa )
            {
                if( textureType == TextureTypes::Type2D )
                    textureType = 7u;
                else if( textureType == TextureTypes::Type2DArray )
                    textureType = 8u;
            }
            else
            {
                --textureType;
            }

            size_t dataTypeIdx = textureType * PixelFormatDataTypes::NumPixelFormatDataTypes * 4u +
                                 pfDataType * 4u + accessIdx;
            return c_dataTypes[dataTypeIdx];
        }
        else
        {
            if( isMsaa )
            {
                if( textureType == TextureTypes::Type2D )
                    textureType = 11u;
                else if( textureType == TextureTypes::Type2DArray )
                    textureType = 12u;
            }
            else
            {
                if( textureType == TextureTypes::Type2D )
                    textureType = 9u;
                else
                    textureType = 10u;
            }

            size_t dataTypeIdx = 9u * PixelFormatDataTypes::NumPixelFormatDataTypes * 4u +
                                 (textureType - 9u) * 4u + accessIdx;
            return c_dataTypes[dataTypeIdx];
        }
    }
}
