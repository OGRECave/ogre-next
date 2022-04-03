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

#include "OgreRenderSystemCapabilities.h"

#include "OgreLogManager.h"
#include "OgreString.h"

namespace Ogre {

    //-----------------------------------------------------------------------
    RenderSystemCapabilities::RenderSystemCapabilities()
        : mVendor(GPU_UNKNOWN)
        , mDeviceId(0)
        , mNumWorldMatrices(0)
        , mNumTextureUnits(0)
        , mStencilBufferBitDepth(0)
        , mNumVertexBlendMatrices(0)
        , mNumMultiRenderTargets(1)
        , mMaxTextureResolution2D(1024)
        , mMaxTextureResolution3D(512)
        , mMaxTextureResolutionCubemap(512)
        , mNonPOW2TexturesLimited(false)
        , mMaxSupportedAnisotropy(0)
        , mVertexTextureUnitsShared(0)
        , mGeometryProgramNumOutputVertices(0)
        , mMaxThreadsPerThreadgroup(1024u)
    {
        for(int i = 0; i < CAPS_CATEGORY_COUNT; i++)
        {
            mCapabilities[i] = 0;
        }
        mCategoryRelevant[CAPS_CATEGORY_COMMON] = true;
        mCategoryRelevant[CAPS_CATEGORY_COMMON_2] = true;
        // each rendersystem should enable these
        mCategoryRelevant[CAPS_CATEGORY_D3D9] = false;
        mCategoryRelevant[CAPS_CATEGORY_GL] = false;

        mMaxThreadsPerThreadgroupAxis[0] = 1024u;
        mMaxThreadsPerThreadgroupAxis[1] = 1024u;
        mMaxThreadsPerThreadgroupAxis[2] = 64u;

        // OpenGL & Vulkan guarantee at least 16. D3D11 at least 128. Metal 31
        for( size_t i = 0u; i < NumShaderTypes + 1u; ++i )
            mNumTexturesInTextureDescriptor[i] = 16u;
    }
    //-----------------------------------------------------------------------
    RenderSystemCapabilities::~RenderSystemCapabilities()
    {
    }
    //-----------------------------------------------------------------------
    void RenderSystemCapabilities::log(Log* pLog)
    {
        pLog->logMessage("RenderSystem capabilities");
        pLog->logMessage("-------------------------");
        pLog->logMessage("RenderSystem Name: " + getRenderSystemName());
        pLog->logMessage("GPU Vendor: " + vendorToString(getVendor()));
        pLog->logMessage("Device Name: " + getDeviceName());
        pLog->logMessage("Driver Version: " + getDriverVersion().toString());
        pLog->logMessage(" * Fixed function pipeline: " 
            + StringConverter::toString(hasCapability(RSC_FIXED_FUNCTION), true));
        pLog->logMessage(
            " * Hardware generation of mipmaps: "
            + StringConverter::toString(hasCapability(RSC_AUTOMIPMAP), true));
        pLog->logMessage(
            " * Texture blending: "
            + StringConverter::toString(hasCapability(RSC_BLENDING), true));
        pLog->logMessage(
            " * Anisotropic texture filtering: "
            + StringConverter::toString(hasCapability(RSC_ANISOTROPY), true));
        pLog->logMessage(
            " * Dot product texture operation: "
            + StringConverter::toString(hasCapability(RSC_DOT3), true));
        pLog->logMessage(
            " * Cube mapping: "
            + StringConverter::toString(hasCapability(RSC_CUBEMAPPING), true));
        pLog->logMessage(
            " * Hardware stencil buffer: "
            + StringConverter::toString(hasCapability(RSC_HWSTENCIL), true));
        if (hasCapability(RSC_HWSTENCIL))
        {
            pLog->logMessage(
                "   - Stencil depth: "
                + StringConverter::toString(getStencilBufferBitDepth()));
            pLog->logMessage(
                "   - Two sided stencil support: "
                + StringConverter::toString(hasCapability(RSC_TWO_SIDED_STENCIL), true));
            pLog->logMessage(
                "   - Wrap stencil values: "
                + StringConverter::toString(hasCapability(RSC_STENCIL_WRAP), true));
        }
        pLog->logMessage(
            " * Hardware vertex / index buffers: "
            + StringConverter::toString(hasCapability(RSC_VBO), true));
        if(hasCapability(RSC_VBO))
        {
            pLog->logMessage(
                " * 32-bit index buffers: "
                + StringConverter::toString(hasCapability(RSC_32BIT_INDEX), true));
        }
        pLog->logMessage(
            " * Vertex programs: "
            + StringConverter::toString(hasCapability(RSC_VERTEX_PROGRAM), true));
        pLog->logMessage(
             " * Number of floating-point constants for vertex programs: "
             + StringConverter::toString(mVertexProgramConstantFloatCount));
        pLog->logMessage(
             " * Number of integer constants for vertex programs: "
             + StringConverter::toString(mVertexProgramConstantIntCount));
        pLog->logMessage(
             " * Number of boolean constants for vertex programs: "
             + StringConverter::toString(mVertexProgramConstantBoolCount));
        pLog->logMessage(
            " * Fragment programs: "
            + StringConverter::toString(hasCapability(RSC_FRAGMENT_PROGRAM), true));
        pLog->logMessage(
             " * Number of floating-point constants for fragment programs: "
             + StringConverter::toString(mFragmentProgramConstantFloatCount));
        pLog->logMessage(
             " * Number of integer constants for fragment programs: "
             + StringConverter::toString(mFragmentProgramConstantIntCount));
        pLog->logMessage(
             " * Number of boolean constants for fragment programs: "
             + StringConverter::toString(mFragmentProgramConstantBoolCount));
        pLog->logMessage(
            " * Geometry programs: "
            + StringConverter::toString(hasCapability(RSC_GEOMETRY_PROGRAM), true));
        pLog->logMessage(
             " * Number of floating-point constants for geometry programs: "
             + StringConverter::toString(mGeometryProgramConstantFloatCount));
        pLog->logMessage(
             " * Number of integer constants for geometry programs: "
             + StringConverter::toString(mGeometryProgramConstantIntCount));
        pLog->logMessage(
             " * Number of boolean constants for geometry programs: "
             + StringConverter::toString(mGeometryProgramConstantBoolCount));
        pLog->logMessage(
            " * Tessellation Hull programs: "
            + StringConverter::toString(hasCapability(RSC_TESSELLATION_HULL_PROGRAM), true));
        pLog->logMessage(
             " * Number of floating-point constants for tessellation hull programs: "
             + StringConverter::toString(mTessellationHullProgramConstantFloatCount));
        pLog->logMessage(
             " * Number of integer constants for tessellation hull programs: "
             + StringConverter::toString(mTessellationHullProgramConstantIntCount));
        pLog->logMessage(
             " * Number of boolean constants for tessellation hull programs: "
             + StringConverter::toString(mTessellationHullProgramConstantBoolCount));
        pLog->logMessage(
            " * Tessellation Domain programs: "
            + StringConverter::toString(hasCapability(RSC_TESSELLATION_DOMAIN_PROGRAM), true));
        pLog->logMessage(
             " * Number of floating-point constants for tessellation domain programs: "
             + StringConverter::toString(mTessellationDomainProgramConstantFloatCount));
        pLog->logMessage(
             " * Number of integer constants for tessellation domain programs: "
             + StringConverter::toString(mTessellationDomainProgramConstantIntCount));
        pLog->logMessage(
             " * Number of boolean constants for tessellation domain programs: "
             + StringConverter::toString(mTessellationDomainProgramConstantBoolCount));
        pLog->logMessage(
            " * Compute programs: "
            + StringConverter::toString(hasCapability(RSC_COMPUTE_PROGRAM), true));
        pLog->logMessage(
             " * Number of floating-point constants for compute programs: "
             + StringConverter::toString(mComputeProgramConstantFloatCount));
        pLog->logMessage(
             " * Number of integer constants for compute programs: "
             + StringConverter::toString(mComputeProgramConstantIntCount));
        pLog->logMessage(
             " * Number of boolean constants for compute programs: "
             + StringConverter::toString(mComputeProgramConstantBoolCount));
        String profileList = "";
        for(ShaderProfiles::iterator iter = mSupportedShaderProfiles.begin(), end = mSupportedShaderProfiles.end();
            iter != end; ++iter)
        {
            profileList += " " + *iter;
        }
        pLog->logMessage(" * Supported Shader Profiles:" + profileList);

        pLog->logMessage(
            " * Texture Compression: "
            + StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION), true));
        if (hasCapability(RSC_TEXTURE_COMPRESSION))
        {
            pLog->logMessage(
                "   - DXT: "
                + StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_DXT), true));
            pLog->logMessage(
                "   - VTC: "
                + StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_VTC), true));
            pLog->logMessage(
                 "   - PVRTC: "
                 + StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_PVRTC), true));
            pLog->logMessage(
                 "   - ATC: "
                 + StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_ATC), true));
            pLog->logMessage(
                 "   - ETC1: "
                 + StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_ETC1), true));
            pLog->logMessage(
                 "   - ETC2: "
                 + StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_ETC2), true));
            pLog->logMessage(
                 "   - BC4/BC5: "
                 + StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_BC4_BC5), true));
            pLog->logMessage(
                 "   - BC6H/BC7: "
                 + StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_BC6H_BC7), true));
            pLog->logMessage(
                 "   - ASTC: "
                 + StringConverter::toString(hasCapability(RSC_TEXTURE_COMPRESSION_ASTC), true));
        }

        pLog->logMessage(
            " * Hardware Occlusion Query: "
            + StringConverter::toString(hasCapability(RSC_HWOCCLUSION), true));
        pLog->logMessage(
            " * User clip planes: "
            + StringConverter::toString(hasCapability(RSC_USER_CLIP_PLANES), true));
        pLog->logMessage(
            " * VET_UBYTE4 vertex element type: "
            + StringConverter::toString(hasCapability(RSC_VERTEX_FORMAT_UBYTE4), true));
        pLog->logMessage(
            " * Infinite far plane projection: "
            + StringConverter::toString(hasCapability(RSC_INFINITE_FAR_PLANE), true));
        pLog->logMessage(
            " * Hardware render-to-texture: "
            + StringConverter::toString(hasCapability(RSC_HWRENDER_TO_TEXTURE), true));
        pLog->logMessage(
            " * Floating point textures: "
            + StringConverter::toString(hasCapability(RSC_TEXTURE_FLOAT), true));
        pLog->logMessage(
            " * Non-power-of-two textures: "
            + StringConverter::toString(hasCapability(RSC_NON_POWER_OF_2_TEXTURES), true)
            + (mNonPOW2TexturesLimited ? " (limited)" : ""));
        pLog->logMessage(
            " * 1d textures: "
            + StringConverter::toString(hasCapability(RSC_TEXTURE_1D), true));
        pLog->logMessage(
            " * Volume textures: "
            + StringConverter::toString(hasCapability(RSC_TEXTURE_3D), true));
        pLog->logMessage( " * Max Texture resolution (2D) "
            + StringConverter::toString(mMaxTextureResolution2D) );
        pLog->logMessage( " * Max Texture resolution (3D) "
            + StringConverter::toString(mMaxTextureResolution3D) );
        pLog->logMessage( " * Max Texture resolution (Cubemaps) "
            + StringConverter::toString(mMaxTextureResolutionCubemap) );
        pLog->logMessage(
            " * Multiple Render Targets: "
            + StringConverter::toString(mNumMultiRenderTargets));
        pLog->logMessage(
            "   - With different bit depths: " + StringConverter::toString(hasCapability(RSC_MRT_DIFFERENT_BIT_DEPTHS), true));
        pLog->logMessage(
            " * Point Sprites: "
            + StringConverter::toString(hasCapability(RSC_POINT_SPRITES), true));
        pLog->logMessage(
            " * Extended point parameters: "
            + StringConverter::toString(hasCapability(RSC_POINT_EXTENDED_PARAMETERS), true));
        if(hasCapability(RSC_POINT_SPRITES))
        {
            pLog->logMessage(
                " * Max Point Size: "
                + StringConverter::toString(mMaxPointSize));
        }
        pLog->logMessage(
            " * Vertex texture fetch: "
            + StringConverter::toString(hasCapability(RSC_VERTEX_TEXTURE_FETCH), true));
        pLog->logMessage(
             " * Number of world matrices: "
             + StringConverter::toString(mNumWorldMatrices));
        pLog->logMessage(
             " * Number of texture units: "
             + StringConverter::toString(mNumTextureUnits));
        pLog->logMessage(
             " * Stencil buffer depth: "
             + StringConverter::toString(mStencilBufferBitDepth));
        pLog->logMessage(
             " * Number of vertex blend matrices: "
             + StringConverter::toString(mNumVertexBlendMatrices));
        if (hasCapability(RSC_VERTEX_TEXTURE_FETCH))
        {
            pLog->logMessage(
                "   - Max vertex textures: "
                + StringConverter::toString(mNumVertexTextureUnits));
            pLog->logMessage(
                "   - Vertex textures shared: "
                + StringConverter::toString(mVertexTextureUnitsShared, true));

        }
        pLog->logMessage(
            " * Render to Vertex Buffer : "
            + StringConverter::toString(hasCapability(RSC_HWRENDER_TO_VERTEX_BUFFER), true));
        pLog->logMessage(
            " * Hardware Atomic Counters: "
            + StringConverter::toString(hasCapability(RSC_ATOMIC_COUNTERS), true));

        pLog->logMessage( " * Shader 16-bit floating point (half): " +
                          StringConverter::toString( hasCapability( RSC_SHADER_FLOAT16 ), true ) );

        if( hasCapability( RSC_COMPUTE_PROGRAM ) )
        {
            pLog->logMessage(
                " * Compute max threads per threadgroup per axis: "
                + StringConverter::toString( mMaxThreadsPerThreadgroupAxis[0] ) + ", "
                + StringConverter::toString( mMaxThreadsPerThreadgroupAxis[1] ) + ", "
                + StringConverter::toString( mMaxThreadsPerThreadgroupAxis[2] ) );
            pLog->logMessage(
                " * Compute max threads per threadgroup total: "
                + StringConverter::toString( mMaxThreadsPerThreadgroup ) );
        }

        if (mCategoryRelevant[CAPS_CATEGORY_GL])
        {
            pLog->logMessage(
                " * GL 1.5 without VBO workaround: "
                + StringConverter::toString(hasCapability(RSC_GL1_5_NOVBO), true));

            pLog->logMessage(
                " * Frame Buffer objects: "
                + StringConverter::toString(hasCapability(RSC_FBO), true));
            pLog->logMessage(
                " * Frame Buffer objects (ARB extension): "
                + StringConverter::toString(hasCapability(RSC_FBO_ARB), true));
            pLog->logMessage(
                " * Frame Buffer objects (ATI extension): "
                + StringConverter::toString(hasCapability(RSC_FBO_ATI), true));
            pLog->logMessage(
                " * PBuffer support: "
                + StringConverter::toString(hasCapability(RSC_PBUFFER), true));
            pLog->logMessage(
                " * GL 1.5 without HW-occlusion workaround: "
                + StringConverter::toString(hasCapability(RSC_GL1_5_NOHWOCCLUSION), true));
            pLog->logMessage(
                " * Vertex Array Objects: "
                + StringConverter::toString(hasCapability(RSC_VAO), true));
        }

        if (mCategoryRelevant[CAPS_CATEGORY_D3D9])
        {
            pLog->logMessage(
                " * DirectX per stage constants: "
                + StringConverter::toString(hasCapability(RSC_PERSTAGECONSTANT), true));
        }
    }
    //---------------------------------------------------------------------
    StringVector RenderSystemCapabilities::msGPUVendorStrings;
    //---------------------------------------------------------------------
    GPUVendor RenderSystemCapabilities::vendorFromString(const String& vendorString)
    {
        initVendorStrings();
        GPUVendor ret = GPU_UNKNOWN;
        String cmpString = vendorString;
        StringUtil::toLowerCase(cmpString);
        for( size_t i = 0; i < GPU_VENDOR_COUNT; ++i )
        {
            // case insensitive (lower case)
            if( msGPUVendorStrings[i] == cmpString )
            {
                ret = static_cast<GPUVendor>( i );
                break;
            }
        }

        return ret;
        
    }
    //---------------------------------------------------------------------
    String RenderSystemCapabilities::vendorToString(GPUVendor v)
    {
        initVendorStrings();
        return msGPUVendorStrings[v];
    }
    //---------------------------------------------------------------------
    void RenderSystemCapabilities::initVendorStrings()
    {
        if (msGPUVendorStrings.empty())
        {
            // Always lower case!
            msGPUVendorStrings.resize(GPU_VENDOR_COUNT);
            msGPUVendorStrings[GPU_UNKNOWN] = "unknown";
            msGPUVendorStrings[GPU_NVIDIA] = "nvidia";
            msGPUVendorStrings[GPU_AMD] = "amd";
            msGPUVendorStrings[GPU_INTEL] = "intel";
            msGPUVendorStrings[GPU_3DLABS] = "3dlabs";
            msGPUVendorStrings[GPU_S3] = "s3";
            msGPUVendorStrings[GPU_MATROX] = "matrox";
            msGPUVendorStrings[GPU_SIS] = "sis";
            msGPUVendorStrings[GPU_IMAGINATION_TECHNOLOGIES] = "imagination technologies";
            msGPUVendorStrings[GPU_APPLE] = "apple";    // iOS Simulator
            msGPUVendorStrings[GPU_NOKIA] = "nokia";
            msGPUVendorStrings[GPU_MS_SOFTWARE] = "microsoft"; // Microsoft software device
            msGPUVendorStrings[GPU_MS_WARP] = "ms warp";
            msGPUVendorStrings[GPU_ARM] = "arm";
            msGPUVendorStrings[GPU_QUALCOMM] = "qualcomm";
            msGPUVendorStrings[GPU_MOZILLA] = "mozilla";
            msGPUVendorStrings[GPU_WEBKIT] = "webkit";
            msGPUVendorStrings[GPU_IMGTEC] = "powervr";
        }
    }
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    void DriverVersion::fromString( const String &versionString )
    {
        StringVector tokens = StringUtil::split( versionString, "." );
        if( !tokens.empty() )
        {
            major = StringConverter::parseInt( tokens[0] );
            if( tokens.size() > 1 )
                minor = StringConverter::parseInt( tokens[1] );
            if( tokens.size() > 2 )
                release = StringConverter::parseInt( tokens[2] );
            if( tokens.size() > 3 )
                build = StringConverter::parseInt( tokens[3] );
        }
    }
}
