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

#include "OgreStableHeaders.h"

#include "OgreStagingTextureBufferImpl.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreCommon.h"

namespace Ogre
{
    StagingTextureBufferImpl::StagingTextureBufferImpl( VaoManager *vaoManager,
                                                        PixelFormatGpu formatFamily,
                                                        size_t size, size_t internalBufferStart,
                                                        size_t vboPoolIdx ) :
        StagingTexture( vaoManager, formatFamily ),
        mInternalBufferStart( internalBufferStart ),
        mCurrentOffset( 0 ),
        mSize( size ),
        mVboPoolIdx( vboPoolIdx )
    {
    }
    //-----------------------------------------------------------------------------------
	StagingTextureBufferImpl::~StagingTextureBufferImpl()
    {
    }
    //-----------------------------------------------------------------------------------
    bool StagingTextureBufferImpl::supportsFormat( uint32 width, uint32 height, uint32 depth,
                                                   uint32 slices, PixelFormatGpu pixelFormat ) const
    {
        const uint32 rowAlignment = 4u;
        size_t requiredSize = PixelFormatGpuUtils::getSizeBytes( width, height, depth, slices,
                                                                 pixelFormat, rowAlignment );
        return requiredSize <= mSize;
    }
    //-----------------------------------------------------------------------------------
    bool StagingTextureBufferImpl::isSmallerThan( const StagingTexture *other ) const
    {
        assert( dynamic_cast<const StagingTextureBufferImpl*>( other ) );
        return this->mSize < static_cast<const StagingTextureBufferImpl*>( other )->mSize;
    }
    //-----------------------------------------------------------------------------------
    size_t StagingTextureBufferImpl::_getSizeBytes(void)
    {
        return mSize;
    }
    //-----------------------------------------------------------------------------------
    void StagingTextureBufferImpl::startMapRegion(void)
    {
        StagingTexture::startMapRegion();
        mCurrentOffset = 0;
    }
    //-----------------------------------------------------------------------------------
    TextureBox StagingTextureBufferImpl::mapRegionImpl( uint32 width, uint32 height, uint32 depth,
                                                        uint32 slices, PixelFormatGpu pixelFormat )
    {
        const uint32 rowAlignment = 4u;
        const size_t sizeBytes = PixelFormatGpuUtils::getSizeBytes( width, height, depth, slices,
                                                                    pixelFormat, rowAlignment );

        if( PixelFormatGpuUtils::isCompressed( pixelFormat ) )
        {
            //Ensure the offset is always aligned to the block size for compressed formats
            const size_t blockSizeBytes = PixelFormatGpuUtils::getCompressedBlockSize( pixelFormat );
            mCurrentOffset = alignToNextMultiple( mCurrentOffset, blockSizeBytes );
            mCurrentOffset = std::min( mCurrentOffset, mSize );
        }

        TextureBox retVal;

        const size_t availableSize = mSize - mCurrentOffset;

        if( sizeBytes <= availableSize )
        {
            retVal.width    = width;
            retVal.height   = height;
            retVal.depth    = depth;
            retVal.numSlices= slices;
            retVal.bytesPerPixel = PixelFormatGpuUtils::getBytesPerPixel( pixelFormat );
            retVal.bytesPerRow = PixelFormatGpuUtils::getSizeBytes( width, 1, 1, 1,
                                                                    pixelFormat, rowAlignment );
            retVal.bytesPerImage = PixelFormatGpuUtils::getSizeBytes( width, height, 1, 1,
                                                                      pixelFormat, rowAlignment );
            retVal.data = mapRegionImplRawPtr();
            mCurrentOffset += sizeBytes;
        }

        return retVal;
    }
}
