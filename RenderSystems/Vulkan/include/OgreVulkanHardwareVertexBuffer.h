/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE
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

#ifndef _OgreVulkanHardwareVertexBuffer_H_
#define _OgreVulkanHardwareVertexBuffer_H_

#include "OgreVulkanHardwareBufferCommon.h"
#include "OgreHardwareVertexBuffer.h"

namespace Ogre
{
namespace v1
{
    class VulkanHardwareBufferManagerBase;

    class _OgreVulkanExport VulkanHardwareVertexBuffer : public HardwareVertexBuffer
    {
    private:
        VulkanHardwareBufferCommon mVulkanHardwareBufferCommon;
    protected:
        virtual void *lockImpl( size_t offset, size_t length, LockOptions options ) override;
        virtual void unlockImpl( void ) override;
    public:
        VulkanHardwareVertexBuffer( VulkanHardwareBufferManagerBase *mgr, size_t vertexSize,
                                    size_t numVertices,
                              HardwareBuffer::Usage usage, bool useShadowBuffer );
        virtual ~VulkanHardwareVertexBuffer();

        void _notifyDeviceStalled( void );

        /// @copydoc VulkanHardwareBufferCommon::getBufferName
        VkBuffer getBufferName( size_t &outOffset );
        /// @copydoc VulkanHardwareBufferCommon::getBufferNameForGpuWrite
        VkBuffer getBufferNameForGpuWrite( size_t &outOffset );

        virtual void readData( size_t offset, size_t length, void *pDest ) override;
        
        virtual void writeData( size_t offset, size_t length, const void *pSource,
                                bool discardWholeBuffer = false ) override;
        virtual void copyData( HardwareBuffer &srcBuffer, size_t srcOffset, size_t dstOffset,
                               size_t length, bool discardWholeBuffer = false );

        virtual void _updateFromShadow( void );

        virtual void *getRenderSystemData( void );
    };
}
}

#endif
