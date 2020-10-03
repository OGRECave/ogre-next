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

#ifndef _Ogre_VulkanVaoManager_H_
#define _Ogre_VulkanVaoManager_H_

#include "OgreVulkanPrerequisites.h"

#include "Vao/OgreVaoManager.h"

#include "vulkan/vulkan_core.h"

namespace Ogre
{
    class VulkanDelayedFuncBase;
    class VulkanStagingTexture;

    /// Provides a simple interface similar to that of MTLBuffer
    struct VulkanRawBuffer
    {
        VkBuffer mVboName;
        VulkanDynamicBuffer *mDynamicBuffer;
        uint32 mVboFlag;  /// See VulkanVaoManager::VboFlag
        size_t mUnmapTicket;

        size_t mSize;
        size_t mInternalBufferStart;
        size_t mVboPoolIdx;

        void *map( void );
        void unmap( void );
    };

    class _OgreVulkanExport VulkanVaoManager : public VaoManager
    {
    public:
        friend class VulkanStagingBuffer;
        enum VboFlag
        {
            CPU_INACCESSIBLE,
            CPU_WRITE_PERSISTENT,
            CPU_WRITE_PERSISTENT_COHERENT,
            CPU_READ_WRITE,
            /// Where textures go if bufferImageGranularity != 1
            TEXTURES_OPTIMAL,
            MAX_VBO_FLAG
        };

    public:
        struct Block
        {
            size_t offset;
            size_t size;

            Block( size_t _offset, size_t _size ) : offset( _offset ), size( _size ) {}
        };
        struct StrideChanger
        {
            size_t offsetAfterPadding;
            size_t paddedBytes;

            StrideChanger() : offsetAfterPadding( 0 ), paddedBytes( 0 ) {}
            StrideChanger( size_t _offsetAfterPadding, size_t _paddedBytes ) :
                offsetAfterPadding( _offsetAfterPadding ),
                paddedBytes( _paddedBytes )
            {
            }

            bool operator()( const StrideChanger &left, size_t right ) const
            {
                return left.offsetAfterPadding < right;
            }
            bool operator()( size_t left, const StrideChanger &right ) const
            {
                return left < right.offsetAfterPadding;
            }
            bool operator()( const StrideChanger &left, const StrideChanger &right ) const
            {
                return left.offsetAfterPadding < right.offsetAfterPadding;
            }
        };

        typedef vector<Block>::type BlockVec;
        typedef vector<StrideChanger>::type StrideChangerVec;

    protected:
        struct Vbo
        {
            // clang-format off
            VkDeviceMemory      vboName;
            VkBuffer            vkBuffer;
            size_t              sizeBytes;
            uint32              vkMemoryTypeIdx;
            VulkanDynamicBuffer *dynamicBuffer; //Null for CPU_INACCESSIBLE BOs.

            BlockVec            freeBlocks;
            StrideChangerVec    strideChangers;
            // clang-format on
        };

        struct Vao
        {
            uint32 vaoName;

            struct VertexBinding
            {
                VkBuffer vertexBufferVbo;
                VertexElement2Vec vertexElements;

                // OpenGL supports this parameter per attribute, but
                // we're a bit more conservative and do it per buffer
                uint32 instancingDivisor;

                bool operator==( const VertexBinding &_r ) const
                {
                    return vertexBufferVbo == _r.vertexBufferVbo &&
                           vertexElements == _r.vertexElements &&
                           instancingDivisor == _r.instancingDivisor;
                }
            };

            typedef vector<VertexBinding>::type VertexBindingVec;

            /// Not required anymore, however it's still useful for sorting
            /// purposes in the RenderQueue (using the Vao's ID).
            OperationType operationType;
            VertexBindingVec vertexBuffers;
            VkBuffer indexBufferVbo;
            IndexBufferPacked::IndexType indexType;
            uint32 refCount;
        };

        typedef vector<Vbo>::type VboVec;
        typedef vector<Vao>::type VaoVec;
        typedef map<VertexElement2Vec, Vbo>::type VboMap;

