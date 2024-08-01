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

#include "Vao/OgreMetalVaoManager.h"
#include "Vao/OgreMetalBufferInterface.h"
#include "Vao/OgreMetalConstBufferPacked.h"
#include "Vao/OgreMetalStagingBuffer.h"
#include "Vao/OgreMetalTexBufferPacked.h"
#include "Vao/OgreMetalUavBufferPacked.h"
#include "Vao/OgreVertexArrayObject.h"
#ifdef _OGRE_MULTISOURCE_VBO
#    include "Vao/OgreMetalMultiSourceVertexBufferPool.h"
#endif
#include "Vao/OgreMetalAsyncTicket.h"
#include "Vao/OgreMetalDynamicBuffer.h"

#include "Vao/OgreIndirectBufferPacked.h"

#include "OgreMetalDevice.h"
#include "OgreMetalRenderSystem.h"

#include "OgreHlmsManager.h"
#include "OgreRenderQueue.h"
#include "OgreRoot.h"

#include "OgreLwString.h"
#include "OgreStringConverter.h"
#include "OgreTimer.h"

#import <Metal/MTLComputeCommandEncoder.h>
#import <Metal/MTLComputePipeline.h>
#import <Metal/MTLDevice.h>
#import <Metal/MTLRenderCommandEncoder.h>

