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

#include "OgreVulkanDiscardBufferManager.h"

#include "OgreStringConverter.h"
#include "OgreVulkanDevice.h"
#include "OgreVulkanUtils.h"

Ogre::VulkanDiscardBufferManager::VulkanDiscardBufferManager( VulkanDevice *device,
                                                              VaoManager *vaoManager ) :
    mBuffer( 0 ),
    mDevice( device ),
    mVaoManager( vaoManager )
{
    const size_t defaultCapacity = 4 * 1024 * 1024;

    VulkanVaoManager *vaoMagr = static_cast<VulkanVaoManager *>( vaoManager );
    const uint32 *memoryTypeIndex = vaoMagr->getBestVkMemoryTypeIndex();

    VkBufferCreateInfo bufferCi;
    makeVkStruct( bufferCi, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO );
    bufferCi.size = defaultCapacity;
    bufferCi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VkResult result = vkCreateBuffer( mDevice->mDevice, &bufferCi, 0, &mBuffer );
    checkVkResult( result, "vkCreateBuffer" );

    vkGetBufferMemoryRequirements( mDevice->mDevice, mBuffer, &mMemRequirements );

    VkMemoryAllocateInfo memAllocInfo;
    makeVkStruct( memAllocInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO );
    memAllocInfo.allocationSize = mMemRequirements.size;
    memAllocInfo.memoryTypeIndex = memoryTypeIndex[VulkanVaoManager::CPU_ACCESSIBLE_PERSISTENT_COHERENT];

    result = vkAllocateMemory( mDevice->mDevice, &memAllocInfo, NULL, &mDeviceMemory );
    checkVkResult( result, "vkAllocateMemory" );

    // TODO use one large buffer and multiple offsets
    VkDeviceSize offset = 0;
    offset = alignMemory( offset, mMemRequirements.alignment );

    result = vkBindBufferMemory( mDevice->mDevice, mBuffer, mDeviceMemory, offset );
    checkVkResult( result, "vkBindBufferMemory" );

    mFreeBlocks.push_back( VulkanVaoManager::Block( 0, defaultCapacity ) );
}

Ogre::VulkanDiscardBufferManager::~VulkanDiscardBufferManager()
{
    VulkanDiscardBufferVec::const_iterator itor = mDiscardBuffers.begin();
    VulkanDiscardBufferVec::const_iterator end = mDiscardBuffers.end();

    while( itor != end )
        OGRE_DELETE *itor++;
    mDiscardBuffers.clear();

    vkDestroyBuffer( mDevice->mDevice, mBuffer, 0 );
    vkFreeMemory( mDevice->mDevice, mDeviceMemory, 0 );
    mDeviceMemory = 0;
    mBuffer = 0;
}

