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

#include "OgreVulkanHardwareBufferCommon.h"
#include "OgreVulkanDevice.h"
#include "OgreVulkanDiscardBufferManager.h"
#include "Vao/OgreVulkanStagingBuffer.h"
#include "Vao/OgreVulkanVaoManager.h"
#include "OgreStringConverter.h"
#include "OgreVulkanUtils.h"

namespace Ogre
{
namespace v1
{
    VulkanHardwareBufferCommon::VulkanHardwareBufferCommon( size_t sizeBytes,
        HardwareBuffer::Usage usage, uint16 alignment, VulkanDiscardBufferManager *discardBufferManager,
        VulkanDevice *device ) :
        mBuffer( 0 ),
        mDeviceMemory( 0 ),
        mSizeBytes( sizeBytes ),
        mDevice( device ),
        mDiscardBuffer( 0 ),
        mVaoManager( discardBufferManager->getVaoManager() ),
        mStagingBuffer( 0 )
    {
        mLastFrameUsed = mVaoManager->getFrameCount() - mVaoManager->getDynamicBufferMultiplier();
        mLastFrameGpuWrote = mLastFrameUsed;

        VulkanVaoManager *vaoManager = static_cast<VulkanVaoManager *>( mVaoManager );
        const uint32 *memoryTypeIndex = vaoManager->getBestVkMemoryTypeIndex();

         VkBufferCreateInfo bufferCi;
        makeVkStruct( bufferCi, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO );
        bufferCi.size = sizeBytes;
        bufferCi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                         VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
                         VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        VkResult result = vkCreateBuffer( mDevice->mDevice, &bufferCi, 0, &mBuffer );
        checkVkResult( result, "vkCreateBuffer" );

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements( mDevice->mDevice, mBuffer, &memRequirements );

        VkMemoryAllocateInfo memAllocInfo;
        makeVkStruct( memAllocInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO );
        memAllocInfo.allocationSize = memRequirements.size;

        if( usage & HardwareBuffer::HBU_WRITE_ONLY )
        {
            memAllocInfo.memoryTypeIndex = memoryTypeIndex[VulkanVaoManager::CPU_INACCESSIBLE];
            mVboFlag = VulkanVaoManager::CPU_INACCESSIBLE;
        }
        else
        {
            memAllocInfo.memoryTypeIndex = memoryTypeIndex[VulkanVaoManager::CPU_ACCESSIBLE_PERSISTENT_COHERENT];
            mVboFlag = VulkanVaoManager::CPU_ACCESSIBLE_PERSISTENT_COHERENT;
        }

        if( !( usage & HardwareBuffer::HBU_DISCARDABLE ) )
        {
            VkResult result = vkAllocateMemory( mDevice->mDevice, &memAllocInfo, NULL, &mDeviceMemory );
            checkVkResult( result, "vkAllocateMemory" );

            // TODO use one large buffer and multiple offsets
            VkDeviceSize offset = 0;
            offset = alignMemory( offset, memRequirements.alignment );

            result = vkBindBufferMemory( mDevice->mDevice, mBuffer, mDeviceMemory, offset );
            checkVkResult( result, "vkBindBufferMemory" );
        }
        else
        {
            mDiscardBuffer = discardBufferManager->createDiscardBuffer( sizeBytes, alignment );
        }
    }

    VulkanHardwareBufferCommon::~VulkanHardwareBufferCommon()
    {
        vkDestroyBuffer( mDevice->mDevice, mBuffer, 0 );
        mBuffer = 0;
        if( mDiscardBuffer )
        {
            VulkanDiscardBufferManager *discardBufferManager = mDiscardBuffer->getOwner();
            discardBufferManager->destroyDiscardBuffer( mDiscardBuffer );
            mDiscardBuffer = 0;
        }
        else
        {
            vkFreeMemory( mDevice->mDevice, mDeviceMemory, 0 );
            mDeviceMemory = 0;
        }
    }

    void VulkanHardwareBufferCommon::_notifyDeviceStalled()
    {
        mLastFrameUsed = mVaoManager->getFrameCount() - mVaoManager->getDynamicBufferMultiplier();
        mLastFrameGpuWrote = mLastFrameUsed;
    }

    VkBuffer VulkanHardwareBufferCommon::getBufferName( size_t &outOffset )
    {
        mLastFrameUsed = mVaoManager->getFrameCount();
        outOffset = 0;
        return !mDiscardBuffer ? mBuffer : mDiscardBuffer->getBufferName( outOffset );
    }

