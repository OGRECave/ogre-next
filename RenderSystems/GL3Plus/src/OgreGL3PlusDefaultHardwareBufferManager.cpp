/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#include "OgreGL3PlusDefaultHardwareBufferManager.h"

namespace Ogre
{
    namespace v1
    {
        GL3PlusDefaultHardwareVertexBuffer::GL3PlusDefaultHardwareVertexBuffer(
            size_t vertexSize, size_t numVertices, HardwareBuffer::Usage usage ) :
            HardwareVertexBuffer( 0, vertexSize, numVertices, usage, true, false )
        {
            mData =
                static_cast<unsigned char *>( OGRE_MALLOC_SIMD( mSizeInBytes, MEMCATEGORY_GEOMETRY ) );
        }

        GL3PlusDefaultHardwareVertexBuffer::GL3PlusDefaultHardwareVertexBuffer(
            HardwareBufferManagerBase *mgr, size_t vertexSize, size_t numVertices,
            HardwareBuffer::Usage usage ) :
            HardwareVertexBuffer( mgr, vertexSize, numVertices, usage, true, false )
        {
            mData =
                static_cast<unsigned char *>( OGRE_MALLOC_SIMD( mSizeInBytes, MEMCATEGORY_GEOMETRY ) );
        }

        GL3PlusDefaultHardwareVertexBuffer::~GL3PlusDefaultHardwareVertexBuffer()
        {
            OGRE_FREE_SIMD( mData, MEMCATEGORY_GEOMETRY );
        }

        void *GL3PlusDefaultHardwareVertexBuffer::lockImpl( size_t offset, size_t length,
                                                            LockOptions options )
        {
            return mData + offset;
        }

        void GL3PlusDefaultHardwareVertexBuffer::unlockImpl()
        {
            // Nothing to do
        }

        void *GL3PlusDefaultHardwareVertexBuffer::lock( size_t offset, size_t length,
                                                        LockOptions options )
        {
            mIsLocked = true;
            return mData + offset;
        }

        void GL3PlusDefaultHardwareVertexBuffer::unlock()
        {
            mIsLocked = false;
            // Nothing to do
        }

        void GL3PlusDefaultHardwareVertexBuffer::readData( size_t offset, size_t length, void *pDest )
        {
            assert( ( offset + length ) <= mSizeInBytes );
            memcpy( pDest, mData + offset, length );
        }

        void GL3PlusDefaultHardwareVertexBuffer::writeData( size_t offset, size_t length,
                                                            const void *pSource,
                                                            bool discardWholeBuffer )
        {
            assert( ( offset + length ) <= mSizeInBytes );
            // ignore discard, memory is not guaranteed to be zeroised
            memcpy( mData + offset, pSource, length );
        }

        GL3PlusDefaultHardwareIndexBuffer::GL3PlusDefaultHardwareIndexBuffer(
            IndexType idxType, size_t numIndexes, HardwareBuffer::Usage usage ) :
            HardwareIndexBuffer( 0, idxType, numIndexes, usage, true, false )
        // always software, never shadowed
        {
            mData = new unsigned char[mSizeInBytes];
        }

        GL3PlusDefaultHardwareIndexBuffer::~GL3PlusDefaultHardwareIndexBuffer() { delete[] mData; }

        void *GL3PlusDefaultHardwareIndexBuffer::lockImpl( size_t offset, size_t length,
                                                           LockOptions options )
        {
            // Only for use internally, no 'locking' as such
            return mData + offset;
        }

        void GL3PlusDefaultHardwareIndexBuffer::unlockImpl()
        {
            // Nothing to do
        }

        void *GL3PlusDefaultHardwareIndexBuffer::lock( size_t offset, size_t length,
                                                       LockOptions options )
        {
            mIsLocked = true;
            return mData + offset;
        }

        void GL3PlusDefaultHardwareIndexBuffer::unlock()
        {
            mIsLocked = false;
            // Nothing to do
        }

        void GL3PlusDefaultHardwareIndexBuffer::readData( size_t offset, size_t length, void *pDest )
        {
            assert( ( offset + length ) <= mSizeInBytes );
            memcpy( pDest, mData + offset, length );
        }

        void GL3PlusDefaultHardwareIndexBuffer::writeData( size_t offset, size_t length,
                                                           const void *pSource, bool discardWholeBuffer )
        {
            assert( ( offset + length ) <= mSizeInBytes );
            // ignore discard, memory is not guaranteed to be zeroised
            memcpy( mData + offset, pSource, length );
        }

        GL3PlusDefaultHardwareBufferManagerBase::GL3PlusDefaultHardwareBufferManagerBase() {}

        GL3PlusDefaultHardwareBufferManagerBase::~GL3PlusDefaultHardwareBufferManagerBase()
        {
            destroyAllDeclarations();
            destroyAllBindings();
        }

        HardwareVertexBufferSharedPtr GL3PlusDefaultHardwareBufferManagerBase::createVertexBuffer(
            size_t vertexSize, size_t numVerts, HardwareBuffer::Usage usage, bool useShadowBuffer )
        {
            return HardwareVertexBufferSharedPtr(
                new GL3PlusDefaultHardwareVertexBuffer( this, vertexSize, numVerts, usage ) );
        }

        HardwareIndexBufferSharedPtr GL3PlusDefaultHardwareBufferManagerBase::createIndexBuffer(
            HardwareIndexBuffer::IndexType itype, size_t numIndexes, HardwareBuffer::Usage usage,
            bool useShadowBuffer )
        {
            return HardwareIndexBufferSharedPtr(
                new GL3PlusDefaultHardwareIndexBuffer( itype, numIndexes, usage ) );
        }
    }  // namespace v1
}  // namespace Ogre
