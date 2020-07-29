/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

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

#include "OgreVulkanStagingTexture.h"

#include "OgreVulkanDevice.h"
#include "OgreVulkanTextureGpu.h"
#include "OgreVulkanUtils.h"
#include "Vao/OgreVulkanDynamicBuffer.h"
#include "Vao/OgreVulkanVaoManager.h"

#define TODO_deallocate_StagingBufferVbo  // Cannot be destructor

namespace Ogre
{
    VulkanStagingTexture::VulkanStagingTexture( VaoManager *vaoManager, size_t vboIdx, size_t bufferOffset, PixelFormatGpu formatFamily,
                                                size_t size, VkBuffer vboName,
                                                VulkanDynamicBuffer *dynamicBuffer ) :
        StagingTextureBufferImpl( vaoManager, formatFamily, size, 0, 0 ),
        mVboIdx( vboIdx ),
        mBufferOffset( bufferOffset ),
        mVboName( vboName ),
        mDynamicBuffer( dynamicBuffer ),
        mUnmapTicket( std::numeric_limits<size_t>::max() ),
        mMappedPtr( 0 ),
        mLastMappedPtr( 0 )
    {
        mMappedPtr = mDynamicBuffer;
        mLastMappedPtr = mMappedPtr;
    }
    //-----------------------------------------------------------------------------------
    VulkanStagingTexture::~VulkanStagingTexture()
    {
        if( mDynamicBuffer )
            mDynamicBuffer = 0;
        mMappedPtr = 0;

        VulkanVaoManager *vaoManager = static_cast<VulkanVaoManager *>( mVaoManager );
        vaoManager->deallocateVbo( mVboIdx, mBufferOffset, mSize,
                                   BT_DYNAMIC_DEFAULT );
    }
    //-----------------------------------------------------------------------------------
    bool VulkanStagingTexture::belongsToUs( const TextureBox &box )
    {
        return box.data >= mLastMappedPtr &&
               box.data <= static_cast<uint8 *>( mLastMappedPtr ) + mCurrentOffset;
    }
    //-----------------------------------------------------------------------------------
    void *RESTRICT_ALIAS_RETURN VulkanStagingTexture::mapRegionImplRawPtr( void )
    {
        return static_cast<uint8 *>( mMappedPtr ) + mCurrentOffset;
    }
    //-----------------------------------------------------------------------------------
    void VulkanStagingTexture::_unmapBuffer() { mMappedPtr = 0; }
    //-----------------------------------------------------------------------------------
    void VulkanStagingTexture::startMapRegion()
    {
        StagingTextureBufferImpl::startMapRegion();

        OGRE_ASSERT_MEDIUM( mUnmapTicket == std::numeric_limits<size_t>::max() &&
                            "VulkanStagingTexture still mapped!" );
        mMappedPtr = mDynamicBuffer->map( mInternalBufferStart, mSize, mUnmapTicket );
        mLastMappedPtr = mMappedPtr;
    }
    //-----------------------------------------------------------------------------------
    void VulkanStagingTexture::stopMapRegion()
    {
        OGRE_ASSERT_MEDIUM( mUnmapTicket != std::numeric_limits<size_t>::max() &&
                            "VulkanStagingTexture already unmapped!" );

        mDynamicBuffer->flush( mUnmapTicket, mInternalBufferStart, mSize );
        mDynamicBuffer->unmap( mUnmapTicket );

        mMappedPtr = 0;

        StagingTextureBufferImpl::stopMapRegion();
    }
    //-----------------------------------------------------------------------------------
    void VulkanStagingTexture::upload( const TextureBox &srcBox, TextureGpu *dstTexture, uint8 mipLevel,
                                       const TextureBox *cpuSrcBox, const TextureBox *dstBox,
                                       bool skipSysRamCopy )
    {
        StagingTextureBufferImpl::upload( srcBox, dstTexture, mipLevel, cpuSrcBox, dstBox,
                                          skipSysRamCopy );

        VulkanVaoManager *vaoManager = static_cast<VulkanVaoManager *>( mVaoManager );
        VulkanDevice *device = vaoManager->getDevice();

        device->mGraphicsQueue.getCopyEncoder( 0, dstTexture, false );

        size_t bytesPerRow = srcBox.bytesPerRow;
        size_t bytesPerImage = srcBox.bytesPerImage;

        // PVR textures must have 0 on these.
        if( mFormatFamily == PFG_PVRTC2_2BPP || mFormatFamily == PFG_PVRTC2_2BPP_SRGB ||
            mFormatFamily == PFG_PVRTC2_4BPP || mFormatFamily == PFG_PVRTC2_4BPP_SRGB ||
            mFormatFamily == PFG_PVRTC_RGB2 || mFormatFamily == PFG_PVRTC_RGB2_SRGB ||
            mFormatFamily == PFG_PVRTC_RGB4 || mFormatFamily == PFG_PVRTC_RGB4_SRGB ||
            mFormatFamily == PFG_PVRTC_RGBA2 || mFormatFamily == PFG_PVRTC_RGBA2_SRGB ||
            mFormatFamily == PFG_PVRTC_RGBA4 || mFormatFamily == PFG_PVRTC_RGBA4_SRGB )
        {
            bytesPerRow = 0;
            bytesPerImage = 0;
        }

        // Recommended by documentation to set this value to 0 for non-3D textures.
        if( dstTexture->getTextureType() != TextureTypes::Type3D )
            bytesPerImage = 0;

        assert( dynamic_cast<VulkanTextureGpu *>( dstTexture ) );
        VulkanTextureGpu *dstTextureVulkan = static_cast<VulkanTextureGpu *>( dstTexture );

        const size_t srcOffset =
            reinterpret_cast<uint8 *>( srcBox.data ) - reinterpret_cast<uint8 *>( mLastMappedPtr );
        const uint32 destinationSlice =
            ( dstBox ? dstBox->sliceStart : 0 ) + dstTexture->getInternalSliceStart();
        const uint32 numSlices = ( dstBox ? dstBox->numSlices : dstTexture->getNumSlices() );
        uint32 xPos = static_cast<uint32>( dstBox ? dstBox->x : 0 );
        uint32 yPos = static_cast<uint32>( dstBox ? dstBox->y : 0 );
        uint32 zPos = static_cast<uint32>( dstBox ? dstBox->z : 0 );

        for( uint32 i = 0; i < srcBox.numSlices; ++i )
        {
            VkBufferImageCopy region;
            region.bufferOffset = srcOffset + srcBox.bytesPerImage * i;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = mipLevel;
            region.imageSubresource.baseArrayLayer = destinationSlice;
            region.imageSubresource.layerCount = numSlices;

            region.imageOffset.x = static_cast<int32_t>( xPos );
            region.imageOffset.y = static_cast<int32_t>( yPos );
            region.imageOffset.z = static_cast<int32_t>( zPos );
            region.imageExtent.width = srcBox.width;
            region.imageExtent.height = srcBox.height;
            region.imageExtent.depth = srcBox.depth;

            vkCmdCopyBufferToImage( device->mGraphicsQueue.mCurrentCmdBuffer, mVboName,
                                    dstTextureVulkan->getFinalTextureName(),
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &region );
        }
    }
}  // namespace Ogre