    VkBuffer VulkanHardwareBufferCommon::getBufferNameForGpuWrite()
    {
        assert( !mDiscardBuffer && "Discardable buffers can't be written from GPU!" );
        mLastFrameUsed = mVaoManager->getFrameCount();
        mLastFrameGpuWrote = mLastFrameUsed;
        return mBuffer;
    }

    void * VulkanHardwareBufferCommon::lockImpl( size_t offset, size_t length,
        HardwareBuffer::LockOptions options, bool isLocked )
    {
        if( isLocked )
        {
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "Invalid attempt to lock a buffer that has already been locked",
                         "MetalHardwareBufferCommon::lock" );
        }

        void *retPtr = 0;

        const uint32 currentFrame = mVaoManager->getFrameCount();
        const uint32 bufferMultiplier = mVaoManager->getDynamicBufferMultiplier();

        if( mDiscardBuffer )
        {
            // If we're here, it was created with HBU_DISCARDABLE bit
            if( options == HardwareBuffer::HBL_READ_ONLY )
            {
                LogManager::getSingleton().logMessage(
                    "PERFORMANCE WARNING: reads from discardable "
                    "buffers are uncached. May be slow." );

                // We can't write from GPU to discardable memory. No need to check
            }
            else if( options == HardwareBuffer::HBL_NORMAL || options == HardwareBuffer::HBL_WRITE_ONLY )
            {
                if( currentFrame - mLastFrameUsed < bufferMultiplier )
                {
                    LogManager::getSingleton().logMessage(
                        "PERFORMANCE WARNING: locking with HBL_NORMAL or HBL_WRITE_ONLY for a "
                        "buffer created with HBU_DISCARDABLE bit is slow/stalling. Consider "
                        "locking w/ another locking option, or change the buffer's usage flag" );
                    mDevice->stall();
                }
            }

            retPtr = mDiscardBuffer->map( options != HardwareBuffer::HBL_DISCARD );
            retPtr = static_cast<void *>( static_cast<uint8 *>( retPtr ) + offset );
        }
        else
        {
            if( mVboFlag != VulkanVaoManager::CPU_INACCESSIBLE )
            {
                if( options == HardwareBuffer::HBL_READ_ONLY )
                {
                    if( currentFrame - mLastFrameGpuWrote < bufferMultiplier )
                        mDevice->stall();
                }
                else if( options != HardwareBuffer::HBL_NO_OVERWRITE )
                {
                    if( currentFrame - mLastFrameUsed < bufferMultiplier )
                    {
                        LogManager::getSingleton().logMessage(
                            "PERFORMANCE WARNING: locking to a non-HBU_WRITE_ONLY or "
                            "non-HBU_DISCARDABLE for other than reading is slow/stalling." );

                        mDevice->stall();
                    }
                }
                mMappingCount = alignMemory(
                    length, mDevice->mDeviceProperties.limits.nonCoherentAtomSize );

                VkResult result =
                    vkMapMemory( mDevice->mDevice, mDeviceMemory, /*mInternalBufferStart + mMappingStart*/offset,
                                               mMappingCount, 0, &retPtr );
                checkVkResult( result, "vkMapMemory" );
            }
            else
            {
                // If we're here, the buffer was created with HBU_WRITE_ONLY, but not discardable.
                // Write to a staging buffer to avoid blocking. We don't have to care about
                // reading access.
                assert( ( options != HardwareBuffer::HBL_NORMAL ||
                          options != HardwareBuffer::HBL_READ_ONLY ) &&
                        "Reading from a write-only buffer! Create "
                        "the buffer without HBL_WRITE_ONLY bit (or use readData)" );

                assert( !mStagingBuffer && "Invalid state, and mStagingBuffer will leak" );

                mStagingBuffer = mVaoManager->getStagingBuffer( length, true );
                retPtr = mStagingBuffer->map( length );
            }
        }

        return retPtr;
    }

    void VulkanHardwareBufferCommon::unlockImpl( size_t lockStart, size_t lockSize )
    {
        if( mDiscardBuffer )
        {
            mDiscardBuffer->unmap();
        }
        else if( mStagingBuffer )
        {
            static_cast<VulkanStagingBuffer *>( mStagingBuffer )->_unmapToV1( this, lockStart, lockSize );
            mStagingBuffer->removeReferenceCount();
            mStagingBuffer = 0;
        }
        else
        {
            vkUnmapMemory( mDevice->mDevice, mDeviceMemory );
        }
    }

