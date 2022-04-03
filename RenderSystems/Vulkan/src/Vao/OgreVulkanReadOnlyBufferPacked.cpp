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

#include "Vao/OgreVulkanReadOnlyBufferPacked.h"

#include "OgreVulkanRenderSystem.h"
#include "Vao/OgreVulkanBufferInterface.h"
#include "Vao/OgreVulkanTexBufferPacked.h"

#include "OgreException.h"

namespace Ogre
{
    VulkanReadOnlyBufferPacked::VulkanReadOnlyBufferPacked(
        size_t internalBufStartBytes, size_t numElements, uint32 bytesPerElement,
        uint32 numElementsPadding, BufferType bufferType, void *initialData, bool keepAsShadow,
        VulkanRenderSystem *renderSystem, VaoManager *vaoManager, VulkanBufferInterface *bufferInterface,
        PixelFormatGpu pf ) :
        ReadOnlyBufferPacked( internalBufStartBytes, numElements, bytesPerElement, numElementsPadding,
                              bufferType, initialData, keepAsShadow, vaoManager, bufferInterface, pf ),
        mRenderSystem( renderSystem )
    {
    }
    //-----------------------------------------------------------------------------------
    VulkanReadOnlyBufferPacked::~VulkanReadOnlyBufferPacked() {}
    //-------------------------------------------------------------------------
    void VulkanReadOnlyBufferPacked::bindBufferVS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        bindBuffer( slot, offset, sizeBytes );
    }
    //-------------------------------------------------------------------------
    void VulkanReadOnlyBufferPacked::bindBufferPS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        bindBuffer( slot, offset, sizeBytes );
    }
    //-------------------------------------------------------------------------
    void VulkanReadOnlyBufferPacked::bindBufferGS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        bindBuffer( slot, offset, sizeBytes );
    }
    //-------------------------------------------------------------------------
    void VulkanReadOnlyBufferPacked::bindBufferHS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        bindBuffer( slot, offset, sizeBytes );
    }
    //-------------------------------------------------------------------------
    void VulkanReadOnlyBufferPacked::bindBufferDS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        bindBuffer( slot, offset, sizeBytes );
    }
    //-----------------------------------------------------------------------------------
    void VulkanReadOnlyBufferPacked::bindBufferCS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "Use DescriptorSetTexture2 instead",
                     "VulkanReadOnlyBufferPacked::bindBufferCS" );
    }
    //-------------------------------------------------------------------------
    void VulkanReadOnlyBufferPacked::bindBuffer( uint16 slot, size_t offset, size_t sizeBytes )
    {
        OGRE_ASSERT_LOW( offset <= getTotalSizeBytes() );
        OGRE_ASSERT_LOW( sizeBytes <= getTotalSizeBytes() );
        OGRE_ASSERT_LOW( ( offset + sizeBytes ) <= getTotalSizeBytes() );

        sizeBytes = !sizeBytes ? ( mNumElements * mBytesPerElement - offset ) : sizeBytes;

        OGRE_ASSERT_HIGH( dynamic_cast<VulkanBufferInterface *>( mBufferInterface ) );
        VulkanBufferInterface *bufferInterface =
            static_cast<VulkanBufferInterface *>( mBufferInterface );

        VkDescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = bufferInterface->getVboName();
        bufferInfo.offset = mFinalBufferStart * mBytesPerElement + offset;
        bufferInfo.range = sizeBytes;
        mRenderSystem->_setReadOnlyBuffer( slot, bufferInfo );
    }
    //-----------------------------------------------------------------------------------
    void VulkanReadOnlyBufferPacked::setupBufferInfo( VkDescriptorBufferInfo &outBufferInfo,
                                                      size_t offset, size_t sizeBytes )
    {
        OGRE_ASSERT_LOW( offset <= getTotalSizeBytes() );
        OGRE_ASSERT_LOW( sizeBytes <= getTotalSizeBytes() );
        OGRE_ASSERT_LOW( ( offset + sizeBytes ) <= getTotalSizeBytes() );

        sizeBytes = !sizeBytes ? ( mNumElements * mBytesPerElement - offset ) : sizeBytes;

        OGRE_ASSERT_HIGH( dynamic_cast<VulkanBufferInterface *>( mBufferInterface ) );
        VulkanBufferInterface *bufferInterface =
            static_cast<VulkanBufferInterface *>( mBufferInterface );

        outBufferInfo.buffer = bufferInterface->getVboName();
        outBufferInfo.offset = mFinalBufferStart * mBytesPerElement + offset;
        outBufferInfo.range = sizeBytes;
    }
}  // namespace Ogre
