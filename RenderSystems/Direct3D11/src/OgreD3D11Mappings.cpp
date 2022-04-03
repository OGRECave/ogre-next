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
#include "OgreD3D11Mappings.h"

#include "OgreD3D11RenderSystem.h"
#include "OgreRoot.h"

namespace Ogre
{
    //---------------------------------------------------------------------
    D3D11_TEXTURE_ADDRESS_MODE D3D11Mappings::get( TextureAddressingMode tam )
    {
        D3D11RenderSystem *rsys =
            static_cast<D3D11RenderSystem *>( Root::getSingleton().getRenderSystem() );
        if( rsys->_getFeatureLevel() == D3D_FEATURE_LEVEL_9_1 )
            return D3D11_TEXTURE_ADDRESS_WRAP;

        // return D3D11_TEXTURE_ADDRESS_WRAP;
        switch( tam )
        {
        case TAM_WRAP:
            return D3D11_TEXTURE_ADDRESS_WRAP;
        case TAM_MIRROR:
            return D3D11_TEXTURE_ADDRESS_MIRROR;
        case TAM_CLAMP:
            return D3D11_TEXTURE_ADDRESS_CLAMP;
        case TAM_BORDER:
            return D3D11_TEXTURE_ADDRESS_BORDER;
        }
        return D3D11_TEXTURE_ADDRESS_WRAP;
    }
    //---------------------------------------------------------------------
    D3D11_BLEND D3D11Mappings::get( SceneBlendFactor sbf, bool forAlpha )
    {
        switch( sbf )
        {
            // clang-format off
        case SBF_ONE:                       return D3D11_BLEND_ONE;
        case SBF_ZERO:                      return D3D11_BLEND_ZERO;
        case SBF_DEST_COLOUR:               return forAlpha ? D3D11_BLEND_DEST_ALPHA : D3D11_BLEND_DEST_COLOR;
        case SBF_SOURCE_COLOUR:             return forAlpha ? D3D11_BLEND_SRC_ALPHA : D3D11_BLEND_SRC_COLOR;
        case SBF_ONE_MINUS_DEST_COLOUR:     return forAlpha ? D3D11_BLEND_INV_DEST_ALPHA : D3D11_BLEND_INV_DEST_COLOR;
        case SBF_ONE_MINUS_SOURCE_COLOUR:   return forAlpha ? D3D11_BLEND_INV_SRC_ALPHA : D3D11_BLEND_INV_SRC_COLOR;
        case SBF_DEST_ALPHA:                return D3D11_BLEND_DEST_ALPHA;
        case SBF_SOURCE_ALPHA:              return D3D11_BLEND_SRC_ALPHA;
        case SBF_ONE_MINUS_DEST_ALPHA:      return D3D11_BLEND_INV_DEST_ALPHA;
        case SBF_ONE_MINUS_SOURCE_ALPHA:    return D3D11_BLEND_INV_SRC_ALPHA;
            // clang-format on
        }
        return D3D11_BLEND_ZERO;
    }
    //---------------------------------------------------------------------
    D3D11_BLEND_OP D3D11Mappings::get( SceneBlendOperation sbo )
    {
        switch( sbo )
        {
        case SBO_ADD:
            return D3D11_BLEND_OP_ADD;
        case SBO_SUBTRACT:
            return D3D11_BLEND_OP_SUBTRACT;
        case SBO_REVERSE_SUBTRACT:
            return D3D11_BLEND_OP_REV_SUBTRACT;
        case SBO_MIN:
            return D3D11_BLEND_OP_MIN;
        case SBO_MAX:
            return D3D11_BLEND_OP_MAX;
        }
        return D3D11_BLEND_OP_ADD;
    }
    //---------------------------------------------------------------------
    D3D11_COMPARISON_FUNC D3D11Mappings::get( CompareFunction cf )
    {
        switch( cf )
        {
        case CMPF_ALWAYS_FAIL:
            return D3D11_COMPARISON_NEVER;
        case CMPF_ALWAYS_PASS:
            return D3D11_COMPARISON_ALWAYS;
        case CMPF_LESS:
            return D3D11_COMPARISON_LESS;
        case CMPF_LESS_EQUAL:
            return D3D11_COMPARISON_LESS_EQUAL;
        case CMPF_EQUAL:
            return D3D11_COMPARISON_EQUAL;
        case CMPF_NOT_EQUAL:
            return D3D11_COMPARISON_NOT_EQUAL;
        case CMPF_GREATER_EQUAL:
            return D3D11_COMPARISON_GREATER_EQUAL;
        case CMPF_GREATER:
            return D3D11_COMPARISON_GREATER;
        };
        return D3D11_COMPARISON_ALWAYS;
    }
    //---------------------------------------------------------------------
    D3D11_CULL_MODE D3D11Mappings::get( CullingMode cm, bool flip )
    {
        switch( cm )
        {
        case CULL_NONE:
            return D3D11_CULL_NONE;
        case CULL_CLOCKWISE:
            return flip ? D3D11_CULL_FRONT : D3D11_CULL_BACK;
        case CULL_ANTICLOCKWISE:
            return flip ? D3D11_CULL_BACK : D3D11_CULL_FRONT;
        }
        return D3D11_CULL_NONE;
    }
    //---------------------------------------------------------------------
    D3D11_FILL_MODE D3D11Mappings::get( PolygonMode level )
    {
        switch( level )
        {
        case PM_POINTS:
            return D3D11_FILL_SOLID;  // this will done in a geometry shader like in the FixedFuncEMU
                                      // sample  and the shader needs solid
        case PM_WIREFRAME:
            return D3D11_FILL_WIREFRAME;
        case PM_SOLID:
            return D3D11_FILL_SOLID;
        }
        return D3D11_FILL_SOLID;
    }
    //---------------------------------------------------------------------
    D3D11_STENCIL_OP D3D11Mappings::get( StencilOperation op, bool invert )
    {
        switch( op )
        {
        case SOP_KEEP:
            return D3D11_STENCIL_OP_KEEP;
        case SOP_ZERO:
            return D3D11_STENCIL_OP_ZERO;
        case SOP_REPLACE:
            return D3D11_STENCIL_OP_REPLACE;
        case SOP_INCREMENT:
            return invert ? D3D11_STENCIL_OP_DECR_SAT : D3D11_STENCIL_OP_INCR_SAT;
        case SOP_DECREMENT:
            return invert ? D3D11_STENCIL_OP_INCR_SAT : D3D11_STENCIL_OP_DECR_SAT;
        case SOP_INCREMENT_WRAP:
            return invert ? D3D11_STENCIL_OP_DECR : D3D11_STENCIL_OP_INCR;
        case SOP_DECREMENT_WRAP:
            return invert ? D3D11_STENCIL_OP_INCR : D3D11_STENCIL_OP_DECR;
        case SOP_INVERT:
            return D3D11_STENCIL_OP_INVERT;
        }
        return D3D11_STENCIL_OP_KEEP;
    }
    //---------------------------------------------------------------------
    D3D11_FILTER D3D11Mappings::get( const FilterOptions min, const FilterOptions mag,
                                     const FilterOptions mip, const bool comparison )
    {
        // anisotropic means trilinear and anisotropic, handle this case early
        if( min == FO_ANISOTROPIC || mag == FO_ANISOTROPIC || mip == FO_ANISOTROPIC )
            return comparison ? D3D11_FILTER_COMPARISON_ANISOTROPIC : D3D11_FILTER_ANISOTROPIC;

            // FilterOptions::FO_NONE is not supported
#define MERGE_FOR_SWITCH( _comparison_, _min_, _mag_, _mip_ ) \
    ( ( _comparison_ ? 8 : 0 ) | ( _min_ == FO_LINEAR ? 4 : 0 ) | ( _mag_ == FO_LINEAR ? 2 : 0 ) | \
      ( _mip_ == FO_LINEAR ? 1 : 0 ) )

        switch( ( MERGE_FOR_SWITCH( comparison, min, mag, mip ) ) )
        {
        case MERGE_FOR_SWITCH( true, FO_POINT, FO_POINT, FO_POINT ):
            return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        case MERGE_FOR_SWITCH( true, FO_POINT, FO_POINT, FO_LINEAR ):
            return D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
        case MERGE_FOR_SWITCH( true, FO_POINT, FO_LINEAR, FO_POINT ):
            return D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case MERGE_FOR_SWITCH( true, FO_POINT, FO_LINEAR, FO_LINEAR ):
            return D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
        case MERGE_FOR_SWITCH( true, FO_LINEAR, FO_POINT, FO_POINT ):
            return D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
        case MERGE_FOR_SWITCH( true, FO_LINEAR, FO_POINT, FO_LINEAR ):
            return D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case MERGE_FOR_SWITCH( true, FO_LINEAR, FO_LINEAR, FO_POINT ):
            return D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        case MERGE_FOR_SWITCH( true, FO_LINEAR, FO_LINEAR, FO_LINEAR ):
            return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        case MERGE_FOR_SWITCH( false, FO_POINT, FO_POINT, FO_POINT ):
            return D3D11_FILTER_MIN_MAG_MIP_POINT;
        case MERGE_FOR_SWITCH( false, FO_POINT, FO_POINT, FO_LINEAR ):
            return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        case MERGE_FOR_SWITCH( false, FO_POINT, FO_LINEAR, FO_POINT ):
            return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case MERGE_FOR_SWITCH( false, FO_POINT, FO_LINEAR, FO_LINEAR ):
            return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
        case MERGE_FOR_SWITCH( false, FO_LINEAR, FO_POINT, FO_POINT ):
            return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        case MERGE_FOR_SWITCH( false, FO_LINEAR, FO_POINT, FO_LINEAR ):
            return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case MERGE_FOR_SWITCH( false, FO_LINEAR, FO_LINEAR, FO_POINT ):
            return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        case MERGE_FOR_SWITCH( false, FO_LINEAR, FO_LINEAR, FO_LINEAR ):
            return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        }
#undef MERGE_FOR_SWITCH

        return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    }
    //---------------------------------------------------------------------
    D3D11_MAP D3D11Mappings::get( v1::HardwareBuffer::LockOptions options,
                                  v1::HardwareBuffer::Usage usage )
    {
        D3D11_MAP ret = D3D11_MAP_READ_WRITE;
        if( options == v1::HardwareBuffer::HBL_DISCARD )
        {
            // D3D doesn't like discard or no_overwrite on non-dynamic buffers
            if( usage & v1::HardwareBuffer::HBU_DYNAMIC )
                ret = D3D11_MAP_WRITE_DISCARD;
        }
        if( options == v1::HardwareBuffer::HBL_READ_ONLY )
        {
            // D3D debug runtime doesn't like you locking managed buffers readonly
            // when they were created with write-only (even though you CAN read
            // from the software backed version)
            if( !( usage & v1::HardwareBuffer::HBU_WRITE_ONLY ) )
                ret = D3D11_MAP_READ;
        }
        if( options == v1::HardwareBuffer::HBL_NO_OVERWRITE )
        {
            // D3D doesn't like discard or no_overwrite on non-dynamic buffers
            if( usage & v1::HardwareBuffer::HBU_DYNAMIC )
                ret = D3D11_MAP_WRITE_NO_OVERWRITE;
        }

        return ret;
    }
    //---------------------------------------------------------------------
    DXGI_FORMAT D3D11Mappings::getFormat( v1::HardwareIndexBuffer::IndexType itype )
    {
        return itype == v1::HardwareIndexBuffer::IT_32BIT ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
    }
    //---------------------------------------------------------------------
    DXGI_FORMAT D3D11Mappings::get( VertexElementType vType )
    {
        switch( vType )
        {
        case VET_COLOUR:
        case VET_COLOUR_ABGR:
        case VET_COLOUR_ARGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case VET_FLOAT1:
            return DXGI_FORMAT_R32_FLOAT;
        case VET_FLOAT2:
            return DXGI_FORMAT_R32G32_FLOAT;
        case VET_FLOAT3:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case VET_FLOAT4:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case VET_SHORT2:
            return DXGI_FORMAT_R16G16_SINT;
        case VET_SHORT4:
            return DXGI_FORMAT_R16G16B16A16_SINT;
        case VET_USHORT2:
            return DXGI_FORMAT_R16G16_UINT;
        case VET_USHORT4:
            return DXGI_FORMAT_R16G16B16A16_UINT;
        case VET_INT1:
            return DXGI_FORMAT_R32_SINT;
        case VET_INT2:
            return DXGI_FORMAT_R32G32_SINT;
        case VET_INT3:
            return DXGI_FORMAT_R32G32B32_SINT;
        case VET_INT4:
            return DXGI_FORMAT_R32G32B32A32_SINT;
        case VET_UINT1:
            return DXGI_FORMAT_R32_UINT;
        case VET_UINT2:
            return DXGI_FORMAT_R32G32_UINT;
        case VET_UINT3:
            return DXGI_FORMAT_R32G32B32_UINT;
        case VET_UINT4:
            return DXGI_FORMAT_R32G32B32A32_UINT;
        case VET_BYTE4:
            return DXGI_FORMAT_R8G8B8A8_SINT;
        case VET_BYTE4_SNORM:
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case VET_UBYTE4_NORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case VET_SHORT2_SNORM:
            return DXGI_FORMAT_R16G16_SNORM;
        case VET_SHORT4_SNORM:
            return DXGI_FORMAT_R16G16B16A16_SNORM;
        case VET_USHORT2_NORM:
            return DXGI_FORMAT_R16G16_UNORM;
        case VET_USHORT4_NORM:
            return DXGI_FORMAT_R16G16B16A16_UNORM;
        case VET_HALF2:
            return DXGI_FORMAT_R16G16_FLOAT;
        case VET_HALF4:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case VET_UBYTE4:
            return DXGI_FORMAT_R8G8B8A8_UINT;
        }
        // to keep compiler happy
        return DXGI_FORMAT_R32G32B32_FLOAT;
    }
    //---------------------------------------------------------------------
    VertexElementSemantic D3D11Mappings::get( LPCSTR sem )
    {
        // todo - add to ogre - POSITIONT and PSIZE ("Transformed vertex position" and "Point size")

        if( strcmp( sem, "BLENDINDICES" ) == 0 )
            return VES_BLEND_INDICES;
        if( strcmp( sem, "BLENDWEIGHT" ) == 0 )
            return VES_BLEND_WEIGHTS;
        if( strcmp( sem, "COLOR" ) == 0 )
            return VES_DIFFUSE;
        // if( strcmp(sem, "COLOR") == 0 )
        //  return VES_SPECULAR;
        if( strcmp( sem, "NORMAL" ) == 0 )
            return VES_NORMAL;
        if( strcmp( sem, "POSITION" ) == 0 )
            return VES_POSITION;
        if( strcmp( sem, "TEXCOORD" ) == 0 )
            return VES_TEXTURE_COORDINATES;
        if( strcmp( sem, "BINORMAL" ) == 0 )
            return VES_BINORMAL;
        if( strcmp( sem, "TANGENT" ) == 0 )
            return VES_TANGENT;

        // to keep compiler happy
        return VES_POSITION;
    }
    //---------------------------------------------------------------------
    LPCSTR D3D11Mappings::get( VertexElementSemantic sem )
    {
        // todo - add to ogre - POSITIONT and PSIZE ("Transformed vertex position" and "Point size")
        switch( sem )
        {
        case VES_BLEND_INDICES:
            return "BLENDINDICES";
        case VES_BLEND_WEIGHTS:
            return "BLENDWEIGHT";
        case VES_DIFFUSE:
            return "COLOR";  // NB index will differentiate
        case VES_SPECULAR:
            return "COLOR";  // NB index will differentiate
        case VES_NORMAL:
            return "NORMAL";
        case VES_POSITION:
            return "POSITION";
        case VES_TEXTURE_COORDINATES:
            return "TEXCOORD";
        case VES_BINORMAL:
            return "BINORMAL";
        case VES_TANGENT:
            return "TANGENT";
        }
        // to keep compiler happy
        return "";
    }
    //---------------------------------------------------------------------
    void D3D11Mappings::get( const ColourValue &inColour, float *outColour )
    {
        outColour[0] = inColour.r;
        outColour[1] = inColour.g;
        outColour[2] = inColour.b;
        outColour[3] = inColour.a;
    }
    //---------------------------------------------------------------------
    D3D11_USAGE D3D11Mappings::_getUsage( v1::HardwareBuffer::Usage usage )
    {
        return _isDynamic( usage ) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    }
    //---------------------------------------------------------------------
    bool D3D11Mappings::_isDynamic( v1::HardwareBuffer::Usage usage )
    {
        return ( usage & v1::HardwareBuffer::HBU_DYNAMIC ) != 0;
    }
    //---------------------------------------------------------------------
    UINT D3D11Mappings::_getAccessFlags( v1::HardwareBuffer::Usage usage )
    {
        return _isDynamic( usage ) ? D3D11_CPU_ACCESS_WRITE : 0;
    }
    //---------------------------------------------------------------------
    UINT D3D11Mappings::get( MsaaPatterns::MsaaPatterns msaaPatterns )
    {
        switch( msaaPatterns )
        {
            // clang-format off
        case MsaaPatterns::Undefined:       return 0;
        case MsaaPatterns::Standard:        return D3D11_STANDARD_MULTISAMPLE_PATTERN;
        case MsaaPatterns::Center:          return D3D11_STANDARD_MULTISAMPLE_PATTERN;
        case MsaaPatterns::CenterZero:      return D3D11_CENTER_MULTISAMPLE_PATTERN;
            // clang-format on
        }

        return 0;
    }
    //---------------------------------------------------------------------
    D3D11_SRV_DIMENSION D3D11Mappings::get( TextureTypes::TextureTypes type, bool cubemapsAs2DArrays,
                                            bool forMsaa )
    {
        switch( type )
        {
        case TextureTypes::Unknown:
            return D3D11_SRV_DIMENSION_UNKNOWN;
        case TextureTypes::Type1D:
            return D3D11_SRV_DIMENSION_TEXTURE1D;
        case TextureTypes::Type1DArray:
            return D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
        case TextureTypes::Type2D:
            return forMsaa ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
        case TextureTypes::Type2DArray:
            return forMsaa ? D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY : D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        case TextureTypes::TypeCube:
            return cubemapsAs2DArrays ? D3D11_SRV_DIMENSION_TEXTURE2DARRAY
                                      : D3D11_SRV_DIMENSION_TEXTURECUBE;
        case TextureTypes::TypeCubeArray:
            return cubemapsAs2DArrays ? D3D11_SRV_DIMENSION_TEXTURE2DARRAY
                                      : D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
        case TextureTypes::Type3D:
            return D3D11_SRV_DIMENSION_TEXTURE3D;
        }

        return D3D11_SRV_DIMENSION_UNKNOWN;
    }
    //---------------------------------------------------------------------
    DXGI_FORMAT D3D11Mappings::get( PixelFormatGpu pf )
    {
        switch( pf )
        {
            // clang-format off
        case PFG_UNKNOWN:                   return DXGI_FORMAT_UNKNOWN;
        case PFG_NULL:                      return DXGI_FORMAT_UNKNOWN;
        case PFG_RGBA32_FLOAT:              return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case PFG_RGBA32_UINT:               return DXGI_FORMAT_R32G32B32A32_UINT;
        case PFG_RGBA32_SINT:               return DXGI_FORMAT_R32G32B32A32_SINT;
        case PFG_RGB32_FLOAT:               return DXGI_FORMAT_R32G32B32_FLOAT;
        case PFG_RGB32_UINT:                return DXGI_FORMAT_R32G32B32_UINT;
        case PFG_RGB32_SINT:                return DXGI_FORMAT_R32G32B32_SINT;
        case PFG_RGBA16_FLOAT:              return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case PFG_RGBA16_UNORM:              return DXGI_FORMAT_R16G16B16A16_UNORM;
        case PFG_RGBA16_UINT:               return DXGI_FORMAT_R16G16B16A16_UINT;
        case PFG_RGBA16_SNORM:              return DXGI_FORMAT_R16G16B16A16_SNORM;
        case PFG_RGBA16_SINT:               return DXGI_FORMAT_R16G16B16A16_SINT;
        case PFG_RG32_FLOAT:                return DXGI_FORMAT_R32G32_FLOAT;
        case PFG_RG32_UINT:                 return DXGI_FORMAT_R32G32_UINT;
        case PFG_RG32_SINT:                 return DXGI_FORMAT_R32G32_SINT;
        case PFG_D32_FLOAT_S8X24_UINT:      return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        case PFG_R10G10B10A2_UNORM:         return DXGI_FORMAT_R10G10B10A2_UNORM;
        case PFG_R10G10B10A2_UINT:          return DXGI_FORMAT_R10G10B10A2_UINT;
        case PFG_R11G11B10_FLOAT:           return DXGI_FORMAT_R11G11B10_FLOAT;
        case PFG_RGBA8_UNORM:               return DXGI_FORMAT_R8G8B8A8_UNORM;
        case PFG_RGBA8_UNORM_SRGB:          return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case PFG_RGBA8_UINT:                return DXGI_FORMAT_R8G8B8A8_UINT;
        case PFG_RGBA8_SNORM:               return DXGI_FORMAT_R8G8B8A8_SNORM;
        case PFG_RGBA8_SINT:                return DXGI_FORMAT_R8G8B8A8_SINT;
        case PFG_RG16_FLOAT:                return DXGI_FORMAT_R16G16_FLOAT;
        case PFG_RG16_UNORM:                return DXGI_FORMAT_R16G16_UNORM;
        case PFG_RG16_UINT:                 return DXGI_FORMAT_R16G16_UINT;
        case PFG_RG16_SNORM:                return DXGI_FORMAT_R16G16_SNORM;
        case PFG_RG16_SINT:                 return DXGI_FORMAT_R16G16_SINT;
        case PFG_D32_FLOAT:                 return DXGI_FORMAT_D32_FLOAT;
        case PFG_R32_FLOAT:                 return DXGI_FORMAT_R32_FLOAT;
        case PFG_R32_UINT:                  return DXGI_FORMAT_R32_UINT;
        case PFG_R32_SINT:                  return DXGI_FORMAT_R32_SINT;
        case PFG_D24_UNORM:                 return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case PFG_D24_UNORM_S8_UINT:         return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case PFG_RG8_UNORM:                 return DXGI_FORMAT_R8G8_UNORM;
        case PFG_RG8_UINT:                  return DXGI_FORMAT_R8G8_UINT;
        case PFG_RG8_SNORM:                 return DXGI_FORMAT_R8G8_SNORM;
        case PFG_RG8_SINT:                  return DXGI_FORMAT_R8G8_SINT;
        case PFG_R16_FLOAT:                 return DXGI_FORMAT_R16_FLOAT;
        case PFG_D16_UNORM:                 return DXGI_FORMAT_D16_UNORM;
        case PFG_R16_UNORM:                 return DXGI_FORMAT_R16_UNORM;
        case PFG_R16_UINT:                  return DXGI_FORMAT_R16_UINT;
        case PFG_R16_SNORM:                 return DXGI_FORMAT_R16_SNORM;
        case PFG_R16_SINT:                  return DXGI_FORMAT_R16_SINT;
        case PFG_R8_UNORM:                  return DXGI_FORMAT_R8_UNORM;
        case PFG_R8_UINT:                   return DXGI_FORMAT_R8_UINT;
        case PFG_R8_SNORM:                  return DXGI_FORMAT_R8_SNORM;
        case PFG_R8_SINT:                   return DXGI_FORMAT_R8_SINT;
        case PFG_A8_UNORM:                  return DXGI_FORMAT_A8_UNORM;
        case PFG_R1_UNORM:                  return DXGI_FORMAT_R1_UNORM;
        case PFG_R9G9B9E5_SHAREDEXP:        return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
        case PFG_R8G8_B8G8_UNORM:           return DXGI_FORMAT_R8G8_B8G8_UNORM;
        case PFG_G8R8_G8B8_UNORM:           return DXGI_FORMAT_G8R8_G8B8_UNORM;
        case PFG_BC1_UNORM:                 return DXGI_FORMAT_BC1_UNORM;
        case PFG_BC1_UNORM_SRGB:            return DXGI_FORMAT_BC1_UNORM_SRGB;
        case PFG_BC2_UNORM:                 return DXGI_FORMAT_BC2_UNORM;
        case PFG_BC2_UNORM_SRGB:            return DXGI_FORMAT_BC2_UNORM_SRGB;
        case PFG_BC3_UNORM:                 return DXGI_FORMAT_BC3_UNORM;
        case PFG_BC3_UNORM_SRGB:            return DXGI_FORMAT_BC3_UNORM_SRGB;
        case PFG_BC4_UNORM:                 return DXGI_FORMAT_BC4_UNORM;
        case PFG_BC4_SNORM:                 return DXGI_FORMAT_BC4_SNORM;
        case PFG_BC5_UNORM:                 return DXGI_FORMAT_BC5_UNORM;
        case PFG_BC5_SNORM:                 return DXGI_FORMAT_BC5_SNORM;
        case PFG_B5G6R5_UNORM:              return DXGI_FORMAT_B5G6R5_UNORM;
        case PFG_B5G5R5A1_UNORM:            return DXGI_FORMAT_B5G5R5A1_UNORM;
        case PFG_BGRA8_UNORM:               return DXGI_FORMAT_B8G8R8A8_UNORM;
        case PFG_BGRX8_UNORM:               return DXGI_FORMAT_B8G8R8X8_UNORM;
        case PFG_R10G10B10_XR_BIAS_A2_UNORM:return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
        case PFG_BGRA8_UNORM_SRGB:          return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case PFG_BGRX8_UNORM_SRGB:          return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
        case PFG_BC6H_UF16:                 return DXGI_FORMAT_BC6H_UF16;
        case PFG_BC6H_SF16:                 return DXGI_FORMAT_BC6H_SF16;
        case PFG_BC7_UNORM:                 return DXGI_FORMAT_BC7_UNORM;
        case PFG_BC7_UNORM_SRGB:            return DXGI_FORMAT_BC7_UNORM_SRGB;
#if defined(_WIN32_WINNT_WIN8) && (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
        case PFG_AYUV:                      return DXGI_FORMAT_AYUV;
        case PFG_Y410:                      return DXGI_FORMAT_Y410;
        case PFG_Y416:                      return DXGI_FORMAT_Y416;
        case PFG_NV12:                      return DXGI_FORMAT_NV12;
        case PFG_P010:                      return DXGI_FORMAT_P010;
        case PFG_P016:                      return DXGI_FORMAT_P016;
        case PFG_420_OPAQUE:                return DXGI_FORMAT_420_OPAQUE;
        case PFG_YUY2:                      return DXGI_FORMAT_YUY2;
        case PFG_Y210:                      return DXGI_FORMAT_Y210;
        case PFG_Y216:                      return DXGI_FORMAT_Y216;
        case PFG_NV11:                      return DXGI_FORMAT_NV11;
        case PFG_AI44:                      return DXGI_FORMAT_AI44;
        case PFG_IA44:                      return DXGI_FORMAT_IA44;
        case PFG_P8:                        return DXGI_FORMAT_P8;
        case PFG_A8P8:                      return DXGI_FORMAT_A8P8;
        case PFG_B4G4R4A4_UNORM:            return DXGI_FORMAT_B4G4R4A4_UNORM;
#endif
#if 0
        //TODO: Not always defined. Must research why.
        case PFG_P208:                      return DXGI_FORMAT_P208;
        case PFG_V208:                      return DXGI_FORMAT_V208;
        case PFG_V408:                      return DXGI_FORMAT_V408;
#endif
        case PFG_P208:
        case PFG_V208:
        case PFG_V408:
        case PFG_RGB8_UNORM:
        case PFG_RGB8_UNORM_SRGB:
        case PFG_BGR8_UNORM:
        case PFG_BGR8_UNORM_SRGB:
        case PFG_RGB16_UNORM:
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
            // clang-format on
        default:
            return DXGI_FORMAT_UNKNOWN;
        }

        return DXGI_FORMAT_UNKNOWN;
    }
    //---------------------------------------------------------------------
    DXGI_FORMAT D3D11Mappings::getForSrv( PixelFormatGpu pf )
    {
        switch( pf )
        {
            // clang-format off
        case PFG_D16_UNORM:                 return DXGI_FORMAT_R16_UNORM;
        case PFG_D24_UNORM:                 return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case PFG_D24_UNORM_S8_UINT:         return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case PFG_D32_FLOAT:                 return DXGI_FORMAT_R32_FLOAT;
        case PFG_D32_FLOAT_S8X24_UINT:      return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
            // clang-format on
        default:
            return get( pf );
        }

        return get( pf );
    }
    //---------------------------------------------------------------------
    DXGI_FORMAT D3D11Mappings::getFamily( PixelFormatGpu pf )
    {
        switch( pf )
        {
        case PFG_RGBA32_FLOAT:
        case PFG_RGBA32_UINT:
        case PFG_RGBA32_SINT:
            return DXGI_FORMAT_R32G32B32A32_TYPELESS;
        case PFG_RGB32_FLOAT:
        case PFG_RGB32_UINT:
        case PFG_RGB32_SINT:
            return DXGI_FORMAT_R32G32B32_TYPELESS;
        case PFG_RGBA16_FLOAT:
        case PFG_RGBA16_UNORM:
        case PFG_RGBA16_UINT:
        case PFG_RGBA16_SNORM:
        case PFG_RGBA16_SINT:
            return DXGI_FORMAT_R16G16B16A16_TYPELESS;
        case PFG_RG32_FLOAT:
        case PFG_RG32_UINT:
        case PFG_RG32_SINT:
            return DXGI_FORMAT_R32G32_TYPELESS;

        case PFG_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_R32G8X24_TYPELESS;

        case PFG_R10G10B10A2_UNORM:
        case PFG_R10G10B10A2_UINT:
            return DXGI_FORMAT_R10G10B10A2_TYPELESS;

        case PFG_RGBA8_UNORM:
        case PFG_RGBA8_UNORM_SRGB:
        case PFG_RGBA8_UINT:
        case PFG_RGBA8_SNORM:
        case PFG_RGBA8_SINT:
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;

        case PFG_RG16_FLOAT:
        case PFG_RG16_UNORM:
        case PFG_RG16_UINT:
        case PFG_RG16_SNORM:
        case PFG_RG16_SINT:
            return DXGI_FORMAT_R16G16_TYPELESS;

        case PFG_D32_FLOAT:
        case PFG_R32_FLOAT:
        case PFG_R32_UINT:
        case PFG_R32_SINT:
            return DXGI_FORMAT_R32_TYPELESS;

        case PFG_D24_UNORM:
        case PFG_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_R24G8_TYPELESS;

        case PFG_RG8_UNORM:
        case PFG_RG8_UINT:
        case PFG_RG8_SNORM:
        case PFG_RG8_SINT:
            return DXGI_FORMAT_R8G8_TYPELESS;

        case PFG_R16_FLOAT:
        case PFG_D16_UNORM:
        case PFG_R16_UNORM:
        case PFG_R16_UINT:
        case PFG_R16_SNORM:
        case PFG_R16_SINT:
            return DXGI_FORMAT_R16_TYPELESS;

        case PFG_R8_UNORM:
        case PFG_R8_UINT:
        case PFG_R8_SNORM:
        case PFG_R8_SINT:
            return DXGI_FORMAT_R8_TYPELESS;

        case PFG_BC1_UNORM:
        case PFG_BC1_UNORM_SRGB:
            return DXGI_FORMAT_BC1_TYPELESS;
        case PFG_BC2_UNORM:
        case PFG_BC2_UNORM_SRGB:
            return DXGI_FORMAT_BC2_TYPELESS;
        case PFG_BC3_UNORM:
        case PFG_BC3_UNORM_SRGB:
            return DXGI_FORMAT_BC3_TYPELESS;
        case PFG_BC4_UNORM:
        case PFG_BC4_SNORM:
            return DXGI_FORMAT_BC4_TYPELESS;
        case PFG_BC5_UNORM:
        case PFG_BC5_SNORM:
            return DXGI_FORMAT_BC5_TYPELESS;

        case PFG_BGRA8_UNORM:
        case PFG_BGRA8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_TYPELESS;

        case PFG_BGRX8_UNORM:
        case PFG_BGRX8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8X8_TYPELESS;

        case PFG_BC6H_UF16:
        case PFG_BC6H_SF16:
            return DXGI_FORMAT_BC6H_TYPELESS;

        case PFG_BC7_UNORM:
        case PFG_BC7_UNORM_SRGB:
            return DXGI_FORMAT_BC7_TYPELESS;

        default:
            return get( pf );
        }

        return get( pf );
    }
}  // namespace Ogre
