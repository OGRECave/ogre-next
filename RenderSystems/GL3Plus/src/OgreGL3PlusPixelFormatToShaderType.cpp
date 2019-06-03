/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2016 Torus Knot Software Ltd

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

#include "OgreGL3PlusPixelFormatToShaderType.h"

#include "OgrePixelFormatGpuUtils.h"
#include "OgreTextureGpu.h"

namespace Ogre
{
    const char* GL3PlusPixelFormatToShaderType::getPixelFormatType( PixelFormatGpu pixelFormat ) const
    {
        switch( pixelFormat )
        {
        //UNORM formats
        case PFG_R8_UNORM:
        case PFG_A8_UNORM:
            return "r8";
        case PFG_R16_UNORM:
            return "r16";
        case PFG_RG8_UNORM:
            return "rg8";
        case PFG_RG16_UNORM:
            return "rg16";
        case PFG_RGBA8_UNORM:
        case PFG_RGBA8_UNORM_SRGB:
        case PFG_BGRA8_UNORM:
        case PFG_BGRA8_UNORM_SRGB:
        case PFG_BGRX8_UNORM:
        case PFG_BGRX8_UNORM_SRGB:
            return "rgba8";
        case PFG_R10G10B10A2_UNORM:
            return "rgb10_a2";
        case PFG_RGBA16_UNORM:
            return "rgba16";

        //SNORM formats
        case PFG_R8_SNORM:
            return "r8_snorm";
        case PFG_R16_SNORM:
            return "r16_snorm";
        case PFG_RG8_SNORM:
            return "rg8_snorm";
        case PFG_RG16_SNORM:
            return "rg16_snorm";
        case PFG_RGBA8_SNORM:
            return "rgba8_snorm";
        case PFG_RGBA16_SNORM:
            return "rgba16_snorm";

        //SINT formats
        case PFG_R8_SINT:
            return "r8i";
        case PFG_R16_SINT:
            return "r16i";
        case PFG_R32_SINT:
            return "r32i";
        case PFG_RG8_SINT:
            return "rg8i";
        case PFG_RG16_SINT:
            return "rg16i";
        case PFG_RG32_SINT:
            return "rg32i";
        case PFG_RGBA8_SINT:
            return "rgba8i";
        case PFG_RGBA16_SINT:
            return "rgba16i";
        case PFG_RGB32_SINT:
        case PFG_RGBA32_SINT:
            return "rgba32i";

        //UINT formats
        case PFG_R8_UINT:
            return "r8ui";
        case PFG_R16_UINT:
            return "r16ui";
        case PFG_R32_UINT:
            return "r32ui";
        case PFG_RG8_UINT:
            return "rg8ui";
        case PFG_RG16_UINT:
            return "rg16ui";
        case PFG_RG32_UINT:
            return "rg32ui";
        case PFG_RGBA8_UINT:
            return "rgba8ui";
        case PFG_R10G10B10A2_UINT:
            return "rgb10_a2ui";
        case PFG_RGBA16_UINT:
            return "rgba16ui";
        case PFG_RGB32_UINT:
        case PFG_RGBA32_UINT:
            return "rgba32ui";

        //FLOAT formats
        case PFG_R16_FLOAT:
            return "r16f";
        case PFG_R32_FLOAT:
            return "r32f";
        case PFG_RG16_FLOAT:
            return "rg16f";
        case PFG_RG32_FLOAT:
            return "rg32f";
        case PFG_R11G11B10_FLOAT:
            return "r11f_g11f_b10f";
        case PFG_RGBA16_FLOAT:
            return "rgba16f";
        case PFG_RGB32_FLOAT:
        case PFG_RGBA32_FLOAT:
            return "rgba32f";
        default:
            return 0;
        }

        return 0;
    }
    //-------------------------------------------------------------------------
    const char* GL3PlusPixelFormatToShaderType::getDataType( PixelFormatGpu pixelFormat,
                                                             uint32 _textureType, bool isMsaa,
                                                             ResourceAccess::
                                                             ResourceAccess access ) const
    {
        const bool bIsInteger = PixelFormatGpuUtils::isInteger( pixelFormat );
        TextureTypes::TextureTypes textureType = static_cast<TextureTypes::TextureTypes>( _textureType );

        if( !bIsInteger )
        {
            switch( textureType )
            {
            case TextureTypes::Type1D:
                return "image1D";
            case TextureTypes::Type1DArray:
                return "image1DArray";
            case TextureTypes::Type2D:
            case TextureTypes::Unknown:
                return isMsaa ? "image2DMS" : "image2D";
            case TextureTypes::Type2DArray:
                return isMsaa ? "image2DMSArray" : "image2DArray";
            case TextureTypes::TypeCube:
                return "imageCube";
            case TextureTypes::TypeCubeArray:
                return "imageCubeArray";
            case TextureTypes::Type3D:
                return "image3D";
            }
        }
        else
        {
            const bool bIsSigned = PixelFormatGpuUtils::isSigned( pixelFormat );
            if( !bIsSigned )
            {
                switch( textureType )
                {
                case TextureTypes::Type1D:
                    return "uimage1D";
                case TextureTypes::Type1DArray:
                    return "uimage1DArray";
                case TextureTypes::Type2D:
                case TextureTypes::Unknown:
                    return isMsaa ? "uimage2DMS" : "uimage2D";
                case TextureTypes::Type2DArray:
                    return isMsaa ? "uimage2DMSArray" : "uimage2DArray";
                case TextureTypes::TypeCube:
                    return "uimageCube";
                case TextureTypes::TypeCubeArray:
                    return "uimageCubeArray";
                case TextureTypes::Type3D:
                    return "uimage3D";
                }
            }
            else
            {
                switch( textureType )
                {
                case TextureTypes::Type1D:
                    return "iimage1D";
                case TextureTypes::Type1DArray:
                    return "iimage1DArray";
                case TextureTypes::Type2D:
                case TextureTypes::Unknown:
                    return isMsaa ? "iimage2DMS" : "iimage2D";
                case TextureTypes::Type2DArray:
                    return isMsaa ? "iimage2DMSArray" : "iimage2DArray";
                case TextureTypes::TypeCube:
                    return "iimageCube";
                case TextureTypes::TypeCubeArray:
                    return "iimageCubeArray";
                case TextureTypes::Type3D:
                    return "iimage3D";
                }
            }
        }

        return 0;
    }
}
