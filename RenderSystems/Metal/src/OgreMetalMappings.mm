/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE-Next
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

#include "OgreMetalMappings.h"

#include "OgreHlmsDatablock.h"
#include "OgreMetalDevice.h"
#include "OgrePixelFormatGpuUtils.h"

namespace Ogre
{
    //-----------------------------------------------------------------------------------
    void MetalMappings::getDepthStencilFormat( PixelFormatGpu pf, MetalDevice *device,
                                               MTLPixelFormat &outDepth, MTLPixelFormat &outStencil )
    {
        MTLPixelFormat depthFormat = MTLPixelFormatInvalid;
        MTLPixelFormat stencilFormat = MTLPixelFormatInvalid;

        switch( pf )
        {
        case PFG_D24_UNORM_S8_UINT:
        case PFG_D24_UNORM:
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            // depthFormat = MTLPixelFormatDepth32Float;
            // stencilFormat = MTLPixelFormatStencil8;
            depthFormat = MTLPixelFormatDepth32Float_Stencil8;
            stencilFormat = MTLPixelFormatDepth32Float_Stencil8;
#else
            if( device->mDevice.depth24Stencil8PixelFormatSupported )
            {
                depthFormat = MTLPixelFormatDepth24Unorm_Stencil8;
                stencilFormat = MTLPixelFormatDepth24Unorm_Stencil8;
            }
            else
            {
                depthFormat = MTLPixelFormatDepth32Float_Stencil8;
                stencilFormat = MTLPixelFormatDepth32Float_Stencil8;
            }
#endif
            break;
        case PFG_D16_UNORM:
            depthFormat = MTLPixelFormatDepth32Float;
            break;
        case PFG_D32_FLOAT:
            depthFormat = MTLPixelFormatDepth32Float;
            break;
        case PFG_D32_FLOAT_S8X24_UINT:
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            // depthFormat = MTLPixelFormatDepth32Float;
            // stencilFormat = MTLPixelFormatStencil8;
            depthFormat = MTLPixelFormatDepth32Float_Stencil8;
            stencilFormat = MTLPixelFormatDepth32Float_Stencil8;
#else
            depthFormat = MTLPixelFormatDepth32Float_Stencil8;
            stencilFormat = MTLPixelFormatDepth32Float_Stencil8;
#endif
            break;
        // case PFG_X32_X24_S8_UINT:
        //  stencilFormat = MTLPixelFormatStencil8;
        //  break;
        default:
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "PixelFormat '" + String( PixelFormatGpuUtils::toString( pf ) ) +
                             "' is not a valid depth buffer format",
                         "MetalRenderSystem::_createDepthBufferFor" );
        }