void Ogre::VulkanDiscardBufferManager::growToFit( size_t extraBytes,
                                                  VulkanDiscardBuffer *forDiscardBuffer )
{
    assert( !( extraBytes & 0x04 ) && "extraBytes must be multiple of 4!" );

    const size_t oldCapacity = mMemRequirements.size;
    const size_t newCapacity =
        std::max( oldCapacity + extraBytes, oldCapacity + ( oldCapacity >> 1u ) + 1u );

    VulkanVaoManager *vaoMagr = static_cast<VulkanVaoManager *>( mVaoManager );
    const uint32 *memoryTypeIndex = vaoMagr->getBestVkMemoryTypeIndex();

    VkBuffer oldBuffer = mBuffer;
    VkDeviceMemory oldMemory = mDeviceMemory;

    VkBufferCreateInfo bufferCi;
    makeVkStruct( bufferCi, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO );
    bufferCi.size = newCapacity;
    bufferCi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;;
    VkResult result = vkCreateBuffer( mDevice->mDevice, &bufferCi, 0, &mBuffer );
    checkVkResult( result, "vkCreateBuffer" );

    vkGetBufferMemoryRequirements( mDevice->mDevice, mBuffer, &mMemRequirements );

    VkMemoryAllocateInfo memAllocInfo;
    makeVkStruct( memAllocInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO );
    memAllocInfo.allocationSize = mMemRequirements.size;
    memAllocInfo.memoryTypeIndex = memoryTypeIndex[VulkanVaoManager::CPU_ACCESSIBLE_PERSISTENT_COHERENT];

    result = vkAllocateMemory( mDevice->mDevice, &memAllocInfo, NULL, &mDeviceMemory );
    checkVkResult( result, "vkAllocateMemory" );

    // TODO use one large buffer and multiple offsets
    VkDeviceSize offset = 0;
    offset = alignMemory( offset, mMemRequirements.alignment );

    result = vkBindBufferMemory( mDevice->mDevice, mBuffer, mDeviceMemory, offset );
    checkVkResult( result, "vkBindBufferMemory" );

    {
        // Update our buffers so they point to the new buffer, copy their blocks in use from old
        // MTLBuffer to new one, and tag all of them as in use by GPU (due to the copyFromBuffer);
        // except 'forDiscardBuffer' which we were told this data won't be used.

        const uint32 currentFrame = mVaoManager->getFrameCount();
        VulkanDiscardBufferVec::iterator itor = mDiscardBuffers.begin();
        VulkanDiscardBufferVec::iterator end = mDiscardBuffers.end();

        while( itor != end )
        {
            if( *itor != forDiscardBuffer )
            {
                ( *itor )->mBuffer = mBuffer;

                VkBufferCopy region;
                region.srcOffset = ( *itor )->getBlockStart();
                region.dstOffset = ( *itor )->getBlockStart();
                region.size = alignToNextMultiple( ( *itor )->getBlockSize(), 4u );
                vkCmdCopyBuffer( mDevice->mGraphicsQueue.mCurrentCmdBuffer, oldBuffer, mBuffer, 1u,
                                 &region );
                // [blitEncoder copyFromBuffer:oldBuffer
                //                sourceOffset:( *itor )->getBlockStart()
                //                    toBuffer:mBuffer
                //           destinationOffset:( *itor )->getBlockStart()
                //                        size:( *itor )->getBlockSize()];
                ( *itor )->mLastFrameUsed = currentFrame;
            }
            else
            {
                ( *itor )->mLastFrameUsed = currentFrame - mVaoManager->getDynamicBufferMultiplier();
            }

            ++itor;
        }
    }

    LogManager::getSingleton().logMessage(
        "PERFORMANCE WARNING: MetalDiscardBufferManager::growToFit must stall."
        "Consider increasing the default discard capacity to at least " +
        StringConverter::toString( newCapacity ) + " bytes" );

    // According to Metal docs, it's undefined behavior if both CPU & GPU
    // write to the same resource even if the regions don't overlap.
    mDevice->stall();

    mFreeBlocks.push_back( VulkanVaoManager::Block( oldCapacity, newCapacity - oldCapacity ) );

    {
        // All "unsafe" blocks are no longer unsafe, since we're using a new buffer.
        UnsafeBlockVec::const_iterator itor = mUnsafeBlocks.begin();
        UnsafeBlockVec::const_iterator end = mUnsafeBlocks.end();

        while( itor != end )
        {
            mFreeBlocks.push_back( *itor );
            VulkanVaoManager::mergeContiguousBlocks( mFreeBlocks.end() - 1, mFreeBlocks );
            ++itor;
        }

        mUnsafeBlocks.clear();
    }
}

void Ogre::VulkanDiscardBufferManager::updateUnsafeBlocks()
{
    const uint32 currentFrame = mVaoManager->getFrameCount();
    const uint32 bufferMultiplier = mVaoManager->getDynamicBufferMultiplier();

    UnsafeBlockVec::iterator itor = mUnsafeBlocks.begin();
    UnsafeBlockVec::iterator end = mUnsafeBlocks.end();

    while( itor != end && ( currentFrame - itor->lastFrameUsed ) >= bufferMultiplier )
    {
        // This block is safe now to put back into free blocks.
        mFreeBlocks.push_back( *itor );
        VulkanVaoManager::mergeContiguousBlocks( mFreeBlocks.end() - 1, mFreeBlocks );
        ++itor;
    }

    mUnsafeBlocks.erase( mUnsafeBlocks.begin(), itor );
}

