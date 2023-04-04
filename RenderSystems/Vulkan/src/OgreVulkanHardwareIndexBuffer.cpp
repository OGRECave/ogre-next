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

#include "OgreVulkanHardwareIndexBuffer.h"
#include "OgreVulkanDiscardBufferManager.h"
#include "OgreVulkanHardwareBufferManager.h"

namespace Ogre
{
    namespace v1
    {
#ifdef OGRE_VK_WORKAROUND_PVR_ALIGNMENT
#    define OGRE_VHIB_ALIGNMENT \
        Workarounds::mPowerVRAlignment ? uint16_t( Workarounds::mPowerVRAlignment ) : 4u
#else
#    define OGRE_VHIB_ALIGNMENT 4u
#endif
        VulkanHardwareIndexBuffer::VulkanHardwareIndexBuffer( VulkanHardwareBufferManagerBase *mgr,
                                                              IndexType idxType, size_t numIndexes,
                                                              HardwareBuffer::Usage usage,
                                                              bool useShadowBuffer ) :
            HardwareIndexBuffer( mgr, idxType, numIndexes, usage, false, useShadowBuffer ),
            mVulkanHardwareBufferCommon( mSizeInBytes, usage, OGRE_VHIB_ALIGNMENT,
                                         mgr->_getDiscardBufferManager(),
                                         mgr->_getDiscardBufferManager()->getDevice() )
        {
        }
        //-----------------------------------------------------------------------------------
        VulkanHardwareIndexBuffer::~VulkanHardwareIndexBuffer() {}
        //-----------------------------------------------------------------------------------
        void *VulkanHardwareIndexBuffer::lockImpl( size_t offset, size_t length, LockOptions options )
        {
            return mVulkanHardwareBufferCommon.lockImpl( offset, length, options, mIsLocked );
        }
        //-----------------------------------------------------------------------------------
        void VulkanHardwareIndexBuffer::unlockImpl()
        {
            mVulkanHardwareBufferCommon.unlockImpl( mLockStart, mLockSize );
        }
        //-----------------------------------------------------------------------------------
        void VulkanHardwareIndexBuffer::_notifyDeviceStalled()
        {
            mVulkanHardwareBufferCommon._notifyDeviceStalled();
        }
        //-----------------------------------------------------------------------------------
        VkBuffer VulkanHardwareIndexBuffer::getBufferName( size_t &outOffset )
        {
            return mVulkanHardwareBufferCommon.getBufferName( outOffset );
        }
        //-----------------------------------------------------------------------------------
        VkBuffer VulkanHardwareIndexBuffer::getBufferNameForGpuWrite( size_t &outOffset )
        {
            return mVulkanHardwareBufferCommon.getBufferNameForGpuWrite( outOffset );
        }
        //-----------------------------------------------------------------------------------
        void VulkanHardwareIndexBuffer::readData( size_t offset, size_t length, void *pDest )
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
        void VulkanHardwareIndexBuffer::writeData( size_t offset, size_t length, const void *pSource,
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
        void VulkanHardwareIndexBuffer::copyData( HardwareBuffer &srcBuffer, size_t srcOffset,
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
        void VulkanHardwareIndexBuffer::_updateFromShadow()
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
        void *VulkanHardwareIndexBuffer::getRenderSystemData() { return &mVulkanHardwareBufferCommon; }
    }  // namespace v1
}  // namespace Ogre