namespace Ogre
{
    const uint32 MetalVaoManager::VERTEX_ATTRIBUTE_INDEX[VES_COUNT] = {
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

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
    // On iOS 16 byte alignment makes it good to go for everything; but we need to satisfy
    // both original alignment for baseVertex reasons. So find LCM between both.
    const uint32 c_minimumAlignment = 16u;
#else
    // On OSX we have several alignments (just like GL & D3D11), but it can never be lower than 4.
    const uint32 c_minimumAlignment = 4u;
#endif
    const uint32 c_indexBufferAlignment = 4u;

    static const char *c_vboTypes[] = {
        "CPU_INACCESSIBLE",
        "CPU_ACCESSIBLE_SHARED",
        "CPU_ACCESSIBLE_DEFAULT",
        "CPU_ACCESSIBLE_PERSISTENT",
        "CPU_ACCESSIBLE_PERSISTENT_COHERENT",
    };

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
    static const char c_gpuMemcpyComputeShader[] =
        "#include <metal_stdlib>\n"
        "using namespace metal;\n"
        "\n"
        "struct Params\n"
        "{\n"
        "	uint32_t dstOffset;\n"
        "	uint32_t srcOffset;\n"
        "	uint32_t sizeBytes;\n"
        "};\n"
        "\n"
        "kernel void ogre_gpu_memcpy\n"
        "(\n"
        "	device uint8_t* dst			[[ buffer(0) ]],\n"
        "	device uint8_t* src			[[ buffer(1) ]],\n"
        "	constant Params &p			[[ buffer(2) ]],\n"
        "	uint3 gl_GlobalInvocationID	[[thread_position_in_grid]]\n"
        ")\n"
        "{\n"
        "//	for( uint32_t i=0u; i<p.sizeBytes; ++i )\n"
        "//		dst[i + p.dstOffset] = src[i + p.srcOffset];\n"
        "	uint32_t srcOffsetStart	= p.srcOffset + gl_GlobalInvocationID.x * 64u;\n"
        "	uint32_t dstOffsetStart	= p.dstOffset + gl_GlobalInvocationID.x * 64u;\n"
        "	uint32_t numBytesToCopy	= min( 64u, p.sizeBytes - gl_GlobalInvocationID.x * 64u );\n"
        "	for( uint32_t i=0u; i<numBytesToCopy; ++i )\n"
        "		dst[i + dstOffsetStart] = src[i + srcOffsetStart];\n"
        "}";
#endif

    MetalVaoManager::MetalVaoManager( MetalDevice *device, const NameValuePairList *params ) :
        VaoManager( params ),
        mVaoNames( 1 ),
        mSemaphoreFlushed( true ),
        mDevice( device ),
        mDrawId( 0 )
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        // On iOS alignment must match "the maximum accessed object" type. e.g.
        // if it's all float, then alignment = 4. if it's a float2, then alignment = 8.
        // The max. object is float4, so alignment = 16
#    if TARGET_OS_SIMULATOR == 0 || OGRE_CPU == OGRE_CPU_ARM 
        mConstBufferAlignment = 16;
        mTexBufferAlignment = 16;
#    else
        mConstBufferAlignment = 256;
        mTexBufferAlignment = 256;
#    endif

        // Keep pools of 16MB for static buffers
        mDefaultPoolSize[CPU_INACCESSIBLE] = 16 * 1024 * 1024;

        mDefaultPoolSize[CPU_ACCESSIBLE_SHARED] = 16 * 1024 * 1024;

        // Keep pools of 4MB each for dynamic buffers
        for( size_t i = CPU_ACCESSIBLE_DEFAULT; i <= CPU_ACCESSIBLE_PERSISTENT_COHERENT; ++i )
            mDefaultPoolSize[i] = 4 * 1024 * 1024;

        // TODO: iOS v3 family does support indirect buffers.
        mSupportsIndirectBuffers = false;
#else
        // OS X restrictions.
        mConstBufferAlignment = 256;
        mTexBufferAlignment = 256;

        // Keep pools of 32MB for static buffers
        mDefaultPoolSize[CPU_INACCESSIBLE] = 32 * 1024 * 1024;

        mDefaultPoolSize[CPU_ACCESSIBLE_SHARED] = 32 * 1024 * 1024;

        // Keep pools of 4MB each for dynamic buffers
        for( size_t i = CPU_ACCESSIBLE_DEFAULT; i <= CPU_ACCESSIBLE_PERSISTENT_COHERENT; ++i )
            mDefaultPoolSize[i] = 4 * 1024 * 1024;

        mSupportsIndirectBuffers = false;  // supported, but there are no performance benefits
#endif
        if( params )
        {
            for( size_t i = 0; i < MAX_VBO_FLAG; ++i )
            {
                NameValuePairList::const_iterator itor =
                    params->find( String( "VaoManager::" ) + c_vboTypes[i] );
                if( itor != params->end() )
                {
                    mDefaultPoolSize[i] = StringConverter::parseUnsignedInt(
                        itor->second, (unsigned int)mDefaultPoolSize[i] );
                }
            }
        }

        mAlreadyWaitedForSemaphore.resize( mDynamicBufferMultiplier, true );
        mFrameSyncVec.resize( mDynamicBufferMultiplier, 0 );
        for( size_t i = 0; i < mDynamicBufferMultiplier; ++i )
            mFrameSyncVec[i] = dispatch_semaphore_create( 0 );

        mConstBufferMaxSize = 64 * 1024;        // 64kb
        mTexBufferMaxSize = 128 * 1024 * 1024;  // 128MB
        mReadOnlyIsTexBuffer = true;
        mReadOnlyBufferMaxSize = mTexBufferMaxSize;

        mSupportsPersistentMapping = true;

        const uint32 maxNumInstances = 4096u * 2u;
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS && (TARGET_OS_SIMULATOR == 0 || OGRE_CPU == OGRE_CPU_ARM)
        uint32 *drawIdPtr = static_cast<uint32 *>(
            OGRE_MALLOC_SIMD( maxNumInstances * sizeof( uint32 ), MEMCATEGORY_GEOMETRY ) );
        for( uint32 i = 0; i < maxNumInstances; ++i )
            drawIdPtr[i] = i;
        mDrawId =
            createConstBuffer( maxNumInstances * sizeof( uint32 ), BT_IMMUTABLE, drawIdPtr, false );
        OGRE_FREE_SIMD( drawIdPtr, MEMCATEGORY_GEOMETRY );
        drawIdPtr = 0;
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS && TARGET_OS_SIMULATOR == 1
        uint32 *drawIdPtr =
            static_cast<uint32 *>( OGRE_MALLOC_SIMD( maxNumInstances * 256u, MEMCATEGORY_GEOMETRY ) );
        for( uint32 i = 0; i < maxNumInstances; ++i )
        {
            drawIdPtr[64u * i] = i;
            for( uint32 j = 1; j < 64u; ++j )
                drawIdPtr[64u * i + j] = 0xD256D256;
        }
        mDrawId =
            createConstBuffer( maxNumInstances * sizeof( uint32 ), BT_IMMUTABLE, drawIdPtr, false );
        OGRE_FREE_SIMD( drawIdPtr, MEMCATEGORY_GEOMETRY );
        drawIdPtr = 0;
#else
        VertexElement2Vec vertexElements;
        vertexElements.push_back( VertexElement2( VET_UINT1, VES_COUNT ) );
        uint32 *drawIdPtr = static_cast<uint32 *>(
            OGRE_MALLOC_SIMD( maxNumInstances * sizeof( uint32 ), MEMCATEGORY_GEOMETRY ) );
        for( uint32 i = 0; i < maxNumInstances; ++i )
            drawIdPtr[i] = i;
        mDrawId = createVertexBuffer( vertexElements, maxNumInstances, BT_IMMUTABLE, drawIdPtr, true );

        createUnalignedCopyShader();
#endif
    }
    //-----------------------------------------------------------------------------------
    MetalVaoManager::~MetalVaoManager()
    {
        destroyAllVertexArrayObjects();
        deleteAllBuffers();

        for( size_t i = 0; i < MAX_VBO_FLAG; ++i )
        {
            VboVec::iterator itor = mVbos[i].begin();
            VboVec::iterator endt = mVbos[i].end();

            while( itor != endt )
            {
                itor->vboName = 0;
                delete itor->dynamicBuffer;
                itor->dynamicBuffer = 0;
                ++itor;
            }
        }
    }
    //-----------------------------------------------------------------------------------
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
    void MetalVaoManager::createUnalignedCopyShader()
    {
        NSError *error;
        id<MTLLibrary> library = [mDevice->mDevice newLibraryWithSource:@( c_gpuMemcpyComputeShader )
                                                                options:nil
                                                                  error:&error];

        if( !library )
        {
            String errorDesc;
            if( error )
                errorDesc = error.localizedDescription.UTF8String;

            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Metal SL Compiler Error while compiling internal c_gpuMemcpyComputeShader:\n" +
                             errorDesc,
                         "MetalVaoManager::MetalVaoManager" );
        }
        else
        {
            if( error )
            {
                String errorDesc;
                if( error )
                    errorDesc = error.localizedDescription.UTF8String;
                LogManager::getSingleton().logMessage(
                    "Metal SL Compiler Warnings in c_gpuMemcpyComputeShader:\n" + errorDesc );
            }
        }
        library.label = @"c_gpuMemcpyComputeShader";
        id<MTLFunction> unalignedCopyFunc = [library newFunctionWithName:@"ogre_gpu_memcpy"];
        if( !unalignedCopyFunc )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Error retrieving entry point from internal 'ogre_gpu_memcpy'",
                         "MetalVaoManager::MetalVaoManager" );
        }

        MTLComputePipelineDescriptor *psd = [[MTLComputePipelineDescriptor alloc] init];
        psd.computeFunction = unalignedCopyFunc;
        mUnalignedCopyPso = [mDevice->mDevice newComputePipelineStateWithDescriptor:psd
                                                                            options:MTLPipelineOptionNone
                                                                         reflection:nil
                                                                              error:&error];
        if( !mUnalignedCopyPso || error )
        {
            String errorDesc;
            if( error )
                errorDesc = error.localizedDescription.UTF8String;

            OGRE_EXCEPT(
                Exception::ERR_RENDERINGAPI_ERROR,
                "Failed to create pipeline state for compute for mUnalignedCopyPso, error " + errorDesc,
                "MetalVaoManager::MetalVaoManager" );
        }
    }
