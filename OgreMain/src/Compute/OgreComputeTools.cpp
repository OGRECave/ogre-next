/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

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

#include "OgreStableHeaders.h"

#include "Compute/OgreComputeTools.h"

#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"

#include "OgreTextureGpu.h"
#include "OgrePixelFormatGpuUtils.h"

namespace Ogre
{
    ComputeTools::ComputeTools( HlmsCompute *hlmsCompute ) :
        mHlmsCompute( hlmsCompute )
    {
    }
    //-------------------------------------------------------------------------
    void ComputeTools::clearUav(TextureGpu *texture, uint32 clearValue[4] )
    {
        const bool bIsInteger = PixelFormatGpuUtils::isInteger( texture->getPixelFormat() );
        const bool bIsSigned = PixelFormatGpuUtils::isSigned( texture->getPixelFormat() );

        OGRE_ASSERT_LOW( !PixelFormatGpuUtils::isCompressed( texture->getPixelFormat() ) );

        HlmsComputeJob *job = mHlmsCompute->findComputeJobNoThrow( "Compute/Tools/ClearUav" );

        if( !job )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "To use ComputeTools, Ogre must be build with JSON support "
                         "and you must include the resources bundled at "
                         "Samples/Media/Compute/Tools",
                         "ComputeTools::clearUav" );
        }

        DescriptorSetUav::TextureSlot uavSlot( DescriptorSetUav::TextureSlot::makeEmpty() );
        uavSlot.texture = texture;
        uavSlot.access = ResourceAccess::Write;
        job->_setUavTexture( 0, uavSlot );

        ShaderParams &shaderParams = job->getShaderParams( "default" );
        shaderParams.mParams.clear();

        ShaderParams::Param param;
        if( !bIsInteger )
        {
            param.name = "fClearValue";
            param.setManualValue( reinterpret_cast<float*>( clearValue ), 4u );
        }
        else
        {
            if( !bIsSigned )
            {
                param.name = "uClearValue";
                param.setManualValue( reinterpret_cast<uint32*>( clearValue ), 4u );
            }
            else
            {
                param.name = "iClearValue";
                param.setManualValue( reinterpret_cast<int32*>( clearValue ), 4u );
            }
        }
        shaderParams.mParams.push_back( param );
        shaderParams.setDirty();

        uint32 threadsPerGroup[3];
        switch( texture->getTextureType() )
        {
        case TextureTypes::Type1D:
            threadsPerGroup[0] = 64u;   threadsPerGroup[1] = 1u;    threadsPerGroup[2] = 1u;
            break;
        case TextureTypes::Unknown:
        case TextureTypes::Type1DArray:
        case TextureTypes::Type2D:
            threadsPerGroup[0] = 8u;    threadsPerGroup[1] = 8u;    threadsPerGroup[2] = 1u;
            break;
        case TextureTypes::Type2DArray:
        case TextureTypes::TypeCube:
        case TextureTypes::TypeCubeArray:
        case TextureTypes::Type3D:
            threadsPerGroup[0] = 4u;    threadsPerGroup[1] = 4u;    threadsPerGroup[2] = 4u;
            break;
        }
        job->setThreadsPerGroup( threadsPerGroup[0], threadsPerGroup[1], threadsPerGroup[2] );

        mHlmsCompute->dispatch( job, 0, 0 );
    }
    //-------------------------------------------------------------------------
    void ComputeTools::clearUavFloat( TextureGpu *texture, float clearValue[4] )
    {
        OGRE_ASSERT_LOW( !PixelFormatGpuUtils::isInteger( texture->getPixelFormat() ) );
        clearUav( texture, reinterpret_cast<uint32*>( clearValue ) );
    }
    //-------------------------------------------------------------------------
    void ComputeTools::clearUavUint( TextureGpu *texture, uint32 clearValue[4] )
    {
        OGRE_ASSERT_LOW( PixelFormatGpuUtils::isInteger( texture->getPixelFormat() ) );
        clearUav( texture, clearValue );
    }
}
