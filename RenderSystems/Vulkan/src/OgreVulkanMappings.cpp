/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-present Torus Knot Software Ltd

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

#include "OgreVulkanMappings.h"

#include "OgrePixelFormatGpuUtils.h"

namespace Ogre
{
    //-----------------------------------------------------------------------------------
    VkPrimitiveTopology VulkanMappings::get( OperationType opType )
    {
        switch( opType )
        {
            // clang-format off
        case OT_POINT_LIST:     return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case OT_LINE_LIST:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case OT_LINE_STRIP:     return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case OT_TRIANGLE_LIST:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case OT_TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case OT_TRIANGLE_FAN:   return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        default:
            return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
            // clang-format on
        }
    }
    //-----------------------------------------------------------------------------------
    VkPolygonMode VulkanMappings::get( PolygonMode polygonMode )
    {
        switch( polygonMode )
        {
            // clang-format off
        case PM_POINTS:     return VK_POLYGON_MODE_POINT;
        case PM_WIREFRAME:  return VK_POLYGON_MODE_LINE;
        case PM_SOLID:      return VK_POLYGON_MODE_FILL;
            // clang-format on
        }
        return VK_POLYGON_MODE_FILL;
    }
    //-----------------------------------------------------------------------------------
    VkCullModeFlags VulkanMappings::get( CullingMode cullMode )
    {
        switch( cullMode )
        {
            // clang-format off
        case CULL_NONE:             return VK_CULL_MODE_NONE;
        case CULL_CLOCKWISE:        return VK_CULL_MODE_BACK_BIT;
        case CULL_ANTICLOCKWISE:    return VK_CULL_MODE_FRONT_BIT;
            // clang-format on
        }
        return VK_CULL_MODE_BACK_BIT;
    }
    //-----------------------------------------------------------------------------------
    VkCompareOp VulkanMappings::get( CompareFunction compareFunc )
    {
        switch( compareFunc )
        {
            // clang-format off
        case NUM_COMPARE_FUNCTIONS: return VK_COMPARE_OP_NEVER;
        case CMPF_ALWAYS_FAIL:      return VK_COMPARE_OP_NEVER;
        case CMPF_ALWAYS_PASS:      return VK_COMPARE_OP_ALWAYS;
        case CMPF_LESS:             return VK_COMPARE_OP_LESS;
        case CMPF_LESS_EQUAL:       return VK_COMPARE_OP_LESS_OR_EQUAL;
        case CMPF_EQUAL:            return VK_COMPARE_OP_EQUAL;
        case CMPF_NOT_EQUAL:        return VK_COMPARE_OP_NOT_EQUAL;
        case CMPF_GREATER_EQUAL:    return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case CMPF_GREATER:          return VK_COMPARE_OP_GREATER;
            // clang-format on
        }
        return VK_COMPARE_OP_NEVER;
    }
    //-----------------------------------------------------------------------------------
    VkStencilOp VulkanMappings::get( StencilOperation stencilOp )
    {
        switch( stencilOp )
        {
            // clang-format off
        case SOP_KEEP:              return VK_STENCIL_OP_KEEP;
        case SOP_ZERO:              return VK_STENCIL_OP_ZERO;
        case SOP_REPLACE:           return VK_STENCIL_OP_REPLACE;
        case SOP_INCREMENT:         return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case SOP_DECREMENT:         return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case SOP_INCREMENT_WRAP:    return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case SOP_DECREMENT_WRAP:    return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        case SOP_INVERT:            return VK_STENCIL_OP_INVERT;
            // clang-format on
        }
        return VK_STENCIL_OP_KEEP;
    }
    //-----------------------------------------------------------------------------------
    VkBlendFactor VulkanMappings::get( SceneBlendFactor blendFactor )
    {
        switch( blendFactor )
        {
            // clang-format off
        case SBF_ONE:                       return VK_BLEND_FACTOR_ONE;
        case SBF_ZERO:                      return VK_BLEND_FACTOR_ZERO;
        case SBF_DEST_COLOUR:               return VK_BLEND_FACTOR_DST_COLOR;
        case SBF_SOURCE_COLOUR:             return VK_BLEND_FACTOR_SRC_COLOR;
        case SBF_ONE_MINUS_DEST_COLOUR:     return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case SBF_ONE_MINUS_SOURCE_COLOUR:   return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case SBF_DEST_ALPHA:                return VK_BLEND_FACTOR_DST_ALPHA;
        case SBF_SOURCE_ALPHA:              return VK_BLEND_FACTOR_SRC_ALPHA;
        case SBF_ONE_MINUS_DEST_ALPHA:      return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case SBF_ONE_MINUS_SOURCE_ALPHA:    return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            // clang-format on
        }
        return VK_BLEND_FACTOR_ONE;
    }
    //-----------------------------------------------------------------------------------
    VkBlendOp VulkanMappings::get( SceneBlendOperation blendOp )
    {
        switch( blendOp )
        {
            // clang-format off
        case SBO_ADD:               return VK_BLEND_OP_ADD;
        case SBO_SUBTRACT:          return VK_BLEND_OP_SUBTRACT;
        case SBO_REVERSE_SUBTRACT:  return VK_BLEND_OP_REVERSE_SUBTRACT;
        case SBO_MIN:               return VK_BLEND_OP_MIN;
        case SBO_MAX:               return VK_BLEND_OP_MAX;
            // clang-format on
        }
        return VK_BLEND_OP_ADD;
    }
    //-----------------------------------------------------------------------------------
    VkImageViewType VulkanMappings::get( TextureTypes::TextureTypes textureType )
    {
        switch( textureType )
        {
        // clang-format off
        case TextureTypes::Unknown:         return VK_IMAGE_VIEW_TYPE_2D;
        case TextureTypes::Type1D:          return VK_IMAGE_VIEW_TYPE_1D;
        case TextureTypes::Type1DArray:     return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        case TextureTypes::Type2D:          return VK_IMAGE_VIEW_TYPE_2D;
        case TextureTypes::Type2DArray:     return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case TextureTypes::TypeCube:        return VK_IMAGE_VIEW_TYPE_CUBE;
        case TextureTypes::TypeCubeArray:   return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        case TextureTypes::Type3D:          return VK_IMAGE_VIEW_TYPE_3D;
            // clang-format on
        }
        return VK_IMAGE_VIEW_TYPE_2D;
    }
    //-----------------------------------------------------------------------------------
    VkFormat VulkanMappings::get( PixelFormatGpu pf )
    {
        // clang-format off
        switch( pf )
        {
        case PFG_UNKNOWN:               return VK_FORMAT_UNDEFINED;
        case PFG_RGBA32_FLOAT:          return VK_FORMAT_R32G32B32A32_SFLOAT;
        case PFG_RGBA32_UINT:           return VK_FORMAT_R32G32B32A32_UINT;
        case PFG_RGBA32_SINT:           return VK_FORMAT_R32G32B32A32_SINT;
        case PFG_RGB32_FLOAT:           return VK_FORMAT_R32G32B32_SFLOAT;
        case PFG_RGB32_UINT:            return VK_FORMAT_R32G32B32_UINT;
        case PFG_RGB32_SINT:            return VK_FORMAT_R32G32B32_SINT;
        case PFG_RGBA16_FLOAT:          return VK_FORMAT_R16G16B16A16_SFLOAT;
        case PFG_RGBA16_UNORM:          return VK_FORMAT_R16G16B16A16_UNORM;
        case PFG_RGBA16_UINT:           return VK_FORMAT_R16G16B16A16_UINT;
        case PFG_RGBA16_SNORM:          return VK_FORMAT_R16G16B16A16_SNORM;
        case PFG_RGBA16_SINT:           return VK_FORMAT_R16G16B16A16_SINT;
        case PFG_RG32_FLOAT:            return VK_FORMAT_R32G32_SFLOAT;
        case PFG_RG32_UINT:             return VK_FORMAT_R32G32_UINT;
        case PFG_RG32_SINT:             return VK_FORMAT_R32G32_SINT;
        case PFG_D32_FLOAT_S8X24_UINT:  return VK_FORMAT_D32_SFLOAT_S8_UINT;
        case PFG_R10G10B10A2_UNORM:     return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        case PFG_R10G10B10A2_UINT:      return VK_FORMAT_A2B10G10R10_UINT_PACK32;
        case PFG_R11G11B10_FLOAT:       return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        case PFG_RGBA8_UNORM:           return VK_FORMAT_R8G8B8A8_UNORM;
        case PFG_RGBA8_UNORM_SRGB:      return VK_FORMAT_R8G8B8A8_SRGB;
        case PFG_RGBA8_UINT:            return VK_FORMAT_R8G8B8A8_UINT;
        case PFG_RGBA8_SNORM:           return VK_FORMAT_R8G8B8A8_SNORM;
        case PFG_RGBA8_SINT:            return VK_FORMAT_R8G8B8A8_SINT;
        case PFG_RG16_FLOAT:            return VK_FORMAT_R16G16_SFLOAT;
        case PFG_RG16_UNORM:            return VK_FORMAT_R16G16_UNORM;
        case PFG_RG16_UINT:             return VK_FORMAT_R16G16_UINT;
        case PFG_RG16_SNORM:            return VK_FORMAT_R16G16_SNORM;
        case PFG_RG16_SINT:             return VK_FORMAT_R16G16_SINT;
        case PFG_D32_FLOAT:             return VK_FORMAT_D32_SFLOAT;
        case PFG_R32_FLOAT:             return VK_FORMAT_R32_SFLOAT;
        case PFG_R32_UINT:              return VK_FORMAT_R32_UINT;
        case PFG_R32_SINT:              return VK_FORMAT_R32_SINT;
        case PFG_D24_UNORM:             return VK_FORMAT_X8_D24_UNORM_PACK32;
        case PFG_D24_UNORM_S8_UINT:     return VK_FORMAT_D24_UNORM_S8_UINT;
        case PFG_RG8_UNORM:             return VK_FORMAT_R8G8_UNORM;
        case PFG_RG8_UINT:              return VK_FORMAT_R8G8_UINT;
        case PFG_RG8_SNORM:             return VK_FORMAT_R8G8_SNORM;
        case PFG_RG8_SINT:              return VK_FORMAT_R8G8_SINT;
        case PFG_R16_FLOAT:             return VK_FORMAT_R16_SFLOAT;
        case PFG_D16_UNORM:             return VK_FORMAT_D16_UNORM;
        case PFG_R16_UNORM:             return VK_FORMAT_R16_UNORM;
        case PFG_R16_UINT:              return VK_FORMAT_R16_UINT;
        case PFG_R16_SNORM:             return VK_FORMAT_R16_SNORM;
        case PFG_R16_SINT:              return VK_FORMAT_R16_SINT;
        case PFG_R8_UNORM:              return VK_FORMAT_R8_UNORM;
        case PFG_R8_UINT:               return VK_FORMAT_R8_UINT;
        case PFG_R8_SNORM:              return VK_FORMAT_R8_SNORM;
        case PFG_R8_SINT:               return VK_FORMAT_R8_SINT;
        case PFG_R9G9B9E5_SHAREDEXP:    return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
        case PFG_R8G8_B8G8_UNORM:       return VK_FORMAT_B8G8R8G8_422_UNORM;
        case PFG_G8R8_G8B8_UNORM:       return VK_FORMAT_G8B8G8R8_422_UNORM;
        case PFG_BC1_UNORM:             return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case PFG_BC1_UNORM_SRGB:        return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
        case PFG_BC2_UNORM:             return VK_FORMAT_BC2_UNORM_BLOCK;
        case PFG_BC2_UNORM_SRGB:        return VK_FORMAT_BC2_SRGB_BLOCK;
        case PFG_BC3_UNORM:             return VK_FORMAT_BC3_UNORM_BLOCK;
        case PFG_BC3_UNORM_SRGB:        return VK_FORMAT_BC3_SRGB_BLOCK;
        case PFG_BC4_UNORM:             return VK_FORMAT_BC4_UNORM_BLOCK;
        case PFG_BC4_SNORM:             return VK_FORMAT_BC4_SNORM_BLOCK;
        case PFG_BC5_UNORM:             return VK_FORMAT_BC5_UNORM_BLOCK;
        case PFG_BC5_SNORM:             return VK_FORMAT_BC5_SNORM_BLOCK;
        case PFG_B5G6R5_UNORM:          return VK_FORMAT_B5G6R5_UNORM_PACK16;
        case PFG_B5G5R5A1_UNORM:        return VK_FORMAT_B5G5R5A1_UNORM_PACK16;
        case PFG_BGRA8_UNORM:           return VK_FORMAT_B8G8R8A8_UNORM;
        case PFG_BGRX8_UNORM:           return VK_FORMAT_B8G8R8A8_UNORM;
        case PFG_R10G10B10_XR_BIAS_A2_UNORM:return VK_FORMAT_A2R10G10B10_USCALED_PACK32;
        case PFG_BGRA8_UNORM_SRGB:      return VK_FORMAT_B8G8R8A8_SRGB;
        case PFG_BGRX8_UNORM_SRGB:      return VK_FORMAT_B8G8R8A8_SRGB;
        case PFG_BC6H_UF16:             return VK_FORMAT_BC6H_UFLOAT_BLOCK;
        case PFG_BC6H_SF16:             return VK_FORMAT_BC6H_SFLOAT_BLOCK;
        case PFG_BC7_UNORM:             return VK_FORMAT_BC7_UNORM_BLOCK;
        case PFG_BC7_UNORM_SRGB:        return VK_FORMAT_BC7_SRGB_BLOCK;
        case PFG_B4G4R4A4_UNORM:        return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
        case PFG_PVRTC_RGB2:            return VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;
        case PFG_PVRTC_RGBA2:           return VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;
        case PFG_PVRTC_RGB4:            return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
        case PFG_PVRTC_RGBA4:           return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
        case PFG_PVRTC2_2BPP:           return VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG;
        case PFG_PVRTC2_4BPP:           return VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG;
        case PFG_ETC1_RGB8_UNORM:       return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
        case PFG_ETC2_RGB8_UNORM:       return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
        case PFG_ETC2_RGB8_UNORM_SRGB:  return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
        case PFG_ETC2_RGBA8_UNORM:      return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
        case PFG_ETC2_RGBA8_UNORM_SRGB: return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
        case PFG_ETC2_RGB8A1_UNORM:     return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
        case PFG_ETC2_RGB8A1_UNORM_SRGB:return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
        case PFG_EAC_R11_UNORM:         return VK_FORMAT_EAC_R11_UNORM_BLOCK;
        case PFG_EAC_R11_SNORM:         return VK_FORMAT_EAC_R11_SNORM_BLOCK;
        case PFG_EAC_R11G11_UNORM:      return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
        case PFG_EAC_R11G11_SNORM:      return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;

        case PFG_ATC_RGB:
        case PFG_ATC_RGBA_EXPLICIT_ALPHA:
        case PFG_ATC_RGBA_INTERPOLATED_ALPHA:
        case PFG_A8_UNORM:
        case PFG_R1_UNORM:
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
        case PFG_COUNT:
        default:
            return VK_FORMAT_UNDEFINED;
        }
        // clang-format on

        return VK_FORMAT_UNDEFINED;
    }
}  // namespace Ogre
