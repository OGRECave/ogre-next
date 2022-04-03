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

#ifndef __GL3PlusDefaultHardwareBufferManager_H__
#define __GL3PlusDefaultHardwareBufferManager_H__

#include "OgreGL3PlusPrerequisites.h"

#include "OgreHardwareBufferManager.h"
#include "OgreHardwareIndexBuffer.h"
#include "OgreHardwareVertexBuffer.h"

namespace Ogre
{
    namespace v1
    {
        /// Specialisation of HardwareVertexBuffer for emulation
        class _OgreGL3PlusExport GL3PlusDefaultHardwareVertexBuffer final : public HardwareVertexBuffer
        {
        protected:
            unsigned char *mData;
            /// @copydoc HardwareBuffer::lock
            void *lockImpl( size_t offset, size_t length, LockOptions options ) override;
            /// @copydoc HardwareBuffer::unlock
            void unlockImpl() override;

        public:
            GL3PlusDefaultHardwareVertexBuffer( size_t vertexSize, size_t numVertices,
                                                HardwareBuffer::Usage usage );
            GL3PlusDefaultHardwareVertexBuffer( HardwareBufferManagerBase *mgr, size_t vertexSize,
                                                size_t numVertices, HardwareBuffer::Usage usage );
            ~GL3PlusDefaultHardwareVertexBuffer() override;
            /// @copydoc HardwareBuffer::readData
            void readData( size_t offset, size_t length, void *pDest ) override;
            /// @copydoc HardwareBuffer::writeData
            void writeData( size_t offset, size_t length, const void *pSource,
                            bool discardWholeBuffer = false ) override;
            /** Override HardwareBuffer to turn off all shadowing. */
            void *lock( size_t offset, size_t length, LockOptions options ) override;
            /** Override HardwareBuffer to turn off all shadowing. */
            void unlock() override;

            void *getDataPtr( size_t offset ) const { return (void *)( mData + offset ); }
        };

        /// Specialisation of HardwareIndexBuffer for emulation
        class _OgreGL3PlusExport GL3PlusDefaultHardwareIndexBuffer final : public HardwareIndexBuffer
        {
        protected:
            unsigned char *mData;
            /// @copydoc HardwareBuffer::lock
            void *lockImpl( size_t offset, size_t length, LockOptions options ) override;
            /// @copydoc HardwareBuffer::unlock
            void unlockImpl() override;

        public:
            GL3PlusDefaultHardwareIndexBuffer( IndexType idxType, size_t numIndexes,
                                               HardwareBuffer::Usage usage );
            ~GL3PlusDefaultHardwareIndexBuffer() override;
            /// @copydoc HardwareBuffer::readData
            void readData( size_t offset, size_t length, void *pDest ) override;
            /// @copydoc HardwareBuffer::writeData
            void writeData( size_t offset, size_t length, const void *pSource,
                            bool discardWholeBuffer = false ) override;
            /** Override HardwareBuffer to turn off all shadowing. */
            void *lock( size_t offset, size_t length, LockOptions options ) override;
            /** Override HardwareBuffer to turn off all shadowing. */
            void unlock() override;

            void *getDataPtr( size_t offset ) const { return (void *)( mData + offset ); }
        };

        /** Specialisation of HardwareBufferManager to emulate hardware buffers.
            @remarks
            You might want to instantiate this class if you want to utilise
            classes like MeshSerializer without having initialised the
            rendering system (which is required to create a 'real' hardware
            buffer manager).
        */
        class _OgreGL3PlusExport GL3PlusDefaultHardwareBufferManagerBase final
            : public HardwareBufferManagerBase
        {
        public:
            GL3PlusDefaultHardwareBufferManagerBase();
            ~GL3PlusDefaultHardwareBufferManagerBase() override;
            /// Creates a vertex buffer
            HardwareVertexBufferSharedPtr createVertexBuffer( size_t vertexSize, size_t numVerts,
                                                              HardwareBuffer::Usage usage,
                                                              bool useShadowBuffer = false ) override;
            /// Create a hardware index buffer
            HardwareIndexBufferSharedPtr createIndexBuffer( HardwareIndexBuffer::IndexType itype,
                                                            size_t                         numIndexes,
                                                            HardwareBuffer::Usage          usage,
                                                            bool useShadowBuffer = false ) override;
        };

        /// GL3PlusDefaultHardwareBufferManagerBase as a Singleton
        class _OgreGL3PlusExport GL3PlusDefaultHardwareBufferManager final : public HardwareBufferManager
        {
        public:
            GL3PlusDefaultHardwareBufferManager() :
                HardwareBufferManager( OGRE_NEW GL3PlusDefaultHardwareBufferManagerBase() )
            {
            }
            ~GL3PlusDefaultHardwareBufferManager() override { OGRE_DELETE mImpl; }
        };
    }  // namespace v1
}  // namespace Ogre
#endif