void Ogre::VulkanDiscardBufferManager::_notifyDeviceStalled()
{
    {
        UnsafeBlockVec::iterator itor = mUnsafeBlocks.begin();
        UnsafeBlockVec::iterator end = mUnsafeBlocks.end();

        while( itor != end )
        {
            // This block is safe now to put back into free blocks.
            mFreeBlocks.push_back( *itor );
            VulkanVaoManager::mergeContiguousBlocks( mFreeBlocks.end() - 1, mFreeBlocks );
            ++itor;
        }

        mUnsafeBlocks.clear();
    }

    {
        const uint32 currentFrame = mVaoManager->getFrameCount();
        const uint32 bufferMultiplier = mVaoManager->getDynamicBufferMultiplier();

        VulkanDiscardBufferVec::const_iterator itor = mDiscardBuffers.begin();
        VulkanDiscardBufferVec::const_iterator end = mDiscardBuffers.end();

        while( itor != end )
        {
            ( *itor )->mLastFrameUsed = currentFrame - bufferMultiplier;
            ++itor;
        }
    }
}

void Ogre::VulkanDiscardBufferManager::_getBlock( VulkanDiscardBuffer *discardBuffer )
{
    const size_t alignment = discardBuffer->getAlignment();
    const size_t sizeBytes = discardBuffer->getSizeBytes();

    if( discardBuffer->mBuffer )
    {
        if( mVaoManager->getFrameCount() - discardBuffer->mLastFrameUsed >=
            mVaoManager->getDynamicBufferMultiplier() )
        {
            return;  // Current block the buffer owns is safe to reuse.
        }
        else
        {
            // Release the block back into the pool (sorted by mLastFrameUsed)
            UnsafeBlock unsafeBlock( discardBuffer->getBlockStart(), discardBuffer->getBlockSize(),
                                     discardBuffer->mLastFrameUsed );
            UnsafeBlockVec::iterator it =
                std::lower_bound( mUnsafeBlocks.begin(), mUnsafeBlocks.end(), unsafeBlock );
            mUnsafeBlocks.insert( it, unsafeBlock );
        }
    }

    updateUnsafeBlocks();

    // Find smallest block.
    VulkanVaoManager::BlockVec::iterator itor = mFreeBlocks.begin();
    VulkanVaoManager::BlockVec::iterator end = mFreeBlocks.end();

    VulkanVaoManager::BlockVec::iterator smallestBlock = end;

    while( itor != end )
    {
        const size_t alignedOffset =
            std::min( itor->offset + itor->size, alignToNextMultiple( itor->offset, alignment ) );

        if( sizeBytes <= itor->size - ( alignedOffset - itor->offset ) )
        {
            // We can use 'itor' block. Now check if it's smaller.
            if( smallestBlock == end || itor->size < smallestBlock->size )
            {
                smallestBlock = itor;
            }
        }

        ++itor;
    }

    if( smallestBlock == end )
    {
        // No block can satisfy us. Resize (slow!)
        growToFit( sizeBytes, discardBuffer );

        // Recursive call: Try again. This time it will work since we have the space.
        discardBuffer->mBuffer = 0;
        _getBlock( discardBuffer );
        discardBuffer->mBuffer = mBuffer;
    }
    else
    {
        // Assign this block and shrink our records.
        discardBuffer->mBufferOffset = alignToNextMultiple( smallestBlock->offset, alignment );
        discardBuffer->mBlockPrePadding = discardBuffer->mBufferOffset - smallestBlock->offset;

        const size_t shrunkBytes = discardBuffer->getBlockSize();

        smallestBlock->offset = discardBuffer->mBufferOffset + discardBuffer->mBufferSize;
        smallestBlock->size -= shrunkBytes;

        if( smallestBlock->size == 0 )
        {
            mFreeBlocks.erase( smallestBlock );
        }
    }
}

