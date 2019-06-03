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

#include "OgreD3D11PixelFormatToShaderType.h"

#include "OgrePixelFormatGpuUtils.h"
#include "OgreTextureGpu.h"

#include "OgreD3D11PixelFormatToShaderType.inl"

namespace Ogre
{
    static const char c_pixelFormatTypes[PixelFormatDataTypes::NumPixelFormatDataTypes][16] =
    {
        { "unorm float" },
        { "unorm float2" },
        { "unorm float3" },
        { "unorm float4" },

        { "snorm float" },
        { "snorm float2" },
        { "snorm float4" },

        { "sint" },
        { "sint2" },
        { "sint3" },
        { "sint4" },

        { "uint" },
        { "uint2" },
        { "uint3" },
        { "uint4" },

        { "float" },
        { "float2" },
        { "float3" },
        { "float4" }
    };

    PixelFormatDataTypes::PixelFormatDataTypes D3D11PixelFormatToShaderType::getPixelFormatDataType(
            PixelFormatGpu pixelFormat )
    {
        switch( pixelFormat )
        {
        //UNORM formats
        case PFG_D24_UNORM:
        case PFG_D24_UNORM_S8_UINT:
        case PFG_D16_UNORM:
        case PFG_R16_UNORM:
        case PFG_R8_UNORM:
        case PFG_BC4_UNORM:
            return PixelFormatDataTypes::UnormFloat;
        case PFG_RG16_UNORM:
        case PFG_RG8_UNORM:
        case PFG_BC5_UNORM:
            return PixelFormatDataTypes::UnormFloat2;
        case PFG_B5G6R5_UNORM:
            return PixelFormatDataTypes::UnormFloat3;
        case PFG_RGBA16_UNORM:
        case PFG_R10G10B10A2_UNORM:
        case PFG_RGBA8_UNORM:
        case PFG_RGBA8_UNORM_SRGB:
        case PFG_A8_UNORM:
        case PFG_R1_UNORM:
        case PFG_R8G8_B8G8_UNORM:
        case PFG_G8R8_G8B8_UNORM:
        case PFG_BC1_UNORM:
        case PFG_BC1_UNORM_SRGB:
        case PFG_BC2_UNORM:
        case PFG_BC2_UNORM_SRGB:
        case PFG_BC3_UNORM:
        case PFG_BC3_UNORM_SRGB:
        case PFG_B5G5R5A1_UNORM:
        case PFG_BGRA8_UNORM:
        case PFG_BGRX8_UNORM:
        case PFG_R10G10B10_XR_BIAS_A2_UNORM:
        case PFG_BGRA8_UNORM_SRGB:
        case PFG_BGRX8_UNORM_SRGB:
        case PFG_BC7_UNORM:
        case PFG_BC7_UNORM_SRGB:
        case PFG_B4G4R4A4_UNORM:
            return PixelFormatDataTypes::UnormFloat4;

        //SNORM formats
        case PFG_R16_SNORM:
        case PFG_R8_SNORM:
        case PFG_BC4_SNORM:
            return PixelFormatDataTypes::SnormFloat;
        case PFG_RG16_SNORM:
        case PFG_RG8_SNORM:
        case PFG_BC5_SNORM:
            return PixelFormatDataTypes::SnormFloat2;
        case PFG_RGBA16_SNORM:
        case PFG_RGBA8_SNORM:
            return PixelFormatDataTypes::SnormFloat4;

        //SINT formats
        case PFG_R32_SINT:
        case PFG_R16_SINT:
        case PFG_R8_SINT:
            return PixelFormatDataTypes::Sint;
        case PFG_RG32_SINT:
        case PFG_RG16_SINT:
        case PFG_RG8_SINT:
            return PixelFormatDataTypes::Sint2;
        case PFG_RGB32_SINT:
            return PixelFormatDataTypes::Sint3;
        case PFG_RGBA32_SINT:
        case PFG_RGBA16_SINT:
        case PFG_RGBA8_SINT:
            return PixelFormatDataTypes::Sint4;

        //UINT formats
        case PFG_R32_UINT:
        case PFG_R16_UINT:
        case PFG_R8_UINT:
            return PixelFormatDataTypes::Uint;
        case PFG_RG32_UINT:
        case PFG_RG16_UINT:
        case PFG_RG8_UINT:
            return PixelFormatDataTypes::Uint2;
        case PFG_RGB32_UINT:
            return PixelFormatDataTypes::Uint3;
        case PFG_RGBA32_UINT:
        case PFG_RGBA16_UINT:
        case PFG_R10G10B10A2_UINT:
        case PFG_RGBA8_UINT:
            return PixelFormatDataTypes::Uint4;

        //FLOAT formats
        case PFG_D32_FLOAT_S8X24_UINT:
        case PFG_D32_FLOAT:
        case PFG_R32_FLOAT:
        case PFG_R16_FLOAT:
            return PixelFormatDataTypes::Float;
        case PFG_RG32_FLOAT:
        case PFG_RG16_FLOAT:
            return PixelFormatDataTypes::Float2;
        case PFG_RGB32_FLOAT:
            return PixelFormatDataTypes::Float3;
        case PFG_RGBA32_FLOAT:
        case PFG_RGBA16_FLOAT:
        case PFG_R11G11B10_FLOAT:
        case PFG_BC6H_UF16:
        case PFG_BC6H_SF16:
            return PixelFormatDataTypes::Float4;

        default:
            return PixelFormatDataTypes::NumPixelFormatDataTypes;
        }

        return PixelFormatDataTypes::NumPixelFormatDataTypes;
    }
    //-------------------------------------------------------------------------
    const char* D3D11PixelFormatToShaderType::getPixelFormatType( PixelFormatGpu pixelFormat ) const
    {
        PixelFormatDataTypes::PixelFormatDataTypes pfDataType = getPixelFormatDataType( pixelFormat );

        if( pfDataType == PixelFormatDataTypes::NumPixelFormatDataTypes )
            return 0;

        return c_pixelFormatTypes[pfDataType];
    }
    //-------------------------------------------------------------------------
    const char* D3D11PixelFormatToShaderType::getDataType( PixelFormatGpu pixelFormat,
                                                           uint32 textureType, bool isMsaa,
                                                           ResourceAccess::ResourceAccess access ) const
    {
        if( textureType == TextureTypes::Unknown )
            textureType = TextureTypes::Type2D;

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

        PixelFormatDataTypes::PixelFormatDataTypes pfDataType = getPixelFormatDataType( pixelFormat );
        if( pfDataType == PixelFormatDataTypes::NumPixelFormatDataTypes )
            return 0;

        size_t dataTypeIdx = textureType * PixelFormatDataTypes::NumPixelFormatDataTypes + pfDataType;
        return c_dataTypes[dataTypeIdx];
    }
}