    void VulkanHardwareBufferCommon::readData( size_t offset, size_t length, void *pDest )
    {
        assert( ( offset + length ) <= mSizeBytes );

        void const *srcData = 0;
        StagingBuffer *stagingBuffer = 0;

        const uint32 currentFrame = mVaoManager->getFrameCount();
        const uint32 bufferMultiplier = mVaoManager->getDynamicBufferMultiplier();

        if( mDiscardBuffer )
        {
            // We can't write from GPU to discardable memory. No need to check.
            srcData = mDiscardBuffer->map( true );
        }
        else
        {
            if( mVboFlag != VulkanVaoManager::CPU_INACCESSIBLE )
            {
                if( currentFrame - mLastFrameGpuWrote < bufferMultiplier )
                    mDevice->stall();
                mMappingCount =
                    alignMemory( length, mDevice->mDeviceProperties.limits.nonCoherentAtomSize );
                void *data = const_cast<void *>( srcData );
                VkResult result = vkMapMemory( mDevice->mDevice, mDeviceMemory,
                                               /*mInternalBufferStart + mMappingStart*/ offset,
                                               mMappingCount, 0, &data );
                checkVkResult( result, "vkMapMemory" );
            }
            else
            {
                // Reading from HBL_WRITE_ONLY.
                stagingBuffer = mVaoManager->getStagingBuffer( length, false );
                size_t stagingBufferOffset = static_cast<VulkanStagingBuffer *>( stagingBuffer )
                                                 ->_asyncDownloadV1( this, offset, length );
                mDevice->stall();
                srcData = stagingBuffer->_mapForRead( stagingBufferOffset, length );
                offset = 0;
            }
        }

        srcData = static_cast<const void *>( static_cast<const uint8 *>( srcData ) + offset );

        memcpy( pDest, srcData, length );

        if( stagingBuffer )
            stagingBuffer->removeReferenceCount();
    }

    void VulkanHardwareBufferCommon::writeData( size_t offset, size_t length, const void *pSource,
        bool discardWholeBuffer )
    {
        if( ( discardWholeBuffer && mDiscardBuffer ) || mVboFlag == VulkanVaoManager::CPU_INACCESSIBLE )
        {
            // Fast path is through locking (it either discards or already uses a StagingBuffer).
            void *dstData = this->lockImpl( offset, length, HardwareBuffer::HBL_DISCARD, false );
            memcpy( dstData, pSource, length );
            this->unlockImpl( offset, length );
        }
        else
        {
            // Use a StagingBuffer to avoid blocking
            StagingBuffer *stagingBuffer = mVaoManager->getStagingBuffer( length, true );
            void *dstData = stagingBuffer->map( length );
            memcpy( dstData, pSource, length );
            static_cast<VulkanStagingBuffer *>( stagingBuffer )->_unmapToV1( this, offset, length );
            stagingBuffer->removeReferenceCount();
        }
    }

    void VulkanHardwareBufferCommon::copyData( VulkanHardwareBufferCommon *srcBuffer, size_t srcOffset,
        size_t dstOffset, size_t length, bool discardWholeBuffer )
    {
        if( !this->mDiscardBuffer || mVboFlag == VulkanVaoManager::CPU_INACCESSIBLE )
        {
            size_t srcOffsetStart = 0;
            VkBuffer srcBuf = srcBuffer->getBufferName( srcOffsetStart );
            VkBuffer dstBuf = this->getBufferNameForGpuWrite();

            VkBufferCopy region;
            region.srcOffset = srcOffset + srcOffsetStart;
            region.dstOffset = dstOffset;
            region.size = alignToNextMultiple( length, 4u );
            vkCmdCopyBuffer( mDevice->mGraphicsQueue.mCurrentCmdBuffer, srcBuf, dstBuf, 1u, &region );

            // __unsafe_unretained id<MTLBlitCommandEncoder> blitEncoder = mDevice->getBlitEncoder();
            // [blitEncoder copyFromBuffer:srcBuf
            //                sourceOffset:srcOffset + srcOffsetStart
            //                    toBuffer:dstBuf
            //           destinationOffset:dstOffset
            //                        size:length];

            if( this->mDiscardBuffer )
                mDevice->stall();
        }
        else
        {
            HardwareBuffer::LockOptions dstOption;
            if( discardWholeBuffer )
                dstOption = HardwareBuffer::HBL_DISCARD;
            else
                dstOption = HardwareBuffer::HBL_WRITE_ONLY;

            const void *srcData =
                srcBuffer->lockImpl( srcOffset, length, HardwareBuffer::HBL_READ_ONLY, false );
            void *dstData = this->lockImpl( dstOffset, length, dstOption, false );

            memcpy( dstData, srcData, length );

            this->unlockImpl( dstOffset, length );
            srcBuffer->unlockImpl( srcOffset, length );
        }
    }
}
}
