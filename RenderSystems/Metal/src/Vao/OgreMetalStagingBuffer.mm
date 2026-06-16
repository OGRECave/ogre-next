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

#include "Vao/OgreMetalStagingBuffer.h"
#include "Vao/OgreMetalBufferInterface.h"
#include "Vao/OgreMetalVaoManager.h"

#include "OgreMetalDevice.h"
#include "OgreMetalHardwareBufferCommon.h"

#include "OgreStringConverter.h"

#import <Metal/MTLBlitCommandEncoder.h>
#import <Metal/MTLCommandBuffer.h>

namespace Ogre
{
    MetalStagingBuffer::MetalStagingBuffer( size_t internalBufferStart, size_t sizeBytes,
                                            VaoManager *vaoManager, bool uploadOnly,
                                            id<MTLBuffer> vboName, MetalDevice *device ) :
        StagingBuffer( internalBufferStart, sizeBytes, vaoManager, uploadOnly ),
        mVboName( vboName ),
        mMappedPtr( 0 ),
        mDevice( device ),
        mFenceThreshold( sizeBytes / 4 ),
        mUnfencedBytes( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    MetalStagingBuffer::~MetalStagingBuffer()
    {
        if( !mFences.empty() )
            wait( mFences.back().fenceName );

        deleteFences( mFences.begin(), mFences.end() );
    }
    //-----------------------------------------------------------------------------------
    void MetalStagingBuffer::addFence( size_t from, size_t to, bool forceFence )
    {
        assert( to <= mSizeBytes );

        MetalFence unfencedHazard( from, to );

        mUnfencedHazards.push_back( unfencedHazard );

        assert( from <= to );

        mUnfencedBytes += to - from;

        if( mUnfencedBytes >= mFenceThreshold || forceFence )
        {
            MetalFenceVec::const_iterator itor = mUnfencedHazards.begin();
            MetalFenceVec::const_iterator endt = mUnfencedHazards.end();

            size_t startRange = itor->start;
            size_t endRange = itor->end;

            while( itor != endt )
            {
                if( endRange <= itor->end )
                {
                    // Keep growing (merging) the fences into one fence
                    endRange = itor->end;
                }
                else
                {
                    // We wrapped back to 0. Can't keep merging. Make a fence.
                    MetalFence fence( startRange, endRange );
                    fence.fenceName = dispatch_semaphore_create( 0 );

                    __block dispatch_semaphore_t blockSemaphore = fence.fenceName;
                    [mDevice->mCurrentCommandBuffer
                        addCompletedHandler:^( id<MTLCommandBuffer> buffer ) {
                          dispatch_semaphore_signal( blockSemaphore );
                        }];
                    mFences.push_back( fence );

                    startRange = itor->start;
                    endRange = itor->end;
                }

                ++itor;
            }

            // Make the last fence.
            MetalFence fence( startRange, endRange );
            fence.fenceName = dispatch_semaphore_create( 0 );
            __block dispatch_semaphore_t blockSemaphore = fence.fenceName;
            [mDevice->mCurrentCommandBuffer addCompletedHandler:^( id<MTLCommandBuffer> buffer ) {
              dispatch_semaphore_signal( blockSemaphore );
            }];

            // Flush the device for accuracy in the fences.
            mDevice->commitAndNextCommandBuffer();
            mFences.push_back( fence );

            mUnfencedHazards.clear();
            mUnfencedBytes = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalStagingBuffer::wait( dispatch_semaphore_t syncObj )
    {
        long result = dispatch_semaphore_wait( syncObj, DISPATCH_TIME_FOREVER );

        if( result != 0 )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Failure while waiting for a MetalFence. Could be out of GPU memory. "
                         "Update your video card drivers. If that doesn't help, "
                         "contact the developers. Error code: " +
                             StringConverter::toString( result ),
                         "MetalStagingBuffer::wait" );
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalStagingBuffer::deleteFences( MetalFenceVec::iterator itor, MetalFenceVec::iterator endt )
    {
        while( itor != endt )
        {
            // ARC will destroy it
            itor->fenceName = 0;
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalStagingBuffer::waitIfNeeded()
    {
        assert( mUploadOnly );

        size_t mappingStart = mMappingStart;
        size_t sizeBytes = mMappingCount;

        if( mappingStart + sizeBytes > mSizeBytes )
        {
            if( !mUnfencedHazards.empty() )
            {
                // mUnfencedHazards will be cleared in addFence
                addFence( mUnfencedHazards.front().start, mSizeBytes - 1, true );
            }

            // Wraps around the ring buffer. Sadly we can't do advanced virtual memory
            // manipulation to keep the virtual space contiguous, so we'll have to reset to 0
            mappingStart = 0;
        }

        MetalFence regionToMap( mappingStart, mappingStart + sizeBytes );

        MetalFenceVec::iterator itor = mFences.begin();
        MetalFenceVec::iterator endt = mFences.end();

        MetalFenceVec::iterator lastWaitableFence = endt;

        while( itor != endt )
        {
            if( regionToMap.overlaps( *itor ) )
                lastWaitableFence = itor;

            ++itor;
        }

        if( lastWaitableFence != endt )
        {
            wait( lastWaitableFence->fenceName );
            deleteFences( mFences.begin(), lastWaitableFence + 1 );
            mFences.erase( mFences.begin(), lastWaitableFence + 1 );
        }

        mMappingStart = mappingStart;
    }
    //-----------------------------------------------------------------------------------
    void *MetalStagingBuffer::mapImpl( size_t sizeBytes )
    {
        assert( mUploadOnly );

        mMappingCount = alignToNextMultiple<size_t>( sizeBytes, 4u );

        waitIfNeeded();  // Will fill mMappingStart

        mMappedPtr =
            reinterpret_cast<uint8 *>( [mVboName contents] ) + mInternalBufferStart + mMappingStart;

        return mMappedPtr;
    }
    //-----------------------------------------------------------------------------------
    void MetalStagingBuffer::unmapImpl( const Destination *destinations, size_t numDestinations )
    {
        //        if( mUploadOnly )
        //        {
        // #if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        //            NSRange range = NSMakeRange( mInternalBufferStart + mMappingStart, mMappingCount );
        //            [mVboName didModifyRange:range];
        // #endif
        //        }

        mMappedPtr = 0;

        for( size_t i = 0; i < numDestinations; ++i )
        {
            const Destination &dst = destinations[i];

            MetalBufferInterface *bufferInterface =
                static_cast<MetalBufferInterface *>( dst.destination->getBufferInterface() );

            assert( dst.destination->getBufferType() == BT_DEFAULT );

            const size_t dstOffset = dst.dstOffset + dst.destination->_getInternalBufferStart() *
                                                         dst.destination->getBytesPerElement();

            __unsafe_unretained id<MTLBlitCommandEncoder> blitEncoder = mDevice->getBlitEncoder();
            [blitEncoder copyFromBuffer:mVboName
                           sourceOffset:mInternalBufferStart + mMappingStart + dst.srcOffset
                               toBuffer:bufferInterface->getVboName()
                      destinationOffset:dstOffset
                                   size:alignToNextMultiple<size_t>( dst.length, 4u )];
        }

        if( mUploadOnly )
        {
            // Add fence to this region (or at least, track the hazard).
            addFence( mMappingStart, mMappingStart + mMappingCount - 1, false );
        }
    }
    //-----------------------------------------------------------------------------------
    StagingStallType MetalStagingBuffer::uploadWillStall( size_t sizeBytes )
    {
        assert( mUploadOnly );

        size_t mappingStart = mMappingStart;

        StagingStallType retVal = STALL_NONE;

        if( mappingStart + sizeBytes > mSizeBytes )
        {
            if( !mUnfencedHazards.empty() )
            {
                MetalFence regionToMap( 0, sizeBytes );
                MetalFence hazardousRegion( mUnfencedHazards.front().start, mSizeBytes - 1 );

                if( hazardousRegion.overlaps( regionToMap ) )
                {
                    retVal = STALL_FULL;
                    return retVal;
                }
            }

            mappingStart = 0;
        }

        MetalFence regionToMap( mappingStart, mappingStart + sizeBytes );

        MetalFenceVec::iterator itor = mFences.begin();
        MetalFenceVec::iterator endt = mFences.end();

        MetalFenceVec::iterator lastWaitableFence = endt;

        while( itor != endt )
        {
            if( regionToMap.overlaps( *itor ) )
                lastWaitableFence = itor;

            ++itor;
        }

        if( lastWaitableFence != endt )
        {
            // Ask Metal to return immediately and tells us about the fence
            long waitRet = dispatch_semaphore_wait( lastWaitableFence->fenceName, DISPATCH_TIME_NOW );

            if( waitRet != 0 )
            {
                retVal = STALL_PARTIAL;
            }
            else
            {
                deleteFences( mFences.begin(), lastWaitableFence + 1 );
                mFences.erase( mFences.begin(), lastWaitableFence + 1 );
            }
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void MetalStagingBuffer::cleanUnfencedHazards()
    {
        if( !mUnfencedHazards.empty() )
            addFence( mUnfencedHazards.front().start, mUnfencedHazards.back().end, true );
    }
    //-----------------------------------------------------------------------------------
    void MetalStagingBuffer::_notifyDeviceStalled()
    {
        deleteFences( mFences.begin(), mFences.end() );
        mFences.clear();
        mUnfencedHazards.clear();
        mUnfencedBytes = 0;
    }
    //-----------------------------------------------------------------------------------
    void MetalStagingBuffer::_unmapToV1( v1::MetalHardwareBufferCommon *hwBuffer, size_t lockStart,
                                         size_t lockSize )
    {
        assert( mUploadOnly );

        if( mMappingState != MS_MAPPED )
        {
            // This stuff would normally be done in StagingBuffer::unmap
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE, "Unmapping an unmapped buffer!",
                         "MetalStagingBuffer::unmap" );
        }

        //        if( mUploadOnly )
        //        {
        // #if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        //            NSRange range = NSMakeRange( mInternalBufferStart + mMappingStart, mMappingCount );
        //            [mVboName didModifyRange:range];
        // #endif
        //        }

        mMappedPtr = 0;

        __unsafe_unretained id<MTLBlitCommandEncoder> blitEncoder = mDevice->getBlitEncoder();
        [blitEncoder copyFromBuffer:mVboName
                       sourceOffset:mInternalBufferStart + mMappingStart
                           toBuffer:hwBuffer->getBufferNameForGpuWrite()
                  destinationOffset:lockStart
                               size:alignToNextMultiple<size_t>( lockSize, 4u )];

        if( mUploadOnly )
        {
            // Add fence to this region (or at least, track the hazard).
            addFence( mMappingStart, mMappingStart + mMappingCount - 1, false );
        }

        // This stuff would normally be done in StagingBuffer::unmap
        mMappingState = MS_UNMAPPED;
        mMappingStart += mMappingCount;

        if( mMappingStart >= mSizeBytes )
            mMappingStart = 0;
    }
    //-----------------------------------------------------------------------------------
    //
    //  DOWNLOADS
    //
    //-----------------------------------------------------------------------------------
    size_t MetalStagingBuffer::_asyncDownload( BufferPacked *source, size_t srcOffset, size_t srcLength )
    {
        assert( !mUploadOnly );
        assert( dynamic_cast<MetalBufferInterface *>( source->getBufferInterface() ) );
        assert( ( srcOffset + srcLength ) <= source->getTotalSizeBytes() );

        // Metal has alignment restrictions of 4 bytes for offset and size in copyFromBuffer
        // Extend range so that both are multiple of 4, then add extra offset
        // to the return value so it gets correctly mapped in _mapForRead.
        size_t extraOffset = srcOffset & 0x03;
        srcOffset -= extraOffset;
        size_t srcLengthPadded = alignToNextMultiple<size_t>( extraOffset + srcLength, 4u );
        size_t freeRegionOffset = getFreeDownloadRegion( srcLengthPadded );

        if( freeRegionOffset == (size_t)( -1 ) )
        {
            OGRE_EXCEPT(
                Exception::ERR_INVALIDPARAMS,
                "Cannot download the request amount of " + StringConverter::toString( srcLength ) +
                    " bytes to this staging buffer. "
                    "Try another one (we're full of requests that haven't been read by CPU yet)",
                "MetalStagingBuffer::_asyncDownload" );
        }

        MetalBufferInterface *bufferInterface =
            static_cast<MetalBufferInterface *>( source->getBufferInterface() );

        __unsafe_unretained id<MTLBlitCommandEncoder> blitEncoder = mDevice->getBlitEncoder();
        [blitEncoder
               copyFromBuffer:bufferInterface->getVboName()
                 sourceOffset:source->_getFinalBufferStart() * source->getBytesPerElement() + srcOffset
                     toBuffer:mVboName
            destinationOffset:mInternalBufferStart + freeRegionOffset
                         size:srcLengthPadded];
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        assert( mVboName.storageMode != MTLStorageModeManaged );
        //[blitEncoder synchronizeResource:mVboName];
#endif

        return freeRegionOffset + extraOffset;
    }
    //-----------------------------------------------------------------------------------
    bool MetalStagingBuffer::canDownload( size_t length ) const
    {
        return StagingBuffer::canDownload( alignToNextMultiple<size_t>( length, 4u ) );
    }
    //-----------------------------------------------------------------------------------
    void MetalStagingBuffer::_cancelDownload( size_t offset, size_t sizeBytes )
    {
        // If offset isn't multiple of 4, we were making it go forward in
        //_asyncDownload. We need to backtrack it so regions stay contiguous.
        size_t delta = offset & 0x03;
        StagingBuffer::_cancelDownload( offset - delta,
                                        alignToNextMultiple<size_t>( delta + sizeBytes, 4u ) );
    }
    //-----------------------------------------------------------------------------------
    const void *MetalStagingBuffer::_mapForReadImpl( size_t offset, size_t sizeBytes )
    {
        assert( !mUploadOnly );

        mMappingStart = offset;
        mMappingCount = alignToNextMultiple<size_t>( sizeBytes, 4u );

        mMappedPtr =
            reinterpret_cast<uint8 *>( [mVboName contents] ) + mInternalBufferStart + mMappingStart;

        // Put the mapped region back to our records as "available" for subsequent _asyncDownload
        _cancelDownload( offset, sizeBytes );

        return mMappedPtr;
    }
    //-----------------------------------------------------------------------------------
    size_t MetalStagingBuffer::_asyncDownloadV1( v1::MetalHardwareBufferCommon *source, size_t srcOffset,
                                                 size_t srcLength )
    {
        assert( !mUploadOnly );
        assert( ( srcOffset + srcLength ) <= source->getSizeBytes() );

        // Metal has alignment restrictions of 4 bytes for offset and size in copyFromBuffer
        // Extend range so that both are multiple of 4, then add extra offset
        // to the return value so it gets correctly mapped in _mapForRead.
        size_t extraOffset = srcOffset & 0x03;
        srcOffset -= extraOffset;
        size_t srcLengthPadded = alignToNextMultiple<size_t>( extraOffset + srcLength, 4u );
        size_t freeRegionOffset = getFreeDownloadRegion( srcLengthPadded );

        if( freeRegionOffset == (size_t)( -1 ) )
        {
            OGRE_EXCEPT(
                Exception::ERR_INVALIDPARAMS,
                "Cannot download the request amount of " + StringConverter::toString( srcLength ) +
                    " bytes to this staging buffer. "
                    "Try another one (we're full of requests that haven't been read by CPU yet)",
                "MetalStagingBuffer::_asyncDownload" );
        }

        size_t srcOffsetStart = 0;
        __unsafe_unretained id<MTLBuffer> srcBuffer = source->getBufferName( srcOffsetStart );

        __unsafe_unretained id<MTLBlitCommandEncoder> blitEncoder = mDevice->getBlitEncoder();
        [blitEncoder copyFromBuffer:srcBuffer
                       sourceOffset:srcOffset + srcOffsetStart
                           toBuffer:mVboName
                  destinationOffset:mInternalBufferStart + freeRegionOffset
                               size:srcLengthPadded];
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        assert( mVboName.storageMode != MTLStorageModeManaged );
        //[blitEncoder synchronizeResource:mVboName];
#endif

        return freeRegionOffset + extraOffset;
    }
}  // namespace Ogre
