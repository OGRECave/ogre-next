/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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
#include "OgreVulkanRenderSystem.h"
#include "OgreVulkanRootLayout.h"
#include "OgreVulkanUtils.h"
#include "Vao/OgreVertexArrayObject.h"
#include "Vao/OgreVulkanAsyncTicket.h"
#include "Vao/OgreVulkanBufferInterface.h"
#include "Vao/OgreVulkanConstBufferPacked.h"
#include "Vao/OgreVulkanDynamicBuffer.h"
#ifdef _OGRE_MULTISOURCE_VBO
#    include "Vao/OgreVulkanMultiSourceVertexBufferPool.h"
#endif
#include "Vao/OgreVulkanReadOnlyBufferPacked.h"
#include "Vao/OgreVulkanReadOnlyTBufferWorkaround.h"
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

#define TODO_whenImplemented_include_stagingBuffers
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

    static const char *c_vboTypes[] = {
        "CPU_INACCESSIBLE",               //
        "CPU_WRITE_PERSISTENT",           //
        "CPU_WRITE_PERSISTENT_COHERENT",  //
        "CPU_READ_WRITE",                 //
        "TEXTURES_OPTIMAL",               //
        "MAX_VBO_FLAG",                   //
    };

    VulkanVaoManager::VulkanVaoManager( VulkanDevice *device, VulkanRenderSystem *renderSystem,
                                        const NameValuePairList *params ) :
        VaoManager( params ),
        mDelayedBlocksSize( 0u ),
        mDelayedBlocksFlushThreshold( 512u * 1024u * 1024u ),
        mVaoNames( 1u ),
        mDrawId( 0 ),
        mDevice( device ),
        mVkRenderSystem( renderSystem ),
        mFenceFlushedWarningCount( 0u ),
        mFenceFlushed( FenceUnflushed ),
        mSupportsCoherentMemory( false ),
        mSupportsNonCoherentMemory( false ),
        mReadMemoryIsCoherent( false )
    {
        if( params )
        {
            NameValuePairList::const_iterator itor =
                params->find( "VaoManager::mDelayedBlocksFlushThreshold" );
            if( itor != params->end() )
            {
                mDelayedBlocksFlushThreshold =
                    StringConverter::parseSizeT( itor->second, mDelayedBlocksFlushThreshold );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    VulkanVaoManager::~VulkanVaoManager() { destroyVkResources( true ); }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::createVkResources()
    {
        mConstBufferAlignment =
            (uint32)mDevice->mDeviceProperties.limits.minUniformBufferOffsetAlignment;
        mTexBufferAlignment = (uint32)mDevice->mDeviceProperties.limits.minTexelBufferOffsetAlignment;
        mUavBufferAlignment = (uint32)mDevice->mDeviceProperties.limits.minStorageBufferOffsetAlignment;

        mConstBufferMaxSize = mDevice->mDeviceProperties.limits.maxUniformBufferRange;
        mConstBufferMaxSize =
            std::min<size_t>( mConstBufferMaxSize, 64u * 1024u );  // No bigger than 64kb
        mTexBufferMaxSize = mDevice->mDeviceProperties.limits.maxTexelBufferElements;

        mUavBufferMaxSize = mDevice->mDeviceProperties.limits.maxStorageBufferRange;

#ifdef OGRE_VK_WORKAROUND_ADRENO_UBO64K
        Workarounds::mAdrenoUbo64kLimitTriggered = false;
        Workarounds::mAdrenoUbo64kLimit = 0u;
        if( mVkRenderSystem->getCapabilities()->getVendor() == GPU_QUALCOMM &&
            mVkRenderSystem->getCapabilities()->getDeviceName().find( "Turnip" ) == String::npos )
        {
            mConstBufferMaxSize =
                std::min<size_t>( mConstBufferMaxSize, 64u * 1024u - mConstBufferAlignment );
            Workarounds::mAdrenoUbo64kLimit = mConstBufferMaxSize;
        }
#endif
#ifdef OGRE_VK_WORKAROUND_ADRENO_5XX_6XX_MINCAPS
        if( Workarounds::mAdreno5xx6xxMinCaps )
            mTexBufferMaxSize = std::max<size_t>( mTexBufferMaxSize, 128u * 1024u * 1024u );
#endif

#ifdef OGRE_VK_WORKAROUND_PVR_ALIGNMENT
        if( mVkRenderSystem->getCapabilities()->getVendor() == GPU_IMGTEC &&
            !mVkRenderSystem->getCapabilities()->getDriverVersion().hasMinVersion( 1, 426, 234 ) )
        {
            Workarounds::mPowerVRAlignment = 16u;

            mConstBufferAlignment = std::max( mConstBufferAlignment, Workarounds::mPowerVRAlignment );
            mTexBufferAlignment = std::max( mTexBufferAlignment, Workarounds::mPowerVRAlignment );
            mUavBufferAlignment = std::max( mUavBufferAlignment, Workarounds::mPowerVRAlignment );
        }
#endif

        mSupportsPersistentMapping = true;
        mSupportsIndirectBuffers = mDevice->mDeviceFeatures.multiDrawIndirect &&
                                   mDevice->mDeviceFeatures.drawIndirectFirstInstance;

#ifdef OGRE_VK_WORKAROUND_ADRENO_618_0VERTEX_INDIRECT
        if( Workarounds::mAdreno618_0VertexIndirect )
            mSupportsIndirectBuffers = false;
#endif

        mReadOnlyIsTexBuffer = false;
        mReadOnlyBufferMaxSize = mUavBufferMaxSize;

#ifdef OGRE_VK_WORKAROUND_ADRENO_6xx_READONLY_IS_TBUFFER
        if( mVkRenderSystem->getCapabilities()->getVendor() == GPU_QUALCOMM &&
            mVkRenderSystem->getCapabilities()->getDeviceId() >= 0x6000000 &&
            mVkRenderSystem->getCapabilities()->getDeviceId() < 0x7000000 &&
            mVkRenderSystem->getCapabilities()->getDeviceName().find( "Turnip" ) == String::npos )
        {
            Workarounds::mAdreno6xxReadOnlyIsTBuffer = true;
            mReadOnlyIsTexBuffer = true;
            mReadOnlyBufferMaxSize = mTexBufferMaxSize;
        }
#endif

        memset( mUsedHeapMemory, 0, sizeof( mUsedHeapMemory ) );
        memset( mMemoryTypesInUse, 0, sizeof( mMemoryTypesInUse ) );

        // Keep pools of 64MB each for static meshes & most textures
        mDefaultPoolSize[CPU_INACCESSIBLE] = 64u * 1024u * 1024u;

        // Keep pools of 16MB each for write buffers, 4MB for read ones.
        for( size_t i = CPU_WRITE_PERSISTENT; i <= CPU_WRITE_PERSISTENT_COHERENT; ++i )
            mDefaultPoolSize[i] = 16u * 1024u * 1024u;
        mDefaultPoolSize[CPU_READ_WRITE] = 4u * 1024u * 1024u;
        mDefaultPoolSize[TEXTURES_OPTIMAL] = 64u * 1024u * 1024u;

        // Shrink the buffer pools a bit if textures and buffers can't live together
        if( mDevice->mDeviceProperties.limits.bufferImageGranularity != 1u )
            mDefaultPoolSize[CPU_INACCESSIBLE] = 32u * 1024u * 1024u;

        mDelayedFuncs.resize( mDynamicBufferMultiplier );

        determineBestMemoryTypes();

        initDrawIdVertexBuffer();
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyVkResources( bool finalDestruction )
    {
        mDrawId = 0;

        destroyAllVertexArrayObjects();
        deleteAllBuffers();

        for( VkSemaphore sem : mAvailableSemaphores )
            vkDestroySemaphore( mDevice->mDevice, sem, 0 );
        mAvailableSemaphores.clear();

        for( UsedSemaphore &sem : mUsedSemaphores )
            vkDestroySemaphore( mDevice->mDevice, sem.semaphore, 0 );
        mUsedSemaphores.clear();

        for( UsedSemaphore &sem : mUsedPresentSemaphores )
            vkDestroySemaphore( mDevice->mDevice, sem.semaphore, 0 );
        mUsedPresentSemaphores.clear();

        deleteStagingBuffers();

        for( VulkanDelayedFuncBaseArray &fnFrame : mDelayedFuncs )
        {
            for( VulkanDelayedFuncBase *fn : fnFrame )
            {
                fn->execute();
                delete fn;
            }
            fnFrame.clear();
        }
        // it's OK not to clear mDelayedFuncs

        flushAllGpuDelayedBlocks( false );

        for( VulkanDescriptorPoolMap::value_type &pools : mDescriptorPools )
        {
            for( VulkanDescriptorPool *pool : pools.second )
            {
                pool->deinitialize( mDevice );
                delete pool;
            }
        }
        mDescriptorPools.clear();

        mUsedDescriptorPools.clear();

        mEmptyVboPools.clear();

        for( size_t i = 0; i < MAX_VBO_FLAG; ++i )
        {
            for( Vbo &vbo : mVbos[i] )
            {
                if( vbo.isAllocated() )
                {
                    vkDestroyBuffer( mDevice->mDevice, vbo.vkBuffer, 0 );
                    vbo.vkBuffer = 0;
                    vkFreeMemory( mDevice->mDevice, vbo.vboName, 0 );
                    vbo.vboName = 0;
                    delete vbo.dynamicBuffer;
                    vbo.dynamicBuffer = 0;
                }
            }
            mVbos[i].clear();
            mUnallocatedVbos[i].clear();
        }
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
    void VulkanVaoManager::bindDrawIdVertexBuffer( VkCommandBuffer cmdBuffer, uint32 binding )
    {
        VulkanBufferInterface *bufIntf =
            static_cast<VulkanBufferInterface *>( mDrawId->getBufferInterface() );
        VkBuffer vertexBuffers[] = { bufIntf->getVboName() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers( cmdBuffer, binding, 1, vertexBuffers, offsets );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::getMemoryStats( const Block &block, size_t vboIdx, size_t poolIdx,
                                           size_t poolCapacity, LwString &text,
                                           MemoryStatsEntryVec &outStats, Log *log ) const
    {
        if( log )
        {
            text.clear();
            text.a( c_vboTypes[vboIdx], ";", (uint64)block.offset, ";", (uint64)block.size, ";" );
            text.a( (uint64)poolIdx, ";", (uint64)poolCapacity );
            log->logMessage( text.c_str(), LML_CRITICAL );
        }

        const VboFlag vboTextureFlag = mDevice->mDeviceProperties.limits.bufferImageGranularity == 1u
                                           ? CPU_INACCESSIBLE
                                           : TEXTURES_OPTIMAL;

        const bool bPoolHasTextures = vboIdx == vboTextureFlag;

        MemoryStatsEntry entry( (uint32)vboIdx, (uint32)poolIdx, block.offset, block.size, poolCapacity,
                                bPoolHasTextures );
        outStats.push_back( entry );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::getMemoryStats( MemoryStatsEntryVec &outStats, size_t &outCapacityBytes,
                                           size_t &outFreeBytes, Log *log,
                                           bool &outIncludesTextures ) const
    {
        size_t capacityBytes = 0;
        size_t freeBytes = 0;
        MemoryStatsEntryVec statsVec;
        statsVec.swap( outStats );

        vector<char>::type tmpBuffer;
        tmpBuffer.resize( 512 * 1024 );  // 512kb per line should be way more than enough
        LwString text( LwString::FromEmptyPointer( &tmpBuffer[0], tmpBuffer.size() ) );

        if( log )
            log->logMessage( "Pool Type;Offset;Size Bytes;Pool Idx;Pool Capacity", LML_CRITICAL );

        for( unsigned vboIdx = 0; vboIdx < MAX_VBO_FLAG; ++vboIdx )
        {
            VboVec::const_iterator itor = mVbos[vboIdx].begin();
            VboVec::const_iterator endt = mVbos[vboIdx].end();

            while( itor != endt )
            {
                const Vbo &vbo = *itor;
                const size_t poolIdx = static_cast<size_t>( itor - mVbos[vboIdx].begin() );
                capacityBytes += vbo.sizeBytes;

                Block usedBlock( 0, 0 );

                BlockVec freeBlocks = vbo.freeBlocks;
                while( !freeBlocks.empty() )
                {
                    // Find the free block that comes next
                    BlockVec::iterator nextBlock;
                    {
                        BlockVec::iterator itBlock = freeBlocks.begin();
                        BlockVec::iterator enBlock = freeBlocks.end();

                        nextBlock = itBlock;

                        while( itBlock != enBlock )
                        {
                            if( itBlock->offset < nextBlock->offset )
                                nextBlock = itBlock;
                            ++itBlock;
                        }
                    }

                    freeBytes += nextBlock->size;
                    usedBlock.size = nextBlock->offset - usedBlock.offset;

                    OGRE_ASSERT_LOW( usedBlock.size <= vbo.sizeBytes );
                    OGRE_ASSERT_LOW( usedBlock.offset + usedBlock.size <= vbo.sizeBytes );

                    // usedBlock.size could be 0 if:
                    //  1. All of memory is free
                    //  2. There's two contiguous free blocks, which should not happen
                    //     due to mergeContiguousBlocks
                    if( usedBlock.size > 0u )
                    {
                        getMemoryStats( usedBlock, vboIdx, poolIdx, vbo.sizeBytes, text, statsVec, log );
                    }

                    usedBlock.offset += usedBlock.size;
                    usedBlock.size = 0;
                    efficientVectorRemove( freeBlocks, nextBlock );
                }

                if( usedBlock.size > 0u || ( usedBlock.offset == 0 && usedBlock.size == 0 ) )
                {
                    if( vbo.freeBlocks.empty() )
                    {
                        OGRE_ASSERT_LOW( usedBlock.offset == 0 && usedBlock.size == 0 );
                        usedBlock.offset = 0u;
                        usedBlock.size = vbo.sizeBytes;
                    }
                    getMemoryStats( usedBlock, vboIdx, poolIdx, vbo.sizeBytes, text, statsVec, log );
                }

                ++itor;
            }
        }

        outCapacityBytes = capacityBytes;
        outFreeBytes = freeBytes;
        outIncludesTextures = true;
        statsVec.swap( outStats );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::deallocateEmptyVbos( const bool bDeviceStall )
    {
        if( mEmptyVboPools.empty() )
            return;

        if( !bDeviceStall )
            waitForTailFrameToFinish();

        const VkMemoryType *memTypes = mDevice->mDeviceMemoryProperties.memoryTypes;

        set<VboIndex>::type::iterator itor = mEmptyVboPools.begin();
        set<VboIndex>::type::iterator endt = mEmptyVboPools.end();

        while( itor != endt )
        {
            Vbo &vbo = mVbos[itor->vboFlag][itor->vboIdx];
            OGRE_ASSERT_MEDIUM( vbo.isEmpty() );
            OGRE_ASSERT_MEDIUM( vbo.isAllocated() );

            if( ( mFrameCount - vbo.emptyFrame ) >= mDynamicBufferMultiplier || bDeviceStall )
            {
                OGRE_ASSERT_LOW( mUsedHeapMemory[memTypes[vbo.vkMemoryTypeIdx].heapIndex] >=
                                 vbo.sizeBytes );
                mUsedHeapMemory[memTypes[vbo.vkMemoryTypeIdx].heapIndex] -= vbo.sizeBytes;

                vkDestroyBuffer( mDevice->mDevice, vbo.vkBuffer, 0 );
                vkFreeMemory( mDevice->mDevice, vbo.vboName, 0 );

                vbo.vboName = 0;
                vbo.vkBuffer = 0;
                vbo.sizeBytes = 0u;
                delete vbo.dynamicBuffer;
                vbo.dynamicBuffer = 0;

                vbo.freeBlocks.clear();
                vbo.emptyFrame = mFrameCount;

                mUnallocatedVbos[itor->vboFlag].push_back( itor->vboIdx );

                set<VboIndex>::type::iterator toErase = itor++;
                mEmptyVboPools.erase( toErase );
            }
            else
            {
                ++itor;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::switchVboPoolIndexImpl( unsigned internalVboBufferType, size_t oldPoolIdx,
                                                   size_t newPoolIdx, BufferPacked *buffer )
    {
        if( mSupportsIndirectBuffers || buffer->getBufferPackedType() != BP_TYPE_INDIRECT )
        {
            VulkanBufferInterface *bufferInterface =
                static_cast<VulkanBufferInterface *>( buffer->getBufferInterface() );

            // All BufferPacked are created with readCapable = false
            // That is an attribute used for staging buffers
            const VboFlag vboFlag = bufferTypeToVboFlag( buffer->getBufferType(), false );
            if( vboFlag == internalVboBufferType )
            {
                if( bufferInterface->getVboPoolIndex() == oldPoolIdx )
                    bufferInterface->_setVboPoolIndex( newPoolIdx );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    inline uint64 bytesToMegabytes( size_t sizeBytes ) { return uint64_t( sizeBytes >> 20u ); }
    //-----------------------------------------------------------------------------------
    bool VulkanVaoManager::flushAllGpuDelayedBlocks( const bool bIssueBarrier )
    {
        if( bIssueBarrier && !mDevice->isDeviceLost() )
        {
            if( mDevice->mGraphicsQueue.getEncoderState() == VulkanQueue::EncoderGraphicsOpen )
            {
                // We can't flush right now
                return false;
            }

            char tmpBuffer[256];
            LwString text( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
            text.a( "Vulkan: Flushing all mDelayedBlocks(", bytesToMegabytes( mDelayedBlocksSize ),
                    " MB) because mDelayedBlocksFlushThreshold(",
                    bytesToMegabytes( mDelayedBlocksFlushThreshold ),
                    " MB) was exceeded. This prevents async operations (e.g. async compute)",
                    LML_TRIVIAL );

            LogManager::getSingleton().logMessage( text.c_str() );

            VkMemoryBarrier memBarrier;
            makeVkStruct( memBarrier, VK_STRUCTURE_TYPE_MEMORY_BARRIER );

            memBarrier.srcAccessMask =
                VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT |
                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT |
                VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
                VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                VK_ACCESS_TRANSFER_WRITE_BIT /*| VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT*/;
            memBarrier.dstAccessMask =
                VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT |
                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT |
                VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
                VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT |
                VK_ACCESS_TRANSFER_WRITE_BIT /*| VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT*/;

            vkCmdPipelineBarrier( mDevice->mGraphicsQueue.getCurrentCmdBuffer(),
                                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                  0, 1u, &memBarrier, 0u, 0, 0u, 0 );
        }

        DirtyBlockArray::iterator itor = mDelayedBlocks.begin();
        DirtyBlockArray::iterator endt = mDelayedBlocks.end();

        while( itor != endt )
        {
            deallocateVbo( itor->vboIdx, itor->offset, itor->size, itor->vboFlag, true );
            ++itor;
        }

        mDelayedBlocks.clear();
        mDelayedBlocksSize = 0u;

        return true;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::flushGpuDelayedBlocks()
    {
        const uint32 frameCount = mFrameCount;

        size_t bytesFlushed = 0u;

        // Unlike CPU -> GPU (or GPU -> CPU) resources, GPU -> GPU resources
        // are safe to reuse after 1 frame of synchronization.
        DirtyBlockArray::iterator itor = mDelayedBlocks.begin();
        DirtyBlockArray::iterator endt = mDelayedBlocks.end();

        while( itor != endt && ( frameCount - itor->frameIdx ) >= 1u )
        {
            bytesFlushed += itor->size;
            deallocateVbo( itor->vboIdx, itor->offset, itor->size, itor->vboFlag, true );
            ++itor;
        }

        OGRE_ASSERT_LOW( mDelayedBlocksSize >= bytesFlushed );

        mDelayedBlocks.erasePOD( mDelayedBlocks.begin(), itor );
        mDelayedBlocksSize -= bytesFlushed;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::cleanupEmptyPools()
    {
        // And StagingBuffers, StagingTextures, TextureGpu, and anything that
        // calls either allocateVbo or allocateRawBuffer (e.g. AsyncTextureTicket,
        // VulkanDiscardBufferManager, etc)
        // And mEmptyVboPools, and clear mUnallocatedVbos
        TODO_whenImplemented_include_stagingBuffers;

        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "", "VulkanVaoManager::cleanupEmptyPools" );

        for( unsigned vboIdx = 0; vboIdx < MAX_VBO_FLAG; ++vboIdx )
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
                    switchVboPoolIndex( vboIdx, (size_t)( mVbos[vboIdx].size() - 1u ),
                                        (size_t)( itor - mVbos[vboIdx].begin() ) );

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
    // Higher score means more preferred
    static int calculateMemoryTypeScore( VulkanVaoManager::VboFlag vboFlag,
                                         const VkPhysicalDeviceMemoryProperties &memProperties,
                                         const size_t memoryTypeIdx, const size_t otherMemoryTypeIdx )
    {
        int score = 0;

        const VkMemoryType memType = memProperties.memoryTypes[memoryTypeIdx];
        const VkMemoryType otherMemType = memProperties.memoryTypes[otherMemoryTypeIdx];
        switch( vboFlag )
        {
        case VulkanVaoManager::CPU_INACCESSIBLE:
            if( memType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
                score += 1000;

            for( uint32 bitFlag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                 bitFlag <= VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD; bitFlag = bitFlag << 1u )
            {
                // Fewer flags set means better
                if( !( memType.propertyFlags & bitFlag ) )
                {
                    score += 5;
                    if( bitFlag == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
                        score += 100;  // Super extra score for not having this one
                    if( bitFlag == VK_MEMORY_PROPERTY_HOST_CACHED_BIT )
                        score += 5;  // Extra score for not having this one
                }
            }
            break;
        case VulkanVaoManager::CPU_WRITE_PERSISTENT:
        case VulkanVaoManager::CPU_WRITE_PERSISTENT_COHERENT:
            OGRE_ASSERT_LOW( memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );

            // Prefer write combined (aka uncached)
            if( !( memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ) )
                score += 2;
            // Prefer host-local (rather than device-local)
            if( !( memType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) )
                score += 1;

            // Prefer heaps which are significantly bigger. Particularly RADV on iGPUs reports two
            // *identical* heaps except for its size; where heap 0 is 256 MB and heap 1 is 3GB
            if( memProperties.memoryHeaps[memType.heapIndex].size >
                ( memProperties.memoryHeaps[otherMemType.heapIndex].size << 1u ) )
            {
                score += 3;
            }
            break;
        case VulkanVaoManager::CPU_READ_WRITE:
            OGRE_ASSERT_LOW( memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );

            // Prefer cached memory
            if( memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT )
                score += 2;
            break;
        case VulkanVaoManager::TEXTURES_OPTIMAL:
            break;
        case VulkanVaoManager::MAX_VBO_FLAG:
            OGRE_ASSERT_LOW( false && "Internal Error this path should not be reached" );
            break;
        }

        return score;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::addMemoryType( VboFlag vboFlag,
                                          const VkPhysicalDeviceMemoryProperties &memProperties,
                                          const uint32 memoryTypeIdx )
    {
        // Skip zero size heaps, as in https://vulkan.gpuinfo.org/displayreport.php?id=34174#memory
        // on Vulkan Compatibility Pack for Arm64 Windows over Parallels Display Adapter (WDDM)
        if( memProperties.memoryHeaps[memProperties.memoryTypes[memoryTypeIdx].heapIndex].size == 0 )
            return;

        FastArray<uint32>::iterator itor = mBestVkMemoryTypeIndex[vboFlag].begin();
        FastArray<uint32>::iterator endt = mBestVkMemoryTypeIndex[vboFlag].end();

        const uint32_t newHeapIdx = memProperties.memoryTypes[memoryTypeIdx].heapIndex;
        while( itor != endt && memProperties.memoryTypes[*itor].heapIndex != newHeapIdx )
            ++itor;

        if( itor == endt )
        {
            // This is the first time we see this heap
            mBestVkMemoryTypeIndex[vboFlag].push_back( memoryTypeIdx );
        }
        else
        {
            // We already added this heap.
            // Check if this memory type is preferred over the one we already added
            const int oldScore =
                calculateMemoryTypeScore( vboFlag, memProperties, *itor, memoryTypeIdx );
            const int newScore =
                calculateMemoryTypeScore( vboFlag, memProperties, memoryTypeIdx, *itor );

            // If the scores are equal, prefer the one we found first
            // (by spec, lower memory heap idxs must have greater performance)
            if( newScore > oldScore )
                *itor = memoryTypeIdx;
        }
    }
    //-----------------------------------------------------------------------------------
    uint32 VulkanVaoManager::determineSupportedMemoryTypes( VkBufferUsageFlags usageFlags ) const
    {
        VkBuffer tmpBuffer;
        VkBufferCreateInfo bufferCi;
        makeVkStruct( bufferCi, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO );
        bufferCi.size = 64;
        bufferCi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                         VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        VkResult result = vkCreateBuffer( mDevice->mDevice, &bufferCi, 0, &tmpBuffer );
        checkVkResult( mDevice, result, "vkCreateBuffer" );

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements( mDevice->mDevice, tmpBuffer, &memRequirements );
        vkDestroyBuffer( mDevice->mDevice, tmpBuffer, 0 );

        return memRequirements.memoryTypeBits;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::determineBestMemoryTypes()
    {
        for( size_t i = 0u; i < MAX_VBO_FLAG; ++i )
            mBestVkMemoryTypeIndex[i].clear();

        // Not all buffers usage can be bound to all memory types. Filter out those we cannot use
        const uint32 supportedMemoryTypesBuffer = determineSupportedMemoryTypes(
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT );

        const uint32 supportedMemoryTypesRead = determineSupportedMemoryTypes(
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT );

        LogManager &logManager = LogManager::getSingleton();
        logManager.logMessage( "Vulkan: Supported memory types for general buffer usage: " +
                               StringConverter::toString( supportedMemoryTypesBuffer ) );
        logManager.logMessage( "Vulkan: Supported memory types for reading: " +
                               StringConverter::toString( supportedMemoryTypesRead ) );

        const VkPhysicalDeviceMemoryProperties &memProperties = mDevice->mDeviceMemoryProperties;
        const uint32 numMemoryTypes = memProperties.memoryTypeCount;

        for( uint32 i = 0u; i < numMemoryTypes; ++i )
        {
            // Find the best memory that is cached for reading
            if( ( ( 1u << i ) & supportedMemoryTypesRead ) &&
                ( memProperties.memoryTypes[i].propertyFlags &
                  ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT ) ) ==
                    ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT ) )
            {
                addMemoryType( CPU_READ_WRITE, memProperties, i );
            }

            if( !( ( 1u << i ) & supportedMemoryTypesBuffer ) )
                continue;

            if( memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
                addMemoryType( CPU_INACCESSIBLE, memProperties, i );

            // Find non-coherent memory (many desktop GPUs don't provide this)
            // Prefer memory local to CPU
            if( ( memProperties.memoryTypes[i].propertyFlags &
                  ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) ==
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
            {
                addMemoryType( CPU_WRITE_PERSISTENT, memProperties, i );
            }

            // Find coherent memory (many desktop GPUs don't provide this)
            // Prefer memory local to CPU
            if( ( memProperties.memoryTypes[i].propertyFlags &
                  ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) ==
                ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) )
            {
                addMemoryType( CPU_WRITE_PERSISTENT_COHERENT, memProperties, i );
            }
        }

        if( mBestVkMemoryTypeIndex[CPU_INACCESSIBLE].empty() )
        {
            // This is BS. No heap is device-local. Sigh, just pick any and try to get the best score
            logManager.logMessage(
                "Vulkan: No heap found with DEVICE_LOCAL bit set. This should be impossible",
                LML_CRITICAL );
            for( uint32 i = 0u; i < numMemoryTypes; ++i )
                addMemoryType( CPU_INACCESSIBLE, memProperties, i );
        }

        mSupportsNonCoherentMemory = !mBestVkMemoryTypeIndex[CPU_WRITE_PERSISTENT].empty();
        mSupportsCoherentMemory = !mBestVkMemoryTypeIndex[CPU_WRITE_PERSISTENT_COHERENT].empty();

        mPreferCoherentMemory = false;
        if( !mBestVkMemoryTypeIndex[CPU_WRITE_PERSISTENT_COHERENT].empty() )
        {
            // When it comes to writing (and at least in theory), the following order should be
            // preferred:
            //
            // 1 Uncached, coherent
            // 2 Cached, non-coherent
            // 3 Cached, coherent
            // 4 Anything else (uncached, non-coherent? that doesn't make sense)
            //
            // "Uncached, coherent" means CPU caches are bypassed. Hence GPU doesn't have to worry about
            // them and can automatically synchronize its internal caches (since there's nothing to
            // synchronize).
            //
            // "Cached" means CPU caches are important. If it's coherent, HW (likely) needs to do heavy
            // work to keep both CPU & GPU caches in sync. Hence non-coherent makes sense to be superior
            // here since we just invalidate the GPU caches by hand.
            //
            // Now there are a few gotchas:
            //
            // - Whether 1 is faster than 2 mostly depends on whether the code makes correct use of
            //   write-combining (or else perf goes down fast).
            // - Whether 2 is faster than 3 or viceversa may be HW specific. For example Intel only
            //   exposes Cached + Coherent and nothing else. It seems they have no performance problems
            //   at all.
            //
            // This assumes CPU write-combining is fast. If it's not, then this is wrong.
            const uint32 idx = mBestVkMemoryTypeIndex[CPU_WRITE_PERSISTENT_COHERENT].back();
            // Prefer coherent memory if it's uncached, or if we have no choice.
            if( !mSupportsNonCoherentMemory ||
                !( memProperties.memoryTypes[idx].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ) )
            {
                mPreferCoherentMemory = true;
            }
        }

        logManager.logMessage( "Vulkan: VkDevice will use coherent memory buffers: " +
                               StringConverter::toString( mSupportsCoherentMemory ) );
        logManager.logMessage( "Vulkan: VkDevice will use non-coherent memory buffers: " +
                               StringConverter::toString( mSupportsNonCoherentMemory ) );
        logManager.logMessage( "Vulkan: VkDevice will prefer coherent memory buffers: " +
                               StringConverter::toString( mPreferCoherentMemory ) );

        if( mBestVkMemoryTypeIndex[CPU_READ_WRITE].empty() )
        {
            logManager.logMessage(
                "Vulkan: could not find cached host-visible memory. GPU -> CPU transfers could be "
                "slow",
                LML_CRITICAL );

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

        if( !mSupportsNonCoherentMemory && !mSupportsCoherentMemory )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Device does not expose coherent nor non-coherent CPU-accessible "
                         "memory that can use all the buffer types we need! "
                         "This app cannot be used with this GPU. Try updating your drivers",
                         "VulkanVaoManager::determineBestMemoryTypes" );
        }

        mReadMemoryIsCoherent =
            ( memProperties.memoryTypes[mBestVkMemoryTypeIndex[CPU_READ_WRITE].back()].propertyFlags &
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) != 0;

        if( mDevice->mDeviceProperties.limits.bufferImageGranularity != 1u )
            mBestVkMemoryTypeIndex[TEXTURES_OPTIMAL] = mBestVkMemoryTypeIndex[CPU_INACCESSIBLE];

        logManager.logMessage( "Vulkan: VkDevice will use coherent memory for reading: " +
                               StringConverter::toString( mReadMemoryIsCoherent ) );

        // Fill mMemoryTypesInUse
        for( size_t i = 0u; i < MAX_VBO_FLAG; ++i )
        {
            mMemoryTypesInUse[i] = 0u;

            FastArray<uint32>::const_iterator itor = mBestVkMemoryTypeIndex[i].begin();
            FastArray<uint32>::const_iterator endt = mBestVkMemoryTypeIndex[i].end();

            while( itor != endt )
            {
                mMemoryTypesInUse[i] |= 1u << *itor;
                ++itor;
            }
        }

        logManager.logMessage( "Vulkan: VkDevice read memory is coherent: " +
                               StringConverter::toString( mReadMemoryIsCoherent ) );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::allocateVbo( size_t sizeBytes, size_t alignment, BufferType bufferType,
                                        bool readCapable, bool skipDynBufferMultiplier,
                                        size_t &outVboIdx, size_t &outBufferOffset )
    {
        OGRE_ASSERT_LOW( alignment > 0 );

        TODO_if_memory_non_coherent_align_size;

        const VboFlag vboFlag = bufferTypeToVboFlag( bufferType, readCapable );

        if( bufferType >= BT_DYNAMIC_DEFAULT && !readCapable && !skipDynBufferMultiplier )
            sizeBytes *= mDynamicBufferMultiplier;

        allocateVbo( sizeBytes, alignment, vboFlag, mMemoryTypesInUse[vboFlag], outVboIdx,
                     outBufferOffset );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::allocateVbo( size_t sizeBytes, size_t alignment, VboFlag vboFlag,
                                        uint32 textureMemTypeBits, size_t &outVboIdx,
                                        size_t &outBufferOffset )
    {
        if( mDelayedBlocksSize >= mDelayedBlocksFlushThreshold )
            flushAllGpuDelayedBlocks( true );

        VboVec &vboVec = mVbos[vboFlag];

        VboVec::const_iterator itor = vboVec.begin();
        VboVec::const_iterator endt = vboVec.end();

        // clang-format off
        // Find a suitable VBO that can hold the requested size. We prefer those free
        // blocks that have a matching stride (the current offset is a multiple of
        // bytesPerElement) in order to minimize the amount of memory padding.
        size_t bestVboIdx   = std::numeric_limits<size_t>::max();
        ptrdiff_t bestBlockIdx = -1;
        bool foundMatchingStride = false;
        // clang-format on

        while( itor != endt && !foundMatchingStride )
        {
            // First check the allocation can be done inside this Vbo
            if( ( 1u << itor->vkMemoryTypeIdx ) & textureMemTypeBits )
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
                        bestBlockIdx    = blockIt - itor->freeBlocks.begin();
                        // clang-format on

                        if( newOffset == block.offset )
                            foundMatchingStride = true;
                    }

                    ++blockIt;
                }
            }

            ++itor;
        }

        if( bestBlockIdx == -1 )
        {
            // clang-format off
            bestVboIdx      = vboVec.size();
            bestBlockIdx    = 0;
            foundMatchingStride = true;
            // clang-format on

            Vbo newVbo;

            const VkMemoryHeap *memHeaps = mDevice->mDeviceMemoryProperties.memoryHeaps;
            const VkMemoryType *memTypes = mDevice->mDeviceMemoryProperties.memoryTypes;

            const FastArray<uint32> &vkMemoryTypeIndex = mBestVkMemoryTypeIndex[vboFlag];

            size_t poolSize = std::numeric_limits<size_t>::max();

            // No luck, allocate a new buffer.
            uint32 chosenMemoryTypeIdx = std::numeric_limits<uint32>::max();
            bool bIsTextureOnly = false;
            {
                // Find the first heap that can satisfy this request
                // (vkMemoryTypeIndex is sorted by preference)
                FastArray<uint32>::const_iterator itMemTypeIdx = vkMemoryTypeIndex.begin();
                FastArray<uint32>::const_iterator enMemTypeIdx = vkMemoryTypeIndex.end();

                while( itMemTypeIdx != enMemTypeIdx )
                {
                    if( textureMemTypeBits & ( 1u << *itMemTypeIdx ) )
                    {
                        const size_t heapIdx = memTypes[*itMemTypeIdx].heapIndex;
                        // Technically this is wrong. memHeaps[heapIdx].size is that max size of a
                        // single allocation, not the total consumption. We should compare against
                        // VkPhysicalDeviceMemoryBudgetPropertiesEXT::heapBudget
                        // if the extension is available.
                        //
                        // However it should be a safe bet that memHeaps[heapIdx].size is roughly
                        // the max size of all allocations combined too.
                        if( mUsedHeapMemory[heapIdx] + sizeBytes < memHeaps[heapIdx].size )
                        {
                            // Found one!
                            size_t defaultPoolSize =
                                std::min( (VkDeviceSize)mDefaultPoolSize[vboFlag],
                                          memHeaps[heapIdx].size - mUsedHeapMemory[heapIdx] );
                            poolSize = std::max( defaultPoolSize, sizeBytes );
                            break;
                        }
                    }
                    ++itMemTypeIdx;
                }

                if( itMemTypeIdx == enMemTypeIdx )
                {
                    // Failed to find a suitable heap that can allocate our request
                    if( ( textureMemTypeBits & mMemoryTypesInUse[vboFlag] ) != textureMemTypeBits )
                    {
                        // There were some memory types we ignored because they weren't in
                        // mBestVkMemoryTypeIndex
                        //
                        // However these other memory types may satisfy the allocation. Try once again
                        bIsTextureOnly = true;
                        const size_t numMemoryTypes = mDevice->mDeviceMemoryProperties.memoryTypeCount;
                        for( size_t i = 0u; i < numMemoryTypes; ++i )
                        {
                            if( !( ( 1u << i ) & mMemoryTypesInUse[vboFlag] ) &&
                                ( ( 1u << i ) & textureMemTypeBits ) )
                            {
                                // We didn't try this memory type. Let's check if we can use it
                                // TODO: See comment above about memHeaps[heapIdx].size
                                const size_t heapIdx = memTypes[i].heapIndex;
                                if( mUsedHeapMemory[heapIdx] + poolSize < memHeaps[heapIdx].size )
                                {
                                    // Found one!
                                    size_t defaultPoolSize =
                                        std::min( (VkDeviceSize)mDefaultPoolSize[vboFlag],
                                                  memHeaps[heapIdx].size - mUsedHeapMemory[heapIdx] );
                                    chosenMemoryTypeIdx = static_cast<uint32>( i );
                                    poolSize = std::max( defaultPoolSize, sizeBytes );
                                    break;
                                }
                            }
                        }
                    }

                    if( chosenMemoryTypeIdx == std::numeric_limits<uint32>::max() )
                    {
                        OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                                     "Out of Memory trying to allocate " +
                                         StringConverter::toString( sizeBytes ) + " bytes",
                                     "VulkanVaoManager::allocateVbo" );
                    }
                }
                else
                    chosenMemoryTypeIdx = *itMemTypeIdx;
            }

            const size_t usablePoolSize = poolSize;

            VkDeviceSize buffOffset = 0;
            newVbo.vkBuffer = 0;
            newVbo.vkMemoryTypeIdx = chosenMemoryTypeIdx;
            if( !bIsTextureOnly )
            {
                // We must create the buffer before the heap, so we know the memory size requirements
                VkBufferCreateInfo bufferCi;
                makeVkStruct( bufferCi, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO );
                bufferCi.size = poolSize;
                bufferCi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                 VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
                                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
                if( vboFlag == CPU_READ_WRITE )
                    bufferCi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                VkResult result = vkCreateBuffer( mDevice->mDevice, &bufferCi, 0, &newVbo.vkBuffer );
                checkVkResult( mDevice, result, "vkCreateBuffer" );

                VkMemoryRequirements buffMemRequirements;
                vkGetBufferMemoryRequirements( mDevice->mDevice, newVbo.vkBuffer, &buffMemRequirements );

                buffOffset = alignMemory( buffOffset, buffMemRequirements.alignment );
                poolSize = buffMemRequirements.size + buffOffset;
            }

            VkMemoryAllocateInfo memAllocInfo;
            makeVkStruct( memAllocInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO );
            memAllocInfo.allocationSize = poolSize;
            memAllocInfo.memoryTypeIndex = chosenMemoryTypeIdx;

            VkResult result = vkAllocateMemory( mDevice->mDevice, &memAllocInfo, NULL, &newVbo.vboName );
            checkVkResult( mDevice, result, "vkAllocateMemory" );

            mUsedHeapMemory[memTypes[chosenMemoryTypeIdx].heapIndex] += poolSize;

            if( !bIsTextureOnly )
            {
                result =
                    vkBindBufferMemory( mDevice->mDevice, newVbo.vkBuffer, newVbo.vboName, buffOffset );
                checkVkResult( mDevice, result, "vkBindBufferMemory" );
            }

            newVbo.sizeBytes = usablePoolSize;
            newVbo.freeBlocks.push_back( Block( 0, usablePoolSize ) );
            newVbo.dynamicBuffer = 0;

            if( vboFlag != CPU_INACCESSIBLE )
            {
                const bool isCoherent = isVboFlagCoherent( vboFlag );
                newVbo.dynamicBuffer = new VulkanDynamicBuffer(
                    newVbo.vboName, newVbo.sizeBytes, isCoherent, vboFlag == CPU_READ_WRITE, mDevice );
            }

            if( !mUnallocatedVbos[vboFlag].empty() )
            {
                // There is an unallocated pool wasting space. Place the new pool there
                bestVboIdx = mUnallocatedVbos[vboFlag].back();
                mUnallocatedVbos[vboFlag].pop_back();
                vboVec[bestVboIdx] = newVbo;
            }
            else
            {
                vboVec.push_back( newVbo );
            }
        }
        else
        {
            Vbo &bestVbo = vboVec[bestVboIdx];
            if( bestVbo.isEmpty() )
            {
                OGRE_ASSERT_HIGH( bestVbo.isAllocated() );

                // The block will no longer be empty, hence no unschedule from destruction
                VboIndex vboIndex;
                vboIndex.vboFlag = vboFlag;
                vboIndex.vboIdx = static_cast<uint32>( bestVboIdx );
                OGRE_ASSERT_HIGH( mEmptyVboPools.find( vboIndex ) != mEmptyVboPools.end() &&
                                  "If the Vbo pool was empty, it should be in mEmptyVboPools" );
                mEmptyVboPools.erase( vboIndex );
            }
        }

        // clang-format off
        Vbo &bestVbo        = vboVec[bestVboIdx];
        Block &bestBlock    = bestVbo.freeBlocks[static_cast<size_t>(bestBlockIdx)];
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
                                          BufferType bufferType, bool readCapable,
                                          bool skipDynBufferMultiplier )
    {
        VboFlag vboFlag = bufferTypeToVboFlag( bufferType, readCapable );

        bool bImmediately = false;
        if( bufferType >= BT_DYNAMIC_DEFAULT && !readCapable && !skipDynBufferMultiplier )
        {
            sizeBytes *= mDynamicBufferMultiplier;
            bImmediately = true;  // Dynamic buffers are already delayed
        }

        deallocateVbo( vboIdx, bufferOffset, sizeBytes, vboFlag, bImmediately );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::deallocateVbo( size_t vboIdx, size_t bufferOffset, size_t sizeBytes,
                                          const VboFlag vboFlag, bool bImmediately )
    {
        if( !bImmediately )
        {
            mDelayedBlocksSize += sizeBytes;

            bool bBlocksFlushed = false;
            if( mDelayedBlocksSize >= mDelayedBlocksFlushThreshold )
                bBlocksFlushed = flushAllGpuDelayedBlocks( true );

            if( !bBlocksFlushed )
            {
                mDelayedBlocks.push_back(
                    DirtyBlock( mFrameCount, vboFlag, vboIdx, bufferOffset, sizeBytes ) );
                return;
            }
        }

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
        mergeContiguousBlocks( vbo.freeBlocks.end() - 1u, vbo.freeBlocks );

        if( vbo.isEmpty() )
        {
            // This pool is empty. Schedule for removal
            // We may reuse their memory if more memory is requested before they're actually removed.
            OGRE_ASSERT_LOW( vbo.strideChangers.empty() );
            vbo.emptyFrame = mFrameCount;
            VboIndex vboIndex;
            vboIndex.vboFlag = vboFlag;
            vboIndex.vboIdx = static_cast<uint32>( vboIdx );
            mEmptyVboPools.insert( vboIndex );
        }
    }
    //-----------------------------------------------------------------------------------
    VkDeviceMemory VulkanVaoManager::allocateTexture( const VkMemoryRequirements &memReq,
                                                      size_t &outVboIdx, size_t &outBufferOffset )
    {
        const VboFlag vboFlag = mDevice->mDeviceProperties.limits.bufferImageGranularity == 1u
                                    ? CPU_INACCESSIBLE
                                    : TEXTURES_OPTIMAL;
        allocateVbo( memReq.size, memReq.alignment, vboFlag, memReq.memoryTypeBits, outVboIdx,
                     outBufferOffset );

        return mVbos[vboFlag][outVboIdx].vboName;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::deallocateTexture( size_t vboIdx, size_t bufferOffset, size_t sizeBytes )
    {
        const VboFlag vboFlag = mDevice->mDeviceProperties.limits.bufferImageGranularity == 1u
                                    ? CPU_INACCESSIBLE
                                    : TEXTURES_OPTIMAL;
        deallocateVbo( vboIdx, bufferOffset, sizeBytes, vboFlag, false );
    }
    //-----------------------------------------------------------------------------------
    VulkanRawBuffer VulkanVaoManager::allocateRawBuffer( VboFlag vboFlag, size_t sizeBytes,
                                                         size_t alignment )
    {
        // Override what user prefers (or change if unavailable).
        if( vboFlag == CPU_WRITE_PERSISTENT && mPreferCoherentMemory )
            vboFlag = CPU_WRITE_PERSISTENT_COHERENT;
        else if( vboFlag == CPU_WRITE_PERSISTENT_COHERENT && !mSupportsCoherentMemory )
            vboFlag = CPU_WRITE_PERSISTENT;

        VulkanRawBuffer retVal;
        allocateVbo( sizeBytes, alignment, vboFlag, mMemoryTypesInUse[vboFlag], retVal.mVboPoolIdx,
                     retVal.mInternalBufferStart );
        Vbo &vbo = mVbos[vboFlag][retVal.mVboPoolIdx];
        retVal.mVboFlag = vboFlag;
        retVal.mVboName = vbo.vkBuffer;
        retVal.mDynamicBuffer = vbo.dynamicBuffer;
        retVal.mUnmapTicket = std::numeric_limits<size_t>::max();
        retVal.mSize = sizeBytes;
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::deallocateRawBuffer( VulkanRawBuffer &rawBuffer, bool bImmediately )
    {
        OGRE_ASSERT_LOW( rawBuffer.mUnmapTicket == std::numeric_limits<size_t>::max() &&
                         "VulkanRawBuffer not unmapped (or dangling)" );
        deallocateVbo( rawBuffer.mVboPoolIdx, rawBuffer.mInternalBufferStart, rawBuffer.mSize,
                       static_cast<VboFlag>( rawBuffer.mVboFlag ), bImmediately );
        memset( &rawBuffer, 0, sizeof( rawBuffer ) );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::addDelayedFunc( VulkanDelayedFuncBase *cmd )
    {
        cmd->frameIdx = mFrameCount;
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
                ptrdiff_t idx = itor - blocks.begin();

                // When blockToMerge is the last one, its index won't be the same
                // after removing the other iterator, they will swap.
                if( static_cast<size_t>( idx ) == blocks.size() - 1u )
                    idx = blockToMerge - blocks.begin();

                efficientVectorRemove( blocks, blockToMerge );

                blockToMerge = blocks.begin() + idx;
                itor = blocks.begin();
                endt = blocks.end();
            }
            else if( blockToMerge->offset + blockToMerge->size == itor->offset )
            {
                blockToMerge->size += itor->size;
                ptrdiff_t idx = blockToMerge - blocks.begin();

                // When blockToMerge is the last one, its index won't be the same
                // after removing the other iterator, they will swap.
                if( static_cast<size_t>( idx ) == blocks.size() - 1u )
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

        size_t sizeBytes = numElements * bytesPerElement;
        uint32 alignment = bytesPerElement;
        uint32 numElementsPadding = 0u;
#ifdef OGRE_VK_WORKAROUND_PVR_ALIGNMENT
        if( Workarounds::mPowerVRAlignment )
        {
            alignment = uint32( Math::lcm( alignment, Workarounds::mPowerVRAlignment ) );
            sizeBytes = alignToNextMultiple<size_t>( sizeBytes, alignment );
            numElementsPadding = uint32( sizeBytes - numElements * bytesPerElement ) / bytesPerElement;
        }
#endif

        allocateVbo( sizeBytes, alignment, bufferType, false, false, vboIdx, bufferOffset );

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType, false );
        Vbo &vbo = mVbos[vboFlag][vboIdx];
        VulkanBufferInterface *bufferInterface =
            new VulkanBufferInterface( vboIdx, vbo.vkBuffer, vbo.dynamicBuffer );

        VertexBufferPacked *retVal = OGRE_NEW VertexBufferPacked(
            bufferOffset, numElements, bytesPerElement, numElementsPadding, bufferType, initialData,
            keepAsShadow, this, bufferInterface, vElements );

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
                       vertexBuffer->_getInternalTotalSizeBytes(), vertexBuffer->getBufferType(), false,
                       false );
    }
    //-----------------------------------------------------------------------------------
#ifdef _OGRE_MULTISOURCE_VBO
    MultiSourceVertexBufferPool *VulkanVaoManager::createMultiSourceVertexBufferPoolImpl(
        const VertexElement2VecVec &vertexElementsBySource, size_t maxNumVertices,
        size_t totalBytesPerVertex, BufferType bufferType )
    {
        return OGRE_NEW VulkanMultiSourceVertexBufferPool( 0, vertexElementsBySource, maxNumVertices,
                                                           bufferType, 0, this );
    }
#endif
    //-----------------------------------------------------------------------------------
    IndexBufferPacked *VulkanVaoManager::createIndexBufferImpl( size_t numElements,
                                                                uint32 bytesPerElement,
                                                                BufferType bufferType, void *initialData,
                                                                bool keepAsShadow )
    {
        size_t vboIdx;
        size_t bufferOffset;

        size_t sizeBytes = numElements * bytesPerElement;
        uint32 alignment = bytesPerElement;
        uint32 numElementsPadding = 0u;
#ifdef OGRE_VK_WORKAROUND_PVR_ALIGNMENT
        if( Workarounds::mPowerVRAlignment )
        {
            alignment = uint32( Math::lcm( alignment, Workarounds::mPowerVRAlignment ) );
            sizeBytes = alignToNextMultiple<size_t>( sizeBytes, alignment );
            numElementsPadding = uint32( sizeBytes - numElements * bytesPerElement ) / bytesPerElement;
        }
#endif

        allocateVbo( sizeBytes, alignment, bufferType, false, false, vboIdx, bufferOffset );

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType, false );
        Vbo &vbo = mVbos[vboFlag][vboIdx];
        VulkanBufferInterface *bufferInterface =
            new VulkanBufferInterface( vboIdx, vbo.vkBuffer, vbo.dynamicBuffer );

        IndexBufferPacked *retVal =
            OGRE_NEW IndexBufferPacked( bufferOffset, numElements, bytesPerElement, numElementsPadding,
                                        bufferType, initialData, keepAsShadow, this, bufferInterface );

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
                       indexBuffer->_getInternalTotalSizeBytes(), indexBuffer->getBufferType(), false,
                       false );
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

        allocateVbo( sizeBytes, alignment, bufferType, false, false, vboIdx, bufferOffset );

        Vbo &vbo = mVbos[vboFlag][vboIdx];
        VulkanBufferInterface *bufferInterface =
            new VulkanBufferInterface( vboIdx, vbo.vkBuffer, vbo.dynamicBuffer );

        VulkanConstBufferPacked *retVal = OGRE_NEW VulkanConstBufferPacked(
            bufferOffset, requestedSize, 1u, (uint32)( sizeBytes - requestedSize ), bufferType,
            initialData, keepAsShadow, mVkRenderSystem, this, bufferInterface );

        if( initialData )
            bufferInterface->_firstUpload( initialData, 0, requestedSize );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyConstBufferImpl( ConstBufferPacked *constBuffer )
    {
        VulkanBufferInterface *bufferInterface =
            static_cast<VulkanBufferInterface *>( constBuffer->getBufferInterface() );

        deallocateVbo( bufferInterface->getVboPoolIndex(),
                       constBuffer->_getInternalBufferStart() * constBuffer->getBytesPerElement(),
                       constBuffer->_getInternalTotalSizeBytes(), constBuffer->getBufferType(), false,
                       false );
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

        allocateVbo( sizeBytes, alignment, bufferType, false, false, vboIdx, bufferOffset );

        Vbo &vbo = mVbos[vboFlag][vboIdx];
        VulkanBufferInterface *bufferInterface =
            new VulkanBufferInterface( vboIdx, vbo.vkBuffer, vbo.dynamicBuffer );

        VulkanTexBufferPacked *retVal = OGRE_NEW VulkanTexBufferPacked(
            bufferOffset, requestedSize, 1u, (uint32)( sizeBytes - requestedSize ), bufferType,
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
                       texBuffer->_getInternalTotalSizeBytes(), texBuffer->getBufferType(), false,
                       false );
    }
    //-----------------------------------------------------------------------------------
    ReadOnlyBufferPacked *VulkanVaoManager::createReadOnlyBufferImpl( PixelFormatGpu pixelFormat,
                                                                      size_t sizeBytes,
                                                                      BufferType bufferType,
                                                                      void *initialData,
                                                                      bool keepAsShadow )
    {
        size_t vboIdx;
        size_t bufferOffset;

#ifdef OGRE_VK_WORKAROUND_ADRENO_6xx_READONLY_IS_TBUFFER
        const size_t alignment =
            Workarounds::mAdreno6xxReadOnlyIsTBuffer
                ? mTexBufferAlignment
                : Math::lcm( mUavBufferAlignment, PixelFormatGpuUtils::getBytesPerPixel( pixelFormat ) );
#else
        const size_t alignment =
            Math::lcm( mUavBufferAlignment, PixelFormatGpuUtils::getBytesPerPixel( pixelFormat ) );
#endif
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

        allocateVbo( sizeBytes, alignment, bufferType, false, false, vboIdx, bufferOffset );

        Vbo &vbo = mVbos[vboFlag][vboIdx];
        VulkanBufferInterface *bufferInterface =
            new VulkanBufferInterface( vboIdx, vbo.vkBuffer, vbo.dynamicBuffer );

#ifdef OGRE_VK_WORKAROUND_ADRENO_6xx_READONLY_IS_TBUFFER
        ReadOnlyBufferPacked *retVal;
        if( Workarounds::mAdreno6xxReadOnlyIsTBuffer )
        {
            retVal = OGRE_NEW VulkanReadOnlyTBufferWorkaround(
                bufferOffset, requestedSize, 1u, (uint32)( sizeBytes - requestedSize ), bufferType,
                initialData, keepAsShadow, mVkRenderSystem, this, bufferInterface, pixelFormat );
        }
        else
        {
            retVal = OGRE_NEW VulkanReadOnlyBufferPacked(
                bufferOffset, requestedSize, 1u, (uint32)( sizeBytes - requestedSize ), bufferType,
                initialData, keepAsShadow, mVkRenderSystem, this, bufferInterface, pixelFormat );
        }
#else
        VulkanReadOnlyBufferPacked *retVal = OGRE_NEW VulkanReadOnlyBufferPacked(
            bufferOffset, requestedSize, 1u, (uint32)( sizeBytes - requestedSize ), bufferType,
            initialData, keepAsShadow, mVkRenderSystem, this, bufferInterface, pixelFormat );
#endif

        if( initialData )
            bufferInterface->_firstUpload( initialData, 0, requestedSize );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyReadOnlyBufferImpl( ReadOnlyBufferPacked *readOnlyBuffer )
    {
        VulkanBufferInterface *bufferInterface =
            static_cast<VulkanBufferInterface *>( readOnlyBuffer->getBufferInterface() );

        deallocateVbo( bufferInterface->getVboPoolIndex(),
                       readOnlyBuffer->_getInternalBufferStart() * readOnlyBuffer->getBytesPerElement(),
                       readOnlyBuffer->_getInternalTotalSizeBytes(), readOnlyBuffer->getBufferType(),
                       false, false );
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

        allocateVbo( numElements * bytesPerElement, alignment, bufferType, false, false, vboIdx,
                     bufferOffset );

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
                       uavBuffer->_getInternalTotalSizeBytes(), uavBuffer->getBufferType(), false,
                       false );
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

            allocateVbo( sizeBytes, alignment, bufferType, false, false, vboIdx, bufferOffset );

            Vbo &vbo = mVbos[vboFlag][vboIdx];
            bufferInterface = new VulkanBufferInterface( vboIdx, vbo.vkBuffer, vbo.dynamicBuffer );
        }

        IndirectBufferPacked *retVal = OGRE_NEW IndirectBufferPacked(
            bufferOffset, requestedSize, 1, (uint32)( sizeBytes - requestedSize ), bufferType,
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
                indirectBuffer->_getInternalTotalSizeBytes(), indirectBuffer->getBufferType(), false,
                false );
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
            itor = mVaos.begin() + static_cast<ptrdiff_t>( mVaos.size() - 1u );
        }

        ++itor->refCount;

        return itor;
    }
    //-----------------------------------------------------------------------------------
    uint32 VulkanVaoManager::createVao() { return mVaoNames++; }
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
        allocateVbo( sizeBytes, 4u, BT_DYNAMIC_DEFAULT, !forUpload, true, vboIdx, bufferOffset );
        const VboFlag vboFlag = bufferTypeToVboFlag( BT_DYNAMIC_DEFAULT, !forUpload );

        Vbo &vbo = mVbos[vboFlag][vboIdx];

        VulkanStagingBuffer *stagingBuffer = OGRE_NEW VulkanStagingBuffer(
            vboIdx, bufferOffset, sizeBytes, this, forUpload, vbo.vkBuffer, vbo.dynamicBuffer );
        mRefedStagingBuffers[forUpload].push_back( stagingBuffer );

        if( mNextStagingBufferTimestampCheckpoint == std::numeric_limits<uint64>::max() )
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
        allocateVbo( sizeBytes, alignment, BT_DYNAMIC_DEFAULT, false, true, vboIdx, bufferOffset );
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
        // Staging textures are already delayed so they can be deallocated immediately.
        // BT_DYNAMIC_DEFAULT already signals this information so no extra info is required.
        deallocateVbo( stagingTexture->getVboPoolIndex(), stagingTexture->_getInternalBufferStart(),
                       stagingTexture->_getInternalTotalSizeBytes(), BT_DYNAMIC_DEFAULT, false, true );
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
    void VulkanVaoManager::_update()
    {
        if( mFenceFlushed == FenceUnflushed )
        {
            // We could only reach here if _update() was called
            // twice in a row without completing a full frame.
            //
            // Without this, mFrameCount won't actually advance, and if we increment
            // mFrameCount ourselves, waitForTailFrameToFinish would become unsafe.
            //
            // This must be done at the beginning, because normally the following sequence would happen:
            //  1. VulkanVaoManager::_update - mFrameCount = 0
            //  2. _notifyNewCommandBuffer   - mFrameCount = 1
            //  3. VulkanVaoManager::_update - mFrameCount = 1
            //  4. _notifyNewCommandBuffer   - mFrameCount = 2
            //  5. And so on...
            //
            // However we reached here because we performed the following:
            //  1. VulkanVaoManager::_update - mFrameCount = 0
            //  2. VulkanVaoManager::_update - mFrameCount = ???
            //
            // Thus we MUST insert a _notifyNewCommandBuffer in-between the first two:
            //  1. VulkanVaoManager::_update - mFrameCount = 0
            //  2a._notifyNewCommandBuffer   - mFrameCount = 1
            //  2b.VulkanVaoManager::_update - mFrameCount = 1
            //
            // Previously this block of code was at the end of VulkanVaoManager::_update
            // and this was causing troubles. See https://github.com/OGRECave/ogre-next/issues/433
            mDevice->commitAndNextCommandBuffer( SubmissionType::NewFrameIdx );
        }

        mFenceFlushed = FenceUnflushed;

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
            mNextStagingBufferTimestampCheckpoint = std::numeric_limits<uint64>::max();

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

        if( !mDelayedFuncs[mDynamicBufferCurrentFrame].empty() )
        {
            waitForTailFrameToFinish();

            VulkanDelayedFuncBaseArray::iterator itor =
                mDelayedFuncs[mDynamicBufferCurrentFrame].begin();
            VulkanDelayedFuncBaseArray::iterator endt = mDelayedFuncs[mDynamicBufferCurrentFrame].end();

            while( itor != endt && ( *itor )->frameIdx != mFrameCount )
            {
                ( *itor )->execute();
                delete *itor;
                ++itor;
            }

            mDelayedFuncs[mDynamicBufferCurrentFrame].erase(
                mDelayedFuncs[mDynamicBufferCurrentFrame].begin(), itor );
        }

        if( !mDelayedBlocks.empty() && ( mFrameCount - mDelayedBlocks.front().frameIdx ) >= 1u )
        {
            waitForTailFrameToFinish();
            flushGpuDelayedBlocks();
        }

        deallocateEmptyVbos( false );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::_notifyNewCommandBuffer()
    {
        if( mFenceFlushed == FenceFlushed )
        {
            if( mFenceFlushedWarningCount < 5u )
            {
                LogManager::getSingleton().logMessage(
                    "WARNING: Calling RenderSystem::_endFrameOnce() twice in a row without calling "
                    "RenderSystem::_update. This can lead to strange results.",
                    LML_CRITICAL );
                ++mFenceFlushedWarningCount;
            }

            _update();
        }
        mFenceFlushed = FenceFlushed;
        mDynamicBufferCurrentFrame = ( mDynamicBufferCurrentFrame + 1 ) % mDynamicBufferMultiplier;
        ++mFrameCount;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::getAvailableSemaphores( VkSemaphoreArray &semaphoreArray,
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
                checkVkResult( mDevice, result, "vkCreateSemaphore" );
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
    VkSemaphore VulkanVaoManager::getAvailableSemaphore()
    {
        VkSemaphore retVal;
        if( mAvailableSemaphores.empty() )
        {
            VkSemaphoreCreateInfo semaphoreCreateInfo;
            makeVkStruct( semaphoreCreateInfo, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO );

            const VkResult result =
                vkCreateSemaphore( mDevice->mDevice, &semaphoreCreateInfo, 0, &retVal );
            checkVkResult( mDevice, result, "vkCreateSemaphore" );
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
    void VulkanVaoManager::notifyPresentationWaitSemaphoreSubmitted( VkSemaphore semaphore,
                                                                     uint32 swapchainIdx )
    {
        mUsedPresentSemaphores.push_back( UsedSemaphore( semaphore, swapchainIdx ) );
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::notifySwapchainIndexAcquired( const uint32 swapchainIdx )
    {
        FastArray<UsedSemaphore>::iterator itor = mUsedPresentSemaphores.begin();
        FastArray<UsedSemaphore>::iterator endt = mUsedPresentSemaphores.end();

        while( itor != endt )
        {
            if( swapchainIdx == itor->frame )
            {
                mAvailableSemaphores.push_back( itor->semaphore );
                itor = efficientVectorRemove( mUsedPresentSemaphores, itor );
                endt = mUsedPresentSemaphores.end();
            }
            else
            {
                ++itor;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::notifySemaphoreUnused( VkSemaphore semaphore )
    {
        vkDestroySemaphore( mDevice->mDevice, semaphore, 0 );
    }
    //-----------------------------------------------------------------------------------
    uint8 VulkanVaoManager::waitForTailFrameToFinish()
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
        checkVkResult( queue->mOwnerDevice, result, "VulkanStagingBuffer::wait" );
        queue->releaseFence( fenceName );
        return 0;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::_notifyDeviceStalled()
    {
        mFenceFlushed = GpuStalled;

        flushAllGpuDelayedBlocks( false );

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

        deallocateEmptyVbos( true );

        for( const UsedSemaphore &sem : mUsedSemaphores )
            mAvailableSemaphores.push_back( sem.semaphore );
        for( const UsedSemaphore &sem : mUsedPresentSemaphores )
            mAvailableSemaphores.push_back( sem.semaphore );
        mUsedSemaphores.clear();
        mUsedPresentSemaphores.clear();

        // If this isn't empty, then some deallocation got delayed after our flushAllGpuDelayedBlocks
        // call even though it shouldn't!
        OGRE_ASSERT_LOW( mDelayedBlocks.empty() );

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

        VboFlag vboFlag = MAX_VBO_FLAG;

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
        case BT_DEFAULT_SHARED:
        case BT_DYNAMIC_PERSISTENT_COHERENT:
            vboFlag = CPU_WRITE_PERSISTENT_COHERENT;
            break;
        }

        // Override what user prefers (or change if unavailable).
        if( vboFlag == CPU_WRITE_PERSISTENT && mPreferCoherentMemory )
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
    void *VulkanRawBuffer::map()
    {
        OGRE_ASSERT_LOW( mDynamicBuffer && "CPU_INACCESSIBLE buffers cannot be mapped!" );
        OGRE_ASSERT_LOW( mUnmapTicket == std::numeric_limits<size_t>::max() &&
                         "Mapping VulkanRawBuffer twice!" );
        return mDynamicBuffer->map( mInternalBufferStart, mSize, mUnmapTicket );
    }
    //-----------------------------------------------------------------------------------
    void VulkanRawBuffer::unmap()
    {
        mDynamicBuffer->unmap( mUnmapTicket );
        mUnmapTicket = std::numeric_limits<size_t>::max();
    }
}  // namespace Ogre
