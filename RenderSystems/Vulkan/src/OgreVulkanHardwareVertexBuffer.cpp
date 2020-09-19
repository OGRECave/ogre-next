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

#include "OgreVulkanDiscardBufferManager.h"
#include "OgreVulkanHardwareBufferManager.h"
#include "OgreVulkanHardwareVertexBuffer.h"

namespace Ogre
{
namespace v1
{
    VulkanHardwareVertexBuffer::VulkanHardwareVertexBuffer( VulkanHardwareBufferManagerBase *mgr,
                                                          size_t vertexSize, size_t numVertices,
                                                          HardwareBuffer::Usage usage,
                                                          bool useShadowBuffer ) :
        HardwareVertexBuffer( mgr, vertexSize, numVertices, usage, false, false ),
        mVulkanHardwareBufferCommon( mSizeInBytes, usage, 16, mgr->_getDiscardBufferManager(),
                                    mgr->_getDiscardBufferManager()->getDevice() )
    {
    }
    //-----------------------------------------------------------------------------------
    VulkanHardwareVertexBuffer::~VulkanHardwareVertexBuffer() {}
    //-----------------------------------------------------------------------------------
    void *VulkanHardwareVertexBuffer::lockImpl( size_t offset, size_t length, LockOptions options )
    {
        return mVulkanHardwareBufferCommon.lockImpl( offset, length, options, mIsLocked );
    }
    //-----------------------------------------------------------------------------------
    void VulkanHardwareVertexBuffer::unlockImpl( void )
    {
        mVulkanHardwareBufferCommon.unlockImpl( mLockStart, mLockSize );
    }
    //-----------------------------------------------------------------------------------
    void VulkanHardwareVertexBuffer::_notifyDeviceStalled( void )
    {
        mVulkanHardwareBufferCommon._notifyDeviceStalled();
    }
    //-----------------------------------------------------------------------------------
    VkBuffer VulkanHardwareVertexBuffer::getBufferName( size_t &outOffset )
    {
        return mVulkanHardwareBufferCommon.getBufferName( outOffset );
    }
    //-----------------------------------------------------------------------------------
    VkBuffer VulkanHardwareVertexBuffer::getBufferNameForGpuWrite( size_t &outOffset )
    {
        return mVulkanHardwareBufferCommon.getBufferNameForGpuWrite( outOffset );
    }
    //-----------------------------------------------------------------------------------
    void VulkanHardwareVertexBuffer::readData( size_t offset, size_t length, void *pDest )
    {
        if( mUseShadowBuffer )
        {
            void *srcData = mShadowBuffer->lock( offset, length, HBL_READ_ONLY );
            memcpy( pDest, srcData, length );
            mShadowBuffer->unlock();
        }
        else
        {
            mVulkanHardwareBufferCommon.readData( offset, length, pDest );
        }
    }
    //-----------------------------------------------------------------------------------
    void VulkanHardwareVertexBuffer::writeData( size_t offset, size_t length, const void *pSource,
                                               bool discardWholeBuffer )
    {
        // Update the shadow buffer
        if( mUseShadowBuffer )
        {
            void *destData =
                mShadowBuffer->lock( offset, length, discardWholeBuffer ? HBL_DISCARD : HBL_NORMAL );
            memcpy( destData, pSource, length );
            mShadowBuffer->unlock();
        }

        mVulkanHardwareBufferCommon.writeData(
            offset, length, pSource,
            discardWholeBuffer || ( offset == 0 && length == mSizeInBytes ) );
    }
    //-----------------------------------------------------------------------------------
    void VulkanHardwareVertexBuffer::copyData( HardwareBuffer &srcBuffer, size_t srcOffset,
                                              size_t dstOffset, size_t length,
                                              bool discardWholeBuffer )
    {
        if( srcBuffer.isSystemMemory() )
        {
            HardwareBuffer::copyData( srcBuffer, srcOffset, dstOffset, length, discardWholeBuffer );
        }
        else
        {
            VulkanHardwareBufferCommon *VulkanBuffer =
                reinterpret_cast<VulkanHardwareBufferCommon *>( srcBuffer.getRenderSystemData() );
            mVulkanHardwareBufferCommon.copyData(
                VulkanBuffer, srcOffset, dstOffset, length,
                discardWholeBuffer || ( dstOffset == 0 && length == mSizeInBytes ) );
        }
    }
    //-----------------------------------------------------------------------------------
    void VulkanHardwareVertexBuffer::_updateFromShadow( void )
    {
        if( mUseShadowBuffer && mShadowUpdated && !mSuppressHardwareUpdate )
        {
            const void *srcData = mShadowBuffer->lock( mLockStart, mLockSize, HBL_READ_ONLY );

            const bool discardBuffer = mLockStart == 0 && mLockSize == mSizeInBytes;
            mVulkanHardwareBufferCommon.writeData( mLockStart, mLockSize, srcData, discardBuffer );

            mShadowBuffer->unlock();
            mShadowUpdated = false;
        }
    }
    //-----------------------------------------------------------------------------------
    void *VulkanHardwareVertexBuffer::getRenderSystemData( void )
    {
        return &mVulkanHardwareBufferCommon;
    }
}  // namespace v1
}  // namespace Ogre
