/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

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
#ifndef _OgreMetalHardwareVertexBuffer_H_
#define _OgreMetalHardwareVertexBuffer_H_

#include "OgreHardwareVertexBuffer.h"

#include "OgreMetalHardwareBufferCommon.h"
#include "OgreMetalHardwareBufferManager.h"

namespace Ogre
{
    namespace v1
    {
        /// Specialisation of HardwareVertexBuffer for Metal
        class _OgreMetalExport MetalHardwareVertexBuffer final : public HardwareVertexBuffer
        {
        private:
            MetalHardwareBufferCommon mMetalHardwareBufferCommon;

        protected:
            void *lockImpl( size_t offset, size_t length, LockOptions options ) override;
            void  unlockImpl() override;

        public:
            MetalHardwareVertexBuffer( MetalHardwareBufferManagerBase *mgr, size_t vertexSize,
                                       size_t numVertices, HardwareBuffer::Usage usage,
                                       bool useShadowBuffer );
            ~MetalHardwareVertexBuffer() override;

            void _notifyDeviceStalled();

            /// @copydoc MetalHardwareBufferCommon::getBufferName
            id<MTLBuffer> getBufferName( size_t &outOffset );
            /// @copydoc MetalHardwareBufferCommon::getBufferNameForGpuWrite
            id<MTLBuffer> getBufferNameForGpuWrite();

            void readData( size_t offset, size_t length, void *pDest ) override;
            void writeData( size_t offset, size_t length, const void *pSource,
                            bool discardWholeBuffer = false ) override;
            void copyData( HardwareBuffer &srcBuffer, size_t srcOffset, size_t dstOffset, size_t length,
                           bool discardWholeBuffer = false ) override;

            void _updateFromShadow() override;

            void *getRenderSystemData() override;
        };
    }  // namespace v1
}  // namespace Ogre
#endif  // __MetalHARDWAREVERTEXBUFFER_H__
