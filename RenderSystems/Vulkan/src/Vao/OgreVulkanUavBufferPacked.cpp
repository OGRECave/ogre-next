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

#include "Vao/OgreVulkanUavBufferPacked.h"

#include "Vao/OgreVulkanBufferInterface.h"
#include "Vao/OgreVulkanReadOnlyBufferPacked.h"
#include "Vao/OgreVulkanTexBufferPacked.h"
#include "Vao/OgreVulkanVaoManager.h"

#include "OgreException.h"

namespace Ogre
{
    VulkanUavBufferPacked::VulkanUavBufferPacked( size_t internalBufStartBytes, size_t numElements,
                                                  uint32 bytesPerElement, uint32 bindFlags,
                                                  void *initialData, bool keepAsShadow,
                                                  VaoManager *vaoManager,
                                                  VulkanBufferInterface *bufferInterface ) :
        UavBufferPacked( internalBufStartBytes, numElements, bytesPerElement, bindFlags, initialData,
                         keepAsShadow, vaoManager, bufferInterface )
    {
    }
    //-----------------------------------------------------------------------------------
    VulkanUavBufferPacked::~VulkanUavBufferPacked() {}
    //-----------------------------------------------------------------------------------
    TexBufferPacked *VulkanUavBufferPacked::getAsTexBufferImpl( PixelFormatGpu pixelFormat )
    {
        OGRE_ASSERT_HIGH( dynamic_cast<VulkanBufferInterface *>( mBufferInterface ) );

        VulkanBufferInterface *bufferInterface =
            static_cast<VulkanBufferInterface *>( mBufferInterface );

        TexBufferPacked *retVal = OGRE_NEW VulkanTexBufferPacked(
            mInternalBufferStart * mBytesPerElement, mNumElements, mBytesPerElement, 0, mBufferType,
            (void *)0, false, (VulkanRenderSystem *)0, mVaoManager, bufferInterface, pixelFormat );
        // We were overridden by the BufferPacked we just created. Restore this back!
        bufferInterface->_notifyBuffer( this );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    ReadOnlyBufferPacked *VulkanUavBufferPacked::getAsReadOnlyBufferImpl()
    {
#ifdef OGRE_VK_WORKAROUND_ADRENO_6xx_READONLY_IS_TBUFFER
        if( Workarounds::mAdreno6xxReadOnlyIsTBuffer )
        {
            OGRE_EXCEPT(
                Exception::ERR_RENDERINGAPI_ERROR,
                "This Adreno driver version does not support this operation due to applied workarounds.",
                "VulkanUavBufferPacked::getAsReadOnlyBufferImpl" );
        }
#endif
        OGRE_ASSERT_HIGH( dynamic_cast<VulkanBufferInterface *>( mBufferInterface ) );

        VulkanBufferInterface *bufferInterface =
            static_cast<VulkanBufferInterface *>( mBufferInterface );

        VulkanRenderSystem *vkRenderSystem =
            static_cast<VulkanVaoManager *>( mVaoManager )->getVkRenderSystem();

        ReadOnlyBufferPacked *retVal = OGRE_NEW VulkanReadOnlyBufferPacked(
            mInternalBufferStart * mBytesPerElement, mNumElements, mBytesPerElement, 0, mBufferType,
            (void *)0, false, vkRenderSystem, mVaoManager, bufferInterface, PFG_NULL );
        // We were overridden by the BufferPacked we just created. Restore this back!
        bufferInterface->_notifyBuffer( this );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanUavBufferPacked::bindBufferCS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "Use DescriptorSetUav instead",
                     "VulkanUavBufferPacked::bindBufferCS" );
    }
    //-----------------------------------------------------------------------------------
    void VulkanUavBufferPacked::setupBufferInfo( VkDescriptorBufferInfo &outBufferInfo, size_t offset,
                                                 size_t sizeBytes )
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
