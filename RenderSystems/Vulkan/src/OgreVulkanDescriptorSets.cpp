/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-present Torus Knot Software Ltd

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

#include "OgreVulkanDescriptorSets.h"

#include "OgreVulkanTextureGpu.h"
#include "OgreVulkanUtils.h"
#include "Vao/OgreVulkanUavBufferPacked.h"

#include "OgreDescriptorSetSampler.h"
#include "OgreDescriptorSetUav.h"
#include "OgreHlmsSamplerblock.h"

namespace Ogre
{
    VulkanDescriptorSetSampler::VulkanDescriptorSetSampler( const DescriptorSetSampler &descSet )
    {
        mSamplers.reserve( descSet.mSamplers.size() );

        FastArray<const HlmsSamplerblock *>::const_iterator itor = descSet.mSamplers.begin();
        FastArray<const HlmsSamplerblock *>::const_iterator endt = descSet.mSamplers.end();

        while( itor != endt )
        {
            const HlmsSamplerblock *samplerblock = *itor;
            VkSampler sampler = reinterpret_cast<VkSampler>( samplerblock->mRsData );
            VkDescriptorImageInfo imageInfo;
            imageInfo.sampler = sampler;
            imageInfo.imageView = 0;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            mSamplers.push_back( imageInfo );
            ++itor;
        }

        makeVkStruct( mWriteDescSet, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET );
        mWriteDescSet.dstArrayElement = 1u;
        mWriteDescSet.descriptorCount = static_cast<uint32>( mSamplers.size() );
        mWriteDescSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        mWriteDescSet.pImageInfo = mSamplers.begin();
    }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    VulkanDescriptorSetUav::VulkanDescriptorSetUav( const DescriptorSetUav &descSetUav )
    {
        if( descSetUav.mUavs.empty() )
        {
            memset( mWriteDescSets, 0, sizeof( mWriteDescSets ) );
            return;
        }

        size_t numTextures = 0u;
        size_t numBuffers = 0u;

        FastArray<DescriptorSetUav::Slot>::const_iterator itor = descSetUav.mUavs.begin();
        FastArray<DescriptorSetUav::Slot>::const_iterator endt = descSetUav.mUavs.end();

        while( itor != endt )
        {
            if( itor->isBuffer() )
                ++numBuffers;
            else
                ++numTextures;
            ++itor;
        }

        mTextures.reserve( numTextures );
        mBuffers.resize( numBuffers );
        numBuffers = 0u;

        itor = descSetUav.mUavs.begin();

        while( itor != endt )
        {
            if( itor->isBuffer() )
            {
                const DescriptorSetUav::BufferSlot &bufferSlot = itor->getBuffer();
                OGRE_ASSERT_HIGH( dynamic_cast<VulkanUavBufferPacked *>( bufferSlot.buffer ) );
                VulkanUavBufferPacked *vulkanBuffer =
                    static_cast<VulkanUavBufferPacked *>( bufferSlot.buffer );

                vulkanBuffer->setupBufferInfo( mBuffers[numBuffers], bufferSlot.offset,
                                               bufferSlot.sizeBytes );
                ++numBuffers;
            }
            else
            {
                const DescriptorSetUav::TextureSlot &texSlot = itor->getTexture();
                OGRE_ASSERT_HIGH( dynamic_cast<VulkanTextureGpu *>( texSlot.texture ) );
                VulkanTextureGpu *vulkanTexture = static_cast<VulkanTextureGpu *>( texSlot.texture );

                VkDescriptorImageInfo imageInfo;
                imageInfo.sampler = 0;
                imageInfo.imageView = vulkanTexture->createView( texSlot );
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                mTextures.push_back( imageInfo );
            }

            ++itor;
        }

        if( numBuffers != 0u )
        {
            VkWriteDescriptorSet *writeDescSet = &mWriteDescSets[0];
            makeVkStruct( *writeDescSet, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET );

            writeDescSet->dstArrayElement = 1u;
            writeDescSet->descriptorCount = static_cast<uint32>( numBuffers );
            writeDescSet->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writeDescSet->pBufferInfo = mBuffers.begin();
        }

        if( numTextures != 0u )
        {
            VkWriteDescriptorSet *writeDescSet = &mWriteDescSets[1];
            makeVkStruct( *writeDescSet, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET );

            writeDescSet->dstArrayElement = 1u;
            writeDescSet->descriptorCount = static_cast<uint32>( numTextures );
            writeDescSet->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            writeDescSet->pImageInfo = mTextures.begin();
        }
    }
    //-------------------------------------------------------------------------
    void VulkanDescriptorSetUav::destroy( const DescriptorSetUav &descSetUav )
    {
        FastArray<VkDescriptorImageInfo>::const_iterator imgInfoIt = mTextures.begin();

        FastArray<DescriptorSetUav::Slot>::const_iterator itor = descSetUav.mUavs.begin();
        FastArray<DescriptorSetUav::Slot>::const_iterator endt = descSetUav.mUavs.end();

        while( itor != endt )
        {
            if( itor->isTexture() )
            {
                const DescriptorSetUav::TextureSlot &texSlot = itor->getTexture();
                OGRE_ASSERT_HIGH( dynamic_cast<VulkanTextureGpu *>( texSlot.texture ) );
                VulkanTextureGpu *vulkanTexture = static_cast<VulkanTextureGpu *>( texSlot.texture );
                vulkanTexture->destroyView( texSlot, imgInfoIt->imageView );
            }
            ++itor;
        }
    }
}  // namespace Ogre
