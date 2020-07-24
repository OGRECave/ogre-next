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

#include "Vao/OgreVulkanVaoManager.h"

#include "OgreVulkanDevice.h"
#include "OgreVulkanUtils.h"
#include "Vao/OgreVulkanAsyncTicket.h"
#include "Vao/OgreVulkanBufferInterface.h"
#include "Vao/OgreVulkanConstBufferPacked.h"
#include "Vao/OgreVulkanDynamicBuffer.h"
#include "Vao/OgreVulkanMultiSourceVertexBufferPool.h"
#include "Vao/OgreVulkanStagingBuffer.h"
#include "Vao/OgreVulkanTexBufferPacked.h"
#include "Vao/OgreVulkanUavBufferPacked.h"
#include "Vao/OgreVulkanVertexArrayObject.h"

#include "Vao/OgreIndirectBufferPacked.h"

#include "OgreRenderQueue.h"

#include "OgreStringConverter.h"
#include "OgreTimer.h"

#define TODO_implement
#define TODO_whenImplemented_include_stagingBuffers

namespace Ogre
{
    VulkanVaoManager::VulkanVaoManager( uint8 dynBufferMultiplier, VulkanDevice *device ) :
        VaoManager( 0 ),
        mDrawId( 0 ),
        mDevice( device ),
        mFenceFlushed( true )
    {
        mConstBufferAlignment = 256;
        mTexBufferAlignment = 256;

        mConstBufferMaxSize = 64 * 1024;        // 64kb
        mTexBufferMaxSize = 128 * 1024 * 1024;  // 128MB

        mSupportsPersistentMapping = true;
        mSupportsIndirectBuffers = false;

        // Keep pools of 64MB each for static meshes
        mDefaultPoolSize[CPU_INACCESSIBLE] = 64u * 1024u * 1024u;

        // Keep pools of 4MB each for most dynamic buffers, 16 for the most common one.
        for( size_t i = CPU_ACCESSIBLE_DEFAULT; i <= CPU_ACCESSIBLE_PERSISTENT_COHERENT; ++i )
            mDefaultPoolSize[i] = 4u * 1024u * 1024u;
        mDefaultPoolSize[CPU_ACCESSIBLE_PERSISTENT] = 16u * 1024u * 1024u;

        determineBestMemoryTypes();

        mDynamicBufferMultiplier = dynBufferMultiplier;

        VertexElement2Vec vertexElements;
        vertexElements.push_back( VertexElement2( VET_UINT1, VES_COUNT ) );
        uint32 *drawIdPtr =
            static_cast<uint32 *>( OGRE_MALLOC_SIMD( 4096 * sizeof( uint32 ), MEMCATEGORY_GEOMETRY ) );
        for( uint32 i = 0; i < 4096; ++i )
            drawIdPtr[i] = i;
        mDrawId = createVertexBuffer( vertexElements, 4096, BT_IMMUTABLE, drawIdPtr, true );
    }
    //-----------------------------------------------------------------------------------
    VulkanVaoManager::~VulkanVaoManager()
    {
        destroyAllVertexArrayObjects();
        deleteAllBuffers();

        mDevice->stall();

        {
            VkSemaphoreArray::const_iterator itor = mAvailableSemaphores.begin();
            VkSemaphoreArray::const_iterator endt = mAvailableSemaphores.end();

            while( itor != endt )
                vkDestroySemaphore( mDevice->mDevice, *itor++, 0 );
        }
        {
            FastArray<UsedSemaphore>::const_iterator itor = mUsedSemaphores.begin();
            FastArray<UsedSemaphore>::const_iterator endt = mUsedSemaphores.end();

            while( itor != endt )
            {
                vkDestroySemaphore( mDevice->mDevice, itor->semaphore, 0 );
                ++itor;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::getMemoryStats( MemoryStatsEntryVec &outStats, size_t &outCapacityBytes,
                                           size_t &outFreeBytes, Log *log ) const
    {
        TODO_implement;
        outCapacityBytes = 0;
        outFreeBytes = 0;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::cleanupEmptyPools( void ) { TODO_whenImplemented_include_stagingBuffers; }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::determineBestMemoryTypes( void )
    {
        for( size_t i = 0u; i < MAX_VBO_FLAG; ++i )
            mBestVkMemoryTypeIndex[i] = std::numeric_limits<uint32>::max();

        const VkPhysicalDeviceMemoryProperties &memProperties = mDevice->mMemoryProperties;
        const uint32 numMemoryTypes = memProperties.memoryTypeCount;

        for( uint32 i = 0u; i < numMemoryTypes; ++i )
        {
            if( memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
            {
                uint32 newValue = i;
                const uint32 oldValue = mBestVkMemoryTypeIndex[CPU_INACCESSIBLE];
                if( oldValue < numMemoryTypes )
                {
                    if( !( memProperties.memoryTypes[oldValue].propertyFlags &
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) )
                    {
                        // We had already found our best match
                        newValue = oldValue;
                    }
                }
                mBestVkMemoryTypeIndex[CPU_INACCESSIBLE] = newValue;
            }

            // Find memory that isn't coherent (many desktop GPUs don't provide this)
            if( ( memProperties.memoryTypes[i].propertyFlags &
                  ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) ==
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT &&
                mBestVkMemoryTypeIndex[CPU_ACCESSIBLE_PERSISTENT] >= numMemoryTypes )
            {
                mBestVkMemoryTypeIndex[CPU_ACCESSIBLE_PERSISTENT] = i;
            }

            // Find memory that is coherent, prefer write-combined (aka uncached)
            if( ( memProperties.memoryTypes[i].propertyFlags &
                  ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) ==
                ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) )
            {
                uint32 newValue = i;
                const uint32 oldValue = mBestVkMemoryTypeIndex[CPU_ACCESSIBLE_PERSISTENT_COHERENT];
                if( oldValue < numMemoryTypes )
                {
                    if( !( memProperties.memoryTypes[oldValue].propertyFlags &
                           VK_MEMORY_PROPERTY_HOST_CACHED_BIT ) )
                    {
                        // We already found our best match
                        newValue = oldValue;
                    }
                }
                mBestVkMemoryTypeIndex[CPU_ACCESSIBLE_PERSISTENT_COHERENT] = newValue;
            }
        }

        // Set CPU_ACCESSIBLE_DEFAULT
        if( mBestVkMemoryTypeIndex[CPU_ACCESSIBLE_PERSISTENT] >= numMemoryTypes )
        {
            mBestVkMemoryTypeIndex[CPU_ACCESSIBLE_DEFAULT] =
                mBestVkMemoryTypeIndex[CPU_ACCESSIBLE_PERSISTENT_COHERENT];
        }
        if( mBestVkMemoryTypeIndex[CPU_ACCESSIBLE_PERSISTENT_COHERENT] >= numMemoryTypes )
        {
            mBestVkMemoryTypeIndex[CPU_ACCESSIBLE_DEFAULT] =
                mBestVkMemoryTypeIndex[CPU_ACCESSIBLE_PERSISTENT];
        }
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::allocateVbo( size_t sizeBytes, size_t alignment, BufferType bufferType,
                                        size_t &outVboIdx, size_t &outBufferOffset )
    {
        OGRE_ASSERT_LOW( alignment > 0 );

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType );

        if( bufferType >= BT_DYNAMIC_DEFAULT )
            sizeBytes *= mDynamicBufferMultiplier;

        VboVec::const_iterator itor = mVbos[vboFlag].begin();
        VboVec::const_iterator endt = mVbos[vboFlag].end();

        // clang-format off
        // Find a suitable VBO that can hold the requested size. We prefer those free
        // blocks that have a matching stride (the current offset is a multiple of
        // bytesPerElement) in order to minimize the amount of memory padding.
        size_t bestVboIdx   = (size_t)-1;
        size_t bestBlockIdx = (size_t)-1;
        bool foundMatchingStride = false;
        // clang-format on

        while( itor != endt && !foundMatchingStride )
        {
            BlockVec::const_iterator blockIt = itor->freeBlocks.begin();
            BlockVec::const_iterator blockEn = itor->freeBlocks.end();

            while( blockIt != blockEn && !foundMatchingStride )
            {
                const Block &block = *blockIt;

                // Round to next multiple of alignment
                size_t newOffset = ( ( block.offset + alignment - 1 ) / alignment ) * alignment;
                size_t padding = newOffset - block.offset;

                if( sizeBytes + padding <= block.size )
                {
                    // clang-format off
                    bestVboIdx      = static_cast<size_t>( itor - mVbos[vboFlag].begin());
                    bestBlockIdx    = static_cast<size_t>( blockIt - itor->freeBlocks.begin() );
                    // clang-format on

                    if( newOffset == block.offset )
                        foundMatchingStride = true;
                }

                ++blockIt;
            }

            ++itor;
        }

        if( bestBlockIdx == (size_t)-1 )
        {
            // clang-format off
            bestVboIdx      = mVbos[vboFlag].size();
            bestBlockIdx    = 0;
            foundMatchingStride = true;
            // clang-format on

            Vbo newVbo;

            size_t poolSize = std::max( mDefaultPoolSize[vboFlag], sizeBytes );

            // No luck, allocate a new buffer.
            OGRE_ASSERT_LOW( mBestVkMemoryTypeIndex[vboFlag] <
                             mDevice->mMemoryProperties.memoryTypeCount );

            VkMemoryAllocateInfo memAllocInfo;
            makeVkStruct( memAllocInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO );
            memAllocInfo.allocationSize = poolSize;
            memAllocInfo.memoryTypeIndex = mBestVkMemoryTypeIndex[vboFlag];

            VkResult result = vkAllocateMemory( mDevice->mDevice, &memAllocInfo, NULL, &newVbo.vboName );
            checkVkResult( result, "vkAllocateMemory" );

            newVbo.sizeBytes = poolSize;
            newVbo.freeBlocks.push_back( Block( 0, poolSize ) );
            newVbo.dynamicBuffer = 0;

            if( vboFlag != CPU_INACCESSIBLE )
            {
                newVbo.dynamicBuffer = new VulkanDynamicBuffer( newVbo.vboName, newVbo.sizeBytes, this,
                                                                bufferType, mDevice );
            }

            mVbos[vboFlag].push_back( newVbo );
        }

        // clang-format off
        Vbo &bestVbo        = mVbos[vboFlag][bestVboIdx];
        Block &bestBlock    = bestVbo.freeBlocks[bestBlockIdx];
        // clang-format on

        size_t newOffset = ( ( bestBlock.offset + alignment - 1 ) / alignment ) * alignment;
        size_t padding = newOffset - bestBlock.offset;
        // clang-format off
        // Shrink our records about available data.
        bestBlock.size   -= sizeBytes + padding;
        bestBlock.offset = newOffset + sizeBytes;
        // clang-format on

        if( !foundMatchingStride )
        {
            // This is a stride changer, record as such.
            StrideChangerVec::iterator itStride = std::lower_bound( bestVbo.strideChangers.begin(),
                                                                    bestVbo.strideChangers.end(),  //
                                                                    newOffset, StrideChanger() );
            bestVbo.strideChangers.insert( itStride, StrideChanger( newOffset, padding ) );
        }

        if( bestBlock.size == 0u )
            bestVbo.freeBlocks.erase( bestVbo.freeBlocks.begin() + bestBlockIdx );

        // clang-format off
        outVboIdx       = bestVboIdx;
        outBufferOffset = newOffset;
        // clang-format on
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::deallocateVbo( size_t vboIdx, size_t bufferOffset, size_t sizeBytes,
                                          BufferType bufferType )
    {
        VboFlag vboFlag = bufferTypeToVboFlag( bufferType );

        if( bufferType >= BT_DYNAMIC_DEFAULT )
            sizeBytes *= mDynamicBufferMultiplier;

        Vbo &vbo = mVbos[vboFlag][vboIdx];
        StrideChangerVec::iterator itStride =
            std::lower_bound( vbo.strideChangers.begin(), vbo.strideChangers.end(),  //
                              bufferOffset, StrideChanger() );

        if( itStride != vbo.strideChangers.end() && itStride->offsetAfterPadding == bufferOffset )
        {
            // clang-format off
            bufferOffset    -= itStride->paddedBytes;
            sizeBytes       += itStride->paddedBytes;
            // clang-format on

            vbo.strideChangers.erase( itStride );
        }

        // See if we're contiguous to a free block and make that block grow.
        vbo.freeBlocks.push_back( Block( bufferOffset, sizeBytes ) );
        mergeContiguousBlocks( vbo.freeBlocks.end() - 1, vbo.freeBlocks );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::mergeContiguousBlocks( BlockVec::iterator blockToMerge, BlockVec &blocks )
    {
        BlockVec::iterator itor = blocks.begin();
        BlockVec::iterator endt = blocks.end();

        while( itor != endt )
        {
            if( itor->offset + itor->size == blockToMerge->offset )
            {
                itor->size += blockToMerge->size;
                size_t idx = itor - blocks.begin();

                // When blockToMerge is the last one, its index won't be the same
                // after removing the other iterator, they will swap.
                if( idx == blocks.size() - 1u )
                    idx = blockToMerge - blocks.begin();

                efficientVectorRemove( blocks, blockToMerge );

                blockToMerge = blocks.begin() + idx;
                itor = blocks.begin();
                endt = blocks.end();
            }
            else if( blockToMerge->offset + blockToMerge->size == itor->offset )
            {
                blockToMerge->size += itor->size;
                size_t idx = blockToMerge - blocks.begin();

                // When blockToMerge is the last one, its index won't be the same
                // after removing the other iterator, they will swap.
                if( idx == blocks.size() - 1u )
                    idx = itor - blocks.begin();

                efficientVectorRemove( blocks, itor );

                blockToMerge = blocks.begin() + idx;
                itor = blocks.begin();
                endt = blocks.end();
            }
            else
            {
                ++itor;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    VertexBufferPacked *VulkanVaoManager::createVertexBufferImpl( size_t numElements,
                                                                  uint32 bytesPerElement,
                                                                  BufferType bufferType,
                                                                  void *initialData, bool keepAsShadow,
                                                                  const VertexElement2Vec &vElements )
    {
        size_t vboIdx;
        size_t bufferOffset;

        allocateVbo( numElements * bytesPerElement, bytesPerElement, bufferType, vboIdx, bufferOffset );

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType );
        Vbo &vbo = mVbos[vboFlag][vboIdx];
        VulkanBufferInterface *bufferInterface =
            new VulkanBufferInterface( vboIdx, vbo.vkBuffer, vbo.dynamicBuffer );

        VertexBufferPacked *retVal = OGRE_NEW VertexBufferPacked(
            bufferOffset, numElements, bytesPerElement, 0, bufferType, initialData, keepAsShadow, this,
            bufferInterface, vElements, 0, 0, 0 );

        if( initialData )
            bufferInterface->_firstUpload( initialData, 0, numElements );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyVertexBufferImpl( VertexBufferPacked *vertexBuffer )
    {
        VulkanBufferInterface *bufferInterface =
            static_cast<VulkanBufferInterface *>( vertexBuffer->getBufferInterface() );

        deallocateVbo( bufferInterface->getVboPoolIndex(),
                       vertexBuffer->_getInternalBufferStart() * vertexBuffer->getBytesPerElement(),
                       vertexBuffer->_getInternalTotalSizeBytes(), vertexBuffer->getBufferType() );
    }
    //-----------------------------------------------------------------------------------
    MultiSourceVertexBufferPool *VulkanVaoManager::createMultiSourceVertexBufferPoolImpl(
        const VertexElement2VecVec &vertexElementsBySource, size_t maxNumVertices,
        size_t totalBytesPerVertex, BufferType bufferType )
    {
        return OGRE_NEW VulkanMultiSourceVertexBufferPool( 0, vertexElementsBySource, maxNumVertices,
                                                           bufferType, 0, this );
    }
    //-----------------------------------------------------------------------------------
    IndexBufferPacked *VulkanVaoManager::createIndexBufferImpl( size_t numElements,
                                                                uint32 bytesPerElement,
                                                                BufferType bufferType, void *initialData,
                                                                bool keepAsShadow )
    {
        size_t vboIdx;
        size_t bufferOffset;

        allocateVbo( numElements * bytesPerElement, bytesPerElement, bufferType, vboIdx, bufferOffset );

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType );
        Vbo &vbo = mVbos[vboFlag][vboIdx];
        VulkanBufferInterface *bufferInterface =
            new VulkanBufferInterface( vboIdx, vbo.vkBuffer, vbo.dynamicBuffer );

        IndexBufferPacked *retVal =
            OGRE_NEW IndexBufferPacked( bufferOffset, numElements, bytesPerElement, 0, bufferType,
                                        initialData, keepAsShadow, this, bufferInterface );

        if( initialData )
            bufferInterface->_firstUpload( initialData, 0, numElements );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyIndexBufferImpl( IndexBufferPacked *indexBuffer )
    {
        VulkanBufferInterface *bufferInterface =
            static_cast<VulkanBufferInterface *>( indexBuffer->getBufferInterface() );

        deallocateVbo( bufferInterface->getVboPoolIndex(),
                       indexBuffer->_getInternalBufferStart() * indexBuffer->getBytesPerElement(),
                       indexBuffer->_getInternalTotalSizeBytes(), indexBuffer->getBufferType() );
    }
    //-----------------------------------------------------------------------------------
    ConstBufferPacked *VulkanVaoManager::createConstBufferImpl( size_t sizeBytes, BufferType bufferType,
                                                                void *initialData, bool keepAsShadow )
    {
        size_t vboIdx;
        size_t bufferOffset;

        size_t alignment = mConstBufferAlignment;
        size_t requestedSize = sizeBytes;

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType );

        if( bufferType >= BT_DYNAMIC_DEFAULT )
        {
            // For dynamic buffers, the size will be 3x times larger
            //(depending on mDynamicBufferMultiplier); we need the
            // offset after each map to be aligned; and for that, we
            // sizeBytes to be multiple of alignment.
            sizeBytes = alignToNextMultiple( sizeBytes, alignment );
        }

        allocateVbo( sizeBytes, alignment, bufferType, vboIdx, bufferOffset );

        Vbo &vbo = mVbos[vboFlag][vboIdx];
        VulkanBufferInterface *bufferInterface =
            new VulkanBufferInterface( vboIdx, vbo.vkBuffer, vbo.dynamicBuffer );

        ConstBufferPacked *retVal = OGRE_NEW VulkanConstBufferPacked(
            bufferOffset, requestedSize, 1u, ( uint32 )( sizeBytes - requestedSize ), bufferType,
            initialData, keepAsShadow, this, bufferInterface );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyConstBufferImpl( ConstBufferPacked *constBuffer )
    {
        VulkanBufferInterface *bufferInterface =
            static_cast<VulkanBufferInterface *>( constBuffer->getBufferInterface() );

        deallocateVbo( bufferInterface->getVboPoolIndex(),
                       constBuffer->_getInternalBufferStart() * constBuffer->getBytesPerElement(),
                       constBuffer->_getInternalTotalSizeBytes(), constBuffer->getBufferType() );
    }
    //-----------------------------------------------------------------------------------
    TexBufferPacked *VulkanVaoManager::createTexBufferImpl( PixelFormatGpu pixelFormat, size_t sizeBytes,
                                                            BufferType bufferType, void *initialData,
                                                            bool keepAsShadow )
    {
        size_t vboIdx;
        size_t bufferOffset;

        size_t alignment = mTexBufferAlignment;
        size_t requestedSize = sizeBytes;

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType );

        if( bufferType >= BT_DYNAMIC_DEFAULT )
        {
            // For dynamic buffers, the size will be 3x times larger
            //(depending on mDynamicBufferMultiplier); we need the
            // offset after each map to be aligned; and for that, we
            // sizeBytes to be multiple of alignment.
            sizeBytes = alignToNextMultiple( sizeBytes, alignment );
        }

        allocateVbo( sizeBytes, alignment, bufferType, vboIdx, bufferOffset );

        Vbo &vbo = mVbos[vboFlag][vboIdx];
        VulkanBufferInterface *bufferInterface =
            new VulkanBufferInterface( vboIdx, vbo.vkBuffer, vbo.dynamicBuffer );

        TexBufferPacked *retVal = OGRE_NEW VulkanTexBufferPacked(
            bufferOffset, requestedSize, 1u, ( uint32 )( sizeBytes - requestedSize ), bufferType,
            initialData, keepAsShadow, this, bufferInterface, pixelFormat );

        if( initialData )
            bufferInterface->_firstUpload( initialData, 0, requestedSize );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyTexBufferImpl( TexBufferPacked *texBuffer )
    {
        VulkanBufferInterface *bufferInterface =
            static_cast<VulkanBufferInterface *>( texBuffer->getBufferInterface() );

        deallocateVbo( bufferInterface->getVboPoolIndex(),
                       texBuffer->_getInternalBufferStart() * texBuffer->getBytesPerElement(),
                       texBuffer->_getInternalTotalSizeBytes(), texBuffer->getBufferType() );
    }
    //-----------------------------------------------------------------------------------
    UavBufferPacked *VulkanVaoManager::createUavBufferImpl( size_t numElements, uint32 bytesPerElement,
                                                            uint32 bindFlags, void *initialData,
                                                            bool keepAsShadow )
    {
        size_t vboIdx;
        size_t bufferOffset;

        size_t alignment = Math::lcm( mUavBufferAlignment, bytesPerElement );

        // UAV Buffers can't be dynamic.
        const BufferType bufferType = BT_DEFAULT;
        VboFlag vboFlag = bufferTypeToVboFlag( bufferType );

        allocateVbo( numElements * bytesPerElement, alignment, bufferType, vboIdx, bufferOffset );

        Vbo &vbo = mVbos[vboFlag][vboIdx];
        VulkanBufferInterface *bufferInterface =
            new VulkanBufferInterface( vboIdx, vbo.vkBuffer, vbo.dynamicBuffer );

        UavBufferPacked *retVal =
            OGRE_NEW VulkanUavBufferPacked( bufferOffset, numElements, bytesPerElement, bindFlags,
                                            initialData, keepAsShadow, this, bufferInterface );

        if( initialData )
            bufferInterface->_firstUpload( initialData, 0, numElements );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyUavBufferImpl( UavBufferPacked *uavBuffer )
    {
        VulkanBufferInterface *bufferInterface =
            static_cast<VulkanBufferInterface *>( uavBuffer->getBufferInterface() );

        deallocateVbo( bufferInterface->getVboPoolIndex(),
                       uavBuffer->_getInternalBufferStart() * uavBuffer->getBytesPerElement(),
                       uavBuffer->_getInternalTotalSizeBytes(), uavBuffer->getBufferType() );
    }
    //-----------------------------------------------------------------------------------
    IndirectBufferPacked *VulkanVaoManager::createIndirectBufferImpl( size_t sizeBytes,
                                                                      BufferType bufferType,
                                                                      void *initialData,
                                                                      bool keepAsShadow )
    {
        const size_t alignment = 4u;
        size_t bufferOffset = 0u;
        size_t requestedSize = sizeBytes;

        if( bufferType >= BT_DYNAMIC_DEFAULT )
        {
            // For dynamic buffers, the size will be 3x times larger
            //(depending on mDynamicBufferMultiplier); we need the
            // offset after each map to be aligned; and for that, we
            // sizeBytes to be multiple of alignment.
            sizeBytes = alignToNextMultiple( sizeBytes, alignment );
        }

        VulkanBufferInterface *bufferInterface = 0;
        if( mSupportsIndirectBuffers )
        {
            size_t vboIdx;
            VboFlag vboFlag = bufferTypeToVboFlag( bufferType );

            allocateVbo( sizeBytes, alignment, bufferType, vboIdx, bufferOffset );

            Vbo &vbo = mVbos[vboFlag][vboIdx];
            bufferInterface = new VulkanBufferInterface( vboIdx, vbo.vkBuffer, vbo.dynamicBuffer );
        }

        IndirectBufferPacked *retVal = OGRE_NEW IndirectBufferPacked(
            bufferOffset, requestedSize, 1, ( uint32 )( sizeBytes - requestedSize ), bufferType,
            initialData, keepAsShadow, this, bufferInterface );

        if( initialData )
        {
            if( mSupportsIndirectBuffers )
                bufferInterface->_firstUpload( initialData, 0, requestedSize );
            else
                memcpy( retVal->getSwBufferPtr(), initialData, requestedSize );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyIndirectBufferImpl( IndirectBufferPacked *indirectBuffer )
    {
        if( mSupportsIndirectBuffers )
        {
            VulkanBufferInterface *bufferInterface =
                static_cast<VulkanBufferInterface *>( indirectBuffer->getBufferInterface() );

            deallocateVbo(
                bufferInterface->getVboPoolIndex(),
                indirectBuffer->_getInternalBufferStart() * indirectBuffer->getBytesPerElement(),
                indirectBuffer->_getInternalTotalSizeBytes(), indirectBuffer->getBufferType() );
        }
    }
    //-----------------------------------------------------------------------------------
    VertexArrayObject *VulkanVaoManager::createVertexArrayObjectImpl(
        const VertexBufferPackedVec &vertexBuffers, IndexBufferPacked *indexBuffer,
        OperationType opType )
    {
        size_t idx = mVertexArrayObjects.size();

        const int bitsOpType = 3;
        const int bitsVaoGl = 2;
        const uint32 maskOpType = OGRE_RQ_MAKE_MASK( bitsOpType );
        const uint32 maskVaoGl = OGRE_RQ_MAKE_MASK( bitsVaoGl );
        const uint32 maskVao = OGRE_RQ_MAKE_MASK( RqBits::MeshBits - bitsOpType - bitsVaoGl );

        const uint32 shiftOpType = RqBits::MeshBits - bitsOpType;
        const uint32 shiftVaoGl = shiftOpType - bitsVaoGl;

        uint32 renderQueueId = ( ( opType & maskOpType ) << shiftOpType ) |
                               ( ( idx & maskVaoGl ) << shiftVaoGl ) | ( idx & maskVao );

        VulkanVertexArrayObject *retVal =
            OGRE_NEW VulkanVertexArrayObject( idx, renderQueueId, vertexBuffers, indexBuffer, opType );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyVertexArrayObjectImpl( VertexArrayObject *vao )
    {
        VulkanVertexArrayObject *glVao = static_cast<VulkanVertexArrayObject *>( vao );
        OGRE_DELETE glVao;
    }
    //-----------------------------------------------------------------------------------
    StagingBuffer *VulkanVaoManager::createStagingBuffer( size_t sizeBytes, bool forUpload )
    {
        sizeBytes = std::max<size_t>( sizeBytes, 4u * 1024u * 1024u );

        size_t vboIdx;
        size_t bufferOffset;
        allocateVbo( sizeBytes, 4u, BT_DYNAMIC_PERSISTENT, vboIdx, bufferOffset );

        const VboFlag vboFlag = CPU_ACCESSIBLE_PERSISTENT;
        Vbo &vbo = mVbos[vboFlag][vboIdx];
        VulkanBufferInterface *bufferInterface =
            new VulkanBufferInterface( vboIdx, vbo.vkBuffer, vbo.dynamicBuffer );

        VulkanStagingBuffer *stagingBuffer =
            OGRE_NEW VulkanStagingBuffer( bufferOffset, sizeBytes, this, forUpload, bufferInterface );
        mRefedStagingBuffers[forUpload].push_back( stagingBuffer );

        if( mNextStagingBufferTimestampCheckpoint == (unsigned long)( ~0 ) )
        {
            mNextStagingBufferTimestampCheckpoint =
                mTimer->getMilliseconds() + mDefaultStagingBufferLifetime;
        }

        return stagingBuffer;
    }
    //-----------------------------------------------------------------------------------
    AsyncTicketPtr VulkanVaoManager::createAsyncTicket( BufferPacked *creator,
                                                        StagingBuffer *stagingBuffer,
                                                        size_t elementStart, size_t elementCount )
    {
        return AsyncTicketPtr(
            OGRE_NEW VulkanAsyncTicket( creator, stagingBuffer, elementStart, elementCount ) );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::_update( void )
    {
        VaoManager::_update();

        if( !mFenceFlushed )
        {
            // Undo the increment from VaoManager::_update. We'll do it later
            --mFrameCount;
        }

        unsigned long currentTimeMs = mTimer->getMilliseconds();

        if( currentTimeMs >= mNextStagingBufferTimestampCheckpoint )
        {
            mNextStagingBufferTimestampCheckpoint = (unsigned long)( ~0 );

            for( size_t i = 0; i < 2; ++i )
            {
                StagingBufferVec::iterator itor = mZeroRefStagingBuffers[i].begin();
                StagingBufferVec::iterator end = mZeroRefStagingBuffers[i].end();

                while( itor != end )
                {
                    StagingBuffer *stagingBuffer = *itor;

                    mNextStagingBufferTimestampCheckpoint = std::min(
                        mNextStagingBufferTimestampCheckpoint,
                        stagingBuffer->getLastUsedTimestamp() + stagingBuffer->getLifetimeThreshold() );

                    /*if( stagingBuffer->getLastUsedTimestamp() +
                    stagingBuffer->getUnfencedTimeThreshold() < currentTimeMs )
                    {
                        static_cast<VulkanStagingBuffer*>( stagingBuffer )->cleanUnfencedHazards();
                    }*/

                    if( stagingBuffer->getLastUsedTimestamp() + stagingBuffer->getLifetimeThreshold() <
                        currentTimeMs )
                    {
                        // Time to delete this buffer.
                        delete *itor;

                        itor = efficientVectorRemove( mZeroRefStagingBuffers[i], itor );
                        end = mZeroRefStagingBuffers[i].end();
                    }
                    else
                    {
                        ++itor;
                    }
                }
            }
        }

        if( !mUsedSemaphores.empty() )
        {
            waitForTailFrameToFinish();

            FastArray<UsedSemaphore>::iterator itor = mUsedSemaphores.begin();
            FastArray<UsedSemaphore>::iterator endt = mUsedSemaphores.end();

            while( itor != endt && ( mFrameCount - itor->frame ) >= mDynamicBufferMultiplier )
            {
                mAvailableSemaphores.push_back( itor->semaphore );
                ++itor;
            }

            mUsedSemaphores.erasePOD( mUsedSemaphores.begin(), itor );
        }

        if( !mDelayedDestroyBuffers.empty() &&
            mDelayedDestroyBuffers.front().frameNumDynamic == mDynamicBufferCurrentFrame )
        {
            waitForTailFrameToFinish();
            destroyDelayedBuffers( mDynamicBufferCurrentFrame );
        }

        if( !mFenceFlushed )
        {
            // We could only reach here if _update() was called
            // twice in a row without completing a full frame.
            // Without this, waitForTailFrameToFinish becomes unsafe.
            mDevice->commitAndNextCommandBuffer( false );
            mDynamicBufferCurrentFrame = ( mDynamicBufferCurrentFrame + 1 ) % mDynamicBufferMultiplier;
        }

        mFenceFlushed = false;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::_notifyNewCommandBuffer()
    {
        mFenceFlushed = true;
        mDynamicBufferCurrentFrame = ( mDynamicBufferCurrentFrame + 1 ) % mDynamicBufferMultiplier;
        ++mFrameCount;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::getAvailableSempaphores( VkSemaphoreArray &semaphoreArray,
                                                    size_t numSemaphores )
    {
        semaphoreArray.reserve( semaphoreArray.size() + numSemaphores );

        if( mAvailableSemaphores.size() < numSemaphores )
        {
            const size_t requiredNewSemaphores = numSemaphores - mAvailableSemaphores.size();
            VkSemaphoreCreateInfo semaphoreCreateInfo;
            makeVkStruct( semaphoreCreateInfo, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO );

            for( size_t i = 0u; i < requiredNewSemaphores; ++i )
            {
                VkSemaphore semaphore = 0;
                const VkResult result =
                    vkCreateSemaphore( mDevice->mDevice, &semaphoreCreateInfo, 0, &semaphore );
                checkVkResult( result, "vkCreateSemaphore" );
                semaphoreArray.push_back( semaphore );
            }

            numSemaphores -= requiredNewSemaphores;
        }

        for( size_t i = 0u; i < numSemaphores; ++i )
        {
            VkSemaphore semaphore = mAvailableSemaphores.back();
            semaphoreArray.push_back( semaphore );
            mAvailableSemaphores.pop_back();
        }
    }
    //-----------------------------------------------------------------------------------
    VkSemaphore VulkanVaoManager::getAvailableSempaphore( void )
    {
        VkSemaphore retVal;
        if( mAvailableSemaphores.empty() )
        {
            VkSemaphoreCreateInfo semaphoreCreateInfo;
            makeVkStruct( semaphoreCreateInfo, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO );

            const VkResult result =
                vkCreateSemaphore( mDevice->mDevice, &semaphoreCreateInfo, 0, &retVal );
            checkVkResult( result, "vkCreateSemaphore" );
        }
        else
        {
            retVal = mAvailableSemaphores.back();
            mAvailableSemaphores.pop_back();
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::notifyWaitSemaphoreSubmitted( VkSemaphore semaphore )
    {
        mUsedSemaphores.push_back( UsedSemaphore( semaphore, mFrameCount ) );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::notifyWaitSemaphoresSubmitted( const VkSemaphoreArray &semaphores )
    {
        mUsedSemaphores.reserve( mUsedSemaphores.size() + semaphores.size() );

        VkSemaphoreArray::const_iterator itor = semaphores.begin();
        VkSemaphoreArray::const_iterator endt = semaphores.end();

        while( itor != endt )
            mUsedSemaphores.push_back( UsedSemaphore( *itor++, mFrameCount ) );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::notifySemaphoreUnused( VkSemaphore semaphore )
    {
        vkDestroySemaphore( mDevice->mDevice, semaphore, 0 );
    }
    //-----------------------------------------------------------------------------------
    uint8 VulkanVaoManager::waitForTailFrameToFinish( void )
    {
        mDevice->mGraphicsQueue._waitOnFrame( mDynamicBufferCurrentFrame );
        return mDynamicBufferCurrentFrame;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::waitForSpecificFrameToFinish( uint32 frameCount ) {}
    //-----------------------------------------------------------------------------------
    bool VulkanVaoManager::isFrameFinished( uint32 frameCount ) { return true; }
    //-----------------------------------------------------------------------------------
    VulkanVaoManager::VboFlag VulkanVaoManager::bufferTypeToVboFlag( BufferType bufferType )
    {
        return static_cast<VboFlag>(
            std::max( 0, ( bufferType - BT_DYNAMIC_DEFAULT ) + CPU_ACCESSIBLE_DEFAULT ) );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::switchVboPoolIndexImpl( size_t oldPoolIdx, size_t newPoolIdx,
                                                   BufferPacked *buffer )
    {
        if( mSupportsIndirectBuffers || buffer->getBufferPackedType() != BP_TYPE_INDIRECT )
        {
            VulkanBufferInterface *bufferInterface =
                static_cast<VulkanBufferInterface *>( buffer->getBufferInterface() );
            if( bufferInterface->getVboPoolIndex() == oldPoolIdx )
                bufferInterface->_setVboPoolIndex( newPoolIdx );
        }
    }
}  // namespace Ogre