Ogre::VulkanDiscardBuffer *Ogre::VulkanDiscardBufferManager::createDiscardBuffer( size_t bufferSize,
                                                                                  uint16 alignment )
{
    alignment = std::max<uint16>( 4u, alignment );  // Prevent alignments lower than 4 bytes.
    VulkanDiscardBuffer *retVal =
        OGRE_NEW VulkanDiscardBuffer( bufferSize, alignment, mVaoManager, mDevice, this );
    mDiscardBuffers.push_back( retVal );
    _getBlock( retVal );
    retVal->mBuffer = mBuffer;
    retVal->mDeviceMemory = mDeviceMemory;

    return retVal;
}

void Ogre::VulkanDiscardBufferManager::destroyDiscardBuffer( VulkanDiscardBuffer *discardBuffer )
{
    VulkanDiscardBufferVec::iterator itor =
        std::find( mDiscardBuffers.begin(), mDiscardBuffers.end(), discardBuffer );

    if( itor != mDiscardBuffers.end() )
    {
        assert( discardBuffer->mOwner == this &&
                "Manager says it owns the discard buffer, but discard buffer says it doesn't" );

        // Release the block back into the pool (sorted by mLastFrameUsed)
        UnsafeBlock unsafeBlock( discardBuffer->getBlockStart(), discardBuffer->getBlockSize(),
                                 discardBuffer->mLastFrameUsed );
        UnsafeBlockVec::iterator it =
            std::lower_bound( mUnsafeBlocks.begin(), mUnsafeBlocks.end(), unsafeBlock );
        mUnsafeBlocks.insert( it, unsafeBlock );

        efficientVectorRemove( mDiscardBuffers, itor );
        OGRE_DELETE discardBuffer;
    }
    else
    {
        OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR,
                     "discardBuffer doesn't belong to this "
                     "MetalDiscardBufferManager or was already freed",
                     "MetalDiscardBufferManager::destroyDiscardBuffer" );
    }
}

Ogre::VulkanDiscardBuffer::VulkanDiscardBuffer( size_t bufferSize, uint16 alignment,
                                                VaoManager *vaoManager, VulkanDevice *device,
                                                VulkanDiscardBufferManager *owner ) :
    mBuffer( 0 ),
    mDevice( device ),
    mBlockPrePadding( 0 ),
    mBufferOffset( 0 ),
    mBufferSize( bufferSize ),
    mAlignment( alignment ),
    mLastFrameUsed( vaoManager->getFrameCount() - vaoManager->getDynamicBufferMultiplier() ),
    mVaoManager( vaoManager ),
    mOwner( owner )
{
}

void *Ogre::VulkanDiscardBuffer::map( bool noOverwrite )
{
    if( !noOverwrite )
        mOwner->_getBlock( this );

    VulkanVaoManager *vaoMagr = static_cast<VulkanVaoManager *>( mVaoManager );
    const size_t offset =
        alignMemory( mBufferOffset, mDevice->mDeviceProperties.limits.nonCoherentAtomSize );
    void *data = 0;
    VkResult result =
        vkMapMemory( mDevice->mDevice, mDeviceMemory,
                     /*mInternalBufferStart + mMappingStart*/ offset, mBufferSize, 0, &data );
    checkVkResult( result, "vkMapMemory" );
    return data;
}

void Ogre::VulkanDiscardBuffer::unmap()
{
    vkUnmapMemory( mDevice->mDevice, mDeviceMemory );
}

VkBuffer Ogre::VulkanDiscardBuffer::getBufferName( size_t &outOffset )
{
    mLastFrameUsed = mVaoManager->getFrameCount();
    outOffset = mBufferOffset;
    return mBuffer;
}
