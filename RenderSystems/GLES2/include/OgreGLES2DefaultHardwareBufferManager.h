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

#ifndef __GLES2DefaultHardwareBufferManager_H__
#define __GLES2DefaultHardwareBufferManager_H__

#include "OgreGLES2Prerequisites.h"
#include "OgreHardwareBufferManager.h"
#include "OgreHardwareVertexBuffer.h"
#include "OgreHardwareUniformBuffer.h"
#include "OgreHardwareIndexBuffer.h"

namespace Ogre {
namespace v1 {
    /// Specialisation of HardwareVertexBuffer for emulation
    class _OgreGLES2Export GLES2DefaultHardwareVertexBuffer : public HardwareVertexBuffer
    {
        protected:
            unsigned char* mData;
            /// @copydoc HardwareBuffer::lock
            void* lockImpl(size_t offset, size_t length, LockOptions options);
            /// @copydoc HardwareBuffer::unlock
            void unlockImpl();

        public:
            GLES2DefaultHardwareVertexBuffer(size_t vertexSize, size_t numVertices,
                                          HardwareBuffer::Usage usage);
            GLES2DefaultHardwareVertexBuffer(HardwareBufferManagerBase* mgr, size_t vertexSize, size_t numVertices, 
                                          HardwareBuffer::Usage usage);
            virtual ~GLES2DefaultHardwareVertexBuffer();
            /// @copydoc HardwareBuffer::readData
            void readData(size_t offset, size_t length, void* pDest);
            /// @copydoc HardwareBuffer::writeData
            void writeData(size_t offset, size_t length, const void* pSource,
                           bool discardWholeBuffer = false);
            /** Override HardwareBuffer to turn off all shadowing. */
            void* lock(size_t offset, size_t length, LockOptions options);
            /** Override HardwareBuffer to turn off all shadowing. */
            void unlock();

            void* getDataPtr(size_t offset) const { return (void*)(mData + offset); }
    };

    /// Specialisation of HardwareIndexBuffer for emulation
    class _OgreGLES2Export GLES2DefaultHardwareIndexBuffer : public HardwareIndexBuffer
    {
        protected:
            unsigned char* mData;
            /// @copydoc HardwareBuffer::lock
            void* lockImpl(size_t offset, size_t length, LockOptions options);
            /// @copydoc HardwareBuffer::unlock
            void unlockImpl();

        public:
            GLES2DefaultHardwareIndexBuffer(IndexType idxType, size_t numIndexes, HardwareBuffer::Usage usage);
            virtual ~GLES2DefaultHardwareIndexBuffer();
            /// @copydoc HardwareBuffer::readData
            void readData(size_t offset, size_t length, void* pDest);
            /// @copydoc HardwareBuffer::writeData
            void writeData(size_t offset, size_t length, const void* pSource,
                    bool discardWholeBuffer = false);
            /** Override HardwareBuffer to turn off all shadowing. */
            void* lock(size_t offset, size_t length, LockOptions options);
            /** Override HardwareBuffer to turn off all shadowing. */
            void unlock();

            void* getDataPtr(size_t offset) const { return (void*)(mData + offset); }
    };

    /// Specialisation of HardwareUniformBuffer for emulation
    class _OgreGLES2Export GLES2DefaultHardwareUniformBuffer : public HardwareUniformBuffer
    {
    protected:
        unsigned char* mData;
        /// @copydoc HardwareBuffer::lock
        void* lockImpl(size_t offset, size_t length, LockOptions options);
        /// @copydoc HardwareBuffer::unlock
        void unlockImpl();

    public:
        GLES2DefaultHardwareUniformBuffer(size_t bufferSize, HardwareBuffer::Usage usage, bool useShadowBuffer, const String& name);
        GLES2DefaultHardwareUniformBuffer(HardwareBufferManagerBase* mgr, size_t bufferSize,
                                            HardwareBuffer::Usage usage, bool useShadowBuffer, const String& name);
        ~GLES2DefaultHardwareUniformBuffer();
        /// @copydoc HardwareBuffer::readData
        void readData(size_t offset, size_t length, void* pDest);
        /// @copydoc HardwareBuffer::writeData
        void writeData(size_t offset, size_t length, const void* pSource,
                       bool discardWholeBuffer = false);
        /** Override HardwareBuffer to turn off all shadowing. */
        void* lock(size_t offset, size_t length, LockOptions options);
        /** Override HardwareBuffer to turn off all shadowing. */
        void unlock();

        void* getDataPtr(size_t offset) const { return (void*)(mData + offset); }
    };

    /** Specialisation of HardwareBufferManager to emulate hardware buffers.
    @remarks
        You might want to instantiate this class if you want to utilise
        classes like MeshSerializer without having initialised the 
        rendering system (which is required to create a 'real' hardware
        buffer manager.
    */
    class _OgreGLES2Export GLES2DefaultHardwareBufferManagerBase : public HardwareBufferManagerBase
    {
        public:
            GLES2DefaultHardwareBufferManagerBase();
            virtual ~GLES2DefaultHardwareBufferManagerBase();
            /// Creates a vertex buffer
            HardwareVertexBufferSharedPtr
                createVertexBuffer(size_t vertexSize, size_t numVerts,
                    HardwareBuffer::Usage usage, bool useShadowBuffer = false);
            /// Create a hardware vertex buffer
            HardwareIndexBufferSharedPtr
                createIndexBuffer(HardwareIndexBuffer::IndexType itype, size_t numIndexes,
                    HardwareBuffer::Usage usage, bool useShadowBuffer = false);
            /// Create a render to vertex buffer
            RenderToVertexBufferSharedPtr createRenderToVertexBuffer();

        HardwareUniformBufferSharedPtr
            createUniformBuffer(size_t sizeBytes, HardwareBuffer::Usage usage,bool useShadowBuffer, const String& name = "");

        HardwareCounterBufferSharedPtr createCounterBuffer(size_t sizeBytes,
                                                           HardwareBuffer::Usage usage = HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE,
                                                           bool useShadowBuffer = false, const String& name = "")
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "GLES2 does not support atomic counter buffers",
                        "GLES2DefaultHardwareBufferManagerBase::createCounterBuffer");
        }
    };

    /// GLES2DefaultHardwareBufferManagerBase as a Singleton
    class _OgreGLES2Export GLES2DefaultHardwareBufferManager : public HardwareBufferManager
    {
    public:
        GLES2DefaultHardwareBufferManager()
            : HardwareBufferManager(OGRE_NEW GLES2DefaultHardwareBufferManagerBase()) 
        {

        }
        ~GLES2DefaultHardwareBufferManager()
        {
            OGRE_DELETE mImpl;
        }
        HardwareUniformBufferSharedPtr
        createUniformBuffer(size_t sizeBytes, HardwareBuffer::Usage usage,bool useShadowBuffer, const String& name = "")
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "GLES does not support render to vertex buffer objects",
                        "GLES2DefaultHardwareBufferManager::createUniformBuffer");
        }
    };
}
}

#endif
