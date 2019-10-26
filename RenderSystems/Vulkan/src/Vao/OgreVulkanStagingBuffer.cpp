/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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

#include "Vao/OgreVulkanStagingBuffer.h"

#include "Vao/OgreVulkanBufferInterface.h"
#include "Vao/OgreVulkanVaoManager.h"

#include "OgreStringConverter.h"

namespace Ogre
{
    VulkanStagingBuffer::VulkanStagingBuffer( size_t internalBufferStart, size_t sizeBytes,
                                              VaoManager *vaoManager, bool uploadOnly ) :
        StagingBuffer( internalBufferStart, sizeBytes, vaoManager, uploadOnly ),
        mMappedPtr( 0 ),
        mVulkanDataPtr( 0 )
    {
        mVulkanDataPtr =
            reinterpret_cast<uint8 *>( OGRE_MALLOC_SIMD( sizeBytes, MEMCATEGORY_RENDERSYS ) );
    }
    //-----------------------------------------------------------------------------------
    VulkanStagingBuffer::~VulkanStagingBuffer()
    {
        OGRE_FREE_SIMD( mVulkanDataPtr, MEMCATEGORY_RENDERSYS );
        mVulkanDataPtr = 0;
    }
    //-----------------------------------------------------------------------------------
    void *VulkanStagingBuffer::mapImpl( size_t sizeBytes )
    {
        assert( mUploadOnly );

        mMappingStart = 0;
        mMappingCount = sizeBytes;

        mMappedPtr = mVulkanDataPtr + mInternalBufferStart + mMappingStart;

        return mMappedPtr;
    }
    //-----------------------------------------------------------------------------------
    void VulkanStagingBuffer::unmapImpl( const Destination *destinations, size_t numDestinations )
    {
        mMappedPtr = 0;

        for( size_t i = 0; i < numDestinations; ++i )
        {
            const Destination &dst = destinations[i];

            VulkanBufferInterface *bufferInterface =
                static_cast<VulkanBufferInterface *>( dst.destination->getBufferInterface() );

            assert( dst.destination->getBufferType() == BT_DEFAULT );

            size_t dstOffset = dst.dstOffset + dst.destination->_getInternalBufferStart() *
                                                   dst.destination->getBytesPerElement();

            uint8 *dstPtr = bufferInterface->getVulkanDataPtr();

            memcpy( dstPtr + dstOffset,
                    mVulkanDataPtr + mInternalBufferStart + mMappingStart + dst.srcOffset, dst.length );
        }
    }
    //-----------------------------------------------------------------------------------
    StagingStallType VulkanStagingBuffer::uploadWillStall( size_t sizeBytes )
    {
        assert( mUploadOnly );
        return STALL_NONE;
    }
    //-----------------------------------------------------------------------------------
    //
    //  DOWNLOADS
    //
    //-----------------------------------------------------------------------------------
    size_t VulkanStagingBuffer::_asyncDownload( BufferPacked *source, size_t srcOffset,
                                                size_t srcLength )
    {
        size_t freeRegionOffset = getFreeDownloadRegion( srcLength );
        size_t errorCode = -1;  // dodge comile error about signed/unsigned compare

        if( freeRegionOffset == errorCode )
        {
            OGRE_EXCEPT(
                Exception::ERR_INVALIDPARAMS,
                "Cannot download the request amount of " + StringConverter::toString( srcLength ) +
                    " bytes to this staging buffer. "
                    "Try another one (we're full of requests that haven't been read by CPU yet)",
                "VulkanStagingBuffer::_asyncDownload" );
        }

        assert( !mUploadOnly );
        assert( dynamic_cast<VulkanBufferInterface *>( source->getBufferInterface() ) );
        assert( ( srcOffset + srcLength ) <= source->getTotalSizeBytes() );

        VulkanBufferInterface *bufferInterface =
            static_cast<VulkanBufferInterface *>( source->getBufferInterface() );

        uint8 *srcPtr = bufferInterface->getVulkanDataPtr();

        memcpy( mVulkanDataPtr + mInternalBufferStart + freeRegionOffset,
                srcPtr + source->_getFinalBufferStart() * source->getBytesPerElement() + srcOffset,
                srcLength );

        return freeRegionOffset;
    }
    //-----------------------------------------------------------------------------------
    const void *VulkanStagingBuffer::_mapForReadImpl( size_t offset, size_t sizeBytes )
    {
        assert( !mUploadOnly );

        mMappingStart = offset;
        mMappingCount = sizeBytes;

        mMappedPtr = mVulkanDataPtr + mInternalBufferStart + mMappingStart;

        // Put the mapped region back to our records as "available" for subsequent _asyncDownload
        _cancelDownload( offset, sizeBytes );

        return mMappedPtr;
    }
}  // namespace Ogre
