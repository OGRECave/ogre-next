/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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

#include "Vao/OgreVulkanVaoManager.h"

#include "OgreVulkanDescriptorPool.h"
#include "OgreVulkanDevice.h"
#include "OgreVulkanRootLayout.h"
#include "OgreVulkanUtils.h"
#include "Vao/OgreVertexArrayObject.h"
#include "Vao/OgreVulkanAsyncTicket.h"
#include "Vao/OgreVulkanBufferInterface.h"
#include "Vao/OgreVulkanConstBufferPacked.h"
#include "Vao/OgreVulkanDynamicBuffer.h"
#include "Vao/OgreVulkanMultiSourceVertexBufferPool.h"
#include "Vao/OgreVulkanStagingBuffer.h"
#include "Vao/OgreVulkanTexBufferPacked.h"
#include "Vao/OgreVulkanUavBufferPacked.h"

#include "Vao/OgreIndirectBufferPacked.h"

#include "../OgreVulkanDelayedFuncs.h"

#include "OgreHlmsManager.h"
#include "OgreRenderQueue.h"
#include "OgreRoot.h"

#include "OgreStringConverter.h"
#include "OgreTimer.h"
#include "OgreVulkanStagingTexture.h"

#define TODO_implement
#define TODO_whenImplemented_include_stagingBuffers
#define TODO_add_cached_read_memory
#define TODO_if_memory_non_coherent_align_size

namespace Ogre
{
    const uint32 VulkanVaoManager::VERTEX_ATTRIBUTE_INDEX[VES_COUNT] = {
        0,  // VES_POSITION - 1
        3,  // VES_BLEND_WEIGHTS - 1
        4,  // VES_BLEND_INDICES - 1
        1,  // VES_NORMAL - 1
        5,  // VES_DIFFUSE - 1
        6,  // VES_SPECULAR - 1
        7,  // VES_TEXTURE_COORDINATES - 1
        // There are up to 8 VES_TEXTURE_COORDINATES. Occupy range [7; 15)
        // Range [13; 15) overlaps with VES_BLEND_WEIGHTS2 & VES_BLEND_INDICES2
        // Index 15 is reserved for draw ID.

        // VES_BINORMAL would use slot 16. Since Binormal is rarely used, we don't support it.
        //(slot 16 is where const buffers start)
        ~0u,  // VES_BINORMAL - 1
        2,    // VES_TANGENT - 1
        13,   // VES_BLEND_WEIGHTS2 - 1
        14,   // VES_BLEND_INDICES2 - 1
    };

