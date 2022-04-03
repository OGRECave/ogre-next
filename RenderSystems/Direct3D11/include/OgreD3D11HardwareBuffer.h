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
#ifndef __D3D11HARDWAREBUFFER_H__
#define __D3D11HARDWAREBUFFER_H__

#include "OgreD3D11Prerequisites.h"

#include "OgreD3D11DeviceResource.h"
#include "OgreHardwareBuffer.h"

namespace Ogre
{
    namespace v1
    {
        /** Base implementation of a D3D11 buffer, dealing with all the common
        aspects.
        */
        class _OgreD3D11Export D3D11HardwareBuffer final : public HardwareBuffer,
                                                           protected D3D11DeviceResource
        {
        public:
            enum BufferType
            {
                VERTEX_BUFFER,
                INDEX_BUFFER
            };

        protected:
            ComPtr<ID3D11Buffer> mlpD3DBuffer;
            bool                 mUseTempStagingBuffer;
            D3D11HardwareBuffer *mpTempStagingBuffer;
            bool                 mStagingUploadNeeded;

            BufferType        mBufferType;
            D3D11Device      &mDevice;
            D3D11_BUFFER_DESC mDesc;

            /** See HardwareBuffer. */
            void *lockImpl( size_t offset, size_t length, LockOptions options ) override;
            /** See HardwareBuffer. */
            void unlockImpl() override;

            void notifyDeviceLost( D3D11Device *device ) override;
            void notifyDeviceRestored( D3D11Device *device, unsigned pass ) override;

        public:
            D3D11HardwareBuffer( BufferType btype, size_t sizeBytes, HardwareBuffer::Usage usage,
                                 D3D11Device &device, bool useSystemMem, bool useShadowBuffer,
                                 bool streamOut );
            ~D3D11HardwareBuffer() override;
            /** See HardwareBuffer. */
            void readData( size_t offset, size_t length, void *pDest ) override;
            /** See HardwareBuffer. */
            void writeData( size_t offset, size_t length, const void *pSource,
                            bool discardWholeBuffer = false ) override;
            /** See HardwareBuffer. We perform a hardware copy here. */
            void copyData( HardwareBuffer &srcBuffer, size_t srcOffset, size_t dstOffset, size_t length,
                           bool discardWholeBuffer = false ) override;
            void copyDataImpl( HardwareBuffer &srcBuffer, size_t srcOffset, size_t dstOffset,
                               size_t length, bool discardWholeBuffer = false );
            /// Updates the real buffer from the shadow buffer, if required
            void _updateFromShadow() override;

            /// Get the D3D-specific buffer
            ID3D11Buffer *getD3DBuffer() { return mlpD3DBuffer.Get(); }
        };

    }  // namespace v1
}  // namespace Ogre

#endif
