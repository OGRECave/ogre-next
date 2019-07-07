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
#ifndef _OgrePixelFormatGpu_H_
#define _OgrePixelFormatGpu_H_

#include "OgrePrerequisites.h"
#include "OgreResourceTransition.h"

#include "OgreHeaderPrefix.h"

namespace Ogre {
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Image
    *  @{
    */
    /** The pixel format used for images, textures, and render surfaces */
    enum PixelFormatGpu
    {
        PFG_UNKNOWN,
        PFG_NULL,

        // Starting Here, start D3D11 formats (it isn't 1:1 with DXGI_FORMAT_* though)

        PFG_RGBA32_FLOAT,
        PFG_RGBA32_UINT,
        PFG_RGBA32_SINT,

        PFG_RGB32_FLOAT,
        PFG_RGB32_UINT,
        PFG_RGB32_SINT,

        PFG_RGBA16_FLOAT,
        PFG_RGBA16_UNORM,
        PFG_RGBA16_UINT,
        PFG_RGBA16_SNORM,
        PFG_RGBA16_SINT,

        PFG_RG32_FLOAT,
        PFG_RG32_UINT,
        PFG_RG32_SINT,

        PFG_D32_FLOAT_S8X24_UINT,

		PFG_R10G10B10A2_UNORM,
        PFG_R10G10B10A2_UINT,
        PFG_R11G11B10_FLOAT,

        PFG_RGBA8_UNORM,
        PFG_RGBA8_UNORM_SRGB,
        PFG_RGBA8_UINT,
        PFG_RGBA8_SNORM,
        PFG_RGBA8_SINT,

        PFG_RG16_FLOAT,
        PFG_RG16_UNORM,
        PFG_RG16_UINT,
        PFG_RG16_SNORM,
        PFG_RG16_SINT,

        PFG_D32_FLOAT,
        PFG_R32_FLOAT,
        PFG_R32_UINT,
        PFG_R32_SINT,

        PFG_D24_UNORM,
        PFG_D24_UNORM_S8_UINT,

        PFG_RG8_UNORM,
        PFG_RG8_UINT,
        PFG_RG8_SNORM,
        PFG_RG8_SINT,

        PFG_R16_FLOAT,
        PFG_D16_UNORM,
        PFG_R16_UNORM,
        PFG_R16_UINT,
        PFG_R16_SNORM,
        PFG_R16_SINT,

        PFG_R8_UNORM,
        PFG_R8_UINT,
        PFG_R8_SNORM,
        PFG_R8_SINT,
        PFG_A8_UNORM,
        PFG_R1_UNORM,
        PFG_R9G9B9E5_SHAREDEXP,
        /// D3D11 only. A four-component, 32-bit unsigned-normalized-integer format.
        /// This packed RGB format is analogous to the UYVY format.
        /// Each 32-bit block describes a pair of pixels: (R8, G8, B8) and (R8, G8, B8)
        /// where the R8/B8 values are repeated, and the G8 values are unique to each pixel.
        PFG_R8G8_B8G8_UNORM,
        /// D3D11 only. See PFG_R8G8_B8G8_UNORM
        PFG_G8R8_G8B8_UNORM,

        /// BC1, aka DXT1 & DXT2
        PFG_BC1_UNORM,
        PFG_BC1_UNORM_SRGB,

        /// BC2, aka DXT3 & DXT4
        PFG_BC2_UNORM,
        PFG_BC2_UNORM_SRGB,

        /// BC3, aka DXT5
        PFG_BC3_UNORM,
        PFG_BC3_UNORM_SRGB,

        /// One channel compressed 4bpp. Ideal for greyscale.
        PFG_BC4_UNORM,
        /// Two channels compressed 8bpp. Ideal for normal maps or greyscale + alpha.
        PFG_BC4_SNORM,

        PFG_BC5_UNORM,
        PFG_BC5_SNORM,

        PFG_B5G6R5_UNORM,
        PFG_B5G5R5A1_UNORM,
        /// Avoid this one (prefer RGBA8).
        PFG_BGRA8_UNORM,
        /// Avoid this one (prefer RGBA8).
        PFG_BGRX8_UNORM,
        PFG_R10G10B10_XR_BIAS_A2_UNORM,

        /// Avoid this one (prefer RGBA8).
        PFG_BGRA8_UNORM_SRGB,
        /// Avoid this one (prefer RGBA8).
        PFG_BGRX8_UNORM_SRGB,

        /// BC6H format (unsigned 16 bit float)
        PFG_BC6H_UF16,
        /// BC6H format (signed 16 bit float)
        PFG_BC6H_SF16,

        PFG_BC7_UNORM,
        PFG_BC7_UNORM_SRGB,

        PFG_AYUV,
        PFG_Y410,
        PFG_Y416,
        PFG_NV12,
        PFG_P010,
        PFG_P016,
        PFG_420_OPAQUE,
        PFG_YUY2,
        PFG_Y210,
        PFG_Y216,
        PFG_NV11,
        PFG_AI44,
        PFG_IA44,
        PFG_P8,
        PFG_A8P8,
        PFG_B4G4R4A4_UNORM,
        PFG_P208,
        PFG_V208,
        PFG_V408,

        // Here ends D3D11 formats (it isn't 1:1 with DXGI_FORMAT_* though)

        /// PVRTC (PowerVR) RGB 2 bpp
        PFG_PVRTC_RGB2,
        PFG_PVRTC_RGB2_SRGB,
        /// PVRTC (PowerVR) RGBA 2 bpp
        PFG_PVRTC_RGBA2,
        PFG_PVRTC_RGBA2_SRGB,
        /// PVRTC (PowerVR) RGB 4 bpp
        PFG_PVRTC_RGB4,
        PFG_PVRTC_RGB4_SRGB,
        /// PVRTC (PowerVR) RGBA 4 bpp
        PFG_PVRTC_RGBA4,
        PFG_PVRTC_RGBA4_SRGB,
        /// PVRTC (PowerVR) Version 2, 2 bpp
        PFG_PVRTC2_2BPP,
        PFG_PVRTC2_2BPP_SRGB,
        /// PVRTC (PowerVR) Version 2, 4 bpp
        PFG_PVRTC2_4BPP,
        PFG_PVRTC2_4BPP_SRGB,

        /// ETC1 (Ericsson Texture Compression)
        PFG_ETC1_RGB8_UNORM,
        /// ETC2 (Ericsson Texture Compression). Mandatory in GLES 3.0
        PFG_ETC2_RGB8_UNORM,
        PFG_ETC2_RGB8_UNORM_SRGB,
        PFG_ETC2_RGBA8_UNORM,
        PFG_ETC2_RGBA8_UNORM_SRGB,
        PFG_ETC2_RGB8A1_UNORM,
        PFG_ETC2_RGB8A1_UNORM_SRGB,
        /// EAC compression (built on top of ETC2) Mandatory in GLES 3.0 for 1 channel & 2 channels
        PFG_EAC_R11_UNORM,
        PFG_EAC_R11_SNORM,
        PFG_EAC_R11G11_UNORM,
        PFG_EAC_R11G11_SNORM,

        /// ATC (AMD_compressed_ATC_texture)
        PFG_ATC_RGB,
        /// ATC (AMD_compressed_ATC_texture)
        PFG_ATC_RGBA_EXPLICIT_ALPHA,
        /// ATC (AMD_compressed_ATC_texture)
        PFG_ATC_RGBA_INTERPOLATED_ALPHA,

        /// ASTC (ARM Adaptive Scalable Texture Compression RGBA, block size 4x4)
        PFG_ASTC_RGBA_UNORM_4X4_LDR,
        PFG_ASTC_RGBA_UNORM_5X4_LDR,
        PFG_ASTC_RGBA_UNORM_5X5_LDR,
        PFG_ASTC_RGBA_UNORM_6X5_LDR,
        PFG_ASTC_RGBA_UNORM_6X6_LDR,
        PFG_ASTC_RGBA_UNORM_8X5_LDR,
        PFG_ASTC_RGBA_UNORM_8X6_LDR,
        PFG_ASTC_RGBA_UNORM_8X8_LDR,
        PFG_ASTC_RGBA_UNORM_10X5_LDR,
        PFG_ASTC_RGBA_UNORM_10X6_LDR,
        PFG_ASTC_RGBA_UNORM_10X8_LDR,
        PFG_ASTC_RGBA_UNORM_10X10_LDR,
        PFG_ASTC_RGBA_UNORM_12X10_LDR,
        PFG_ASTC_RGBA_UNORM_12X12_LDR,

        /// ASTC (ARM Adaptive Scalable Texture Compression RGBA_UNORM sRGB, block size 4x4)
        PFG_ASTC_RGBA_UNORM_4X4_sRGB,
        PFG_ASTC_RGBA_UNORM_5X4_sRGB,
        PFG_ASTC_RGBA_UNORM_5X5_sRGB,
        PFG_ASTC_RGBA_UNORM_6X5_sRGB,
        PFG_ASTC_RGBA_UNORM_6X6_sRGB,
        PFG_ASTC_RGBA_UNORM_8X5_sRGB,
        PFG_ASTC_RGBA_UNORM_8X6_sRGB,
        PFG_ASTC_RGBA_UNORM_8X8_sRGB,
        PFG_ASTC_RGBA_UNORM_10X5_sRGB,
        PFG_ASTC_RGBA_UNORM_10X6_sRGB,
        PFG_ASTC_RGBA_UNORM_10X8_sRGB,
        PFG_ASTC_RGBA_UNORM_10X10_sRGB,
        PFG_ASTC_RGBA_UNORM_12X10_sRGB,
        PFG_ASTC_RGBA_UNORM_12X12_sRGB,

		PFG_COUNT
    };

    class _OgreExport PixelFormatToShaderType
    {
    public:
        /** Converts a PixelFormat into its equivalent layout for image variables (GLSL)
            or its equivalent for D3D11/12 variables (HLSL). Used mostly with UAVs.
        @param pixelFormat
            Pixel format to convert.
        @return
            String for the shader to use "as is". If the Pixel Format doesn't have
            a shader equivalent (i.e. depth formats), a null pointer is returned.
            The validity of the pointer lasts as long as the RenderSystem remains
            loaded.
        */
        virtual const char* getPixelFormatType( PixelFormatGpu pixelFormat ) const = 0;

        /**
        @param pixelFormat
        @param textureType
            See TextureTypes::TextureTypes
        @param isMsaa
        @param access
            Texture access. Use ResourceAccess::Undefined for requesting sampling mode
        @return
            String for the shader to use "as is". Returned pointer may be null.
            The validity of the pointer lasts as long as the RenderSystem remains
            loaded.
        */
        virtual const char* getDataType( PixelFormatGpu pixelFormat, uint32 textureType,
                                         bool isMsaa, ResourceAccess::ResourceAccess access ) const = 0;
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
