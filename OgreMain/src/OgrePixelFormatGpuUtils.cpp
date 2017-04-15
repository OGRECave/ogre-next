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

#include "OgrePixelFormatGpuUtils.h"
#include "OgreCommon.h"
#include "OgreException.h"

namespace Ogre
{
    inline const PixelFormatGpuUtils::PixelFormatDesc& PixelFormatGpuUtils::getDescriptionFor(
            const PixelFormatGpu fmt )
    {
        const int idx = (int)fmt;
        assert( idx >=0 && idx < PFG_COUNT );

        return msPixelFormatDesc[idx];
    }
    //-----------------------------------------------------------------------------------
    size_t PixelFormatGpuUtils::getBytesPerPixel( PixelFormatGpu format )
    {
        const PixelFormatDesc &desc = getDescriptionFor( format );
        return desc.bytesPerPixel;
    }
    //-----------------------------------------------------------------------------------
    size_t PixelFormatGpuUtils::getSizeBytes( uint32 width, uint32 height, uint32 depth,
                                              uint32 slices, PixelFormatGpu format,
                                              uint32 rowAlignment )
    {
        if( !isCompressed( format ) )
        {
            size_t retVal = width * getBytesPerPixel( format );
            retVal = alignToNextMultiple( retVal, rowAlignment );

            retVal *= height * depth * slices;

            return retVal;
        }
        else
        {
            switch( format )
            {
            // BCn formats work by dividing the image into 4x4 blocks, then
            // encoding each 4x4 block with a certain number of bytes.
            case PFG_BC1_UNORM:
            case PFG_BC1_UNORM_SRGB:
            case PFG_BC4_UNORM:
            case PFG_BC4_SNORM:
            case PFG_EAC_R11_UNORM:
            case PFG_EAC_R11_SNORM:
            case PFG_ETC1_RGB8_UNORM:
            case PFG_ETC2_RGB8_UNORM_SRGB:
            case PFG_ETC2_RGB8A1_UNORM:
            case PFG_ETC2_RGB8A1_UNORM_SRGB:
            case PFG_ATC_RGB:
                return ( (width + 3u) / 4u ) * ( (height + 3u) / 4u ) * 8u * depth * slices;
            case PFG_BC2_UNORM:
            case PFG_BC2_UNORM_SRGB:
            case PFG_BC3_UNORM:
            case PFG_BC3_UNORM_SRGB:
            case PFG_BC5_SNORM:
            case PFG_BC5_UNORM:
            case PFG_BC6H_SF16:
            case PFG_BC6H_UF16:
            case PFG_BC7_UNORM:
            case PFG_BC7_UNORM_SRGB:
            case PFG_ETC2_RGBA8_UNORM:
            case PFG_ETC2_RGBA8_UNORM_SRGB:
            case PFG_EAC_R11G11_UNORM:
            case PFG_EAC_R11G11_SNORM:
            case PFG_ATC_RGBA_EXPLICIT_ALPHA:
            case PFG_ATC_RGBA_INTERPOLATED_ALPHA:
                return ( (width + 3u) / 4u) * ( (height + 3u) / 4u ) * 16u * depth * slices;
            // Size calculations from the PVRTC OpenGL extension spec
            // http://www.khronos.org/registry/gles/extensions/IMG/IMG_texture_compression_pvrtc.txt
            // Basically, 32 bytes is the minimum texture size.  Smaller textures are padded up to 32 bytes
            case PFG_PVRTC_RGB2:
            case PFG_PVRTC_RGBA2:
            case PFG_PVRTC2_2BPP:
                return (std::max<uint32>( width, 16u ) * std::max<uint32>( height, 8u ) * 2u + 7u) / 8u
                        * depth * slices;
            case PFG_PVRTC_RGB4:
            case PFG_PVRTC_RGBA4:
            case PFG_PVRTC2_4BPP:
                return (std::max<uint32>( width, 8u ) * std::max<uint32>( height, 8u ) * 4u + 7u) / 8u
                        * depth * slices;
            default:
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Invalid compressed pixel format",
                             "PixelFormatGpuUtils::getSizeBytes" );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    const char* PixelFormatGpuUtils::toString( PixelFormatGpu format )
    {
        const PixelFormatDesc &desc = getDescriptionFor( format );
        return desc.name;
    }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu PixelFormatGpuUtils::getFormatFromName( const String &name, uint32 exclusionFlags )
    {
        return getFormatFromName( name.c_str(), exclusionFlags );
    }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu PixelFormatGpuUtils::getFormatFromName( const char *name, uint32 exclusionFlags )
    {
        for( int i=0; i<PFG_COUNT; ++i )
        {
            PixelFormatGpu format = static_cast<PixelFormatGpu>(i);

            const PixelFormatDesc &desc = getDescriptionFor( format );

            if( (desc.flags & exclusionFlags) == 0 )
            {
                if( strcmp( name, desc.name ) == 0 )
                    return format;
            }
        }

        return PFG_UNKNOWN;
    }
    //-----------------------------------------------------------------------------------
    uint32 PixelFormatGpuUtils::getFlags( PixelFormatGpu format )
    {
        const PixelFormatDesc &desc = getDescriptionFor( format );
        return desc.flags;
    }
    //-----------------------------------------------------------------------------------
    bool PixelFormatGpuUtils::isFloat( PixelFormatGpu format )
    {
        const PixelFormatDesc &desc = getDescriptionFor( format );
        return desc.flags & PFF_FLOAT;
    }
    //-----------------------------------------------------------------------------------
    bool PixelFormatGpuUtils::isHalf( PixelFormatGpu format )
    {
        const PixelFormatDesc &desc = getDescriptionFor( format );
        return desc.flags & PFF_HALF;
    }
    //-----------------------------------------------------------------------------------
    bool PixelFormatGpuUtils::isFloatRare( PixelFormatGpu format )
    {
        const PixelFormatDesc &desc = getDescriptionFor( format );
        return desc.flags & PFF_FLOAT_RARE;
    }
    //-----------------------------------------------------------------------------------
    bool PixelFormatGpuUtils::isInteger( PixelFormatGpu format )
    {
        const PixelFormatDesc &desc = getDescriptionFor( format );
        return desc.flags & PFF_INTEGER;
    }
    //-----------------------------------------------------------------------------------
    bool PixelFormatGpuUtils::isNormalized( PixelFormatGpu format )
    {
        const PixelFormatDesc &desc = getDescriptionFor( format );
        return desc.flags & PFF_NORMALIZED;
    }
    //-----------------------------------------------------------------------------------
    bool PixelFormatGpuUtils::isSigned( PixelFormatGpu format )
    {
        const PixelFormatDesc &desc = getDescriptionFor( format );
        return desc.flags & PFF_SIGNED;
    }
    //-----------------------------------------------------------------------------------
    bool PixelFormatGpuUtils::isDepth( PixelFormatGpu format )
    {
        const PixelFormatDesc &desc = getDescriptionFor( format );
        return desc.flags & PFF_DEPTH;
    }
    //-----------------------------------------------------------------------------------
    bool PixelFormatGpuUtils::isStencil( PixelFormatGpu format )
    {
        const PixelFormatDesc &desc = getDescriptionFor( format );
        return desc.flags & PFF_STENCIL;
    }
    //-----------------------------------------------------------------------------------
    bool PixelFormatGpuUtils::isSRgb( PixelFormatGpu format )
    {
        const PixelFormatDesc &desc = getDescriptionFor( format );
        return desc.flags & PFF_SRGB;
    }
    //-----------------------------------------------------------------------------------
    bool PixelFormatGpuUtils::isCompressed( PixelFormatGpu format )
    {
        const PixelFormatDesc &desc = getDescriptionFor( format );
        return desc.flags & PFF_COMPRESSED;
    }
    //-----------------------------------------------------------------------------------
    bool PixelFormatGpuUtils::isPallete( PixelFormatGpu format )
    {
        const PixelFormatDesc &desc = getDescriptionFor( format );
        return desc.flags & PFF_PALLETE;
    }

    static const uint32 PFF_COMPRESSED_COMMON = PixelFormatGpuUtils::PFF_COMPRESSED|
                                                PixelFormatGpuUtils::PFF_INTEGER|
                                                PixelFormatGpuUtils::PFF_NORMALIZED;

    PixelFormatGpuUtils::PixelFormatDesc PixelFormatGpuUtils::msPixelFormatDesc[PFG_COUNT + 1u] =
    {
        {"PFG_UNKNOWN", 0, 0 },
        {"PFG_RGBA32_FLOAT",		4u * sizeof(uint32),	PFF_FLOAT },
        {"PFG_RGBA32_UINT",			4u * sizeof(uint32),	PFF_INTEGER },
        {"PFG_RGBA32_INT",			4u * sizeof(uint32),	PFF_INTEGER|PFF_SIGNED },

        {"PFG_RGB32_FLOAT",			4u * sizeof(uint32),	PFF_FLOAT },
        {"PFG_RGB32_UINT",			4u * sizeof(uint32),	PFF_INTEGER },
        {"PFG_RGB32_INT",			4u * sizeof(uint32),	PFF_INTEGER|PFF_SIGNED },

        {"PFG_RGBA16_FLOAT",		4u * sizeof(uint16),	PFF_HALF },
        {"PFG_RGBA16_UNORM",		4u * sizeof(uint16),	PFF_INTEGER|PFF_NORMALIZED },
        {"PFG_RGBA16_UINT",			4u * sizeof(uint16),	PFF_INTEGER },
        {"PFG_RGBA16_SNORM",		4u * sizeof(uint16),	PFF_INTEGER|PFF_SIGNED|PFF_NORMALIZED },
        {"PFG_RGBA16_SINT",			4u * sizeof(uint16),	PFF_INTEGER|PFF_SIGNED },

        {"PFG_RG32_FLOAT",			2u * sizeof(uint32),	PFF_FLOAT },
        {"PFG_RG32_UINT",			2u * sizeof(uint32),	PFF_INTEGER },
        {"PFG_RG32_SINT",			2u * sizeof(uint32),	PFF_INTEGER|PFF_SIGNED },

        {"PFG_D32_FLOAT_S8X24_UINT",2u * sizeof(uint32),	PFF_FLOAT|PFF_DEPTH|PFF_STENCIL },

        {"PFG_R10G10B10A2_UNORM",	1u * sizeof(uint32),	PFF_INTEGER|PFF_NORMALIZED },
        {"PFG_R10G10B10A2_UINT",	1u * sizeof(uint32),	PFF_INTEGER },
        {"PFG_R11G11B10_FLOAT",		1u * sizeof(uint32),	PFF_FLOAT_RARE },

        {"PFG_RGBA8_UNORM",			4u * sizeof(uint8),		PFF_INTEGER|PFF_NORMALIZED },
        {"PFG_RGBA8_UNORM_SRGB",	4u * sizeof(uint8),		PFF_INTEGER|PFF_NORMALIZED|PFF_SRGB },
        {"PFG_RGBA8_UINT",			4u * sizeof(uint8),		PFF_INTEGER },
        {"PFG_RGBA8_SNORM",			4u * sizeof(uint8),		PFF_INTEGER|PFF_SIGNED|PFF_NORMALIZED },
        {"PFG_RGBA8_SINT",			4u * sizeof(uint8),		PFF_INTEGER|PFF_SIGNED },

        {"PFG_RG16_FLOAT",			2u * sizeof(uint16),	PFF_HALF },
        {"PFG_RG16_UNORM",			2u * sizeof(uint16),	PFF_INTEGER|PFF_NORMALIZED },
        {"PFG_RG16_UINT",			2u * sizeof(uint16),	PFF_INTEGER },
        {"PFG_RG16_SNORM",			2u * sizeof(uint16),	PFF_INTEGER|PFF_SIGNED|PFF_NORMALIZED },
        {"PFG_RG16_SINT",			2u * sizeof(uint16),	PFF_INTEGER|PFF_SIGNED },

        {"PFG_D32_FLOAT",			1u * sizeof(uint32),	PFF_FLOAT|PFF_DEPTH },
        {"PFG_R32_FLOAT",			1u * sizeof(uint32),	PFF_FLOAT },
        {"PFG_R32_UINT",			1u * sizeof(uint32),	PFF_INTEGER },
        {"PFG_R32_SINT",			1u * sizeof(uint32),	PFF_INTEGER|PFF_SIGNED },

        {"PFG_D24_UNORM",			1u * sizeof(uint32),	PFF_INTEGER|PFF_NORMALIZED|PFF_DEPTH },
        {"PFG_D24_UNORM_S8_UINT",	1u * sizeof(uint32),	PFF_INTEGER|PFF_NORMALIZED|
                                                            PFF_DEPTH|PFF_STENCIL },

        {"PFG_RG8_UNORM",			2u * sizeof(uint8),		PFF_INTEGER|PFF_NORMALIZED },
        {"PFG_RG8_UINT",			2u * sizeof(uint8),		PFF_INTEGER },
        {"PFG_RG8_SNORM",			2u * sizeof(uint8),		PFF_INTEGER|PFF_SIGNED|PFF_NORMALIZED },
        {"PFG_RG8_SINT",			2u * sizeof(uint8),		PFF_INTEGER|PFF_SIGNED },

        {"PFG_R16_FLOAT",			1u * sizeof(uint16),	PFF_HALF },
        {"PFG_D16_UNORM",			1u * sizeof(uint16),	PFF_INTEGER|PFF_NORMALIZED|PFF_DEPTH },
        {"PFG_R16_UNORM",			1u * sizeof(uint16),	PFF_INTEGER|PFF_NORMALIZED },
        {"PFG_R16_UINT",			1u * sizeof(uint16),	PFF_INTEGER },
        {"PFG_R16_SNORM",			1u * sizeof(uint16),	PFF_INTEGER|PFF_SIGNED|PFF_NORMALIZED },
        {"PFG_R16_SINT",			1u * sizeof(uint16),	PFF_INTEGER|PFF_SIGNED },

        {"PFG_R8_UNORM",			1u * sizeof(uint8),		PFF_INTEGER|PFF_NORMALIZED },
        {"PFG_R8_UINT",				1u * sizeof(uint8),		PFF_INTEGER },
        {"PFG_R8_SNORM",			1u * sizeof(uint8),		PFF_INTEGER|PFF_SIGNED|PFF_NORMALIZED },
        {"PFG_R8_SINT",				1u * sizeof(uint8),		PFF_INTEGER|PFF_SIGNED },
        {"PFG_A8_UNORM",			1u * sizeof(uint8),		PFF_INTEGER|PFF_NORMALIZED },
        {"PFG_R1_UNORM",			0,						0 }, // ???

        {"PFG_R9G9B9E5_SHAREDEXP",	1u * sizeof(uint32),	PFF_FLOAT_RARE },

        {"PFG_R8G8_B8G8_UNORM",		4u * sizeof(uint8),		PFF_INTEGER|PFF_NORMALIZED },
        {"PFG_G8R8_G8B8_UNORM",		4u * sizeof(uint8),		PFF_INTEGER|PFF_SIGNED|PFF_NORMALIZED },

        {"PFG_BC1_UNORM",			0,						PFF_COMPRESSED_COMMON },
        {"PFG_BC1_UNORM_SRGB",		0,						PFF_COMPRESSED_COMMON|PFF_SRGB },

        {"PFG_BC2_UNORM",			0,						PFF_COMPRESSED_COMMON },
        {"PFG_BC2_UNORM_SRGB",		0,						PFF_COMPRESSED_COMMON|PFF_SRGB },

        {"PFG_BC3_UNORM",			0,						PFF_COMPRESSED_COMMON },
        {"PFG_BC3_UNORM_SRGB",		0,						PFF_COMPRESSED_COMMON|PFF_SRGB },

        {"PFG_BC4_UNORM",			0,						PFF_COMPRESSED_COMMON },
        {"PFG_BC4_SNORM",			0,						PFF_COMPRESSED_COMMON|PFF_SIGNED },

        {"PFG_BC5_UNORM",			0,						PFF_COMPRESSED_COMMON },
        {"PFG_BC5_SNORM",			0,						PFF_COMPRESSED_COMMON|PFF_SIGNED },

        {"PFG_B5G6R5_UNORM",		1u * sizeof(uint16),	PFF_INTEGER|PFF_NORMALIZED },
        {"PFG_B5G5R5A1_UNORM",		2u * sizeof(uint16),	PFF_INTEGER|PFF_NORMALIZED },
        {"PFG_BGRA8_UNORM",			4u * sizeof(uint8),		PFF_INTEGER|PFF_NORMALIZED },
        {"PFG_BGRX8_UNORM",			4u * sizeof(uint8),		PFF_INTEGER|PFF_NORMALIZED },
        {"PFG_R10G10B10_XR_BIAS_A2_UNORM",1u * sizeof(uint32),PFF_FLOAT_RARE },

        {"PFG_BGRA8_UNORM_SRGB",	4u * sizeof(uint8),		PFF_INTEGER|PFF_NORMALIZED|PFF_SRGB },
        {"PFG_BGRX8_UNORM_SRGB",	4u * sizeof(uint8),		PFF_INTEGER|PFF_NORMALIZED|PFF_SRGB },

        {"PFG_BC6H_UF16",			0,						PFF_COMPRESSED|PFF_FLOAT_RARE },
        {"PFG_BC6H_SF16",			0,						PFF_COMPRESSED|PFF_FLOAT_RARE|PFF_SIGNED },

        {"PFG_BC7_UNORM",			0,						PFF_COMPRESSED_COMMON },
        {"PFG_BC7_UNORM_SRGB",		0,						PFF_COMPRESSED_COMMON|PFF_SRGB },

        {"PFG_AYUV",				0,						0 },
        {"PFG_Y410",				0,						0 },
        {"PFG_Y416",				0,						0 },
        {"PFG_NV12",				0,						0 },
        {"PFG_P010",				0,						0 },
        {"PFG_P016",				0,						0 },
        {"PFG_420_OPAQUE",			0,						0 },
        {"PFG_YUY2",				0,						0 },
        {"PFG_Y210",				0,						0 },
        {"PFG_Y216",				0,						0 },
        {"PFG_NV11",				0,						0 },
        {"PFG_AI44",				0,						0 },
        {"PFG_IA44",				0,						0 },
        {"PFG_P8",					1u * sizeof(uint8),		PFF_PALLETE },
        {"PFG_A8P8",				2u * sizeof(uint8),		PFF_PALLETE },
        {"PFG_B4G4R4A4_UNORM",		2u * sizeof(uint16),	PFF_INTEGER|PFF_NORMALIZED },
        {"PFG_P208",				0,						0 },
        {"PFG_V208",				0,						0 },
        {"PFG_V408",				0,						0 },

        {"PFG_PVRTC_RGB2",			0,						PFF_COMPRESSED_COMMON },
        {"PFG_PVRTC_RGBA2",			0,						PFF_COMPRESSED_COMMON },
        {"PFG_PVRTC_RGB4",			0,						PFF_COMPRESSED_COMMON },
        {"PFG_PVRTC_RGBA4",			0,						PFF_COMPRESSED_COMMON },
        {"PFG_PVRTC2_2BPP",			0,						PFF_COMPRESSED_COMMON },
        {"PFG_PVRTC2_4BPP",			0,						PFF_COMPRESSED_COMMON },

        {"PFG_ETC1_RGB8_UNORM",		0,						PFF_COMPRESSED_COMMON },
        {"PFG_ETC2_RGB8_UNORM",		0,						PFF_COMPRESSED_COMMON },
        {"PFG_ETC2_RGB8_UNORM_SRGB",0,						PFF_COMPRESSED_COMMON|PFF_SRGB },
        {"PFG_ETC2_RGBA8_UNORM",	0,						PFF_COMPRESSED_COMMON },
        {"PFG_ETC2_RGBA8_UNORM_SRGB",0,						PFF_COMPRESSED_COMMON|PFF_SRGB },
        {"PFG_ETC2_RGB8A1_UNORM",	0,						PFF_COMPRESSED_COMMON },
        {"PFG_ETC2_RGB8A1_UNORM_SRGB",0,					PFF_COMPRESSED_COMMON|PFF_SRGB },
        {"PFG_EAC_R11_UNORM",		0,						PFF_COMPRESSED_COMMON },
        {"PFG_EAC_R11_SNORM",		0,						PFF_COMPRESSED_COMMON|PFF_SIGNED },
        {"PFG_EAC_R11G11_UNORM",	0,						PFF_COMPRESSED_COMMON },
        {"PFG_EAC_R11G11_SNORM",	0,						PFF_COMPRESSED_COMMON|PFF_SIGNED },

        {"PFG_ATC_RGB",				0,						PFF_COMPRESSED_COMMON },
        {"PFG_ATC_RGBA_EXPLICIT_ALPHA",			0,			PFF_COMPRESSED_COMMON },
        {"PFG_ATC_RGBA_INTERPOLATED_ALPHA",		0,			PFF_COMPRESSED_COMMON },

        {"PFG_COUNT", 0, 0 },
    };
}
