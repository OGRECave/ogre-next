/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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

#include "Vao/OgreVulkanTexBufferPacked.h"

#include <vulkan/vulkan.h>



#include "OgreVulkanMappings.h"
#include "OgreVulkanUtils.h"
#include "Vao/OgreVulkanBufferInterface.h"
#include "Vao/OgreVulkanVaoManager.h"
#include "OgreVulkanDevice.h"

namespace Ogre
{
    VulkanTexBufferPacked::VulkanTexBufferPacked( size_t internalBufStartBytes, size_t numElements,
                                                  uint32 bytesPerElement, uint32 numElementsPadding,
                                                  BufferType bufferType, void *initialData,
                                                  bool keepAsShadow, VaoManager *vaoManager,
                                                  VulkanBufferInterface *bufferInterface,
                                                  PixelFormatGpu pf ) :
        TexBufferPacked( internalBufStartBytes, numElements, bytesPerElement, numElementsPadding,
                         bufferType, initialData, keepAsShadow, vaoManager, bufferInterface, pf ),
        mBufferView( 0 ),
        mPrevSizeBytes( -1 ),
        mPrevOffset( -1 ),
        mCurrentBinding( -1 ),
        mDirty( false )
    {
    }
    //-----------------------------------------------------------------------------------
    VulkanTexBufferPacked::~VulkanTexBufferPacked() {}

    void VulkanTexBufferPacked::bindBufferVS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        bindBuffer( slot, offset, sizeBytes );
    }

    void VulkanTexBufferPacked::bindBufferPS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        bindBuffer( slot, offset, sizeBytes );
    }

    void VulkanTexBufferPacked::bindBufferCS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        bindBuffer( slot, offset, sizeBytes );
    }

    void VulkanTexBufferPacked::bindBuffer( uint16 slot, size_t offset, size_t sizeBytes )
    {
        assert( dynamic_cast<VulkanBufferInterface *>( mBufferInterface ) );
        assert( offset < ( mNumElements * mBytesPerElement - 1 ) );
        assert( ( offset + sizeBytes ) <= mNumElements * mBytesPerElement );

        size_t currentSizeBytes = !sizeBytes ? ( mNumElements * mBytesPerElement - offset ) : sizeBytes;

        size_t currentOffset = mFinalBufferStart * mBytesPerElement + offset;

        if( currentSizeBytes != mPrevSizeBytes || currentOffset != mPrevOffset )
        {
            VulkanBufferInterface *bufferInterface =
                static_cast<VulkanBufferInterface *>( mBufferInterface );
            VulkanVaoManager *vulkanVaoManager = static_cast<VulkanVaoManager *>( mVaoManager );

            if( mBufferView != 0 )
            {
                // Destroy first? Is there any way to reuse these things instead of destroying and
                // recreating them??
                // vkDestroyBufferView( vulkanVaoManager->getDevice()->mDevice, mBufferView, 0 );
            }

            VkBufferViewCreateInfo bufferCreateInfo;
            makeVkStruct( bufferCreateInfo, VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO );
            bufferCreateInfo.buffer = bufferInterface->getVboName();
            bufferCreateInfo.format = VulkanMappings::get( mPixelFormat );
            bufferCreateInfo.offset = currentOffset;
            bufferCreateInfo.range = currentSizeBytes;

            checkVkResult(
                vkCreateBufferView( vulkanVaoManager->getDevice()->mDevice, &bufferCreateInfo, 0, &mBufferView ), "vkCreateBufferView" );

            mPrevSizeBytes = currentSizeBytes;
            mPrevOffset = currentOffset;
            mCurrentBinding = slot + OGRE_VULKAN_TEX_SLOT_START;
            mDirty = true;
        }
    }

    void VulkanTexBufferPacked::bindBufferForDescriptor( VkBuffer *buffers, VkDeviceSize *offsets,
                                                         size_t offset )
    {
        assert( dynamic_cast<VulkanBufferInterface *>( mBufferInterface ) );
        VulkanBufferInterface *bufferInterface = static_cast<VulkanBufferInterface *>( mBufferInterface );

        *buffers = bufferInterface->getVboName();
        *offsets = mFinalBufferStart * mBytesPerElement + offset;
    }

    //-----------------------------------------------------------------------------------
}  // namespace Ogre