        FastArray<uint32> mBestVkMemoryTypeIndex[MAX_VBO_FLAG];
        uint32 mMemoryTypesInUse[MAX_VBO_FLAG];

        /// Note: mVbos[CPU_INACCESIBLE] may contain some Vbos from memory types which are not
        /// mBestVkMemoryTypeIndex.
        ///
        /// This can happen if a texture request couldn't be fullfilled but other memory
        /// types were available. If that's the case, then
        /// Vbo::vkMemoryTypeIdx & mBestVkMemoryTypeIndex[CPU_INACCESIBLE] will be 0
        VboVec mVbos[MAX_VBO_FLAG];
        size_t mDefaultPoolSize[MAX_VBO_FLAG];
        size_t mUsedHeapMemory[VK_MAX_MEMORY_HEAPS];

        VaoVec mVaos;
        uint32 mVaoNames;

        VertexBufferPacked *mDrawId;

        struct UsedSemaphore
        {
            VkSemaphore semaphore;
            uint32 frame;
            UsedSemaphore( VkSemaphore _semaphore, uint32 _frame ) :
                semaphore( _semaphore ),
                frame( _frame )
            {
            }
        };

        FastArray<UsedSemaphore> mUsedSemaphores;

        VkSemaphoreArray mAvailableSemaphores;

        VulkanDevice *mDevice;
        VulkanRenderSystem *mVkRenderSystem;

        typedef map<VkDescriptorSetLayout, FastArray<VulkanDescriptorPool *> >::type
            VulkanDescriptorPoolMap;

        VulkanDescriptorPoolMap mDescriptorPools;
        FastArray<VulkanDescriptorPool *> mUsedDescriptorPools;

        typedef FastArray<VulkanDelayedFuncBase *> VulkanDelayedFuncBaseArray;
        FastArray<VulkanDelayedFuncBaseArray> mDelayedFuncs;

        bool mFenceFlushed;
        bool mSupportsCoherentMemory;
        bool mSupportsNonCoherentMemory;
        bool mReadMemoryIsCoherent;

        static const uint32 VERTEX_ATTRIBUTE_INDEX[VES_COUNT];

    protected:
        void addMemoryType( VboFlag vboFlag, const VkPhysicalDeviceMemoryProperties &memProperties,
                            const uint32 memoryTypeIdx );
        uint32 determineSupportedMemoryTypes( VkBufferUsageFlags usageFlags ) const;
        void determineBestMemoryTypes( void );

        /** Asks for allocating buffer space in a memory pool.
            If the VBO doesn't exist, all VBOs are full or can't fit this request,
            then a new VBO will be created.
        @remarks
            Can throw if out of video memory

            The 'VBO' stands for Vertex Buffer Object, which is only a remnant from
            how we did things in OpenGL years ago. Today in Ogre we use it as
            synonym for 'Handle to Device Memory"
        @param sizeBytes
            The requested size, in bytes.
        @param bytesPerElement
            The number of bytes per vertex or per index (i.e. 16-bit indices = 2).
            Cannot be 0.
        @param bufferType
            The type of buffer
        @param outVboIdx [out]
            The index to the mVbos.
        @param outBufferOffset [out]
            The offset in bytes at which the buffer data should be placed.
        */
        void allocateVbo( size_t sizeBytes, size_t alignment, BufferType bufferType, bool readCapable,
                          size_t &outVboIdx, size_t &outBufferOffset );

        void allocateVbo( size_t sizeBytes, size_t alignment, VboFlag vboFlag, uint32 textureMemTypeBits,
                          size_t &outVboIdx, size_t &outBufferOffset );

        /** Deallocates a buffer allocated with VulkanVaoManager::allocateVbo.
        @remarks
            All four parameters *must* match with the ones provided to or
            returned from allocateVbo, otherwise the behavior is undefined.
        @param vboIdx
            The index to the mVbos pool that was returned by allocateVbo
        @param bufferOffset
            The buffer offset that was returned by allocateVbo
        @param sizeBytes
            The sizeBytes parameter that was passed to allocateVbos.
        @param bufferType
            The type of buffer that was passed to allocateVbo.
        */
        void deallocateVbo( size_t vboIdx, size_t bufferOffset, size_t sizeBytes, BufferType bufferType,
                            bool readCapable );

