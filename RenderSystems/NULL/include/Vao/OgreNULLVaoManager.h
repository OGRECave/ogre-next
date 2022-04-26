/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#ifndef _Ogre_NULLVaoManager_H_
#define _Ogre_NULLVaoManager_H_

#include "OgreNULLPrerequisites.h"

#include "Vao/OgreVaoManager.h"

namespace Ogre
{
    class _OgreNULLExport NULLVaoManager : public VaoManager
    {
    protected:
        enum VboFlag
        {
            CPU_INACCESSIBLE,
            CPU_ACCESSIBLE_DEFAULT,
            CPU_ACCESSIBLE_PERSISTENT,
            CPU_ACCESSIBLE_PERSISTENT_COHERENT,
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

        typedef vector<Block>::type         BlockVec;
        typedef vector<StrideChanger>::type StrideChangerVec;

    protected:
        struct Vbo
        {
            size_t sizeBytes;

            BlockVec         freeBlocks;
            StrideChangerVec strideChangers;
        };

        struct Vao
        {
            struct VertexBinding
            {
                // GLuint              vertexBufferVbo;
                VertexElement2Vec vertexElements;
                uint32            stride;
                size_t            offset;

                // OpenGL supports this parameter per attribute, but
                // we're a bit more conservative and do it per buffer
                uint32 instancingDivisor;

                bool operator==( const VertexBinding &_r ) const
                {
                    return  // vertexBufferVbo == _r.vertexBufferVbo &&
                        vertexElements == _r.vertexElements && stride == _r.stride &&
                        offset == _r.offset && instancingDivisor == _r.instancingDivisor;
                }
            };

            typedef vector<VertexBinding>::type VertexBindingVec;

            VertexBindingVec             vertexBuffers;
            uint32                       indexBufferVbo;
            IndexBufferPacked::IndexType indexType;
            uint32                       refCount;
        };

        typedef vector<Vbo>::type VboVec;
        typedef vector<Vao>::type VaoVec;

        VboVec mVbos[MAX_VBO_FLAG];

        VaoVec mVaos;

        VertexBufferPacked *mDrawId;

    protected:
        VertexBufferPacked *createVertexBufferImpl( size_t numElements, uint32 bytesPerElement,
                                                    BufferType bufferType, void *initialData,
                                                    bool                     keepAsShadow,
                                                    const VertexElement2Vec &vertexElements ) override;

        void destroyVertexBufferImpl( VertexBufferPacked *vertexBuffer ) override;

#ifdef _OGRE_MULTISOURCE_VBO
        virtual MultiSourceVertexBufferPool *createMultiSourceVertexBufferPoolImpl(
            const VertexElement2VecVec &vertexElementsBySource, size_t maxNumVertices,
            size_t totalBytesPerVertex, BufferType bufferType );
#endif

        IndexBufferPacked *createIndexBufferImpl( size_t numElements, uint32 bytesPerElement,
                                                  BufferType bufferType, void *initialData,
                                                  bool keepAsShadow ) override;

        void destroyIndexBufferImpl( IndexBufferPacked *indexBuffer ) override;

        ConstBufferPacked *createConstBufferImpl( size_t sizeBytes, BufferType bufferType,
                                                  void *initialData, bool keepAsShadow ) override;
        void               destroyConstBufferImpl( ConstBufferPacked *constBuffer ) override;

        TexBufferPacked *createTexBufferImpl( PixelFormatGpu pixelFormat, size_t sizeBytes,
                                              BufferType bufferType, void *initialData,
                                              bool keepAsShadow ) override;
        void             destroyTexBufferImpl( TexBufferPacked *texBuffer ) override;

        ReadOnlyBufferPacked *createReadOnlyBufferImpl( PixelFormatGpu pixelFormat, size_t sizeBytes,
                                                        BufferType bufferType, void *initialData,
                                                        bool keepAsShadow ) override;
        void                  destroyReadOnlyBufferImpl( ReadOnlyBufferPacked *readOnlyBuffer ) override;

        UavBufferPacked *createUavBufferImpl( size_t numElements, uint32 bytesPerElement,
                                              uint32 bindFlags, void *initialData,
                                              bool keepAsShadow ) override;
        void             destroyUavBufferImpl( UavBufferPacked *uavBuffer ) override;

        IndirectBufferPacked *createIndirectBufferImpl( size_t sizeBytes, BufferType bufferType,
                                                        void *initialData, bool keepAsShadow ) override;
        void                  destroyIndirectBufferImpl( IndirectBufferPacked *indirectBuffer ) override;

        VertexArrayObject *createVertexArrayObjectImpl( const VertexBufferPackedVec &vertexBuffers,
                                                        IndexBufferPacked           *indexBuffer,
                                                        OperationType                opType ) override;

        void destroyVertexArrayObjectImpl( VertexArrayObject *vao ) override;

        static VboFlag bufferTypeToVboFlag( BufferType bufferType );

        void switchVboPoolIndexImpl( unsigned internalVboBufferType, size_t oldPoolIdx,
                                     size_t newPoolIdx, BufferPacked *buffer ) override;

    public:
        NULLVaoManager();
        ~NULLVaoManager() override;

        void getMemoryStats( MemoryStatsEntryVec &outStats, size_t &outCapacityBytes,
                             size_t &outFreeBytes, Log *log, bool &outIncludesTextures ) const override;

        void cleanupEmptyPools() override;

        bool supportsArbBufferStorage() const { return false; }

        /** Creates a new staging buffer and adds it to the pool. @see getStagingBuffer.
        @remarks
            The returned buffer starts with a reference count of 1. You should decrease
            it when you're done using it.
        */
        StagingBuffer *createStagingBuffer( size_t sizeBytes, bool forUpload ) override;

        AsyncTicketPtr createAsyncTicket( BufferPacked *creator, StagingBuffer *stagingBuffer,
                                          size_t elementStart, size_t elementCount ) override;

        void _update() override;

        /// Returns the current frame # (which wraps to 0 every mDynamicBufferMultiplier
        /// times). But first stalls until that mDynamicBufferMultiplier-1 frame behind
        /// is finished.
        uint8 waitForTailFrameToFinish() override;
        void  waitForSpecificFrameToFinish( uint32 frameCount ) override;
        bool  isFrameFinished( uint32 frameCount ) override;
    };
}  // namespace Ogre

#endif