#endif
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::getMemoryStats( const Block &block, size_t vboIdx, size_t poolIdx,
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

        MemoryStatsEntry entry( (uint32)vboIdx, (uint32)poolIdx, block.offset, block.size, poolCapacity,
                                false );
        outStats.push_back( entry );
    }
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::getMemoryStats( MemoryStatsEntryVec &outStats, size_t &outCapacityBytes,
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

                    // usedBlock.size could be 0 if:
                    //  1. All of memory is free
                    //  2. There's two contiguous free blocks, which should not happen
                    //     due to mergeContiguousBlocks
                    if( usedBlock.size > 0u )
                        getMemoryStats( usedBlock, vboIdx, poolIdx, vbo.sizeBytes, text, statsVec, log );

                    usedBlock.offset += usedBlock.size;
                    usedBlock.size = 0;
                    efficientVectorRemove( freeBlocks, nextBlock );
                }

                if( usedBlock.size > 0u || ( usedBlock.offset == 0 && usedBlock.size == 0 ) )
                {
                    if( vbo.freeBlocks.empty() )
                    {
                        // Deal with edge case when we're 100% full
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
        outIncludesTextures = false;
        statsVec.swap( outStats );
    }
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::switchVboPoolIndexImpl( unsigned internalVboBufferType, size_t oldPoolIdx,
                                                  size_t newPoolIdx, BufferPacked *buffer )
    {
        if( mSupportsIndirectBuffers || buffer->getBufferPackedType() != BP_TYPE_INDIRECT )
        {
            VboFlag vboFlag = bufferTypeToVboFlag( buffer->getBufferType() );
            if( vboFlag == internalVboBufferType )
            {
                MetalBufferInterface *bufferInterface =
                    static_cast<MetalBufferInterface *>( buffer->getBufferInterface() );
                if( bufferInterface->getVboPoolIndex() == oldPoolIdx )
                    bufferInterface->_setVboPoolIndex( newPoolIdx );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::cleanupEmptyPools()
    {
        for( unsigned vboIdx = 0; vboIdx < MAX_VBO_FLAG; ++vboIdx )
        {
            VboVec::iterator itor = mVbos[vboIdx].begin();
            VboVec::iterator endt = mVbos[vboIdx].end();

            while( itor != endt )
            {
                Vbo &vbo = *itor;
                if( vbo.freeBlocks.size() == 1u && vbo.sizeBytes == vbo.freeBlocks.back().size )
                {
#ifdef OGRE_ASSERTS_ENABLED
                    VaoVec::iterator itVao = mVaos.begin();
                    VaoVec::iterator enVao = mVaos.end();

                    while( itVao != enVao )
                    {
                        bool usesBuffer = false;
                        Vao::VertexBindingVec::const_iterator itBuf = itVao->vertexBuffers.begin();
                        Vao::VertexBindingVec::const_iterator enBuf = itVao->vertexBuffers.end();

                        while( itBuf != enBuf && !usesBuffer )
                        {
                            OGRE_ASSERT_LOW( itBuf->vertexBufferVbo != vbo.vboName &&
                                             "A VertexArrayObject still references "
                                             "a deleted vertex buffer!" );
                            ++itBuf;
                        }

                        OGRE_ASSERT_LOW( itVao->indexBufferVbo != vbo.vboName &&
                                         "A VertexArrayObject still references "
                                         "a deleted index buffer!" );
                        ++itVao;
                    }
#endif

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
    void MetalVaoManager::allocateVbo( size_t sizeBytes, size_t alignment, BufferType bufferType,
                                       size_t &outVboIdx, size_t &outBufferOffset )
    {
        assert( alignment > 0 );

        alignment = Math::lcm( alignment, c_minimumAlignment );

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType );

        if( bufferType >= BT_DYNAMIC_DEFAULT )
            sizeBytes *= mDynamicBufferMultiplier;

        VboVec::const_iterator itor = mVbos[vboFlag].begin();
        VboVec::const_iterator endt = mVbos[vboFlag].end();

        // Find a suitable VBO that can hold the requested size. We prefer those free
        // blocks that have a matching stride (the current offset is a multiple of
        // bytesPerElement) in order to minimize the amount of memory padding.
        size_t bestVboIdx = std::numeric_limits<size_t>::max();
        ptrdiff_t bestBlockIdx = -1;
        bool foundMatchingStride = false;

        while( itor != endt && !foundMatchingStride )
        {
            BlockVec::const_iterator blockIt = itor->freeBlocks.begin();
            BlockVec::const_iterator blockEn = itor->freeBlocks.end();

            while( blockIt != blockEn && !foundMatchingStride )
            {
                const Block &block = *blockIt;
                assert( ( block.offset + block.size ) <= itor->sizeBytes );

                // Round to next multiple of alignment
                size_t newOffset = ( ( block.offset + alignment - 1 ) / alignment ) * alignment;
                size_t padding = newOffset - block.offset;

                if( sizeBytes + padding <= block.size )
                {
                    bestVboIdx = static_cast<size_t>( itor - mVbos[vboFlag].begin() );
                    bestBlockIdx = blockIt - itor->freeBlocks.begin();

                    if( newOffset == block.offset )
                        foundMatchingStride = true;
                }

                ++blockIt;
            }

            ++itor;
        }

        if( bestBlockIdx == -1 )
        {
            bestVboIdx = mVbos[vboFlag].size();
            bestBlockIdx = 0;
            foundMatchingStride = true;

            Vbo newVbo;

            // Ensure pool size is multiple of 4 otherwise some StagingBuffer copies can fail.
            //(when allocations happen very close to the end of the pool)
            size_t poolSize = std::max( mDefaultPoolSize[vboFlag], sizeBytes );
            poolSize = alignToNextMultiple<size_t>( poolSize, 4u );

            // No luck, allocate a new buffer.
            MTLResourceOptions resourceOptions = 0;

            if( vboFlag == CPU_INACCESSIBLE )
                resourceOptions = MTLResourceStorageModePrivate;
            else if( vboFlag == CPU_ACCESSIBLE_SHARED )
                resourceOptions = MTLResourceStorageModeShared;
            else
                resourceOptions = MTLResourceCPUCacheModeWriteCombined;

            newVbo.vboName = [mDevice->mDevice newBufferWithLength:poolSize options:resourceOptions];

            if( !newVbo.vboName )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Out of GPU memory or driver refused.\n"
                             "Requested: " +
                                 StringConverter::toString( poolSize ) + " bytes.",
                             "MetalVaoManager::allocateVbo" );
            }

            newVbo.sizeBytes = poolSize;
            newVbo.freeBlocks.push_back( Block( 0, poolSize ) );
            newVbo.dynamicBuffer = 0;

            if( vboFlag != CPU_INACCESSIBLE )
            {
                newVbo.dynamicBuffer = new MetalDynamicBuffer( newVbo.vboName, newVbo.sizeBytes );
            }

            mVbos[vboFlag].push_back( newVbo );
        }

        Vbo &bestVbo = mVbos[vboFlag][bestVboIdx];
        Block &bestBlock = bestVbo.freeBlocks[static_cast<size_t>( bestBlockIdx )];

        size_t newOffset = ( ( bestBlock.offset + alignment - 1 ) / alignment ) * alignment;
        size_t padding = newOffset - bestBlock.offset;
        // Shrink our records about available data.
        bestBlock.size -= sizeBytes + padding;
        bestBlock.offset = newOffset + sizeBytes;

        if( !foundMatchingStride )
        {
            // This is a stride changer, record as such.
            StrideChangerVec::iterator itStride =
                std::lower_bound( bestVbo.strideChangers.begin(), bestVbo.strideChangers.end(),
                                  newOffset, StrideChanger() );
            bestVbo.strideChangers.insert( itStride, StrideChanger( newOffset, padding ) );
        }

        if( bestBlock.size == 0 )
            bestVbo.freeBlocks.erase( bestVbo.freeBlocks.begin() + bestBlockIdx );

        outVboIdx = bestVboIdx;
        outBufferOffset = newOffset;
    }
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::deallocateVbo( size_t vboIdx, size_t bufferOffset, size_t sizeBytes,
                                         BufferType bufferType )
    {
        VboFlag vboFlag = bufferTypeToVboFlag( bufferType );

        if( bufferType >= BT_DYNAMIC_DEFAULT )
            sizeBytes *= mDynamicBufferMultiplier;

        Vbo &vbo = mVbos[vboFlag][vboIdx];
        StrideChangerVec::iterator itStride = std::lower_bound(
            vbo.strideChangers.begin(), vbo.strideChangers.end(), bufferOffset, StrideChanger() );

        if( itStride != vbo.strideChangers.end() && itStride->offsetAfterPadding == bufferOffset )
        {
            bufferOffset -= itStride->paddedBytes;
            sizeBytes += itStride->paddedBytes;

            vbo.strideChangers.erase( itStride );
        }

        // See if we're contiguous to a free block and make that block grow.
        assert( ( bufferOffset + sizeBytes ) <= vbo.sizeBytes );
        vbo.freeBlocks.push_back( Block( bufferOffset, sizeBytes ) );
        mergeContiguousBlocks( vbo.freeBlocks.end() - 1, vbo.freeBlocks );
    }
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::mergeContiguousBlocks( BlockVec::iterator blockToMerge, BlockVec &blocks )
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
                if( idx == ptrdiff_t( blocks.size() - 1u ) )
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
                if( idx == ptrdiff_t( blocks.size() - 1u ) )
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
    VertexBufferPacked *MetalVaoManager::createVertexBufferImpl( size_t numElements,
                                                                 uint32 bytesPerElement,
                                                                 BufferType bufferType,
                                                                 void *initialData, bool keepAsShadow,
                                                                 const VertexElement2Vec &vElements )
    {
        size_t vboIdx;
        size_t bufferOffset;

        const size_t requestedSize = numElements * bytesPerElement;
        size_t sizeBytes = requestedSize;

        if( bufferType >= BT_DYNAMIC_DEFAULT )
        {
            // For dynamic buffers, the size will be 3x times larger
            //(depending on mDynamicBufferMultiplier); we need the
            // offset after each map to be aligned; and for that, we
            // sizeBytes to be multiple of alignment.
            const size_t alignment = Math::lcm( bytesPerElement, c_minimumAlignment );
            sizeBytes = alignToNextMultiple( sizeBytes, alignment );
        }

        allocateVbo( sizeBytes, bytesPerElement, bufferType, vboIdx, bufferOffset );

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType );
        Vbo &vbo = mVbos[vboFlag][vboIdx];
        MetalBufferInterface *bufferInterface =
            new MetalBufferInterface( vboIdx, vbo.vboName, vbo.dynamicBuffer );

        VertexBufferPacked *retVal = OGRE_NEW VertexBufferPacked(
            bufferOffset, numElements, bytesPerElement,
            uint32( ( sizeBytes - requestedSize ) / bytesPerElement ), bufferType, initialData,
            keepAsShadow, this, bufferInterface, vElements );

        if( initialData )
            bufferInterface->_firstUpload( initialData, 0, numElements );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::destroyVertexBufferImpl( VertexBufferPacked *vertexBuffer )
    {
        MetalBufferInterface *bufferInterface =
            static_cast<MetalBufferInterface *>( vertexBuffer->getBufferInterface() );

        deallocateVbo( bufferInterface->getVboPoolIndex(),
                       vertexBuffer->_getInternalBufferStart() * vertexBuffer->getBytesPerElement(),
                       vertexBuffer->_getInternalTotalSizeBytes(), vertexBuffer->getBufferType() );
    }
    //-----------------------------------------------------------------------------------
#ifdef _OGRE_MULTISOURCE_VBO
    MultiSourceVertexBufferPool *MetalVaoManager::createMultiSourceVertexBufferPoolImpl(
        const VertexElement2VecVec &vertexElementsBySource, size_t maxNumVertices,
        size_t totalBytesPerVertex, BufferType bufferType )
    {
        size_t vboIdx;
        size_t bufferOffset;

        allocateVbo( maxNumVertices * totalBytesPerVertex, totalBytesPerVertex, bufferType, vboIdx,
                     bufferOffset );

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType );

        const Vbo &vbo = mVbos[vboFlag][vboIdx];

        return OGRE_NEW MetalMultiSourceVertexBufferPool( vboIdx, vbo.vboName, vertexElementsBySource,
                                                          maxNumVertices, bufferType, bufferOffset,
                                                          this );
    }
#endif
    //-----------------------------------------------------------------------------------
    IndexBufferPacked *MetalVaoManager::createIndexBufferImpl( size_t numElements,
                                                               uint32 bytesPerElement,
                                                               BufferType bufferType, void *initialData,
                                                               bool keepAsShadow )
    {
        size_t vboIdx;
        size_t bufferOffset;

        const size_t requestedSize = numElements * bytesPerElement;
        size_t sizeBytes = requestedSize;

        if( bufferType >= BT_DYNAMIC_DEFAULT )
        {
            // For dynamic buffers, the size will be 3x times larger
            //(depending on mDynamicBufferMultiplier); we need the
            // offset after each map to be aligned; and for that, we
            // sizeBytes to be multiple of alignment.
            const size_t alignment = Math::lcm( bytesPerElement, c_indexBufferAlignment );
            sizeBytes = alignToNextMultiple( sizeBytes, alignment );
        }

        allocateVbo( sizeBytes, bytesPerElement, bufferType, vboIdx, bufferOffset );

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType );

        Vbo &vbo = mVbos[vboFlag][vboIdx];
        MetalBufferInterface *bufferInterface =
            new MetalBufferInterface( vboIdx, vbo.vboName, vbo.dynamicBuffer );
        IndexBufferPacked *retVal =
            OGRE_NEW IndexBufferPacked( bufferOffset, numElements, bytesPerElement,
                                        uint32( ( sizeBytes - requestedSize ) / bytesPerElement ),
                                        bufferType, initialData, keepAsShadow, this, bufferInterface );

        if( initialData )
            bufferInterface->_firstUpload( initialData, 0, numElements );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::destroyIndexBufferImpl( IndexBufferPacked *indexBuffer )
    {
        MetalBufferInterface *bufferInterface =
            static_cast<MetalBufferInterface *>( indexBuffer->getBufferInterface() );

        deallocateVbo( bufferInterface->getVboPoolIndex(),
                       indexBuffer->_getInternalBufferStart() * indexBuffer->getBytesPerElement(),
                       indexBuffer->_getInternalTotalSizeBytes(), indexBuffer->getBufferType() );
    }
    //-----------------------------------------------------------------------------------
    ConstBufferPacked *MetalVaoManager::createConstBufferImpl( size_t sizeBytes, BufferType bufferType,
                                                               void *initialData, bool keepAsShadow )
    {
        size_t vboIdx;
        size_t bufferOffset;

        size_t alignment = mConstBufferAlignment;

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType );

        const size_t requestedSize = sizeBytes;

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
        MetalBufferInterface *bufferInterface =
            new MetalBufferInterface( vboIdx, vbo.vboName, vbo.dynamicBuffer );
        ConstBufferPacked *retVal = OGRE_NEW MetalConstBufferPacked(
            bufferOffset, requestedSize, 1, uint32( ( sizeBytes - requestedSize ) / 1u ), bufferType,
            initialData, keepAsShadow, this, bufferInterface, mDevice );

        if( initialData )
            bufferInterface->_firstUpload( initialData, 0, requestedSize );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::destroyConstBufferImpl( ConstBufferPacked *constBuffer )
    {
        MetalBufferInterface *bufferInterface =
            static_cast<MetalBufferInterface *>( constBuffer->getBufferInterface() );

        deallocateVbo( bufferInterface->getVboPoolIndex(),
                       constBuffer->_getInternalBufferStart() * constBuffer->getBytesPerElement(),
                       constBuffer->_getInternalTotalSizeBytes(), constBuffer->getBufferType() );
    }
    //-----------------------------------------------------------------------------------
    TexBufferPacked *MetalVaoManager::createTexBufferImpl( PixelFormatGpu pixelFormat, size_t sizeBytes,
                                                           BufferType bufferType, void *initialData,
                                                           bool keepAsShadow )
    {
        size_t vboIdx;
        size_t bufferOffset;

        size_t alignment = mTexBufferAlignment;

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType );

        const size_t requestedSize = sizeBytes;

        if( bufferType >= BT_DYNAMIC_DEFAULT )
        {
            // For dynamic buffers, the size will be 3x times larger
            // (depending on mDynamicBufferMultiplier); we need the
            // offset after each map to be aligned; and for that, we
            // sizeBytes to be multiple of alignment.
            const size_t sizeMultiple = Math::lcm( 1u, c_minimumAlignment );
            sizeBytes = alignToNextMultiple( sizeBytes, sizeMultiple );
        }

        allocateVbo( sizeBytes, alignment, bufferType, vboIdx, bufferOffset );

        Vbo &vbo = mVbos[vboFlag][vboIdx];
        MetalBufferInterface *bufferInterface =
            new MetalBufferInterface( vboIdx, vbo.vboName, vbo.dynamicBuffer );
        TexBufferPacked *retVal = OGRE_NEW MetalTexBufferPacked(
            bufferOffset, requestedSize, 1, uint32( ( sizeBytes - requestedSize ) / 1u ), bufferType,
            initialData, keepAsShadow, this, bufferInterface, pixelFormat, mDevice );

        if( initialData )
            bufferInterface->_firstUpload( initialData, 0, requestedSize );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::destroyTexBufferImpl( TexBufferPacked *texBuffer )
    {
        MetalBufferInterface *bufferInterface =
            static_cast<MetalBufferInterface *>( texBuffer->getBufferInterface() );

        deallocateVbo( bufferInterface->getVboPoolIndex(),
                       texBuffer->_getInternalBufferStart() * texBuffer->getBytesPerElement(),
                       texBuffer->_getInternalTotalSizeBytes(), texBuffer->getBufferType() );
    }
    //-----------------------------------------------------------------------------------
    ReadOnlyBufferPacked *MetalVaoManager::createReadOnlyBufferImpl( PixelFormatGpu pixelFormat,
                                                                     size_t sizeBytes,
                                                                     BufferType bufferType,
                                                                     void *initialData,
                                                                     bool keepAsShadow )
    {
        size_t vboIdx;
        size_t bufferOffset;

        size_t alignment = mTexBufferAlignment;

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType );

        const size_t requestedSize = sizeBytes;

        if( bufferType >= BT_DYNAMIC_DEFAULT )
        {
            // For dynamic buffers, the size will be 3x times larger
            //(depending on mDynamicBufferMultiplier); we need the
            // offset after each map to be aligned; and for that, we
            // sizeBytes to be multiple of alignment.
            const size_t sizeMultiple = Math::lcm( 1, c_minimumAlignment );
            sizeBytes = alignToNextMultiple( sizeBytes, sizeMultiple );
        }

        allocateVbo( sizeBytes, alignment, bufferType, vboIdx, bufferOffset );

        Vbo &vbo = mVbos[vboFlag][vboIdx];
        MetalBufferInterface *bufferInterface =
            new MetalBufferInterface( vboIdx, vbo.vboName, vbo.dynamicBuffer );
        ReadOnlyBufferPacked *retVal = OGRE_NEW MetalReadOnlyBufferPacked(
            bufferOffset, requestedSize, 1, uint32( ( sizeBytes - requestedSize ) / 1u ), bufferType,
            initialData, keepAsShadow, this, bufferInterface, pixelFormat, mDevice );

        if( initialData )
            bufferInterface->_firstUpload( initialData, 0, requestedSize );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::destroyReadOnlyBufferImpl( ReadOnlyBufferPacked *readOnlyBuffer )
    {
        MetalBufferInterface *bufferInterface =
            static_cast<MetalBufferInterface *>( readOnlyBuffer->getBufferInterface() );

        deallocateVbo( bufferInterface->getVboPoolIndex(),
                       readOnlyBuffer->_getInternalBufferStart() * readOnlyBuffer->getBytesPerElement(),
                       readOnlyBuffer->_getInternalTotalSizeBytes(), readOnlyBuffer->getBufferType() );
    }
    //-----------------------------------------------------------------------------------
    UavBufferPacked *MetalVaoManager::createUavBufferImpl( size_t numElements, uint32 bytesPerElement,
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
        MetalBufferInterface *bufferInterface =
            new MetalBufferInterface( vboIdx, vbo.vboName, vbo.dynamicBuffer );
        UavBufferPacked *retVal =
            OGRE_NEW MetalUavBufferPacked( bufferOffset, numElements, bytesPerElement, bindFlags,
                                           initialData, keepAsShadow, this, bufferInterface, mDevice );

        if( initialData )
            bufferInterface->_firstUpload( initialData, 0, numElements );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::destroyUavBufferImpl( UavBufferPacked *uavBuffer )
    {
        MetalBufferInterface *bufferInterface =
            static_cast<MetalBufferInterface *>( uavBuffer->getBufferInterface() );

        deallocateVbo( bufferInterface->getVboPoolIndex(),
                       uavBuffer->_getInternalBufferStart() * uavBuffer->getBytesPerElement(),
                       uavBuffer->_getInternalTotalSizeBytes(), uavBuffer->getBufferType() );
    }
    //-----------------------------------------------------------------------------------
    IndirectBufferPacked *MetalVaoManager::createIndirectBufferImpl( size_t sizeBytes,
                                                                     BufferType bufferType,
                                                                     void *initialData,
                                                                     bool keepAsShadow )
    {
        const size_t alignment = 4;
        size_t bufferOffset = 0;

        const size_t requestedSize = sizeBytes;

        if( bufferType >= BT_DYNAMIC_DEFAULT )
        {
            // For dynamic buffers, the size will be 3x times larger
            //(depending on mDynamicBufferMultiplier); we need the
            // offset after each map to be aligned; and for that, we
            // sizeBytes to be multiple of alignment.
            sizeBytes = alignToNextMultiple( sizeBytes, alignment );
        }

        MetalBufferInterface *bufferInterface = 0;
        if( mSupportsIndirectBuffers )
        {
            size_t vboIdx;
            VboFlag vboFlag = bufferTypeToVboFlag( bufferType );

            allocateVbo( sizeBytes, alignment, bufferType, vboIdx, bufferOffset );

            Vbo &vbo = mVbos[vboFlag][vboIdx];
            bufferInterface = new MetalBufferInterface( vboIdx, vbo.vboName, vbo.dynamicBuffer );
        }

        IndirectBufferPacked *retVal = OGRE_NEW IndirectBufferPacked(
            bufferOffset, requestedSize, 1, uint32( ( sizeBytes - requestedSize ) / 1u ), bufferType,
            initialData, keepAsShadow, this, bufferInterface );

        if( initialData )
        {
            if( mSupportsIndirectBuffers )
            {
                bufferInterface->_firstUpload( initialData, 0, requestedSize );
            }
            else
            {
                memcpy( retVal->getSwBufferPtr(), initialData, requestedSize );
            }
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::destroyIndirectBufferImpl( IndirectBufferPacked *indirectBuffer )
    {
        if( mSupportsIndirectBuffers )
        {
            MetalBufferInterface *bufferInterface =
                static_cast<MetalBufferInterface *>( indirectBuffer->getBufferInterface() );

            deallocateVbo(
                bufferInterface->getVboPoolIndex(),
                indirectBuffer->_getInternalBufferStart() * indirectBuffer->getBytesPerElement(),
                indirectBuffer->_getInternalTotalSizeBytes(), indirectBuffer->getBufferType() );
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::bindDrawId()
    {
        assert( mDevice->mRenderEncoder || mDevice->mFrameAborted );

        MetalBufferInterface *bufferInterface =
            static_cast<MetalBufferInterface *>( mDrawId->getBufferInterface() );

        [mDevice->mRenderEncoder setVertexBuffer:bufferInterface->getVboName() offset:0 atIndex:15];
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        [mDevice->mRenderEncoder setFragmentBuffer:bufferInterface->getVboName() offset:0 atIndex:15];
#endif
    }
    //-----------------------------------------------------------------------------------
    VertexArrayObject *MetalVaoManager::createVertexArrayObjectImpl(
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
    void MetalVaoManager::destroyVertexArrayObjectImpl( VertexArrayObject *vao )
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
    MetalVaoManager::VaoVec::iterator MetalVaoManager::findVao(
        const VertexBufferPackedVec &vertexBuffers, IndexBufferPacked *indexBuffer,
        OperationType opType )
    {
        Vao vao;

        vao.operationType = opType;
        vao.vertexBuffers.reserve( vertexBuffers.size() );

        {
            VertexBufferPackedVec::const_iterator itor = vertexBuffers.begin();
            VertexBufferPackedVec::const_iterator endt = vertexBuffers.end();

            while( itor != endt )
            {
                Vao::VertexBinding vertexBinding;
                vertexBinding.vertexBufferVbo =
                    static_cast<MetalBufferInterface *>( ( *itor )->getBufferInterface() )->getVboName();
                vertexBinding.vertexElements = ( *itor )->getVertexElements();
                vertexBinding.instancingDivisor = 0;

#ifdef _OGRE_MULTISOURCE_VBO
                /*const MultiSourceVertexBufferPool *multiSourcePool = (*itor)->getMultiSourcePool();
                if( multiSourcePool )
                {
                    vertexBinding.offset = multiSourcePool->getBytesOffsetToSource(
                                                            (*itor)->_getSourceIndex() );
                }*/
#endif

                vao.vertexBuffers.push_back( vertexBinding );

                ++itor;
            }
        }

        vao.refCount = 0;

        if( indexBuffer )
        {
            vao.indexBufferVbo =
                static_cast<MetalBufferInterface *>( indexBuffer->getBufferInterface() )->getVboName();
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
            vao.vaoName = createVao( vao );
            mVaos.push_back( vao );
            itor = mVaos.begin() + ptrdiff_t( mVaos.size() - 1u );
        }

        ++itor->refCount;

        return itor;
    }
    //-----------------------------------------------------------------------------------
    uint32 MetalVaoManager::createVao( const Vao &vaoRef ) { return mVaoNames++; }
    //-----------------------------------------------------------------------------------
    uint32 MetalVaoManager::generateRenderQueueId( uint32 vaoName, uint32 uniqueVaoId )
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
        const int bitsVaoGl = 5;
        const uint32 maskVaoGl = OGRE_RQ_MAKE_MASK( bitsVaoGl );
        const uint32 maskVao = OGRE_RQ_MAKE_MASK( RqBits::MeshBits - bitsVaoGl );

        const uint32 shiftVaoGl = uint32( RqBits::MeshBits - bitsVaoGl );

        uint32 renderQueueId = ( ( vaoName & maskVaoGl ) << shiftVaoGl ) | ( uniqueVaoId & maskVao );

        return renderQueueId;
    }
    //-----------------------------------------------------------------------------------
    StagingBuffer *MetalVaoManager::createStagingBuffer( size_t sizeBytes, bool forUpload )
    {
        sizeBytes = std::max<size_t>( sizeBytes, 4 * 1024 * 1024 );
        sizeBytes = alignToNextMultiple<size_t>( sizeBytes, 4u );

        MTLResourceOptions resourceOptions = 0;

        resourceOptions |= MTLResourceStorageModeShared;

        if( forUpload )
            resourceOptions |= MTLResourceCPUCacheModeWriteCombined;
        else
            resourceOptions |= MTLResourceCPUCacheModeDefaultCache;

        id<MTLBuffer> bufferName = [mDevice->mDevice newBufferWithLength:sizeBytes
                                                                 options:resourceOptions];
        if( !bufferName )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Out of GPU memory or driver refused.\n"
                         "Requested: " +
                             StringConverter::toString( sizeBytes ) + " bytes.",
                         "MetalVaoManager::createStagingBuffer" );
        }

        MetalStagingBuffer *stagingBuffer =
            OGRE_NEW MetalStagingBuffer( 0, sizeBytes, this, forUpload, bufferName, mDevice );
        mRefedStagingBuffers[forUpload].push_back( stagingBuffer );

        return stagingBuffer;
    }
    //-----------------------------------------------------------------------------------
    AsyncTicketPtr MetalVaoManager::createAsyncTicket( BufferPacked *creator,
                                                       StagingBuffer *stagingBuffer, size_t elementStart,
                                                       size_t elementCount )
    {
        return AsyncTicketPtr(
            OGRE_NEW MetalAsyncTicket( creator, stagingBuffer, elementStart, elementCount, mDevice ) );
    }
    //-----------------------------------------------------------------------------------
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
    void MetalVaoManager::unalignedCopy( id<MTLBuffer> dstBuffer, size_t dstOffsetBytes,
                                         id<MTLBuffer> srcBuffer, size_t srcOffsetBytes,
                                         size_t sizeBytes )
    {
        __unsafe_unretained id<MTLComputeCommandEncoder> computeEncoder = mDevice->getComputeEncoder();

        [computeEncoder setBuffer:dstBuffer offset:0 atIndex:0];
        [computeEncoder setBuffer:srcBuffer offset:0 atIndex:1];
        uint32_t copyInfo[3] = { static_cast<uint32_t>( dstOffsetBytes ),
                                 static_cast<uint32_t>( srcOffsetBytes ),
                                 static_cast<uint32_t>( sizeBytes ) };
        [computeEncoder setBytes:copyInfo length:sizeof( copyInfo ) atIndex:2];

        MTLSize threadsPerThreadgroup = MTLSizeMake( 1024u, 1u, 1u );
        MTLSize threadgroupsPerGrid = MTLSizeMake( 1u, 1u, 1u );

        const size_t threadsRequired = alignToNextMultiple<size_t>( sizeBytes, 64u ) / 64u;
        threadsPerThreadgroup.width =
            std::min<NSUInteger>( static_cast<NSUInteger>( threadsRequired ), 1024u );
        threadgroupsPerGrid.width = static_cast<NSUInteger>(
            alignToNextMultiple<size_t>( threadsRequired, threadsPerThreadgroup.width ) /
            threadsPerThreadgroup.width );

        [computeEncoder setComputePipelineState:mUnalignedCopyPso];

        [computeEncoder dispatchThreadgroups:threadgroupsPerGrid
                       threadsPerThreadgroup:threadsPerThreadgroup];
    }
#endif
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::_update()
    {
        uint64 currentTimeMs = mTimer->getMilliseconds();

        if( currentTimeMs >= mNextStagingBufferTimestampCheckpoint )
        {
            mNextStagingBufferTimestampCheckpoint = std::numeric_limits<uint64>::max();

            for( size_t i = 0; i < 2; ++i )
            {
                StagingBufferVec::iterator itor = mZeroRefStagingBuffers[i].begin();
                StagingBufferVec::iterator endt = mZeroRefStagingBuffers[i].end();

                while( itor != endt )
                {
                    StagingBuffer *stagingBuffer = *itor;

                    mNextStagingBufferTimestampCheckpoint =
                        std::min( mNextStagingBufferTimestampCheckpoint,
                                  stagingBuffer->getLastUsedTimestamp() + currentTimeMs );

                    /*if( stagingBuffer->getLastUsedTimestamp() - currentTimeMs >
                        stagingBuffer->getUnfencedTimeThreshold() )
                    {
                        static_cast<MetalStagingBuffer*>( stagingBuffer )->cleanUnfencedHazards();
                    }*/

                    if( stagingBuffer->getLastUsedTimestamp() - currentTimeMs >
                        stagingBuffer->getLifetimeThreshold() )
                    {
                        // Time to delete this buffer.
                        delete *itor;

                        itor = efficientVectorRemove( mZeroRefStagingBuffers[i], itor );
                        endt = mZeroRefStagingBuffers[i].end();
                    }
                    else
                    {
                        ++itor;
                    }
                }
            }
        }

        if( !mSemaphoreFlushed )
        {
            // We could only reach here if _update() was called
            // twice in a row without completing a full frame.
            // Without this, waitForTailFrameToFinish will deadlock.
            mDevice->commitAndNextCommandBuffer();
        }

        if( !mDelayedDestroyBuffers.empty() &&
            mDelayedDestroyBuffers.front().frameNumDynamic == mDynamicBufferCurrentFrame )
        {
            waitForTailFrameToFinish();
            destroyDelayedBuffers( mDynamicBufferCurrentFrame );
        }

        // We must call this to raise the semaphore count in case we haven't already
        waitForTailFrameToFinish();

        VaoManager::_update();

        mSemaphoreFlushed = false;
        mAlreadyWaitedForSemaphore[mDynamicBufferCurrentFrame] = false;
        __block dispatch_semaphore_t blockSemaphore = mFrameSyncVec[mDynamicBufferCurrentFrame];
        [mDevice->mCurrentCommandBuffer addCompletedHandler:^( id<MTLCommandBuffer> buffer ) {
          dispatch_semaphore_signal( blockSemaphore );
        }];

        mDynamicBufferCurrentFrame = ( mDynamicBufferCurrentFrame + 1 ) % mDynamicBufferMultiplier;
    }
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::_notifyNewCommandBuffer() { mSemaphoreFlushed = true; }
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::_notifyDeviceStalled()
    {
        mSemaphoreFlushed = true;

        for( size_t i = 0; i < 2u; ++i )
        {
            StagingBufferVec::const_iterator itor = mRefedStagingBuffers[i].begin();
            StagingBufferVec::const_iterator endt = mRefedStagingBuffers[i].end();

            while( itor != endt )
            {
                MetalStagingBuffer *stagingBuffer = static_cast<MetalStagingBuffer *>( *itor );
                stagingBuffer->_notifyDeviceStalled();
                ++itor;
            }

            itor = mZeroRefStagingBuffers[i].begin();
            endt = mZeroRefStagingBuffers[i].end();

            while( itor != endt )
            {
                MetalStagingBuffer *stagingBuffer = static_cast<MetalStagingBuffer *>( *itor );
                stagingBuffer->_notifyDeviceStalled();
                ++itor;
            }
        }

        _destroyAllDelayedBuffers();

        mFrameCount += mDynamicBufferMultiplier;
    }
    //-----------------------------------------------------------------------------------
    uint8 MetalVaoManager::waitForTailFrameToFinish()
    {
        // MetalRenderSystem::_beginFrameOnce does a global waiting for us, but if we're outside
        // the render loop (i.e. user is manually uploading data) we may have to call this earlier.
        if( !mAlreadyWaitedForSemaphore[mDynamicBufferCurrentFrame] )
        {
            waitFor( mFrameSyncVec[mDynamicBufferCurrentFrame], mDevice );
            // Semaphore was just grabbed, so ensure we don't grab it twice.
            mAlreadyWaitedForSemaphore[mDynamicBufferCurrentFrame] = true;
        }
        // mDevice->mRenderSystem->_waitForTailFrameToFinish();
        return mDynamicBufferCurrentFrame;
    }
    //-----------------------------------------------------------------------------------
    void MetalVaoManager::waitForSpecificFrameToFinish( uint32 frameCount )
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

            if( !mAlreadyWaitedForSemaphore[idx] )
            {
                waitFor( mFrameSyncVec[idx], mDevice );
                // Semaphore was just grabbed, so ensure we don't grab it twice.
                mAlreadyWaitedForSemaphore[idx] = true;
            }
        }
        else
        {
            // No stall
        }
    }
    //-----------------------------------------------------------------------------------
    bool MetalVaoManager::isFrameFinished( uint32 frameCount )
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

            if( !mAlreadyWaitedForSemaphore[idx] )
            {
                const long result = dispatch_semaphore_wait( mFrameSyncVec[idx], DISPATCH_TIME_NOW );
                if( result == 0 )
                {
                    retVal = true;
                    // Semaphore was just grabbed, so ensure we don't grab it twice.
                    mAlreadyWaitedForSemaphore[idx] = true;
                }
            }
        }
        else
        {
            // No stall
            retVal = true;
        }

        return retVal;
    }
    
    void MetalVaoManager::_waitUntilCommitedCommandBufferCompleted()
    {
        if(mDevice)
            mDevice->_waitUntilCommitedCommandBufferCompleted();
    }

    //-----------------------------------------------------------------------------------
    dispatch_semaphore_t MetalVaoManager::waitFor( dispatch_semaphore_t fenceName, MetalDevice *device )
    {
        dispatch_time_t timeout = DISPATCH_TIME_NOW;
        while( true )
        {
            long result = dispatch_semaphore_wait( fenceName, timeout );

            if( result == 0 )
                return 0;  // Success waiting.

            if( timeout == DISPATCH_TIME_NOW )
            {
                // After the first time, need to start flushing, and wait for a looong time.
                timeout = DISPATCH_TIME_FOREVER;
                device->commitAndNextCommandBuffer();
            }
            else if( result != 0 )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Failure while waiting for a MetalFence. Could be out of GPU memory. "
                             "Update your video card drivers. If that doesn't help, "
                             "contact the developers. Error code: " +
                                 StringConverter::toString( result ),
                             "MetalStagingBuffer::wait" );

                return fenceName;
            }
        }

        return 0;
    }
    //-----------------------------------------------------------------------------------
    uint32 MetalVaoManager::getAttributeIndexFor( VertexElementSemantic semantic )
    {
        return VERTEX_ATTRIBUTE_INDEX[semantic - 1];
    }
    //-----------------------------------------------------------------------------------
    MetalVaoManager::VboFlag MetalVaoManager::bufferTypeToVboFlag( BufferType bufferType )
    {
        return static_cast<VboFlag>(
            std::max( 0, ( bufferType - BT_DEFAULT_SHARED ) + CPU_ACCESSIBLE_SHARED ) );
    }
}