        void deallocateVbo( size_t vboIdx, size_t bufferOffset, size_t sizeBytes, VboVec &vboVec );

        virtual VertexBufferPacked *createVertexBufferImpl( size_t numElements, uint32 bytesPerElement,
                                                            BufferType bufferType, void *initialData,
                                                            bool keepAsShadow,
                                                            const VertexElement2Vec &vertexElements );

        virtual void destroyVertexBufferImpl( VertexBufferPacked *vertexBuffer );

        virtual MultiSourceVertexBufferPool *createMultiSourceVertexBufferPoolImpl(
            const VertexElement2VecVec &vertexElementsBySource, size_t maxNumVertices,
            size_t totalBytesPerVertex, BufferType bufferType );

        virtual IndexBufferPacked *createIndexBufferImpl( size_t numElements, uint32 bytesPerElement,
                                                          BufferType bufferType, void *initialData,
                                                          bool keepAsShadow );

        virtual void destroyIndexBufferImpl( IndexBufferPacked *indexBuffer );

        virtual ConstBufferPacked *createConstBufferImpl( size_t sizeBytes, BufferType bufferType,
                                                          void *initialData, bool keepAsShadow );
        virtual void destroyConstBufferImpl( ConstBufferPacked *constBuffer );

        virtual TexBufferPacked *createTexBufferImpl( PixelFormatGpu pixelFormat, size_t sizeBytes,
                                                      BufferType bufferType, void *initialData,
                                                      bool keepAsShadow );
        virtual void destroyTexBufferImpl( TexBufferPacked *texBuffer );

        virtual ReadOnlyBufferPacked *createReadOnlyBufferImpl( PixelFormatGpu pixelFormat,
                                                                size_t sizeBytes, BufferType bufferType,
                                                                void *initialData, bool keepAsShadow );
        virtual void destroyReadOnlyBufferImpl( ReadOnlyBufferPacked *readOnlyBuffer );

        virtual UavBufferPacked *createUavBufferImpl( size_t numElements, uint32 bytesPerElement,
                                                      uint32 bindFlags, void *initialData,
                                                      bool keepAsShadow );
        virtual void destroyUavBufferImpl( UavBufferPacked *uavBuffer );

        virtual IndirectBufferPacked *createIndirectBufferImpl( size_t sizeBytes, BufferType bufferType,
                                                                void *initialData, bool keepAsShadow );
        virtual void destroyIndirectBufferImpl( IndirectBufferPacked *indirectBuffer );

        virtual VertexArrayObject *createVertexArrayObjectImpl(
            const VertexBufferPackedVec &vertexBuffers, IndexBufferPacked *indexBuffer,
            OperationType opType );

        virtual void destroyVertexArrayObjectImpl( VertexArrayObject *vao );

        /// Finds the Vao. Calls createVao automatically if not found.
        /// Increases refCount before returning the iterator.
        VaoVec::iterator findVao( const VertexBufferPackedVec &vertexBuffers,
                                  IndexBufferPacked *indexBuffer, OperationType opType );
        uint32 createVao( void );

        static uint32 generateRenderQueueId( uint32 vaoName, uint32 uniqueVaoId );

        VboFlag bufferTypeToVboFlag( BufferType bufferType, const bool readCapable ) const;
        bool isVboFlagCoherent( VboFlag vboFlag ) const;

        virtual void switchVboPoolIndexImpl( size_t oldPoolIdx, size_t newPoolIdx,
                                             BufferPacked *buffer );

    public:
        VulkanVaoManager( VulkanDevice *device, VulkanRenderSystem *renderSystem,
                          const NameValuePairList *params );
        virtual ~VulkanVaoManager();

        void initDrawIdVertexBuffer();
        void bindDrawIdVertexBuffer( VkCommandBuffer cmdBuffer, uint32 binding = 15 );

