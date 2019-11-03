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
#include "Vao/OgreVulkanMultiSourceVertexBufferPool.h"
#include "Vao/OgreVulkanStagingBuffer.h"
#include "Vao/OgreVulkanTexBufferPacked.h"
#include "Vao/OgreVulkanUavBufferPacked.h"
#include "Vao/OgreVulkanVertexArrayObject.h"

#include "Vao/OgreIndirectBufferPacked.h"

#include "OgreRenderQueue.h"

#include "OgreStringConverter.h"
#include "OgreTimer.h"

#define TODO_call_commitAndNextCommandBuffer

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
        outCapacityBytes = 0;
        outFreeBytes = 0;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::cleanupEmptyPools( void ) {}
    //-----------------------------------------------------------------------------------
    VertexBufferPacked *VulkanVaoManager::createVertexBufferImpl( size_t numElements,
                                                                  uint32 bytesPerElement,
                                                                  BufferType bufferType,
                                                                  void *initialData, bool keepAsShadow,
                                                                  const VertexElement2Vec &vElements )
    {
        VulkanBufferInterface *bufferInterface = new VulkanBufferInterface( 0 );
        VertexBufferPacked *retVal =
            OGRE_NEW VertexBufferPacked( 0, numElements, bytesPerElement, 0, bufferType, initialData,
                                         keepAsShadow, this, bufferInterface, vElements, 0, 0, 0 );

        if( initialData )
            bufferInterface->_firstUpload( initialData, 0, numElements );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyVertexBufferImpl( VertexBufferPacked *vertexBuffer ) {}
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
        VulkanBufferInterface *bufferInterface = new VulkanBufferInterface( 0 );
        IndexBufferPacked *retVal =
            OGRE_NEW IndexBufferPacked( 0, numElements, bytesPerElement, 0, bufferType, initialData,
                                        keepAsShadow, this, bufferInterface );

        if( initialData )
            bufferInterface->_firstUpload( initialData, 0, numElements );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyIndexBufferImpl( IndexBufferPacked *indexBuffer ) {}
    //-----------------------------------------------------------------------------------
    ConstBufferPacked *VulkanVaoManager::createConstBufferImpl( size_t sizeBytes, BufferType bufferType,
                                                                void *initialData, bool keepAsShadow )
    {
        uint32 alignment = mConstBufferAlignment;

        size_t bindableSize = sizeBytes;

        if( bufferType >= BT_DYNAMIC_DEFAULT )
        {
            // For dynamic buffers, the size will be 3x times larger
            //(depending on mDynamicBufferMultiplier); we need the
            // offset after each map to be aligned; and for that, we
            // sizeBytes to be multiple of alignment.
            sizeBytes = ( ( sizeBytes + alignment - 1 ) / alignment ) * alignment;
        }

        VulkanBufferInterface *bufferInterface = new VulkanBufferInterface( 0 );
        ConstBufferPacked *retVal =
            OGRE_NEW VulkanConstBufferPacked( 0, sizeBytes, 1, 0, bufferType, initialData, keepAsShadow,
                                              this, bufferInterface, bindableSize );

        if( initialData )
            bufferInterface->_firstUpload( initialData, 0, sizeBytes );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyConstBufferImpl( ConstBufferPacked *constBuffer ) {}
    //-----------------------------------------------------------------------------------
    TexBufferPacked *VulkanVaoManager::createTexBufferImpl( PixelFormatGpu pixelFormat, size_t sizeBytes,
                                                            BufferType bufferType, void *initialData,
                                                            bool keepAsShadow )
    {
        uint32 alignment = mTexBufferAlignment;

        VboFlag vboFlag = bufferTypeToVboFlag( bufferType );

        if( bufferType >= BT_DYNAMIC_DEFAULT )
        {
            // For dynamic buffers, the size will be 3x times larger
            //(depending on mDynamicBufferMultiplier); we need the
            // offset after each map to be aligned; and for that, we
            // sizeBytes to be multiple of alignment.
            sizeBytes = ( ( sizeBytes + alignment - 1 ) / alignment ) * alignment;
        }

        VulkanBufferInterface *bufferInterface = new VulkanBufferInterface( 0 );
        TexBufferPacked *retVal =
            OGRE_NEW VulkanTexBufferPacked( 0, sizeBytes, 1, 0, bufferType, initialData, keepAsShadow,
                                            this, bufferInterface, pixelFormat );

        if( initialData )
            bufferInterface->_firstUpload( initialData, 0, sizeBytes );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyTexBufferImpl( TexBufferPacked *texBuffer ) {}
    //-----------------------------------------------------------------------------------
    UavBufferPacked *VulkanVaoManager::createUavBufferImpl( size_t numElements, uint32 bytesPerElement,
                                                            uint32 bindFlags, void *initialData,
                                                            bool keepAsShadow )
    {
        VulkanBufferInterface *bufferInterface = new VulkanBufferInterface( 0 );
        UavBufferPacked *retVal =
            OGRE_NEW VulkanUavBufferPacked( 0, numElements, bytesPerElement, bindFlags, initialData,
                                            keepAsShadow, this, bufferInterface );

        if( initialData )
            bufferInterface->_firstUpload( initialData, 0, numElements );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyUavBufferImpl( UavBufferPacked *uavBuffer ) {}
    //-----------------------------------------------------------------------------------
    IndirectBufferPacked *VulkanVaoManager::createIndirectBufferImpl( size_t sizeBytes,
                                                                      BufferType bufferType,
                                                                      void *initialData,
                                                                      bool keepAsShadow )
    {
        const size_t alignment = 4;
        size_t bufferOffset = 0;

        if( bufferType >= BT_DYNAMIC_DEFAULT )
        {
            // For dynamic buffers, the size will be 3x times larger
            //(depending on mDynamicBufferMultiplier); we need the
            // offset after each map to be aligned; and for that, we
            // sizeBytes to be multiple of alignment.
            sizeBytes = ( ( sizeBytes + alignment - 1 ) / alignment ) * alignment;
        }

        VulkanBufferInterface *bufferInterface = 0;
        if( mSupportsIndirectBuffers )
        {
            bufferInterface = new VulkanBufferInterface( 0 );
        }

        IndirectBufferPacked *retVal = OGRE_NEW IndirectBufferPacked(
            0, sizeBytes, 1, 0, bufferType, initialData, keepAsShadow, this, bufferInterface );

        if( initialData )
        {
            if( mSupportsIndirectBuffers )
            {
                bufferInterface->_firstUpload( initialData, 0, sizeBytes );
            }
            else
            {
                memcpy( retVal->getSwBufferPtr(), initialData, sizeBytes );
            }
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanVaoManager::destroyIndirectBufferImpl( IndirectBufferPacked *indirectBuffer )
    {
        if( mSupportsIndirectBuffers )
        {
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
        sizeBytes = std::max<size_t>( sizeBytes, 4 * 1024 * 1024 );
        VulkanStagingBuffer *stagingBuffer =
            OGRE_NEW VulkanStagingBuffer( 0, sizeBytes, this, forUpload );
        mRefedStagingBuffers[forUpload].push_back( stagingBuffer );

        if( mNextStagingBufferTimestampCheckpoint == (unsigned long)( ~0 ) )
            mNextStagingBufferTimestampCheckpoint =
                mTimer->getMilliseconds() + mDefaultStagingBufferLifetime;

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
    }
}  // namespace Ogre