    VulkanVaoManager::VulkanVaoManager( uint8 dynBufferMultiplier, VulkanDevice *device,
                                        VulkanRenderSystem *renderSystem ) :
        VaoManager( 0 ),
        mVaoNames( 1u ),
        mDrawId( 0 ),
        mDevice( device ),
        mVkRenderSystem( renderSystem ),
        mFenceFlushed( true ),
        mSupportsCoherentMemory( false ),
        mSupportsNonCoherentMemory( false ),
        mReadMemoryIsCoherent( false )
    {
        mConstBufferAlignment = 256;
        mTexBufferAlignment = 256;

        mConstBufferMaxSize = 64 * 1024;        // 64kb
        mTexBufferMaxSize = 128 * 1024 * 1024;  // 128MB

        mSupportsPersistentMapping = true;
        mSupportsIndirectBuffers = false;

        // Keep pools of 64MB each for static meshes
        mDefaultPoolSize[CPU_INACCESSIBLE] = 64u * 1024u * 1024u;

        // Keep pools of 16MB each for write buffers, 4MB for read ones.
        for( size_t i = CPU_WRITE_PERSISTENT; i <= CPU_WRITE_PERSISTENT_COHERENT; ++i )
            mDefaultPoolSize[i] = 16u * 1024u * 1024u;
        mDefaultPoolSize[CPU_READ_WRITE] = 4u * 1024u * 1024u;

        mDelayedFuncs.resize( dynBufferMultiplier );

        determineBestMemoryTypes();

        mDynamicBufferMultiplier = dynBufferMultiplier;
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

        deleteStagingBuffers();
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::initDrawIdVertexBuffer()
    {
        VertexElement2Vec vertexElements;
        vertexElements.push_back( VertexElement2( VET_UINT1, VES_COUNT ) );
        uint32 *drawIdPtr =
            static_cast<uint32 *>( OGRE_MALLOC_SIMD( 4096 * sizeof( uint32 ), MEMCATEGORY_GEOMETRY ) );
        for( uint32 i = 0; i < 4096; ++i )
            drawIdPtr[i] = i;
        mDrawId = createVertexBuffer( vertexElements, 4096, BT_IMMUTABLE, drawIdPtr, true );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::bindDrawIdVertexBuffer( VkCommandBuffer cmdBuffer )
    {
        VulkanBufferInterface *bufIntf =
            static_cast<VulkanBufferInterface *>( mDrawId->getBufferInterface() );
        VkBuffer vertexBuffers[] = { bufIntf->getVboName() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers( cmdBuffer, 15, 1, vertexBuffers, offsets );
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
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::cleanupEmptyPools( void )
    {
        TODO_whenImplemented_include_stagingBuffers;

        for( int vboIdx = 0; vboIdx < MAX_VBO_FLAG; ++vboIdx )
        {
            VboVec::iterator itor = mVbos[vboIdx].begin();
            VboVec::iterator endt = mVbos[vboIdx].end();

            while( itor != endt )
            {
                Vbo &vbo = *itor;
                if( vbo.freeBlocks.size() == 1u && vbo.sizeBytes == vbo.freeBlocks.back().size )
                {
                    VaoVec::iterator itVao = mVaos.begin();
                    VaoVec::iterator enVao = mVaos.end();

                    while( itVao != enVao )
                    {
                        bool usesBuffer = false;
                        Vao::VertexBindingVec::const_iterator itBuf = itVao->vertexBuffers.begin();
                        Vao::VertexBindingVec::const_iterator enBuf = itVao->vertexBuffers.end();

                        while( itBuf != enBuf && !usesBuffer )
                        {
                            OGRE_ASSERT_LOW( itBuf->vertexBufferVbo != vbo.vkBuffer &&
                                             "A VertexArrayObject still references "
                                             "a deleted vertex buffer!" );
                            ++itBuf;
                        }

                        OGRE_ASSERT_LOW( itVao->indexBufferVbo != vbo.vkBuffer &&
                                         "A VertexArrayObject still references "
                                         "a deleted index buffer!" );
                        ++itVao;
                    }

                    vbo.vboName = 0;
                    delete vbo.dynamicBuffer;
                    vbo.dynamicBuffer = 0;

                    // There's (unrelated) live buffers whose vboIdx will now point out of bounds.
                    // We need to update them so they don't crash deallocateVbo later.
                    switchVboPoolIndex( ( size_t )( mVbos[vboIdx].size() - 1u ),
                                        ( size_t )( itor - mVbos[vboIdx].begin() ) );

                    itor = efficientVectorRemove( mVbos[vboIdx], itor );
                    endt = mVbos[vboIdx].end();
                }
                else
                {
                    ++itor;
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    bool VulkanVaoManager::supportsCoherentMapping() const { return mSupportsCoherentMemory; }
    //-----------------------------------------------------------------------------------
    bool VulkanVaoManager::supportsNonCoherentMapping() const { return mSupportsNonCoherentMemory; }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::determineBestMemoryTypes( void )
    {
        for( size_t i = 0u; i < MAX_VBO_FLAG; ++i )
            mBestVkMemoryTypeIndex[i] = std::numeric_limits<uint32>::max();

        const VkPhysicalDeviceMemoryProperties &memProperties = mDevice->mDeviceMemoryProperties;
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
            // Prefer memory local to CPU
            if( ( memProperties.memoryTypes[i].propertyFlags &
                  ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) ==
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT &&
                mBestVkMemoryTypeIndex[CPU_WRITE_PERSISTENT] >= numMemoryTypes )
            {
                uint32 newValue = i;
                const uint32 oldValue = mBestVkMemoryTypeIndex[CPU_WRITE_PERSISTENT];
                if( oldValue < numMemoryTypes )
                {
                    if( !( memProperties.memoryTypes[oldValue].propertyFlags &
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) )
                    {
                        // We already had found our best match
                        newValue = oldValue;
                    }
                }
                mBestVkMemoryTypeIndex[CPU_WRITE_PERSISTENT] = newValue;
            }

            // Find memory that is coherent, prefer write-combined (aka uncached)
            // Prefer memory local to CPU
            if( ( memProperties.memoryTypes[i].propertyFlags &
                  ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) ==
                ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) )
            {
                if( mBestVkMemoryTypeIndex[CPU_WRITE_PERSISTENT_COHERENT] >= numMemoryTypes ||
                    ( !( memProperties.memoryTypes[i].propertyFlags &
                         VK_MEMORY_PROPERTY_HOST_CACHED_BIT ) &&
                      !( memProperties.memoryTypes[i].propertyFlags &
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) ) )
                {
                    mBestVkMemoryTypeIndex[CPU_WRITE_PERSISTENT_COHERENT] = i;
                }
            }

            // Find the best memory that is cached for reading
            if( ( memProperties.memoryTypes[i].propertyFlags &
                  ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT ) ) ==
                    ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT ) &&
                mBestVkMemoryTypeIndex[CPU_READ_WRITE] >= numMemoryTypes )
            {
                mBestVkMemoryTypeIndex[CPU_READ_WRITE] = i;
            }
        }

        if( mBestVkMemoryTypeIndex[CPU_READ_WRITE] >= numMemoryTypes )
        {
            LogManager::getSingleton().logMessage(
                "VkDevice: could not find cached host-visible memory. GPU -> CPU transfers could be "
                "slow" );

            if( mSupportsCoherentMemory )
            {
                mBestVkMemoryTypeIndex[CPU_READ_WRITE] =
                    mBestVkMemoryTypeIndex[CPU_WRITE_PERSISTENT_COHERENT];
            }
            else
            {
                mBestVkMemoryTypeIndex[CPU_READ_WRITE] = mBestVkMemoryTypeIndex[CPU_WRITE_PERSISTENT];
            }
        }

        mSupportsNonCoherentMemory = mBestVkMemoryTypeIndex[CPU_WRITE_PERSISTENT] < numMemoryTypes;
        mSupportsCoherentMemory = mBestVkMemoryTypeIndex[CPU_WRITE_PERSISTENT_COHERENT] < numMemoryTypes;

        LogManager::getSingleton().logMessage( "VkDevice supports coherent memory: " +
                                               StringConverter::toString( mSupportsCoherentMemory ) );
        LogManager::getSingleton().logMessage( "VkDevice supports non-coherent memory: " +
                                               StringConverter::toString( mSupportsNonCoherentMemory ) );

        if( !mSupportsNonCoherentMemory && !mSupportsCoherentMemory )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Device does not expose coherent nor non-coherent CPU-accessible "
                         "memory! This is a driver error",
                         "VulkanVaoManager::determineBestMemoryTypes" );
        }

        mReadMemoryIsCoherent =
            ( memProperties.memoryTypes[mBestVkMemoryTypeIndex[CPU_READ_WRITE]].propertyFlags &
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) != 0;

        LogManager::getSingleton().logMessage( "VkDevice read memory is coherent: " +
                                               StringConverter::toString( mReadMemoryIsCoherent ) );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::allocateVbo( size_t sizeBytes, size_t alignment, BufferType bufferType,
                                        bool readCapable, size_t &outVboIdx, size_t &outBufferOffset )
    {
        OGRE_ASSERT_LOW( alignment > 0 );

        TODO_add_cached_read_memory;
        TODO_if_memory_non_coherent_align_size;

        const VboFlag vboFlag = bufferTypeToVboFlag( bufferType, readCapable );

        if( bufferType >= BT_DYNAMIC_DEFAULT && !readCapable )
            sizeBytes *= mDynamicBufferMultiplier;

        allocateVbo( sizeBytes, alignment, mVbos[vboFlag], mBestVkMemoryTypeIndex[vboFlag],
                     mDefaultPoolSize[vboFlag], false, vboFlag != CPU_INACCESSIBLE,
                     isVboFlagCoherent( vboFlag ), outVboIdx, outBufferOffset );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::allocateVbo( size_t sizeBytes, size_t alignment, VboVec &vboVec,
                                        uint32 vkMemoryTypeIndex, size_t defaultPoolSize,
                                        bool textureOnly, bool cpuAccessible, bool isCoherent,
                                        size_t &outVboIdx, size_t &outBufferOffset )
    {
        VboVec::const_iterator itor = vboVec.begin();
        VboVec::const_iterator endt = vboVec.end();

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
                    bestVboIdx      = static_cast<size_t>( itor - vboVec.begin());
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
            bestVboIdx      = vboVec.size();
            bestBlockIdx    = 0;
            foundMatchingStride = true;
            // clang-format on

            Vbo newVbo;

            size_t poolSize = std::max( defaultPoolSize, sizeBytes );

            // No luck, allocate a new buffer.
            OGRE_ASSERT_LOW( vkMemoryTypeIndex < mDevice->mDeviceMemoryProperties.memoryTypeCount );

            VkMemoryAllocateInfo memAllocInfo;
            makeVkStruct( memAllocInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO );
            memAllocInfo.allocationSize = poolSize;
            memAllocInfo.memoryTypeIndex = vkMemoryTypeIndex;

            VkResult result = vkAllocateMemory( mDevice->mDevice, &memAllocInfo, NULL, &newVbo.vboName );
            checkVkResult( result, "vkAllocateMemory" );

            newVbo.vkBuffer = 0;
            if( !textureOnly )
            {
                VkBufferCreateInfo bufferCi;
                makeVkStruct( bufferCi, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO );
                bufferCi.size = poolSize;
                bufferCi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                 VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
                                 VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT |
                                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
                result = vkCreateBuffer( mDevice->mDevice, &bufferCi, 0, &newVbo.vkBuffer );
                checkVkResult( result, "vkCreateBuffer" );

                VkMemoryRequirements memRequirements;
                vkGetBufferMemoryRequirements( mDevice->mDevice, newVbo.vkBuffer, &memRequirements );

                // TODO use one large buffer and multiple offsets
                VkDeviceSize offset = 0;
                offset = alignMemory( offset, memRequirements.alignment );

                result = vkBindBufferMemory( mDevice->mDevice, newVbo.vkBuffer, newVbo.vboName, offset );
                checkVkResult( result, "vkBindBufferMemory" );
            }

            newVbo.sizeBytes = poolSize;
            newVbo.freeBlocks.push_back( Block( 0, poolSize ) );
            newVbo.dynamicBuffer = 0;

            if( cpuAccessible )
            {
                newVbo.dynamicBuffer = new VulkanDynamicBuffer( newVbo.vboName, newVbo.sizeBytes,
                                                                isCoherent, false, mDevice );
            }

            vboVec.push_back( newVbo );
        }

        // clang-format off
        Vbo &bestVbo        = vboVec[bestVboIdx];
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
                                          BufferType bufferType, bool readCapable )
    {
        VboFlag vboFlag = bufferTypeToVboFlag( bufferType, readCapable );

        if( bufferType >= BT_DYNAMIC_DEFAULT && !readCapable )
            sizeBytes *= mDynamicBufferMultiplier;

        deallocateVbo( vboIdx, bufferOffset, sizeBytes, mVbos[vboFlag] );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::deallocateVbo( size_t vboIdx, size_t bufferOffset, size_t sizeBytes,
                                          VboVec &vboVec )
    {
        Vbo &vbo = vboVec[vboIdx];
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
        mergeContiguousBlocks( vbo.freeBlocks.end() - 1u, vbo.freeBlocks );
    }
    //-----------------------------------------------------------------------------------
    VkDeviceMemory VulkanVaoManager::allocateTexture( const VkMemoryRequirements &memReq,
                                                      uint16 &outTexMemIdx, size_t &outVboIdx,
                                                      size_t &outBufferOffset )
    {
        TextureMemoryVec::iterator itor = mTextureMemory.begin();
        TextureMemoryVec::iterator endt = mTextureMemory.end();

        while( itor != endt && ( 1u << itor->vkMemoryTypeIndex ) & memReq.memoryTypeBits )
            ++itor;

        if( itor == mTextureMemory.end() )
        {
            // No memory pool is capable of holding this texture. Create a new entry
            TextureMemory texMemory;
            texMemory.vkMemoryTypeIndex =
                findMemoryType( mDevice->mDeviceMemoryProperties, memReq.memoryTypeBits,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
            mTextureMemory.push_back( texMemory );
            itor = mTextureMemory.end() - 1u;
        }

        outTexMemIdx = static_cast<uint16>( itor - mTextureMemory.begin() );

        const bool isTextureOnly = itor->vkMemoryTypeIndex == mBestVkMemoryTypeIndex[CPU_INACCESSIBLE];
        VboVec &vboVec = isTextureOnly ? itor->vbos : mVbos[CPU_INACCESSIBLE];

        allocateVbo( memReq.size, memReq.alignment, vboVec, itor->vkMemoryTypeIndex,
                     mDefaultPoolSize[CPU_INACCESSIBLE], isTextureOnly, false, false, outVboIdx,
                     outBufferOffset );

        return vboVec[outVboIdx].vboName;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::deallocateTexture( uint16 texMemIdx, size_t vboIdx, size_t bufferOffset,
                                              size_t sizeBytes )
    {
        VboVec &vboVec =
            mTextureMemory[texMemIdx].vkMemoryTypeIndex == mBestVkMemoryTypeIndex[CPU_INACCESSIBLE]
                ? mVbos[CPU_INACCESSIBLE]
                : mTextureMemory[texMemIdx].vbos;
        deallocateVbo( vboIdx, bufferOffset, sizeBytes, vboVec );
    }
    //-----------------------------------------------------------------------------------
    VulkanRawBuffer VulkanVaoManager::allocateRawBuffer( VboFlag vboFlag, size_t sizeBytes,
                                                         size_t alignment )
    {
        // Change flag if unavailable
        if( vboFlag == CPU_WRITE_PERSISTENT && !mSupportsNonCoherentMemory )
            vboFlag = CPU_WRITE_PERSISTENT_COHERENT;
        else if( vboFlag == CPU_WRITE_PERSISTENT_COHERENT && !mSupportsCoherentMemory )
            vboFlag = CPU_WRITE_PERSISTENT;

        VulkanRawBuffer retVal;
        allocateVbo( sizeBytes, alignment, mVbos[vboFlag], mBestVkMemoryTypeIndex[vboFlag],
                     mDefaultPoolSize[vboFlag], false, vboFlag != CPU_INACCESSIBLE,
                     isVboFlagCoherent( vboFlag ), retVal.mVboPoolIdx, retVal.mInternalBufferStart );
        Vbo &vbo = mVbos[vboFlag][retVal.mVboPoolIdx];
        retVal.mVboFlag = vboFlag;
        retVal.mVboName = vbo.vkBuffer;
        retVal.mDynamicBuffer = vbo.dynamicBuffer;
        retVal.mUnmapTicket = std::numeric_limits<size_t>::max();
        retVal.mSize = sizeBytes;
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::deallocateRawBuffer( VulkanRawBuffer &rawBuffer )
    {
        OGRE_ASSERT_LOW( rawBuffer.mUnmapTicket == std::numeric_limits<size_t>::max() &&
                         "VulkanRawBuffer not unmapped (or dangling)" );
        deallocateVbo( rawBuffer.mVboPoolIdx, rawBuffer.mInternalBufferStart, rawBuffer.mSize,
                       mVbos[rawBuffer.mVboFlag] );
        memset( &rawBuffer, 0, sizeof( rawBuffer ) );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::addDelayedFunc( VulkanDelayedFuncBase *cmd )
    {
        mDelayedFuncs[mDynamicBufferCurrentFrame].push_back( cmd );
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

        allocateVbo( numElements * bytesPerElement, bytesPerElement, bufferType, false, vboIdx,
                     bufferOffset );

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType, false );
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
                       vertexBuffer->_getInternalTotalSizeBytes(), vertexBuffer->getBufferType(),
                       false );
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

        allocateVbo( numElements * bytesPerElement, bytesPerElement, bufferType, false, vboIdx,
                     bufferOffset );

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType, false );
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
                       indexBuffer->_getInternalTotalSizeBytes(), indexBuffer->getBufferType(), false );
    }
    //-----------------------------------------------------------------------------------
    ConstBufferPacked *VulkanVaoManager::createConstBufferImpl( size_t sizeBytes, BufferType bufferType,
                                                                void *initialData, bool keepAsShadow )
    {
        size_t vboIdx;
        size_t bufferOffset;

        size_t alignment = mConstBufferAlignment;
        size_t requestedSize = sizeBytes;

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType, false );

        if( bufferType >= BT_DYNAMIC_DEFAULT )
        {
            // For dynamic buffers, the size will be 3x times larger
            //(depending on mDynamicBufferMultiplier); we need the
            // offset after each map to be aligned; and for that, we
            // sizeBytes to be multiple of alignment.
            sizeBytes = alignToNextMultiple( sizeBytes, alignment );
        }

        allocateVbo( sizeBytes, alignment, bufferType, false, vboIdx, bufferOffset );

        Vbo &vbo = mVbos[vboFlag][vboIdx];
        VulkanBufferInterface *bufferInterface =
            new VulkanBufferInterface( vboIdx, vbo.vkBuffer, vbo.dynamicBuffer );

        VulkanConstBufferPacked *retVal = OGRE_NEW VulkanConstBufferPacked(
            bufferOffset, requestedSize, 1u, ( uint32 )( sizeBytes - requestedSize ), bufferType,
            initialData, keepAsShadow, mVkRenderSystem, this, bufferInterface );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyConstBufferImpl( ConstBufferPacked *constBuffer )
    {
        VulkanBufferInterface *bufferInterface =
            static_cast<VulkanBufferInterface *>( constBuffer->getBufferInterface() );

        deallocateVbo( bufferInterface->getVboPoolIndex(),
                       constBuffer->_getInternalBufferStart() * constBuffer->getBytesPerElement(),
                       constBuffer->_getInternalTotalSizeBytes(), constBuffer->getBufferType(), false );
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

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType, false );

        if( bufferType >= BT_DYNAMIC_DEFAULT )
        {
            // For dynamic buffers, the size will be 3x times larger
            //(depending on mDynamicBufferMultiplier); we need the
            // offset after each map to be aligned; and for that, we
            // sizeBytes to be multiple of alignment.
            sizeBytes = alignToNextMultiple( sizeBytes, alignment );
        }

        allocateVbo( sizeBytes, alignment, bufferType, false, vboIdx, bufferOffset );

        Vbo &vbo = mVbos[vboFlag][vboIdx];
        VulkanBufferInterface *bufferInterface =
            new VulkanBufferInterface( vboIdx, vbo.vkBuffer, vbo.dynamicBuffer );

        VulkanTexBufferPacked *retVal = OGRE_NEW VulkanTexBufferPacked(
            bufferOffset, requestedSize, 1u, ( uint32 )( sizeBytes - requestedSize ), bufferType,
            initialData, keepAsShadow, mVkRenderSystem, this, bufferInterface, pixelFormat );

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
                       texBuffer->_getInternalTotalSizeBytes(), texBuffer->getBufferType(), false );
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
        VboFlag vboFlag = bufferTypeToVboFlag( bufferType, false );

        allocateVbo( numElements * bytesPerElement, alignment, bufferType, false, vboIdx, bufferOffset );

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
                       uavBuffer->_getInternalTotalSizeBytes(), uavBuffer->getBufferType(), false );
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
            VboFlag vboFlag = bufferTypeToVboFlag( bufferType, false );

            allocateVbo( sizeBytes, alignment, bufferType, false, vboIdx, bufferOffset );

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
                indirectBuffer->_getInternalTotalSizeBytes(), indirectBuffer->getBufferType(), false );
        }
    }
    //-----------------------------------------------------------------------------------
    VertexArrayObject *VulkanVaoManager::createVertexArrayObjectImpl(
        const VertexBufferPackedVec &vertexBuffers, IndexBufferPacked *indexBuffer,
        OperationType opType )
    {
        HlmsManager *hlmsManager = Root::getSingleton().getHlmsManager();
        VertexElement2VecVec vertexElements = VertexArrayObject::getVertexDeclaration( vertexBuffers );
        const uint16 inputLayout = hlmsManager->_getInputLayoutId( vertexElements, opType );

        VaoVec::iterator itor = findVao( vertexBuffers, indexBuffer, opType );

        const uint32 renderQueueId = generateRenderQueueId( itor->vaoName, mNumGeneratedVaos );

        VertexArrayObject *retVal = OGRE_NEW VertexArrayObject(
            itor->vaoName, renderQueueId, inputLayout, vertexBuffers, indexBuffer, opType );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyVertexArrayObjectImpl( VertexArrayObject *vao )
    {
        VaoVec::iterator itor = mVaos.begin();
        VaoVec::iterator endt = mVaos.end();

        while( itor != endt && itor->vaoName != vao->getVaoName() )
            ++itor;

        if( itor != endt )
        {
            --itor->refCount;

            if( !itor->refCount )
                efficientVectorRemove( mVaos, itor );
        }

        // We delete it here because this class has no virtual destructor on purpose
        OGRE_DELETE vao;
    }
    //-----------------------------------------------------------------------------------
    VulkanVaoManager::VaoVec::iterator VulkanVaoManager::findVao(
        const VertexBufferPackedVec &vertexBuffers, IndexBufferPacked *indexBuffer,
        OperationType opType )
    {
        Vao vao;

        vao.operationType = opType;
        vao.vertexBuffers.reserve( vertexBuffers.size() );

        {
            VertexBufferPackedVec::const_iterator itor = vertexBuffers.begin();
            VertexBufferPackedVec::const_iterator end = vertexBuffers.end();

            while( itor != end )
            {
                Vao::VertexBinding vertexBinding;
                vertexBinding.vertexBufferVbo =
                    static_cast<VulkanBufferInterface *>( ( *itor )->getBufferInterface() )
                        ->getVboName();
                vertexBinding.vertexElements = ( *itor )->getVertexElements();
                vertexBinding.instancingDivisor = 0;

                vao.vertexBuffers.push_back( vertexBinding );

                ++itor;
            }
        }

        vao.refCount = 0;

        if( indexBuffer )
        {
            vao.indexBufferVbo =
                static_cast<VulkanBufferInterface *>( indexBuffer->getBufferInterface() )->getVboName();
            vao.indexType = indexBuffer->getIndexType();
        }
        else
        {
            vao.indexBufferVbo = 0;
            vao.indexType = IndexBufferPacked::IT_16BIT;
        }

        bool bFound = false;
        VaoVec::iterator itor = mVaos.begin();
        VaoVec::iterator endt = mVaos.end();

        while( itor != endt && !bFound )
        {
            if( itor->operationType == vao.operationType && itor->indexBufferVbo == vao.indexBufferVbo &&
                itor->indexType == vao.indexType && itor->vertexBuffers == vao.vertexBuffers )
            {
                bFound = true;
            }
            else
            {
                ++itor;
            }
        }

        if( !bFound )
        {
            vao.vaoName = createVao();
            mVaos.push_back( vao );
            itor = mVaos.begin() + mVaos.size() - 1u;
        }

        ++itor->refCount;

        return itor;
    }
    //-----------------------------------------------------------------------------------
    uint32 VulkanVaoManager::createVao( void ) { return mVaoNames++; }
    //-----------------------------------------------------------------------------------
    uint32 VulkanVaoManager::generateRenderQueueId( uint32 vaoName, uint32 uniqueVaoId )
    {
        // Mix mNumGeneratedVaos with the D3D11 Vao for better sorting purposes:
        //  If we only use the D3D11's vao, the RQ will sort Meshes with
        //  multiple submeshes mixed with other meshes.
        //  For cache locality, and assuming all of them have the same GL vao,
        //  we prefer the RQ to sort:
        //      1. Mesh A - SubMesh 0
        //      2. Mesh A - SubMesh 1
        //      3. Mesh B - SubMesh 0
        //      4. Mesh B - SubMesh 1
        //      5. Mesh D - SubMesh 0
        //  If we don't mix mNumGeneratedVaos in it; the following could be possible:
        //      1. Mesh B - SubMesh 1
        //      2. Mesh D - SubMesh 0
        //      3. Mesh A - SubMesh 1
        //      4. Mesh B - SubMesh 0
        //      5. Mesh A - SubMesh 0
        //  Thus thrashing the cache unnecessarily.
        // clang-format off
        const int bitsVaoGl  = 5;
        const uint32 maskVaoGl  = OGRE_RQ_MAKE_MASK( bitsVaoGl );
        const uint32 maskVao    = OGRE_RQ_MAKE_MASK( RqBits::MeshBits - bitsVaoGl );

        const uint32 shiftVaoGl = ( uint32 )( RqBits::MeshBits - bitsVaoGl );

        uint32 renderQueueId =
                ( (vaoName & maskVaoGl) << shiftVaoGl ) |
                (uniqueVaoId & maskVao);
        // clang-format on

        return renderQueueId;
    }
    //-----------------------------------------------------------------------------------
    StagingBuffer *VulkanVaoManager::createStagingBuffer( size_t sizeBytes, bool forUpload )
    {
        sizeBytes = std::max<size_t>( sizeBytes, 4u * 1024u * 1024u );

        size_t vboIdx;
        size_t bufferOffset;
        allocateVbo( sizeBytes, 4u, BT_DYNAMIC_DEFAULT, !forUpload, vboIdx, bufferOffset );
        const VboFlag vboFlag = bufferTypeToVboFlag( BT_DYNAMIC_DEFAULT, !forUpload );

        Vbo &vbo = mVbos[vboFlag][vboIdx];

        VulkanStagingBuffer *stagingBuffer = OGRE_NEW VulkanStagingBuffer(
            vboIdx, bufferOffset, sizeBytes, this, forUpload, vbo.vkBuffer, vbo.dynamicBuffer );
        mRefedStagingBuffers[forUpload].push_back( stagingBuffer );

        if( mNextStagingBufferTimestampCheckpoint == (unsigned long)( ~0 ) )
        {
            mNextStagingBufferTimestampCheckpoint =
                mTimer->getMilliseconds() + mDefaultStagingBufferLifetime;
        }

        return stagingBuffer;
    }
    //-----------------------------------------------------------------------------------
    VulkanStagingTexture *VulkanVaoManager::createStagingTexture( PixelFormatGpu formatFamily,
                                                                  size_t sizeBytes )
    {
        sizeBytes = std::max<size_t>( sizeBytes, 4u * 1024u * 1024u );

        const size_t alignment = PixelFormatGpuUtils::getSizeBytes( 1u, 1u, 1u, 1u, formatFamily, 1u );

        size_t vboIdx;
        size_t bufferOffset;
        allocateVbo( sizeBytes, alignment, BT_DYNAMIC_DEFAULT, false, vboIdx, bufferOffset );
        const VboFlag vboFlag = bufferTypeToVboFlag( BT_DYNAMIC_DEFAULT, false );

        Vbo &vbo = mVbos[vboFlag][vboIdx];

        VulkanStagingTexture *retVal = OGRE_NEW VulkanStagingTexture(
            this, PixelFormatGpuUtils::getFamily( formatFamily ), sizeBytes, bufferOffset, vboIdx,
            vbo.vkBuffer, vbo.dynamicBuffer );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyStagingTexture( VulkanStagingTexture *stagingTexture )
    {
        stagingTexture->_unmapBuffer();
        deallocateVbo( stagingTexture->getVboPoolIndex(), stagingTexture->_getInternalBufferStart(),
                       stagingTexture->_getInternalTotalSizeBytes(), BT_DYNAMIC_DEFAULT, false );
    }
    //-----------------------------------------------------------------------------------
    AsyncTicketPtr VulkanVaoManager::createAsyncTicket( BufferPacked *creator,
                                                        StagingBuffer *stagingBuffer,
                                                        size_t elementStart, size_t elementCount )
    {
        return AsyncTicketPtr( OGRE_NEW VulkanAsyncTicket( creator, stagingBuffer, elementStart,
                                                           elementCount, &mDevice->mGraphicsQueue ) );
    }
    //-----------------------------------------------------------------------------------
    VulkanDescriptorPool *VulkanVaoManager::getDescriptorPool( const VulkanRootLayout *rootLayout,
                                                               size_t setIdx,
                                                               VkDescriptorSetLayout setLayout )
    {
        size_t capacity = 16u;
        VulkanDescriptorPool *retVal = 0;

        VulkanDescriptorPoolMap::iterator itor = mDescriptorPools.find( setLayout );
        if( itor != mDescriptorPools.end() )
        {
            FastArray<VulkanDescriptorPool *>::const_iterator it = itor->second.begin();
            FastArray<VulkanDescriptorPool *>::const_iterator en = itor->second.end();

            while( it != en && !retVal )
            {
                capacity = std::max( capacity, ( *it )->getCurrentCapacity() );

                if( ( *it )->isAvailableInCurrentFrame() )
                    retVal = *it;
                ++it;
            }
        }

        if( !retVal )
        {
            retVal = new VulkanDescriptorPool( this, rootLayout, setIdx, capacity );
            mDescriptorPools[setLayout].push_back( retVal );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::_schedulePoolAdvanceFrame( VulkanDescriptorPool *pool )
    {
        mUsedDescriptorPools.push_back( pool );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::_update( void )
    {
        {
            FastArray<VulkanDescriptorPool *>::const_iterator itor = mUsedDescriptorPools.begin();
            FastArray<VulkanDescriptorPool *>::const_iterator endt = mUsedDescriptorPools.end();

            while( itor != endt )
            {
                ( *itor )->_advanceFrame();
                ++itor;
            }
        }

        VaoManager::_update();
        // Undo the increment from VaoManager::_update. This is done by _notifyNewCommandBuffer
        --mFrameCount;

        mUsedDescriptorPools.clear();

        uint64 currentTimeMs = mTimer->getMilliseconds();

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

        if( mDelayedFuncs[mDynamicBufferCurrentFrame].empty() )
        {
            waitForTailFrameToFinish();

            VulkanDelayedFuncBaseArray::const_iterator itor =
                mDelayedFuncs[mDynamicBufferCurrentFrame].begin();
            VulkanDelayedFuncBaseArray::const_iterator endt =
                mDelayedFuncs[mDynamicBufferCurrentFrame].end();

            while( itor != endt )
            {
                ( *itor )->execute();
                delete *itor;
                ++itor;
            }

            mDelayedFuncs[mDynamicBufferCurrentFrame].clear();
        }

        if( !mFenceFlushed )
        {
            // We could only reach here if _update() was called
            // twice in a row without completing a full frame.
            // Without this, waitForTailFrameToFinish becomes unsafe.
            mDevice->commitAndNextCommandBuffer( false );
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
    void VulkanVaoManager::waitForSpecificFrameToFinish( uint32 frameCount )
    {
        if( frameCount == mFrameCount )
        {
            // Full stall
            mDevice->stall();
            //"mFrameCount += mDynamicBufferMultiplier" is already handled in _notifyDeviceStalled;
        }
        if( mFrameCount - frameCount <= mDynamicBufferMultiplier )
        {
            // Let's wait on one of our existing fences...
            // frameDiff has to be in range [1; mDynamicBufferMultiplier]
            size_t frameDiff = mFrameCount - frameCount;

            const size_t idx = ( mDynamicBufferCurrentFrame + mDynamicBufferMultiplier - frameDiff ) %
                               mDynamicBufferMultiplier;

            mDevice->mGraphicsQueue._waitOnFrame( static_cast<uint8>( idx ) );
        }
        else
        {
            // No stall
        }
    }
    //-----------------------------------------------------------------------------------
    bool VulkanVaoManager::isFrameFinished( uint32 frameCount )
    {
        bool retVal = false;
        if( frameCount == mFrameCount )
        {
            // Full stall
            // retVal = false;
        }
        else if( mFrameCount - frameCount <= mDynamicBufferMultiplier )
        {
            // frameDiff has to be in range [1; mDynamicBufferMultiplier]
            size_t frameDiff = mFrameCount - frameCount;
            const size_t idx = ( mDynamicBufferCurrentFrame + mDynamicBufferMultiplier - frameDiff ) %
                               mDynamicBufferMultiplier;

            retVal = mDevice->mGraphicsQueue._isFrameFinished( static_cast<uint8>( idx ) );
        }
        else
        {
            // No stall
            retVal = true;
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    VkFence VulkanVaoManager::waitFor( VkFence fenceName, VulkanQueue *queue )
    {
        if( !queue->isFenceFlushed( fenceName ) )
            queue->commitAndNextCommandBuffer();

        VkResult result = vkWaitForFences( queue->mDevice, 1u, &fenceName, VK_TRUE,
                                           UINT64_MAX );  // You can't wait forever in Vulkan?!?
        checkVkResult( result, "VulkanStagingBuffer::wait" );
        queue->releaseFence( fenceName );
        return 0;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::_notifyDeviceStalled()
    {
        mFenceFlushed = true;

        for( size_t i = 0; i < 2u; ++i )
        {
            StagingBufferVec::const_iterator itor = mRefedStagingBuffers[i].begin();
            StagingBufferVec::const_iterator endt = mRefedStagingBuffers[i].end();

            while( itor != endt )
            {
                VulkanStagingBuffer *stagingBuffer = static_cast<VulkanStagingBuffer *>( *itor );
                stagingBuffer->_notifyDeviceStalled();
                ++itor;
            }

            itor = mZeroRefStagingBuffers[i].begin();
            endt = mZeroRefStagingBuffers[i].end();

            while( itor != endt )
            {
                VulkanStagingBuffer *stagingBuffer = static_cast<VulkanStagingBuffer *>( *itor );
                stagingBuffer->_notifyDeviceStalled();
                ++itor;
            }
        }

        _destroyAllDelayedBuffers();

        FastArray<VulkanDelayedFuncBaseArray>::iterator itFrame = mDelayedFuncs.begin();
        FastArray<VulkanDelayedFuncBaseArray>::iterator enFrame = mDelayedFuncs.end();

        while( itFrame != enFrame )
        {
            VulkanDelayedFuncBaseArray::const_iterator itor = itFrame->begin();
            VulkanDelayedFuncBaseArray::const_iterator endt = itFrame->end();

            while( itor != endt )
            {
                ( *itor )->execute();
                delete *itor;
                ++itor;
            }

            itFrame->clear();
            ++itFrame;
        }

        mFrameCount += mDynamicBufferMultiplier;
    }
    //-----------------------------------------------------------------------------------
    uint32 VulkanVaoManager::getAttributeIndexFor( VertexElementSemantic semantic )
    {
        return VERTEX_ATTRIBUTE_INDEX[semantic - 1];
    }
    //-----------------------------------------------------------------------------------
    VulkanVaoManager::VboFlag VulkanVaoManager::bufferTypeToVboFlag( BufferType bufferType,
                                                                     const bool readCapable ) const
    {
        if( readCapable )
        {
            OGRE_ASSERT_LOW( bufferType != BT_IMMUTABLE && bufferType != BT_DEFAULT );
            return CPU_READ_WRITE;
        }

        VboFlag vboFlag;

        switch( bufferType )
        {
        case BT_IMMUTABLE:
        case BT_DEFAULT:
            vboFlag = CPU_INACCESSIBLE;
            break;
        case BT_DYNAMIC_DEFAULT:
            vboFlag = CPU_WRITE_PERSISTENT;  // Prefer non-coherent memory by default
            break;
        case BT_DYNAMIC_PERSISTENT:
            vboFlag = CPU_WRITE_PERSISTENT;
            break;
        case BT_DYNAMIC_PERSISTENT_COHERENT:
            vboFlag = CPU_WRITE_PERSISTENT_COHERENT;
            break;
        }

        // Change flag if unavailable
        if( vboFlag == CPU_WRITE_PERSISTENT && !mSupportsNonCoherentMemory )
            vboFlag = CPU_WRITE_PERSISTENT_COHERENT;
        else if( vboFlag == CPU_WRITE_PERSISTENT_COHERENT && !mSupportsCoherentMemory )
            vboFlag = CPU_WRITE_PERSISTENT;

        return vboFlag;
    }
    //-----------------------------------------------------------------------------------
    bool VulkanVaoManager::isVboFlagCoherent( VboFlag vboFlag ) const
    {
        if( vboFlag == CPU_WRITE_PERSISTENT_COHERENT )
            return true;
        if( vboFlag == CPU_READ_WRITE && mReadMemoryIsCoherent )
            return true;
        return false;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void *VulkanRawBuffer::map( void )
    {
        OGRE_ASSERT_LOW( mDynamicBuffer && "CPU_INACCESSIBLE buffers cannot be mapped!" );
        OGRE_ASSERT_LOW( mUnmapTicket == std::numeric_limits<size_t>::max() &&
                         "Mapping VulkanRawBuffer twice!" );
        return mDynamicBuffer->map( mInternalBufferStart, mSize, mUnmapTicket );
    }
    //-----------------------------------------------------------------------------------
    void VulkanRawBuffer::unmap( void )
    {
        mDynamicBuffer->unmap( mUnmapTicket );
        mUnmapTicket = std::numeric_limits<size_t>::max();
    }
}  // namespace Ogre