        VkDeviceMemory allocateTexture( const VkMemoryRequirements &memReq, size_t &outVboIdx,
                                        size_t &outBufferOffset );
        void deallocateTexture( size_t vboIdx, size_t bufferOffset, size_t sizeBytes );

        VulkanRawBuffer allocateRawBuffer( VboFlag vboFlag, size_t sizeBytes, size_t alignment = 4u );
        void deallocateRawBuffer( VulkanRawBuffer &rawBuffer );

        void addDelayedFunc( VulkanDelayedFuncBase *cmd );

        virtual void getMemoryStats( MemoryStatsEntryVec &outStats, size_t &outCapacityBytes,
                                     size_t &outFreeBytes, Log *log ) const;

        virtual void cleanupEmptyPools( void );

        bool supportsCoherentMapping( void ) const;
        bool supportsNonCoherentMapping( void ) const;

        /** Creates a new staging buffer and adds it to the pool. @see getStagingBuffer.
        @remarks
            The returned buffer starts with a reference count of 1. You should decrease
            it when you're done using it.
        */
        virtual StagingBuffer *createStagingBuffer( size_t sizeBytes, bool forUpload );

        virtual AsyncTicketPtr createAsyncTicket( BufferPacked *creator, StagingBuffer *stagingBuffer,
                                                  size_t elementStart, size_t elementCount );

        VulkanDescriptorPool *getDescriptorPool( const VulkanRootLayout *rootLayout, size_t setIdx,
                                                 VkDescriptorSetLayout setLayout );
        void _schedulePoolAdvanceFrame( VulkanDescriptorPool *pool );

        virtual void _update( void );
        void _notifyNewCommandBuffer( void );

        VulkanDevice *getDevice( void ) const { return mDevice; }

        /// Insert into the end of semaphoreArray 'numSemaphores'
        /// number of semaphores that are safe for use.
        void getAvailableSempaphores( VkSemaphoreArray &semaphoreArray, size_t numSemaphores );
        VkSemaphore getAvailableSempaphore( void );

        /// Call this function after you've submitted to the GPU a VkSemaphore that will be waited on.
        /// i.e. 'semaphore' is part of VkSubmitInfo::pWaitSemaphores or part of
        /// VkPresentInfoKHR::pWaitSemaphores
        ///
        /// After enough frames have passed, this semaphore goes
        /// back to a pool for getAvailableSempaphores to use
        void notifyWaitSemaphoreSubmitted( VkSemaphore semaphore );
        void notifyWaitSemaphoresSubmitted( const VkSemaphoreArray &semaphores );
        void notifySemaphoreUnused( VkSemaphore semaphore );

        /// Returns the current frame # (which wraps to 0 every mDynamicBufferMultiplier
        /// times). But first stalls until that mDynamicBufferMultiplier-1 frame behind
        /// is finished.
        uint8 waitForTailFrameToFinish( void );

        /// See VaoManager::waitForSpecificFrameToFinish
        virtual void waitForSpecificFrameToFinish( uint32 frameCount );

        /// See VaoManager::isFrameFinished
        virtual bool isFrameFinished( uint32 frameCount );

        /** Will stall undefinitely until GPU finishes (signals the sync object).
        @param fenceName
            Sync object to wait for. Will be released on success. On failure,
            throws an exception and fenceName will not be released.
        @param queue
            VulkanQueue this fence is linked to.
        @returns
            Null ptr on success. Should throw on failure.
        */
        static VkFence waitFor( VkFence fenceName, VulkanQueue *queue );

        void _notifyDeviceStalled();
        VulkanStagingTexture *createStagingTexture( PixelFormatGpu formatFamily, size_t sizeBytes );
        void destroyStagingTexture( VulkanStagingTexture *stagingTexture );

        /// @see StagingBuffer::mergeContiguousBlocks
        static void mergeContiguousBlocks( BlockVec::iterator blockToMerge, BlockVec &blocks );

        VertexBufferPacked *getDrawId() const { return mDrawId; }

        static uint32 getAttributeIndexFor( VertexElementSemantic semantic );
    };
}  // namespace Ogre

#endif
