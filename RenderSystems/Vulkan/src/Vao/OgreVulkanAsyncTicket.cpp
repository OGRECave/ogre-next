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

#include "Vao/OgreVulkanAsyncTicket.h"

#include "OgreException.h"
#include "OgreVulkanQueue.h"
#include "OgreVulkanUtils.h"
#include "Vao/OgreStagingBuffer.h"
#include "Vao/OgreVulkanVaoManager.h"

namespace Ogre
{
    VulkanAsyncTicket::VulkanAsyncTicket( BufferPacked *creator, StagingBuffer *stagingBuffer,
                                          size_t elementStart, size_t elementCount,
                                          VulkanQueue *queue ) :
        AsyncTicket( creator, stagingBuffer, elementStart, elementCount ),
        mQueue( queue )
    {
        mFenceName = queue->acquireCurrentFence();
        // Flush now for accuracy with downloads.
        mQueue->commitAndNextCommandBuffer();
    }
    //-----------------------------------------------------------------------------------
    VulkanAsyncTicket::~VulkanAsyncTicket()
    {
        if( mFenceName )
            mQueue->releaseFence( mFenceName );
    }
    //-----------------------------------------------------------------------------------
    const void *VulkanAsyncTicket::mapImpl()
    {
        if( mFenceName )
            mFenceName = VulkanVaoManager::waitFor( mFenceName, mQueue );

        return mStagingBuffer->_mapForRead( mStagingBufferMapOffset,
                                            mElementCount * mCreator->getBytesPerElement() );
    }
    //-----------------------------------------------------------------------------------
    bool VulkanAsyncTicket::queryIsTransferDone()
    {
        bool retVal = false;

        if( mFenceName )
        {
            // Ask to return immediately and tell us about the fence
            VkResult result = vkWaitForFences( mQueue->mDevice, 1u, &mFenceName, VK_TRUE, 0 );
            if( result != VK_TIMEOUT )
            {
                mQueue->releaseFence( mFenceName );
                mFenceName = 0;

                checkVkResult( mQueue->mOwnerDevice, result, "vkWaitForFences" );
            }
        }
        else
        {
            retVal = true;
        }

        return retVal;
    }
}  // namespace Ogre
