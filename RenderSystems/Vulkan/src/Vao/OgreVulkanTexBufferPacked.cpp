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

#include "Vao/OgreVulkanTexBufferPacked.h"

#include "../OgreVulkanDelayedFuncs.h"
#include "OgreVulkanDevice.h"
#include "OgreVulkanMappings.h"
#include "OgreVulkanRenderSystem.h"
#include "OgreVulkanUtils.h"
#include "Vao/OgreVulkanBufferInterface.h"
#include "Vao/OgreVulkanVaoManager.h"

namespace Ogre
{
    VulkanTexBufferPacked::VulkanTexBufferPacked( size_t internalBufStartBytes, size_t numElements,
                                                  uint32 bytesPerElement, uint32 numElementsPadding,
                                                  BufferType bufferType, void *initialData,
                                                  bool keepAsShadow, VulkanRenderSystem *renderSystem,
                                                  VaoManager *vaoManager,
                                                  VulkanBufferInterface *bufferInterface,
                                                  PixelFormatGpu pf ) :
        TexBufferPacked( internalBufStartBytes, numElements, bytesPerElement, numElementsPadding,
                         bufferType, initialData, keepAsShadow, vaoManager, bufferInterface, pf ),
        mRenderSystem( renderSystem ),
        mCurrentCacheCursor( 0u )
    {
        memset( mCachedResourceViews, 0, sizeof( mCachedResourceViews ) );
    }
    //-------------------------------------------------------------------------
    VulkanTexBufferPacked::~VulkanTexBufferPacked()
    {
        VulkanVaoManager *vulkanVaoManager = static_cast<VulkanVaoManager *>( mVaoManager );
        for( size_t i = 0u; i < 16u; ++i )
        {
            if( mCachedResourceViews[i].mResourceView )
            {
                delayed_vkDestroyBufferView( mVaoManager, vulkanVaoManager->getDevice()->mDevice,
                                             mCachedResourceViews[i].mResourceView, 0 );
                mCachedResourceViews[i].mResourceView = 0;
            }
        }
    }
    //-------------------------------------------------------------------------
    VkBufferView VulkanTexBufferPacked::createResourceView( int cacheIdx, size_t offset,
                                                            size_t sizeBytes )
    {
        assert( cacheIdx < 16 );

        VulkanVaoManager *vulkanVaoManager = static_cast<VulkanVaoManager *>( mVaoManager );

        if( mCachedResourceViews[cacheIdx].mResourceView )
        {
            delayed_vkDestroyBufferView( mVaoManager, vulkanVaoManager->getDevice()->mDevice,
                                         mCachedResourceViews[cacheIdx].mResourceView, 0 );
            mCachedResourceViews[cacheIdx].mResourceView = 0;
        }

        mCachedResourceViews[cacheIdx].mOffset = ( mFinalBufferStart + offset ) * mBytesPerElement;
        mCachedResourceViews[cacheIdx].mSize = sizeBytes;

        VulkanBufferInterface *bufferInterface =
            static_cast<VulkanBufferInterface *>( mBufferInterface );

        VkBufferViewCreateInfo bufferCreateInfo;
        makeVkStruct( bufferCreateInfo, VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO );
        bufferCreateInfo.buffer = bufferInterface->getVboName();
        bufferCreateInfo.format = VulkanMappings::get( mPixelFormat );
        bufferCreateInfo.offset = mCachedResourceViews[cacheIdx].mOffset;
        bufferCreateInfo.range = mCachedResourceViews[cacheIdx].mSize;

        VkResult result = vkCreateBufferView( vulkanVaoManager->getDevice()->mDevice, &bufferCreateInfo,
                                              0, &mCachedResourceViews[cacheIdx].mResourceView );
        checkVkResult( vulkanVaoManager->getDevice(), result, "vkCreateBufferView" );

        mCurrentCacheCursor = uint8( ( cacheIdx + 1 ) % 16 );

        return mCachedResourceViews[cacheIdx].mResourceView;
    }
    //-------------------------------------------------------------------------
    VkBufferView VulkanTexBufferPacked::_bindBufferCommon( size_t offset, size_t sizeBytes )
    {
        OGRE_ASSERT_LOW( offset <= getTotalSizeBytes() );
        OGRE_ASSERT_LOW( sizeBytes <= getTotalSizeBytes() );
        OGRE_ASSERT_LOW( ( offset + sizeBytes ) <= getTotalSizeBytes() );

        sizeBytes = !sizeBytes ? ( mNumElements * mBytesPerElement - offset ) : sizeBytes;

        VkBufferView resourceView = 0;
        for( int i = 0; i < 16; ++i )
        {
            // Reuse resource views. Reuse res. views that are bigger than what's requested too.
            if( mFinalBufferStart + offset == mCachedResourceViews[i].mOffset &&
                sizeBytes <= mCachedResourceViews[i].mSize )
            {
                resourceView = mCachedResourceViews[i].mResourceView;
                break;
            }
            else if( !mCachedResourceViews[i].mResourceView )
            {
                // We create in-order. If we hit here, the next ones are also null pointers.
                resourceView = createResourceView( i, offset, sizeBytes );
                break;
            }
        }

        if( !resourceView )
        {
            // If we hit here, the cache is full and couldn't find a match.
            resourceView = createResourceView( mCurrentCacheCursor, offset, sizeBytes );
        }

        return resourceView;
    }
    //-------------------------------------------------------------------------
    void VulkanTexBufferPacked::bindBufferVS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        VkBufferView resourceView = _bindBufferCommon( offset, sizeBytes );
        mRenderSystem->_setTexBuffer( slot, resourceView );
    }
    //-------------------------------------------------------------------------
    void VulkanTexBufferPacked::bindBufferPS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        VkBufferView resourceView = _bindBufferCommon( offset, sizeBytes );
        mRenderSystem->_setTexBuffer( slot, resourceView );
    }
    //-------------------------------------------------------------------------
    void VulkanTexBufferPacked::bindBufferGS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        VkBufferView resourceView = _bindBufferCommon( offset, sizeBytes );
        mRenderSystem->_setTexBuffer( slot, resourceView );
    }
    //-------------------------------------------------------------------------
    void VulkanTexBufferPacked::bindBufferDS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        VkBufferView resourceView = _bindBufferCommon( offset, sizeBytes );
        mRenderSystem->_setTexBuffer( slot, resourceView );
    }
    //-------------------------------------------------------------------------
    void VulkanTexBufferPacked::bindBufferHS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        VkBufferView resourceView = _bindBufferCommon( offset, sizeBytes );
        mRenderSystem->_setTexBuffer( slot, resourceView );
    }
    //-------------------------------------------------------------------------
    void VulkanTexBufferPacked::bindBufferCS( uint16 slot, size_t offset, size_t sizeBytes )
    {
        VkBufferView resourceView = _bindBufferCommon( offset, sizeBytes );
        mRenderSystem->_setTexBuffer( slot, resourceView );
    }
    //-------------------------------------------------------------------------
    VkBufferView VulkanTexBufferPacked::createBufferView( size_t offset, size_t sizeBytes )
    {
        OGRE_ASSERT_LOW( offset <= getTotalSizeBytes() );
        OGRE_ASSERT_LOW( sizeBytes <= getTotalSizeBytes() );
        OGRE_ASSERT_LOW( ( offset + sizeBytes ) <= getTotalSizeBytes() );

        OGRE_ASSERT_HIGH( dynamic_cast<VulkanBufferInterface *>( mBufferInterface ) );
        VulkanBufferInterface *bufferInterface =
            static_cast<VulkanBufferInterface *>( mBufferInterface );

        sizeBytes = !sizeBytes ? ( mNumElements * mBytesPerElement - offset ) : sizeBytes;

        VulkanVaoManager *vulkanVaoManager = static_cast<VulkanVaoManager *>( mVaoManager );

        VkBufferViewCreateInfo bufferCreateInfo;
        makeVkStruct( bufferCreateInfo, VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO );
        bufferCreateInfo.buffer = bufferInterface->getVboName();
        bufferCreateInfo.format = VulkanMappings::get( mPixelFormat );
        bufferCreateInfo.offset = ( mFinalBufferStart + offset ) * mBytesPerElement;
        bufferCreateInfo.range = sizeBytes;

        VkBufferView retVal;
        VkResult result =
            vkCreateBufferView( vulkanVaoManager->getDevice()->mDevice, &bufferCreateInfo, 0, &retVal );
        checkVkResult( vulkanVaoManager->getDevice(), result, "vkCreateBufferView" );

        return retVal;
    }
}  // namespace Ogre