        outDepth = depthFormat;
        outStencil = stencilFormat;
    }
    //-----------------------------------------------------------------------------------
    MTLBlendFactor MetalMappings::get( SceneBlendFactor op )
    {
        switch( op )
        {
            // clang-format off
        case SBF_ONE:                   return MTLBlendFactorOne;
        case SBF_ZERO:                  return MTLBlendFactorZero;
        case SBF_DEST_COLOUR:           return MTLBlendFactorDestinationColor;
        case SBF_SOURCE_COLOUR:         return MTLBlendFactorSourceColor;
        case SBF_ONE_MINUS_DEST_COLOUR: return MTLBlendFactorOneMinusDestinationColor;
        case SBF_ONE_MINUS_SOURCE_COLOUR:return MTLBlendFactorOneMinusSourceColor;
        case SBF_DEST_ALPHA:            return MTLBlendFactorDestinationAlpha;
        case SBF_SOURCE_ALPHA:          return MTLBlendFactorSourceAlpha;
        case SBF_ONE_MINUS_DEST_ALPHA:  return MTLBlendFactorOneMinusDestinationAlpha;
        case SBF_ONE_MINUS_SOURCE_ALPHA:return MTLBlendFactorOneMinusSourceAlpha;
            // clang-format on
        }
    }
    //-----------------------------------------------------------------------------------
    MTLBlendOperation MetalMappings::get( SceneBlendOperation op )
    {
        switch( op )
        {
            // clang-format off
        case SBO_ADD:                   return MTLBlendOperationAdd;
        case SBO_SUBTRACT:              return MTLBlendOperationSubtract;
        case SBO_REVERSE_SUBTRACT:      return MTLBlendOperationReverseSubtract;
        case SBO_MIN:                   return MTLBlendOperationMin;
        case SBO_MAX:                   return MTLBlendOperationMax;
            // clang-format on
        }
    }
    //-----------------------------------------------------------------------------------
    MTLColorWriteMask MetalMappings::get( uint8 mask )
    {
        return MTLColorWriteMask( ( ( mask & HlmsBlendblock::BlendChannelRed ) << ( 3u - 0u ) ) |
                                  ( ( mask & HlmsBlendblock::BlendChannelGreen ) << ( 2u - 1u ) ) |
                                  ( ( mask & HlmsBlendblock::BlendChannelBlue ) >> ( 2u - 1u ) ) |
                                  ( ( mask & HlmsBlendblock::BlendChannelAlpha ) >> ( 3u - 0u ) ) );
    }
    //-----------------------------------------------------------------------------------
    MTLStencilOperation MetalMappings::get( StencilOperation op )
    {
        switch( op )
        {
            // clang-format off
        case SOP_KEEP:              return MTLStencilOperationKeep;
        case SOP_ZERO:              return MTLStencilOperationZero;
        case SOP_REPLACE:           return MTLStencilOperationReplace;
        case SOP_INCREMENT:         return MTLStencilOperationIncrementClamp;
        case SOP_DECREMENT:         return MTLStencilOperationDecrementClamp;
        case SOP_INCREMENT_WRAP:    return MTLStencilOperationIncrementWrap;
        case SOP_DECREMENT_WRAP:    return MTLStencilOperationDecrementWrap;
        case SOP_INVERT:            return MTLStencilOperationInvert;
            // clang-format on
        }
    }
    //-----------------------------------------------------------------------------------
    MTLCompareFunction MetalMappings::get( CompareFunction cmp )
    {
        switch( cmp )
        {
            // clang-format off
        case CMPF_ALWAYS_FAIL:          return MTLCompareFunctionNever;
        case CMPF_ALWAYS_PASS:          return MTLCompareFunctionAlways;
        case CMPF_LESS:                 return MTLCompareFunctionLess;
        case CMPF_LESS_EQUAL:           return MTLCompareFunctionLessEqual;
        case CMPF_EQUAL:                return MTLCompareFunctionEqual;
        case CMPF_NOT_EQUAL:            return MTLCompareFunctionNotEqual;
        case CMPF_GREATER_EQUAL:        return MTLCompareFunctionGreaterEqual;
        case CMPF_GREATER:              return MTLCompareFunctionGreater;
            // clang-format on
        case NUM_COMPARE_FUNCTIONS:
            assert( false );  // Should never hit.
            return MTLCompareFunctionAlways;
        }
    }
    //-----------------------------------------------------------------------------------
    MTLVertexFormat MetalMappings::get( VertexElementType vertexElemType )
    {
        switch( vertexElemType )
        {
            // clang-format off
        case VET_FLOAT1:                return MTLVertexFormatFloat;
        case VET_FLOAT2:                return MTLVertexFormatFloat2;
        case VET_FLOAT3:                return MTLVertexFormatFloat3;
        case VET_FLOAT4:                return MTLVertexFormatFloat4;
        case VET_SHORT2:                return MTLVertexFormatShort2;
        case VET_SHORT4:                return MTLVertexFormatShort4;
        case VET_UBYTE4:                return MTLVertexFormatUChar4;
        case VET_USHORT2:               return MTLVertexFormatUShort2;
        case VET_USHORT4:               return MTLVertexFormatUShort4;
        case VET_INT1:                  return MTLVertexFormatInt;
        case VET_INT2:                  return MTLVertexFormatInt2;
        case VET_INT3:                  return MTLVertexFormatInt3;
        case VET_INT4:                  return MTLVertexFormatInt4;
        case VET_UINT1:                 return MTLVertexFormatUInt;
        case VET_UINT2:                 return MTLVertexFormatUInt2;
        case VET_UINT3:                 return MTLVertexFormatUInt3;
        case VET_UINT4:                 return MTLVertexFormatUInt4;
        case VET_BYTE4:                 return MTLVertexFormatChar4;
        case VET_BYTE4_SNORM:           return MTLVertexFormatChar4Normalized;
        case VET_UBYTE4_NORM:           return MTLVertexFormatUChar4Normalized;
        case VET_SHORT2_SNORM:          return MTLVertexFormatShort2Normalized;
        case VET_SHORT4_SNORM:          return MTLVertexFormatShort4Normalized;
        case VET_USHORT2_NORM:          return MTLVertexFormatUShort2Normalized;
        case VET_USHORT4_NORM:          return MTLVertexFormatUShort4Normalized;
        case VET_HALF2:                 return MTLVertexFormatHalf2;
        case VET_HALF4:                 return MTLVertexFormatHalf4;
            // clang-format on

        case VET_COLOUR_ARGB:
            if( @available( macos 10.13, ios 11.0, * ) )
            {
                return MTLVertexFormatUChar4Normalized_BGRA;
            }
            else
            {
                static int warnCount = 0;
                if( warnCount < 10 )
                {
                    LogManager::getSingleton().logMessage(
                        "WARNING: VET_COLOUR_ARGB unsupported on this "
                        "OS/device. Vertex colours will be wrong!!!",
                        LML_CRITICAL );
                    ++warnCount;
                }
                return MTLVertexFormatUChar4Normalized;
            }

        case VET_COLOUR:
        case VET_COLOUR_ABGR:
            return MTLVertexFormatUChar4Normalized;

        case VET_DOUBLE1:
        case VET_DOUBLE2:
        case VET_DOUBLE3:
        case VET_DOUBLE4:
        case VET_USHORT1_DEPRECATED:
        case VET_USHORT3_DEPRECATED:
        default:
            return MTLVertexFormatInvalid;
        }
    }
    //-----------------------------------------------------------------------------------
    MTLSamplerMinMagFilter MetalMappings::get( FilterOptions filter )
    {
        switch( filter )
        {
            // clang-format off
        case FO_NONE:                   return MTLSamplerMinMagFilterNearest;
        case FO_POINT:                  return MTLSamplerMinMagFilterNearest;
        case FO_LINEAR:                 return MTLSamplerMinMagFilterLinear;
        case FO_ANISOTROPIC:            return MTLSamplerMinMagFilterLinear;
            // clang-format on
        }
    }
    //-----------------------------------------------------------------------------------
    MTLSamplerMipFilter MetalMappings::getMipFilter( FilterOptions filter )
    {
        switch( filter )
        {
            // clang-format off
        case FO_NONE:                   return MTLSamplerMipFilterNotMipmapped;
        case FO_POINT:                  return MTLSamplerMipFilterNearest;
        case FO_LINEAR:                 return MTLSamplerMipFilterLinear;
        case FO_ANISOTROPIC:            return MTLSamplerMipFilterLinear;
            // clang-format on
        }
    }
    //-----------------------------------------------------------------------------------
    MTLSamplerAddressMode MetalMappings::get( TextureAddressingMode mode )
    {
        switch( mode )
        {
            // clang-format off
        case TAM_WRAP:                  return MTLSamplerAddressModeRepeat;
        case TAM_MIRROR:                return MTLSamplerAddressModeMirrorRepeat;
        case TAM_CLAMP:                 return MTLSamplerAddressModeClampToEdge;
        //Not supported. Get the best next thing.
        case TAM_BORDER:                return MTLSamplerAddressModeClampToEdge;
            // clang-format on
        default:
            return MTLSamplerAddressModeClampToEdge;
        }
    }
    //-----------------------------------------------------------------------------------
    MTLVertexFormat MetalMappings::dataTypeToVertexFormat( MTLDataType dataType )
    {
        // clang-format off
        switch( dataType )
        {
        case MTLDataTypeNone:           return MTLVertexFormatInvalid;
        case MTLDataTypeStruct:         return MTLVertexFormatInvalid;
        case MTLDataTypeArray:          return MTLVertexFormatInvalid;

        case MTLDataTypeFloat:          return MTLVertexFormatFloat;
        case MTLDataTypeFloat2:         return MTLVertexFormatFloat2;
        case MTLDataTypeFloat3:         return MTLVertexFormatFloat3;
        case MTLDataTypeFloat4:         return MTLVertexFormatFloat4;

        case MTLDataTypeFloat2x2:       return MTLVertexFormatInvalid;
        case MTLDataTypeFloat2x3:       return MTLVertexFormatInvalid;
        case MTLDataTypeFloat2x4:       return MTLVertexFormatInvalid;

        case MTLDataTypeFloat3x2:       return MTLVertexFormatInvalid;
        case MTLDataTypeFloat3x3:       return MTLVertexFormatInvalid;
        case MTLDataTypeFloat3x4:       return MTLVertexFormatInvalid;

        case MTLDataTypeFloat4x2:       return MTLVertexFormatInvalid;
        case MTLDataTypeFloat4x3:       return MTLVertexFormatInvalid;
        case MTLDataTypeFloat4x4:       return MTLVertexFormatInvalid;

        case MTLDataTypeHalf:           return MTLVertexFormatHalf2;
        case MTLDataTypeHalf2:          return MTLVertexFormatHalf2;
        case MTLDataTypeHalf3:          return MTLVertexFormatHalf3;
        case MTLDataTypeHalf4:          return MTLVertexFormatHalf4;

        case MTLDataTypeHalf2x2:        return MTLVertexFormatInvalid;
        case MTLDataTypeHalf2x3:        return MTLVertexFormatInvalid;
        case MTLDataTypeHalf2x4:        return MTLVertexFormatInvalid;

        case MTLDataTypeHalf3x2:        return MTLVertexFormatInvalid;
        case MTLDataTypeHalf3x3:        return MTLVertexFormatInvalid;
        case MTLDataTypeHalf3x4:        return MTLVertexFormatInvalid;

        case MTLDataTypeHalf4x2:        return MTLVertexFormatInvalid;
        case MTLDataTypeHalf4x3:        return MTLVertexFormatInvalid;
        case MTLDataTypeHalf4x4:        return MTLVertexFormatInvalid;

        case MTLDataTypeInt:            return MTLVertexFormatInt;
        case MTLDataTypeInt2:           return MTLVertexFormatInt2;
        case MTLDataTypeInt3:           return MTLVertexFormatInt3;
        case MTLDataTypeInt4:           return MTLVertexFormatInt4;

        case MTLDataTypeUInt:           return MTLVertexFormatUInt;
        case MTLDataTypeUInt2:          return MTLVertexFormatUInt2;
        case MTLDataTypeUInt3:          return MTLVertexFormatUInt3;
        case MTLDataTypeUInt4:          return MTLVertexFormatUInt4;

        case MTLDataTypeShort:          return MTLVertexFormatShort2;
        case MTLDataTypeShort2:         return MTLVertexFormatShort2;
        case MTLDataTypeShort3:         return MTLVertexFormatShort3;
        case MTLDataTypeShort4:         return MTLVertexFormatShort4;

        case MTLDataTypeUShort:         return MTLVertexFormatUShort2;
        case MTLDataTypeUShort2:        return MTLVertexFormatUShort2;
        case MTLDataTypeUShort3:        return MTLVertexFormatUShort3;
        case MTLDataTypeUShort4:        return MTLVertexFormatUShort4;

        case MTLDataTypeChar:           return MTLVertexFormatChar2;
        case MTLDataTypeChar2:          return MTLVertexFormatChar2;
        case MTLDataTypeChar3:          return MTLVertexFormatChar3;
        case MTLDataTypeChar4:          return MTLVertexFormatChar4;

        case MTLDataTypeUChar:          return MTLVertexFormatUChar2;
        case MTLDataTypeUChar2:         return MTLVertexFormatUChar2;
        case MTLDataTypeUChar3:         return MTLVertexFormatUChar3;
        case MTLDataTypeUChar4:         return MTLVertexFormatUChar4;

        case MTLDataTypeBool:           return MTLVertexFormatFloat;
        case MTLDataTypeBool2:          return MTLVertexFormatFloat2;
        case MTLDataTypeBool3:          return MTLVertexFormatFloat3;
        case MTLDataTypeBool4:          return MTLVertexFormatFloat4;

        default:                        return MTLVertexFormatInvalid;
        }
        // clang-format on
    }
    //-----------------------------------------------------------------------------------
    MTLPixelFormat MetalMappings::get( PixelFormatGpu pf, MetalDevice *device )
    {
        switch( pf )
        {
            // clang-format off
        case PFG_UNKNOWN:                       return MTLPixelFormatInvalid;
        case PFG_NULL:                          return MTLPixelFormatInvalid;
        case PFG_RGBA32_FLOAT:		            return MTLPixelFormatRGBA32Float;
        case PFG_RGBA32_UINT:		            return MTLPixelFormatRGBA32Uint;
        case PFG_RGBA32_SINT:		            return MTLPixelFormatRGBA32Sint;
        case PFG_RGB32_FLOAT:		            return MTLPixelFormatInvalid; //Not supported
        case PFG_RGB32_UINT:		            return MTLPixelFormatInvalid; //Not supported
        case PFG_RGB32_SINT:		            return MTLPixelFormatInvalid; //Not supported
        case PFG_RGBA16_FLOAT:		            return MTLPixelFormatRGBA16Float;
        case PFG_RGBA16_UNORM:		            return MTLPixelFormatRGBA16Unorm;
        case PFG_RGBA16_UINT:		            return MTLPixelFormatRGBA16Uint;
        case PFG_RGBA16_SNORM:		            return MTLPixelFormatRGBA16Snorm;
        case PFG_RGBA16_SINT:		            return MTLPixelFormatRGBA16Sint;
        case PFG_RG32_FLOAT:		            return MTLPixelFormatRG32Float;
        case PFG_RG32_UINT:                     return MTLPixelFormatRG32Uint;
        case PFG_RG32_SINT:                     return MTLPixelFormatRG32Sint;
        case PFG_D32_FLOAT_S8X24_UINT:		    return MTLPixelFormatDepth32Float_Stencil8;
        case PFG_R10G10B10A2_UNORM:             return MTLPixelFormatRGB10A2Unorm;
        case PFG_R10G10B10A2_UINT:              return MTLPixelFormatRGB10A2Uint;
        case PFG_R11G11B10_FLOAT:		        return MTLPixelFormatRG11B10Float;
        case PFG_RGBA8_UNORM:		            return MTLPixelFormatRGBA8Unorm;
        case PFG_RGBA8_UNORM_SRGB:              return MTLPixelFormatRGBA8Unorm_sRGB;
        case PFG_RGBA8_UINT:		            return MTLPixelFormatRGBA8Uint;
        case PFG_RGBA8_SNORM:		            return MTLPixelFormatRGBA8Snorm;
        case PFG_RGBA8_SINT:		            return MTLPixelFormatRGBA8Sint;
        case PFG_RG16_FLOAT:		            return MTLPixelFormatRG16Float;
        case PFG_RG16_UNORM:                    return MTLPixelFormatRG16Unorm;
        case PFG_RG16_UINT:		                return MTLPixelFormatRG16Uint;
        case PFG_RG16_SNORM:                    return MTLPixelFormatRG16Snorm;
        case PFG_RG16_SINT:		                return MTLPixelFormatRG16Sint;
        case PFG_D32_FLOAT:		                return MTLPixelFormatDepth32Float;
        case PFG_R32_FLOAT:		                return MTLPixelFormatR32Float;
        case PFG_R32_UINT:		                return MTLPixelFormatR32Uint;
        case PFG_R32_SINT:		                return MTLPixelFormatR32Sint;
// clang-format on
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        case PFG_D24_UNORM:
        case PFG_D24_UNORM_S8_UINT:
            return device->mDevice.depth24Stencil8PixelFormatSupported
                       ? MTLPixelFormatDepth24Unorm_Stencil8
                       : MTLPixelFormatDepth32Float_Stencil8;
#else
            // clang-format off
        case PFG_D24_UNORM:		                return MTLPixelFormatDepth32Float;
        case PFG_D24_UNORM_S8_UINT:             return MTLPixelFormatDepth32Float_Stencil8;
// clang-format on
#endif
            // clang-format off
        case PFG_RG8_UNORM:		                return MTLPixelFormatRG8Unorm;
        case PFG_RG8_UINT:		                return MTLPixelFormatRG8Uint;
        case PFG_RG8_SNORM:		                return MTLPixelFormatRG8Snorm;
        case PFG_RG8_SINT:		                return MTLPixelFormatRG8Sint;
        case PFG_R16_FLOAT:		                return MTLPixelFormatR16Float;
        case PFG_D16_UNORM:		                return MTLPixelFormatDepth32Float; //Unsupported
        case PFG_R16_UNORM:		                return MTLPixelFormatR16Unorm;
        case PFG_R16_UINT:		                return MTLPixelFormatR16Uint;
        case PFG_R16_SNORM:		                return MTLPixelFormatR16Snorm;
        case PFG_R16_SINT:		                return MTLPixelFormatR16Sint;
        case PFG_R8_UNORM:		                return MTLPixelFormatR8Unorm;
        case PFG_R8_UINT:		                return MTLPixelFormatR8Uint;
        case PFG_R8_SNORM:		                return MTLPixelFormatR8Snorm;
        case PFG_R8_SINT:		                return MTLPixelFormatR8Sint;
        case PFG_A8_UNORM:		                return MTLPixelFormatA8Unorm;
        case PFG_R1_UNORM:                      return MTLPixelFormatInvalid;   //Not supported
        case PFG_R9G9B9E5_SHAREDEXP:		    return MTLPixelFormatRGB9E5Float;
        case PFG_R8G8_B8G8_UNORM:               return MTLPixelFormatGBGR422;
        case PFG_G8R8_G8B8_UNORM:               return MTLPixelFormatBGRG422;
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        case PFG_BC1_UNORM:                     return MTLPixelFormatBC1_RGBA;
        case PFG_BC1_UNORM_SRGB:                return MTLPixelFormatBC1_RGBA_sRGB;
        case PFG_BC2_UNORM:                     return MTLPixelFormatBC2_RGBA;
        case PFG_BC2_UNORM_SRGB:                return MTLPixelFormatBC2_RGBA_sRGB;
        case PFG_BC3_UNORM:                     return MTLPixelFormatBC3_RGBA;
        case PFG_BC3_UNORM_SRGB:                return MTLPixelFormatBC3_RGBA_sRGB;
        case PFG_BC4_UNORM:                     return MTLPixelFormatBC4_RUnorm;
        case PFG_BC4_SNORM:                     return MTLPixelFormatBC4_RSnorm;
        case PFG_BC5_UNORM:                     return MTLPixelFormatBC5_RGUnorm;
        case PFG_BC5_SNORM:                     return MTLPixelFormatBC5_RGSnorm;
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        case PFG_B5G6R5_UNORM:                  return MTLPixelFormatB5G6R5Unorm;
        case PFG_B5G5R5A1_UNORM:                return MTLPixelFormatBGR5A1Unorm;
#endif
        case PFG_BGRA8_UNORM:                   return MTLPixelFormatBGRA8Unorm;
        case PFG_BGRX8_UNORM:                   return MTLPixelFormatBGRA8Unorm;
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        case PFG_R10G10B10_XR_BIAS_A2_UNORM:    return MTLPixelFormatBGRA10_XR;
#endif
        case PFG_BGRA8_UNORM_SRGB:              return MTLPixelFormatBGRA8Unorm_sRGB;
        case PFG_BGRX8_UNORM_SRGB:              return MTLPixelFormatBGRA8Unorm_sRGB;
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        case PFG_BC6H_UF16:                     return MTLPixelFormatBC6H_RGBUfloat;
        case PFG_BC6H_SF16:                     return MTLPixelFormatBC6H_RGBFloat;
        case PFG_BC7_UNORM:                     return MTLPixelFormatBC7_RGBAUnorm;
        case PFG_BC7_UNORM_SRGB:                return MTLPixelFormatBC7_RGBAUnorm_sRGB;
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        case PFG_B4G4R4A4_UNORM:                return MTLPixelFormatABGR4Unorm;
        case PFG_PVRTC_RGB2:		            return MTLPixelFormatPVRTC_RGB_2BPP;
        case PFG_PVRTC_RGB2_SRGB:               return MTLPixelFormatPVRTC_RGB_2BPP_sRGB;
        case PFG_PVRTC_RGBA2:		            return MTLPixelFormatPVRTC_RGBA_2BPP;
        case PFG_PVRTC_RGBA2_SRGB:              return MTLPixelFormatPVRTC_RGBA_2BPP_sRGB;
        case PFG_PVRTC_RGB4:		            return MTLPixelFormatPVRTC_RGB_4BPP;
        case PFG_PVRTC_RGB4_SRGB:               return MTLPixelFormatPVRTC_RGB_4BPP_sRGB;
        case PFG_PVRTC_RGBA4:		            return MTLPixelFormatPVRTC_RGBA_4BPP;
        case PFG_PVRTC_RGBA4_SRGB:              return MTLPixelFormatPVRTC_RGBA_4BPP_sRGB;
        case PFG_PVRTC2_2BPP:		            return MTLPixelFormatInvalid;   //Not supported
        case PFG_PVRTC2_2BPP_SRGB:              return MTLPixelFormatInvalid;   //Not supported
        case PFG_PVRTC2_4BPP:		            return MTLPixelFormatInvalid;   //Not supported
        case PFG_PVRTC2_4BPP_SRGB:              return MTLPixelFormatInvalid;   //Not supported
        case PFG_ETC1_RGB8_UNORM:               return MTLPixelFormatETC2_RGB8; //Backwards compatible
        case PFG_ETC2_RGB8_UNORM:               return MTLPixelFormatETC2_RGB8;
        case PFG_ETC2_RGB8_UNORM_SRGB:		    return MTLPixelFormatETC2_RGB8_sRGB;
        case PFG_ETC2_RGBA8_UNORM:              return MTLPixelFormatEAC_RGBA8;
        case PFG_ETC2_RGBA8_UNORM_SRGB:		    return MTLPixelFormatEAC_RGBA8_sRGB;
        case PFG_ETC2_RGB8A1_UNORM:             return MTLPixelFormatETC2_RGB8A1;
        case PFG_ETC2_RGB8A1_UNORM_SRGB:        return MTLPixelFormatETC2_RGB8A1_sRGB;
        case PFG_EAC_R11_UNORM:                 return MTLPixelFormatEAC_R11Unorm;
        case PFG_EAC_R11_SNORM:                 return MTLPixelFormatEAC_R11Snorm;
        case PFG_EAC_R11G11_UNORM:              return MTLPixelFormatEAC_RG11Unorm;
        case PFG_EAC_R11G11_SNORM:              return MTLPixelFormatEAC_RG11Snorm;
        case PFG_ATC_RGB:                       return MTLPixelFormatInvalid;   //Not supported
        case PFG_ATC_RGBA_EXPLICIT_ALPHA:       return MTLPixelFormatInvalid;   //Not supported
        case PFG_ATC_RGBA_INTERPOLATED_ALPHA:   return MTLPixelFormatInvalid;   //Not supported

        case PFG_ASTC_RGBA_UNORM_4X4_LDR:       return MTLPixelFormatASTC_4x4_LDR;
        case PFG_ASTC_RGBA_UNORM_5X4_LDR:       return MTLPixelFormatASTC_5x4_LDR;
        case PFG_ASTC_RGBA_UNORM_5X5_LDR:       return MTLPixelFormatASTC_5x5_LDR;
        case PFG_ASTC_RGBA_UNORM_6X5_LDR:       return MTLPixelFormatASTC_6x5_LDR;
        case PFG_ASTC_RGBA_UNORM_6X6_LDR:       return MTLPixelFormatASTC_6x6_LDR;
        case PFG_ASTC_RGBA_UNORM_8X5_LDR:       return MTLPixelFormatASTC_8x5_LDR;
        case PFG_ASTC_RGBA_UNORM_8X6_LDR:       return MTLPixelFormatASTC_8x6_LDR;
        case PFG_ASTC_RGBA_UNORM_8X8_LDR:       return MTLPixelFormatASTC_8x8_LDR;
        case PFG_ASTC_RGBA_UNORM_10X5_LDR:      return MTLPixelFormatASTC_10x5_LDR;
        case PFG_ASTC_RGBA_UNORM_10X6_LDR:      return MTLPixelFormatASTC_10x6_LDR;
        case PFG_ASTC_RGBA_UNORM_10X8_LDR:      return MTLPixelFormatASTC_10x8_LDR;
        case PFG_ASTC_RGBA_UNORM_10X10_LDR:     return MTLPixelFormatASTC_10x10_LDR;
        case PFG_ASTC_RGBA_UNORM_12X10_LDR:     return MTLPixelFormatASTC_12x10_LDR;
        case PFG_ASTC_RGBA_UNORM_12X12_LDR:     return MTLPixelFormatASTC_12x12_LDR;
        case PFG_ASTC_RGBA_UNORM_4X4_sRGB:      return MTLPixelFormatASTC_4x4_sRGB;
        case PFG_ASTC_RGBA_UNORM_5X4_sRGB:      return MTLPixelFormatASTC_5x4_sRGB;
        case PFG_ASTC_RGBA_UNORM_5X5_sRGB:      return MTLPixelFormatASTC_5x5_sRGB;
        case PFG_ASTC_RGBA_UNORM_6X5_sRGB:      return MTLPixelFormatASTC_6x5_sRGB;
        case PFG_ASTC_RGBA_UNORM_6X6_sRGB:      return MTLPixelFormatASTC_6x6_sRGB;
        case PFG_ASTC_RGBA_UNORM_8X5_sRGB:      return MTLPixelFormatASTC_8x5_sRGB;
        case PFG_ASTC_RGBA_UNORM_8X6_sRGB:      return MTLPixelFormatASTC_8x6_sRGB;
        case PFG_ASTC_RGBA_UNORM_8X8_sRGB:      return MTLPixelFormatASTC_8x8_sRGB;
        case PFG_ASTC_RGBA_UNORM_10X5_sRGB:     return MTLPixelFormatASTC_10x5_sRGB;
        case PFG_ASTC_RGBA_UNORM_10X6_sRGB:     return MTLPixelFormatASTC_10x6_sRGB;
        case PFG_ASTC_RGBA_UNORM_10X8_sRGB:     return MTLPixelFormatASTC_10x8_sRGB;
        case PFG_ASTC_RGBA_UNORM_10X10_sRGB:    return MTLPixelFormatASTC_10x10_sRGB;
        case PFG_ASTC_RGBA_UNORM_12X10_sRGB:    return MTLPixelFormatASTC_12x10_sRGB;
        case PFG_ASTC_RGBA_UNORM_12X12_sRGB:    return MTLPixelFormatASTC_12x12_sRGB;
#endif
            // clang-format on
        case PFG_RGB8_UNORM:
        case PFG_RGB8_UNORM_SRGB:
        case PFG_BGR8_UNORM:
        case PFG_BGR8_UNORM_SRGB:
        case PFG_RGB16_UNORM:
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
            return MTLPixelFormatInvalid;
        }

        return MTLPixelFormatInvalid;
    }
    //-----------------------------------------------------------------------------------
    GpuConstantType MetalMappings::get( MTLDataType dataType )
    {
        // clang-format off
        switch( dataType )
        {
        case MTLDataTypeNone:           return GCT_UNKNOWN;
        case MTLDataTypeStruct:         return GCT_UNKNOWN;
        case MTLDataTypeArray:          return GCT_UNKNOWN;

        case MTLDataTypeFloat:          return GCT_FLOAT1;
        case MTLDataTypeFloat2:         return GCT_FLOAT2;
        case MTLDataTypeFloat3:         return GCT_FLOAT3;
        case MTLDataTypeFloat4:         return GCT_FLOAT4;

        case MTLDataTypeFloat2x2:       return GCT_MATRIX_2X2;
        case MTLDataTypeFloat2x3:       return GCT_MATRIX_2X3;
        case MTLDataTypeFloat2x4:       return GCT_MATRIX_2X4;

        case MTLDataTypeFloat3x2:       return GCT_MATRIX_3X2;
        case MTLDataTypeFloat3x3:       return GCT_MATRIX_3X3;
        case MTLDataTypeFloat3x4:       return GCT_MATRIX_3X4;

        case MTLDataTypeFloat4x2:       return GCT_MATRIX_4X2;
        case MTLDataTypeFloat4x3:       return GCT_MATRIX_4X3;
        case MTLDataTypeFloat4x4:       return GCT_MATRIX_4X4;

        case MTLDataTypeHalf:           return GCT_UNKNOWN;
        case MTLDataTypeHalf2:          return GCT_UNKNOWN;
        case MTLDataTypeHalf3:          return GCT_UNKNOWN;
        case MTLDataTypeHalf4:          return GCT_UNKNOWN;

        case MTLDataTypeHalf2x2:        return GCT_UNKNOWN;
        case MTLDataTypeHalf2x3:        return GCT_UNKNOWN;
        case MTLDataTypeHalf2x4:        return GCT_UNKNOWN;

        case MTLDataTypeHalf3x2:        return GCT_UNKNOWN;
        case MTLDataTypeHalf3x3:        return GCT_UNKNOWN;
        case MTLDataTypeHalf3x4:        return GCT_UNKNOWN;

        case MTLDataTypeHalf4x2:        return GCT_UNKNOWN;
        case MTLDataTypeHalf4x3:        return GCT_UNKNOWN;
        case MTLDataTypeHalf4x4:        return GCT_UNKNOWN;

        case MTLDataTypeInt:            return GCT_INT1;
        case MTLDataTypeInt2:           return GCT_INT2;
        case MTLDataTypeInt3:           return GCT_INT3;
        case MTLDataTypeInt4:           return GCT_INT4;

        case MTLDataTypeUInt:           return GCT_UINT1;
        case MTLDataTypeUInt2:          return GCT_UINT2;
        case MTLDataTypeUInt3:          return GCT_UINT3;
        case MTLDataTypeUInt4:          return GCT_UINT4;

        case MTLDataTypeShort:          return GCT_UNKNOWN;
        case MTLDataTypeShort2:         return GCT_UNKNOWN;
        case MTLDataTypeShort3:         return GCT_UNKNOWN;
        case MTLDataTypeShort4:         return GCT_UNKNOWN;

        case MTLDataTypeUShort:         return GCT_UNKNOWN;
        case MTLDataTypeUShort2:        return GCT_UNKNOWN;
        case MTLDataTypeUShort3:        return GCT_UNKNOWN;
        case MTLDataTypeUShort4:        return GCT_UNKNOWN;

        case MTLDataTypeChar:           return GCT_UNKNOWN;
        case MTLDataTypeChar2:          return GCT_UNKNOWN;
        case MTLDataTypeChar3:          return GCT_UNKNOWN;
        case MTLDataTypeChar4:          return GCT_UNKNOWN;

        case MTLDataTypeUChar:          return GCT_UNKNOWN;
        case MTLDataTypeUChar2:         return GCT_UNKNOWN;
        case MTLDataTypeUChar3:         return GCT_UNKNOWN;
        case MTLDataTypeUChar4:         return GCT_UNKNOWN;

        case MTLDataTypeBool:           return GCT_BOOL1;
        case MTLDataTypeBool2:          return GCT_BOOL2;
        case MTLDataTypeBool3:          return GCT_BOOL3;
        case MTLDataTypeBool4:          return GCT_BOOL4;

        default:                        return GCT_UNKNOWN;
        }
        // clang-format on
    }
}  // namespace Ogre
