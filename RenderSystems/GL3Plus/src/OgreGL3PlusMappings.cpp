/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

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

#include "OgreGL3PlusMappings.h"
#include "OgrePixelFormatGpuUtils.h"

#include "OgreException.h"

namespace Ogre
{
    GLenum GL3PlusMappings::get( TextureTypes::TextureTypes textureType, bool cubemapsAs2DArrays )
    {
        switch( textureType )
        {
        case TextureTypes::Unknown:
            return GL_TEXTURE_2D;
        case TextureTypes::Type1D:
            return GL_TEXTURE_1D;
        case TextureTypes::Type1DArray:
            return GL_TEXTURE_1D_ARRAY;
        case TextureTypes::Type2D:
            return GL_TEXTURE_2D;
        case TextureTypes::Type2DArray:
            return GL_TEXTURE_2D_ARRAY;
        case TextureTypes::TypeCube:
            return cubemapsAs2DArrays ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_CUBE_MAP;
        case TextureTypes::TypeCubeArray:
            return cubemapsAs2DArrays ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_CUBE_MAP_ARRAY;
        case TextureTypes::Type3D:
            return GL_TEXTURE_3D;
        }

        //Satisfy some compiler warnings
        return GL_TEXTURE_2D;
    }
    //-----------------------------------------------------------------------------------
    GLenum GL3PlusMappings::get( PixelFormatGpu pf )
    {
        switch( pf )
        {
        case PFG_UNKNOWN:                       return GL_NONE;
        case PFG_NULL:                          return GL_NONE;

        case PFG_RGBA32_FLOAT:                  return GL_RGBA32F;
        case PFG_RGBA32_UINT:                   return GL_RGBA32UI;
        case PFG_RGBA32_SINT:                   return GL_RGBA32I;

        case PFG_RGB32_FLOAT:                   return GL_RGB32F;
        case PFG_RGB32_UINT:                    return GL_RGB32UI;
        case PFG_RGB32_SINT:                    return GL_RGB32I;

        case PFG_RGBA16_FLOAT:                  return GL_RGBA16F;
        case PFG_RGBA16_UNORM:                  return GL_RGBA16;
        case PFG_RGBA16_UINT:                   return GL_RGBA16UI;
        case PFG_RGBA16_SNORM:                  return GL_RGBA16_SNORM;
        case PFG_RGBA16_SINT:                   return GL_RGBA16I;

        case PFG_RG32_FLOAT:                    return GL_RG32F;
        case PFG_RG32_UINT:                     return GL_RG32UI;
        case PFG_RG32_SINT:                     return GL_RG32I;

        case PFG_D32_FLOAT_S8X24_UINT:          return GL_DEPTH32F_STENCIL8;

        case PFG_R10G10B10A2_UNORM:             return GL_RGB10_A2;
        case PFG_R10G10B10A2_UINT:              return GL_RGB10_A2UI;
        case PFG_R11G11B10_FLOAT:               return GL_R11F_G11F_B10F;

        case PFG_RGBA8_UNORM:                   return GL_RGBA8;
        case PFG_RGBA8_UNORM_SRGB:              return GL_SRGB8_ALPHA8;
        case PFG_RGBA8_UINT:                    return GL_RGBA8UI;
        case PFG_RGBA8_SNORM:                   return GL_RGBA8_SNORM;
        case PFG_RGBA8_SINT:                    return GL_RGBA8I;

        case PFG_RG16_FLOAT:                    return GL_RG16F;
        case PFG_RG16_UNORM:                    return GL_RG16;
        case PFG_RG16_UINT:                     return GL_RG16UI;
        case PFG_RG16_SNORM:                    return GL_RG16I;
        case PFG_RG16_SINT:                     return GL_RG16_SNORM;

        case PFG_D32_FLOAT:                     return GL_DEPTH_COMPONENT32F;
        case PFG_R32_FLOAT:                     return GL_R32F;
        case PFG_R32_UINT:                      return GL_R32UI;
        case PFG_R32_SINT:                      return GL_R32I;

        case PFG_D24_UNORM:                     return GL_DEPTH_COMPONENT24;
        case PFG_D24_UNORM_S8_UINT:             return GL_DEPTH24_STENCIL8;

        case PFG_RG8_UNORM:                     return GL_RG8;
        case PFG_RG8_UINT:                      return GL_RG8UI;
        case PFG_RG8_SNORM:                     return GL_RG8_SNORM;
        case PFG_RG8_SINT:                      return GL_RG8I;

        case PFG_R16_FLOAT:                     return GL_R16F;
        case PFG_D16_UNORM:                     return GL_DEPTH_COMPONENT16;
        case PFG_R16_UNORM:                     return GL_R16;
        case PFG_R16_UINT:                      return GL_R16UI;
        case PFG_R16_SNORM:                     return GL_R16_SNORM;
        case PFG_R16_SINT:                      return GL_R16I;

        case PFG_R8_UNORM:                      return GL_R8;
        case PFG_R8_UINT:                       return GL_R8UI;
        case PFG_R8_SNORM:                      return GL_R8_SNORM;
        case PFG_R8_SINT:                       return GL_R8I;
        case PFG_A8_UNORM:                      return GL_R8;
        case PFG_R1_UNORM:                      return GL_NONE;
        case PFG_R9G9B9E5_SHAREDEXP:            return GL_RGB9_E5;
        case PFG_R8G8_B8G8_UNORM:               return GL_NONE;
        case PFG_G8R8_G8B8_UNORM:               return GL_NONE;

        case PFG_BC1_UNORM:                     return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        case PFG_BC1_UNORM_SRGB:                return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;

        case PFG_BC2_UNORM:                     return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        case PFG_BC2_UNORM_SRGB:                return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;

        case PFG_BC3_UNORM:                     return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        case PFG_BC3_UNORM_SRGB:                return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;

        case PFG_BC4_UNORM:                     return GL_COMPRESSED_RED_RGTC1;
        case PFG_BC4_SNORM:                     return GL_COMPRESSED_SIGNED_RED_RGTC1;

        case PFG_BC5_UNORM:                     return GL_COMPRESSED_RG_RGTC2;
        case PFG_BC5_SNORM:                     return GL_COMPRESSED_SIGNED_RG_RGTC2;

        case PFG_B5G6R5_UNORM:                  return GL_RGB565;
        case PFG_B5G5R5A1_UNORM:                return GL_RGB5_A1;
        case PFG_BGRA8_UNORM:                   return GL_RGBA8;
        case PFG_BGRX8_UNORM:                   return GL_RGB8;
        case PFG_R10G10B10_XR_BIAS_A2_UNORM:    return GL_NONE;

        case PFG_BGRA8_UNORM_SRGB:              return GL_SRGB8_ALPHA8;
        case PFG_BGRX8_UNORM_SRGB:              return GL_SRGB8;

        case PFG_BC6H_UF16:                     return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB;
        case PFG_BC6H_SF16:                     return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB;

        case PFG_BC7_UNORM:                     return GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
        case PFG_BC7_UNORM_SRGB:                return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB;

        case PFG_AYUV:                          return GL_NONE;
        case PFG_Y410:                          return GL_NONE;
        case PFG_Y416:                          return GL_NONE;
        case PFG_NV12:                          return GL_NONE;
        case PFG_P010:                          return GL_NONE;
        case PFG_P016:                          return GL_NONE;
        case PFG_420_OPAQUE:                    return GL_NONE;
        case PFG_YUY2:                          return GL_NONE;
        case PFG_Y210:                          return GL_NONE;
        case PFG_Y216:                          return GL_NONE;
        case PFG_NV11:                          return GL_NONE;
        case PFG_AI44:                          return GL_NONE;
        case PFG_IA44:                          return GL_NONE;
        case PFG_P8:                            return GL_NONE;
        case PFG_A8P8:                          return GL_NONE;
        case PFG_B4G4R4A4_UNORM:                return GL_RGBA4;
        case PFG_P208:                          return GL_NONE;
        case PFG_V208:                          return GL_NONE;
        case PFG_V408:                          return GL_NONE;

        case PFG_PVRTC_RGB2:                    return GL_NONE;
        case PFG_PVRTC_RGB2_SRGB:               return GL_NONE;
        case PFG_PVRTC_RGBA2:                   return GL_NONE;
        case PFG_PVRTC_RGBA2_SRGB:              return GL_NONE;
        case PFG_PVRTC_RGB4:                    return GL_NONE;
        case PFG_PVRTC_RGB4_SRGB:               return GL_NONE;
        case PFG_PVRTC_RGBA4:                   return GL_NONE;
        case PFG_PVRTC_RGBA4_SRGB:              return GL_NONE;
        case PFG_PVRTC2_2BPP:                   return GL_NONE;
        case PFG_PVRTC2_2BPP_SRGB:              return GL_NONE;
        case PFG_PVRTC2_4BPP:                   return GL_NONE;
        case PFG_PVRTC2_4BPP_SRGB:              return GL_NONE;

        //ETC2 is backwards compatible with ETC1
        case PFG_ETC1_RGB8_UNORM:               return GL_COMPRESSED_RGB8_ETC2;
        case PFG_ETC2_RGB8_UNORM:               return GL_COMPRESSED_RGB8_ETC2;
        case PFG_ETC2_RGB8_UNORM_SRGB:          return GL_COMPRESSED_SRGB8_ETC2;
        case PFG_ETC2_RGBA8_UNORM:              return GL_COMPRESSED_RGBA8_ETC2_EAC;
        case PFG_ETC2_RGBA8_UNORM_SRGB:         return GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC;
        case PFG_ETC2_RGB8A1_UNORM:             return GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
        case PFG_ETC2_RGB8A1_UNORM_SRGB:        return GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2;

        case PFG_EAC_R11_UNORM:                 return GL_COMPRESSED_R11_EAC;
        case PFG_EAC_R11_SNORM:                 return GL_COMPRESSED_SIGNED_R11_EAC;
        case PFG_EAC_R11G11_UNORM:              return GL_COMPRESSED_RG11_EAC;
        case PFG_EAC_R11G11_SNORM:              return GL_COMPRESSED_SIGNED_RG11_EAC;

        case PFG_ATC_RGB:                       return GL_NONE;
        case PFG_ATC_RGBA_EXPLICIT_ALPHA:       return GL_NONE;
        case PFG_ATC_RGBA_INTERPOLATED_ALPHA:   return GL_NONE;

        case PFG_ASTC_RGBA_UNORM_4X4_LDR:       return GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
        case PFG_ASTC_RGBA_UNORM_5X4_LDR:       return GL_COMPRESSED_RGBA_ASTC_5x4_KHR;
        case PFG_ASTC_RGBA_UNORM_5X5_LDR:       return GL_COMPRESSED_RGBA_ASTC_5x5_KHR;
        case PFG_ASTC_RGBA_UNORM_6X5_LDR:       return GL_COMPRESSED_RGBA_ASTC_6x5_KHR;
        case PFG_ASTC_RGBA_UNORM_6X6_LDR:       return GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
        case PFG_ASTC_RGBA_UNORM_8X5_LDR:       return GL_COMPRESSED_RGBA_ASTC_8x5_KHR;
        case PFG_ASTC_RGBA_UNORM_8X6_LDR:       return GL_COMPRESSED_RGBA_ASTC_8x6_KHR;
        case PFG_ASTC_RGBA_UNORM_8X8_LDR:       return GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
        case PFG_ASTC_RGBA_UNORM_10X5_LDR:      return GL_COMPRESSED_RGBA_ASTC_10x5_KHR;
        case PFG_ASTC_RGBA_UNORM_10X6_LDR:      return GL_COMPRESSED_RGBA_ASTC_10x6_KHR;
        case PFG_ASTC_RGBA_UNORM_10X8_LDR:      return GL_COMPRESSED_RGBA_ASTC_10x8_KHR;
        case PFG_ASTC_RGBA_UNORM_10X10_LDR:     return GL_COMPRESSED_RGBA_ASTC_10x10_KHR;
        case PFG_ASTC_RGBA_UNORM_12X10_LDR:     return GL_COMPRESSED_RGBA_ASTC_12x10_KHR;
        case PFG_ASTC_RGBA_UNORM_12X12_LDR:     return GL_COMPRESSED_RGBA_ASTC_12x12_KHR;
        case PFG_ASTC_RGBA_UNORM_4X4_sRGB:      return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR;
        case PFG_ASTC_RGBA_UNORM_5X4_sRGB:      return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR;
        case PFG_ASTC_RGBA_UNORM_5X5_sRGB:      return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR;
        case PFG_ASTC_RGBA_UNORM_6X5_sRGB:      return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR;
        case PFG_ASTC_RGBA_UNORM_6X6_sRGB:      return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR;
        case PFG_ASTC_RGBA_UNORM_8X5_sRGB:      return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR;
        case PFG_ASTC_RGBA_UNORM_8X6_sRGB:      return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR;
        case PFG_ASTC_RGBA_UNORM_8X8_sRGB:      return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR;
        case PFG_ASTC_RGBA_UNORM_10X5_sRGB:     return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR;
        case PFG_ASTC_RGBA_UNORM_10X6_sRGB:     return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR;
        case PFG_ASTC_RGBA_UNORM_10X8_sRGB:     return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR;
        case PFG_ASTC_RGBA_UNORM_10X10_sRGB:    return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR;
        case PFG_ASTC_RGBA_UNORM_12X10_sRGB:    return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR;
        case PFG_ASTC_RGBA_UNORM_12X12_sRGB:    return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR;

        default:
            return GL_NONE;
        }
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusMappings::getFormatAndType( PixelFormatGpu pixelFormat, GLenum &format, GLenum &type )
    {
        switch( pixelFormat )
        {
        case PFG_D32_FLOAT_S8X24_UINT:
        case PFG_D24_UNORM_S8_UINT:
            format = GL_DEPTH_STENCIL;
            break;
        case PFG_D32_FLOAT:
        case PFG_D24_UNORM:
        case PFG_D16_UNORM:
            format = GL_DEPTH_COMPONENT;
            break;

        case PFG_R32_FLOAT:
        case PFG_R16_FLOAT:
        case PFG_R16_UNORM:
        case PFG_R16_SNORM:
        case PFG_R8_UNORM:
        case PFG_R8_SNORM:
        case PFG_A8_UNORM:
        case PFG_R1_UNORM:
            format = GL_RED;
            break;
        case PFG_R32_UINT:
        case PFG_R32_SINT:
        case PFG_R16_UINT:
        case PFG_R16_SINT:
        case PFG_R8_UINT:
        case PFG_R8_SINT:
            format = GL_RED_INTEGER;
            break;
        case PFG_RG32_FLOAT:
        case PFG_RG16_FLOAT:
        case PFG_RG16_UNORM:
        case PFG_RG16_SNORM:
        case PFG_RG8_UNORM:
        case PFG_RG8_SNORM:
            format = GL_RG;
            break;
        case PFG_RG32_UINT:
        case PFG_RG32_SINT:
        case PFG_RG16_UINT:
        case PFG_RG16_SINT:
        case PFG_RG8_UINT:
        case PFG_RG8_SINT:
            format = GL_RG_INTEGER;
            break;
        case PFG_RGB32_FLOAT:
        case PFG_R11G11B10_FLOAT:
        case PFG_B5G6R5_UNORM:
            format = GL_RGB;
            break;
        case PFG_RGB32_UINT:
        case PFG_RGB32_SINT:
            format = GL_RGB_INTEGER;
            break;
        case PFG_RGBA32_FLOAT:
        case PFG_RGBA16_FLOAT:
        case PFG_RGBA16_UNORM:
        case PFG_RGBA16_SNORM:
        case PFG_RGBA8_UNORM:
        case PFG_RGBA8_UNORM_SRGB:
        case PFG_RGBA8_SNORM:
        case PFG_R10G10B10A2_UNORM:
        case PFG_R9G9B9E5_SHAREDEXP:
        case PFG_R8G8_B8G8_UNORM:
        case PFG_R10G10B10_XR_BIAS_A2_UNORM:
            format = GL_RGBA;
            break;
        case PFG_RGBA32_UINT:
        case PFG_RGBA32_SINT:
        case PFG_RGBA16_UINT:
        case PFG_RGBA16_SINT:
        case PFG_RGBA8_UINT:
        case PFG_RGBA8_SINT:
        case PFG_R10G10B10A2_UINT:
            format = GL_RGBA_INTEGER;
            break;
        case PFG_G8R8_G8B8_UNORM:
        case PFG_BGRA8_UNORM:
        case PFG_BGRX8_UNORM:
        case PFG_BGRA8_UNORM_SRGB:
        case PFG_BGRX8_UNORM_SRGB:
        case PFG_B5G5R5A1_UNORM:
        case PFG_B4G4R4A4_UNORM:
            format = GL_BGRA;
            break;
        case PFG_AYUV:
        case PFG_Y410:
        case PFG_Y416:
        case PFG_NV12:
        case PFG_P010:
        case PFG_P016:
        case PFG_420_OPAQUE:
        case PFG_YUY2:
        case PFG_Y210:
        case PFG_Y216:
        case PFG_NV11:
        case PFG_AI44:
        case PFG_IA44:
        case PFG_P8:
        case PFG_A8P8:
        case PFG_P208:
        case PFG_V208:
        case PFG_V408:
            format = GL_NONE;
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Unsupported by OpenGL or unimplemented pixel format: " +
                         String( PixelFormatGpuUtils::toString( pixelFormat ) ),
                         "GL3PlusMappings::getFormatAndType" );
            break;
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
        case PFG_BC6H_UF16:
        case PFG_BC6H_SF16:
        case PFG_BC7_UNORM:
        case PFG_BC7_UNORM_SRGB:
        case PFG_PVRTC_RGB2:
        case PFG_PVRTC_RGBA2:
        case PFG_PVRTC_RGB4:
        case PFG_PVRTC_RGBA4:
        case PFG_PVRTC2_2BPP:
        case PFG_PVRTC2_4BPP:
        case PFG_PVRTC_RGB2_SRGB:
        case PFG_PVRTC_RGBA2_SRGB:
        case PFG_PVRTC_RGB4_SRGB:
        case PFG_PVRTC_RGBA4_SRGB:
        case PFG_PVRTC2_2BPP_SRGB:
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
            format = GL_NONE;
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR,
                         "This should never happen. Compressed formats must use "
                         "GL3PlusMappings::get instead. PixelFormat: " +
                         String( PixelFormatGpuUtils::toString( pixelFormat ) ),
                         "GL3PlusMappings::getFormatAndType" );
            break;
        case PFG_UNKNOWN:
        case PFG_NULL:
        case PFG_COUNT:
            format = GL_NONE;
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "PixelFormat invalid or not initialized. PixelFormat: " +
                         String( PixelFormatGpuUtils::toString( pixelFormat ) ),
                         "GL3PlusMappings::getFormatAndType" );
            break;
        }

        switch( pixelFormat )
        {
        case PFG_D32_FLOAT_S8X24_UINT:
            type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
            break;
        case PFG_D24_UNORM:
        case PFG_D24_UNORM_S8_UINT:
            type = GL_UNSIGNED_INT_24_8;
            break;
        case PFG_D32_FLOAT:
            type = GL_FLOAT;
            break;
        case PFG_D16_UNORM:
            type = GL_UNSIGNED_SHORT;
            break;

        case PFG_RGBA32_FLOAT:
        case PFG_RGB32_FLOAT:
        case PFG_RG32_FLOAT:
        case PFG_R32_FLOAT:
            type = GL_FLOAT;
            break;
        case PFG_RGBA16_FLOAT:
        case PFG_RG16_FLOAT:
        case PFG_R16_FLOAT:
            type = GL_HALF_FLOAT;
            break;
        case PFG_RGBA32_UINT:
        case PFG_RGB32_UINT:
        case PFG_RG32_UINT:
        case PFG_R32_UINT:
            type = GL_UNSIGNED_INT;
            break;
        case PFG_RGBA32_SINT:
        case PFG_RGB32_SINT:
        case PFG_RG32_SINT:
        case PFG_R32_SINT:
            type = GL_INT;
            break;
        case PFG_RGBA16_UNORM:
        case PFG_RGBA16_UINT:
        case PFG_RG16_UNORM:
        case PFG_RG16_UINT:
        case PFG_R16_UNORM:
        case PFG_R16_UINT:
            type = GL_UNSIGNED_SHORT;
            break;
        case PFG_RGBA16_SNORM:
        case PFG_RGBA16_SINT:
        case PFG_RG16_SNORM:
        case PFG_RG16_SINT:
        case PFG_R16_SNORM:
        case PFG_R16_SINT:
            type = GL_SHORT;
            break;

        case PFG_RGBA8_UNORM:
        case PFG_RGBA8_UNORM_SRGB:
        case PFG_RGBA8_UINT:
            type = GL_UNSIGNED_INT_8_8_8_8_REV;
            break;

        case PFG_RG8_UNORM:
        case PFG_RG8_UINT:
        case PFG_R8_UNORM:
        case PFG_R8_UINT:
        case PFG_A8_UNORM:
            type = GL_UNSIGNED_BYTE;
            break;

        case PFG_RGBA8_SNORM:
        case PFG_RGBA8_SINT:
        case PFG_RG8_SNORM:
        case PFG_RG8_SINT:
        case PFG_R8_SNORM:
        case PFG_R8_SINT:
            type = GL_BYTE;
            break;

        case PFG_R10G10B10A2_UNORM:
        case PFG_R10G10B10A2_UINT:
        case PFG_R10G10B10_XR_BIAS_A2_UNORM:
            type = GL_UNSIGNED_INT_2_10_10_10_REV;
            break;
        case PFG_R11G11B10_FLOAT:
            type = GL_UNSIGNED_INT_10F_11F_11F_REV;
            break;

        case PFG_R1_UNORM:
            format = GL_NONE;
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Unsupported by OpenGL or unimplemented pixel format: " +
                         String( PixelFormatGpuUtils::toString( pixelFormat ) ),
                         "GL3PlusMappings::getFormatAndType" );
            break;

        case PFG_R9G9B9E5_SHAREDEXP:
            type = GL_UNSIGNED_INT_5_9_9_9_REV_EXT;
            break;
        case PFG_R8G8_B8G8_UNORM:
        case PFG_G8R8_G8B8_UNORM:
            type = GL_UNSIGNED_BYTE;
            break;

        case PFG_B5G6R5_UNORM:
            type = GL_UNSIGNED_SHORT_5_6_5;
            break;
        case PFG_B5G5R5A1_UNORM:
            type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
            break;
        case PFG_BGRA8_UNORM:
        case PFG_BGRX8_UNORM:
        case PFG_BGRA8_UNORM_SRGB:
        case PFG_BGRX8_UNORM_SRGB:
            type = GL_UNSIGNED_INT_8_8_8_8_REV;
            break;
        case PFG_B4G4R4A4_UNORM:
            type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
            break;

        case PFG_AYUV:
        case PFG_Y410:
        case PFG_Y416:
        case PFG_NV12:
        case PFG_P010:
        case PFG_P016:
        case PFG_420_OPAQUE:
        case PFG_YUY2:
        case PFG_Y210:
        case PFG_Y216:
        case PFG_NV11:
        case PFG_AI44:
        case PFG_IA44:
        case PFG_P8:
        case PFG_A8P8:
        case PFG_P208:
        case PFG_V208:
        case PFG_V408:
            format = GL_NONE;
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Unsupported by OpenGL or unimplemented pixel format: " +
                         String( PixelFormatGpuUtils::toString( pixelFormat ) ),
                         "GL3PlusMappings::getFormatAndType" );
            break;
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
        case PFG_BC6H_UF16:
        case PFG_BC6H_SF16:
        case PFG_BC7_UNORM:
        case PFG_BC7_UNORM_SRGB:
        case PFG_PVRTC_RGB2:
        case PFG_PVRTC_RGBA2:
        case PFG_PVRTC_RGB4:
        case PFG_PVRTC_RGBA4:
        case PFG_PVRTC2_2BPP:
        case PFG_PVRTC2_4BPP:
        case PFG_PVRTC_RGB2_SRGB:
        case PFG_PVRTC_RGBA2_SRGB:
        case PFG_PVRTC_RGB4_SRGB:
        case PFG_PVRTC_RGBA4_SRGB:
        case PFG_PVRTC2_2BPP_SRGB:
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
            format = GL_NONE;
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR,
                         "This should never happen. Compressed formats must use "
                         "GL3PlusMappings::get instead. PixelFormat: " +
                         String( PixelFormatGpuUtils::toString( pixelFormat ) ),
                         "GL3PlusMappings::getFormatAndType" );
            break;
        case PFG_UNKNOWN:
        case PFG_NULL:
        case PFG_COUNT:
            format = GL_NONE;
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "PixelFormat invalid or not initialized. PixelFormat: " +
                         String( PixelFormatGpuUtils::toString( pixelFormat ) ),
                         "GL3PlusMappings::getFormatAndType" );
            break;
        }
    }
}
