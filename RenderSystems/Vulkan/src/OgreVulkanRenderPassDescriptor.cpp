/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

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

#include "OgreVulkanRenderPassDescriptor.h"

#include "OgreVulkanDevice.h"
#include "OgreVulkanRenderSystem.h"
#include "OgreVulkanTextureGpu.h"
#include "OgreVulkanTextureGpuWindow.h"

#include "OgreHlmsDatablock.h"
#include "OgrePixelFormatGpuUtils.h"

#include "OgreVulkanMappings.h"
#include "OgreVulkanUtils.h"

#include <execinfo.h>  //backtrace

namespace Ogre
{
    VulkanRenderPassDescriptor::VulkanRenderPassDescriptor( VulkanQueue *graphicsQueue,
                                                            VulkanRenderSystem *renderSystem ) :
        mRenderPass( 0 ),
        mNumImageViews( 0u ),
#if VULKAN_DISABLED
        mSharedFboItor( renderSystem->_getFrameBufferDescMap().end() ),
#endif
        mTargetWidth( 0u ),
        mTargetHeight( 0u ),
        mQueue( graphicsQueue ),
        mRenderSystem( renderSystem )
    {
        memset( mImageViews, 0, sizeof( mImageViews ) );
    }
    //-----------------------------------------------------------------------------------
    VulkanRenderPassDescriptor::~VulkanRenderPassDescriptor()
    {
        destroyFbo();
#if VULKAN_DISABLED
        VulkanFrameBufferDescMap &frameBufferDescMap = mRenderSystem->_getFrameBufferDescMap();
        if( mSharedFboItor != frameBufferDescMap.end() )
        {
            --mSharedFboItor->second.refCount;
            if( !mSharedFboItor->second.refCount )
                frameBufferDescMap.erase( mSharedFboItor );
            mSharedFboItor = frameBufferDescMap.end();
        }

#endif
    }
    //-----------------------------------------------------------------------------------
    void VulkanRenderPassDescriptor::checkRenderWindowStatus( void )
    {
        if( ( mNumColourEntries > 0 && mColour[0].texture->isRenderWindowSpecific() ) ||
            ( mDepth.texture && mDepth.texture->isRenderWindowSpecific() ) ||
            ( mStencil.texture && mStencil.texture->isRenderWindowSpecific() ) )
        {
            if( mNumColourEntries > 1u )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Cannot use RenderWindow as MRT with other colour textures",
                             "VulkanRenderPassDescriptor::colourEntriesModified" );
            }

            if( ( mNumColourEntries > 0 && !mColour[0].texture->isRenderWindowSpecific() ) ||
                ( mDepth.texture && !mDepth.texture->isRenderWindowSpecific() ) ||
                ( mStencil.texture && !mStencil.texture->isRenderWindowSpecific() ) )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Cannot mix RenderWindow colour texture with depth or stencil buffer "
                             "that aren't for RenderWindows, or viceversa",
                             "VulkanRenderPassDescriptor::checkRenderWindowStatus" );
            }
        }

        calculateSharedKey();
    }
    //-----------------------------------------------------------------------------------
    void VulkanRenderPassDescriptor::calculateSharedKey( void )
    {
#if VULKAN_DISABLED
        FrameBufferDescKey key( *this );
        VulkanFrameBufferDescMap &frameBufferDescMap = mRenderSystem->_getFrameBufferDescMap();
        VulkanFrameBufferDescMap::iterator newItor = frameBufferDescMap.find( key );

        if( newItor == frameBufferDescMap.end() )
        {
            VulkanFrameBufferDescValue value;
            value.refCount = 0;
            frameBufferDescMap[key] = value;
            newItor = frameBufferDescMap.find( key );
        }

        ++newItor->second.refCount;

        if( mSharedFboItor != frameBufferDescMap.end() )
        {
            --mSharedFboItor->second.refCount;
            if( !mSharedFboItor->second.refCount )
                frameBufferDescMap.erase( mSharedFboItor );
        }

        mSharedFboItor = newItor;
#endif
    }
    //-----------------------------------------------------------------------------------
    VkAttachmentLoadOp VulkanRenderPassDescriptor::get( LoadAction::LoadAction action )
    {
        switch( action )
        {
        case LoadAction::DontCare:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        case LoadAction::Clear:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
        case LoadAction::ClearOnTilers:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
#else
        case LoadAction::ClearOnTilers:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
#endif
        case LoadAction::Load:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        }

        return VK_ATTACHMENT_LOAD_OP_LOAD;
    }
    //-----------------------------------------------------------------------------------
    VkAttachmentStoreOp VulkanRenderPassDescriptor::get( StoreAction::StoreAction action )
    {
        switch( action )
        {
        case StoreAction::DontCare:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        case StoreAction::Store:
            return VK_ATTACHMENT_STORE_OP_STORE;
        case StoreAction::MultisampleResolve:
            return VK_ATTACHMENT_STORE_OP_STORE;
        case StoreAction::StoreAndMultisampleResolve:
            return VK_ATTACHMENT_STORE_OP_STORE;
        case StoreAction::StoreOrResolve:
            OGRE_ASSERT_LOW( false &&
                             "StoreOrResolve is invalid. "
                             "Compositor should've set one or the other already!" );
            return VK_ATTACHMENT_STORE_OP_STORE;
        }

        return VK_ATTACHMENT_STORE_OP_STORE;
    }
    //-----------------------------------------------------------------------------------
    VkClearColorValue VulkanRenderPassDescriptor::getClearColour( const ColourValue &clearColour,
                                                                  PixelFormatGpu pixelFormat )
    {
        const bool isInteger = PixelFormatGpuUtils::isInteger( pixelFormat );
        const bool isSigned = PixelFormatGpuUtils::isSigned( pixelFormat );
        VkClearColorValue retVal;
        if( !isInteger )
        {
            for( size_t i = 0u; i < 4u; ++i )
                retVal.float32[i] = static_cast<float>( clearColour[i] );
        }
        else
        {
            if( !isSigned )
            {
                for( size_t i = 0u; i < 4u; ++i )
                    retVal.uint32[i] = static_cast<uint32>( clearColour[i] );
            }
            else
            {
                for( size_t i = 0u; i < 4u; ++i )
                    retVal.int32[i] = static_cast<int32>( clearColour[i] );
            }
        }
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanRenderPassDescriptor::sanitizeMsaaResolve( size_t colourIdx )
    {
#if VULKAN_DISABLED
        const size_t i = colourIdx;

        // iOS_GPUFamily3_v2, OSX_GPUFamily1_v2
        if( mColour[i].storeAction == StoreAction::StoreAndMultisampleResolve &&
            !mRenderSystem->hasStoreAndMultisampleResolve() &&
            ( mColour[i].texture->getMsaa() > 1u && mColour[i].resolveTexture ) )
        {
            // Must emulate the behavior (slower)
            mColourAttachment[i].storeAction = MTLStoreActionStore;
            mColourAttachment[i].resolveTexture = nil;
            mResolveColourAttachm[i] = [mColourAttachment[i] copy];
            mResolveColourAttachm[i].loadAction = MTLLoadActionLoad;
            mResolveColourAttachm[i].storeAction = MTLStoreActionMultisampleResolve;

            mRequiresManualResolve = true;
        }
        else if( mColour[i].storeAction == StoreAction::DontCare ||
                 mColour[i].storeAction == StoreAction::Store )
        {
            mColourAttachment[i].resolveTexture = nil;
        }
#endif
    }
    //-----------------------------------------------------------------------------------
    VkImageView VulkanRenderPassDescriptor::setupColourAttachment( VkAttachmentDescription &attachment,
                                                                   VkImage texName,
                                                                   const RenderPassColourTarget &colour,
                                                                   bool resolveTex )
    {
        attachment.format = VulkanMappings::get( colour.texture->getPixelFormat() );
        attachment.samples = static_cast<VkSampleCountFlagBits>( colour.texture->getMsaa() );
        attachment.loadOp = get( colour.loadAction );
        attachment.storeOp = get( colour.storeAction );
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        if( colour.texture->isRenderWindowSpecific() )
        {
            attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
        else
        {
            attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        VkImageViewCreateInfo imgViewCreateInfo;
        makeVkStruct( imgViewCreateInfo, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO );
        imgViewCreateInfo.viewType = VulkanMappings::get( colour.texture->getTextureType() );
        imgViewCreateInfo.format = attachment.format;
        imgViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imgViewCreateInfo.subresourceRange.baseMipLevel =
            resolveTex ? colour.resolveMipLevel : colour.mipLevel;
        imgViewCreateInfo.subresourceRange.levelCount = 1u;
        if( colour.allLayers )
        {
            imgViewCreateInfo.subresourceRange.baseArrayLayer = 0u;
            imgViewCreateInfo.subresourceRange.layerCount = colour.texture->getNumSlices();
        }
        else
        {
            imgViewCreateInfo.subresourceRange.baseArrayLayer =
                resolveTex ? colour.resolveSlice : colour.slice;
            imgViewCreateInfo.subresourceRange.layerCount = 1u;
        }

        VkImageView retVal = 0;

        if( !colour.texture->isRenderWindowSpecific() )
        {
            imgViewCreateInfo.image = texName;
            vkCreateImageView( mQueue->mDevice, &imgViewCreateInfo, 0, &retVal );
        }
        else
        {
            OGRE_ASSERT_HIGH( dynamic_cast<VulkanTextureGpuWindow *>( colour.texture ) );
            VulkanTextureGpuWindow *textureVulkan =
                static_cast<VulkanTextureGpuWindow *>( colour.texture );

            OGRE_ASSERT_LOW( mWindowImageViews.empty() && "Only one window can be used as target" );
            const size_t numSurfaces = textureVulkan->getWindowNumSurfaces();
            mWindowImageViews.resize( numSurfaces );
            for( size_t surfIdx = 0u; surfIdx < numSurfaces; ++surfIdx )
            {
                imgViewCreateInfo.image = textureVulkan->getWindowFinalTextureName( surfIdx );
                vkCreateImageView( mQueue->mDevice, &imgViewCreateInfo, 0, &mWindowImageViews[surfIdx] );
            }
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    VkImageView VulkanRenderPassDescriptor::setupDepthAttachment( VkAttachmentDescription &attachment )
    {
        attachment.format = VulkanMappings::get( mDepth.texture->getPixelFormat() );
        attachment.samples = static_cast<VkSampleCountFlagBits>( mDepth.texture->getMsaa() );
        attachment.loadOp = get( mDepth.loadAction );
        attachment.storeOp = get( mDepth.storeAction );
        if( mStencil.texture )
        {
            attachment.stencilLoadOp = get( mStencil.loadAction );
            attachment.stencilStoreOp = get( mStencil.storeAction );
        }
        else
        {
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkImageViewCreateInfo imgViewCreateInfo;
        makeVkStruct( imgViewCreateInfo, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO );
        imgViewCreateInfo.viewType = VulkanMappings::get( mDepth.texture->getTextureType() );
        imgViewCreateInfo.format = attachment.format;
        imgViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if( mStencil.texture )
            imgViewCreateInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        imgViewCreateInfo.subresourceRange.baseMipLevel = mDepth.mipLevel;
        imgViewCreateInfo.subresourceRange.levelCount = 1u;
        imgViewCreateInfo.subresourceRange.baseArrayLayer = mDepth.slice;
        imgViewCreateInfo.subresourceRange.layerCount = 1u;

        VkImageView retVal = 0;

        OGRE_ASSERT_HIGH( dynamic_cast<VulkanTextureGpu *>( mDepth.texture ) );
        VulkanTextureGpu *textureVulkan = static_cast<VulkanTextureGpu *>( mDepth.texture );

        imgViewCreateInfo.image = textureVulkan->getFinalTextureName();
        vkCreateImageView( mQueue->mDevice, &imgViewCreateInfo, 0, &retVal );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanRenderPassDescriptor::updateFbo( void )
    {
        destroyFbo();

        if( mDepth.texture && mDepth.texture->getResidencyStatus() != GpuResidency::Resident )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "RenderTexture '" + mDepth.texture->getNameStr() + "' must be resident!",
                         "VulkanRenderPassDescriptor::updateFbo" );
        }

        if( mStencil.texture && mStencil.texture->getResidencyStatus() != GpuResidency::Resident )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "RenderTexture '" + mStencil.texture->getNameStr() + "' must be resident!",
                         "VulkanRenderPassDescriptor::updateFbo" );
        }

        if( !mDepth.texture )
        {
            if( mStencil.texture )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Stencil without depth (RenderTexture '" + mStencil.texture->getNameStr() +
                                 "'). This is not supported by Vulkan",
                             "VulkanRenderPassDescriptor::updateFbo" );
            }
        }

        bool hasRenderWindow = false;

        uint32 attachmentIdx = 0u;
        uint32 numColourAttachments = 0u;
        uint32 windowAttachmentIdx = std::numeric_limits<uint32>::max();
        bool usesResolveAttachments = false;

        // 1 per MRT
        // 1 per MRT MSAA resolve
        // 1 for Depth buffer
        // 1 for Stencil buffer
        VkAttachmentDescription attachments[OGRE_MAX_MULTIPLE_RENDER_TARGETS * 2u + 2u];
        memset( &attachments, 0, sizeof( attachments ) );

        VkAttachmentReference colourAttachRefs[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        VkAttachmentReference resolveAttachRefs[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        VkAttachmentReference depthAttachRef;

        for( size_t i = 0; i < mNumColourEntries; ++i )
        {
            if( mColour[i].texture->getResidencyStatus() != GpuResidency::Resident )
            {
                OGRE_EXCEPT(
                    Exception::ERR_INVALIDPARAMS,
                    "RenderTexture '" + mColour[i].texture->getNameStr() + "' must be resident!",
                    "VulkanRenderPassDescriptor::updateFbo" );
            }
            if( i > 0 && hasRenderWindow != mColour[i].texture->isRenderWindowSpecific() )
            {
                // This is a GL restriction actually, which we mimic for consistency
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Cannot use RenderWindow as MRT with other colour textures",
                             "VulkanRenderPassDescriptor::updateFbo" );
            }

            hasRenderWindow |= mColour[i].texture->isRenderWindowSpecific();

            if( mColour[i].texture->getPixelFormat() == PFG_NULL )
                continue;

            OGRE_ASSERT_HIGH( dynamic_cast<VulkanTextureGpu *>( mColour[i].texture ) );
            VulkanTextureGpu *textureVulkan = static_cast<VulkanTextureGpu *>( mColour[i].texture );

            VkImage texName = 0;
            VkImage resolveTexName = 0;

            if( mColour[i].texture->getMsaa() > 1u )
            {
                VulkanTextureGpu *resolveTexture = 0;
                if( mColour[i].resolveTexture )
                {
                    OGRE_ASSERT_HIGH( dynamic_cast<VulkanTextureGpu *>( mColour[i].resolveTexture ) );
                    resolveTexture = static_cast<VulkanTextureGpu *>( mColour[i].resolveTexture );
                }

                if( !mColour[i].texture->hasMsaaExplicitResolves() )
                    texName = textureVulkan->getMsaaFramebufferName();
                else
                    texName = textureVulkan->getFinalTextureName();

                if( resolveTexture )
                    resolveTexName = resolveTexture->getFinalTextureName();
            }
            else
            {
                texName = textureVulkan->getFinalTextureName();
            }

            if( textureVulkan->isRenderWindowSpecific() )
                windowAttachmentIdx = attachmentIdx;

            mClearValues[attachmentIdx].color =
                getClearColour( mColour[i].clearColour, mColour[i].texture->getPixelFormat() );

            mImageViews[attachmentIdx] =
                setupColourAttachment( attachments[attachmentIdx], texName, mColour[i], false );
            colourAttachRefs[numColourAttachments].attachment = attachmentIdx;
            colourAttachRefs[numColourAttachments].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            ++attachmentIdx;
            if( resolveTexName )
            {
                mImageViews[attachmentIdx] = setupColourAttachment( attachments[attachmentIdx],
                                                                    resolveTexName, mColour[i], true );
                resolveAttachRefs[numColourAttachments].attachment = attachmentIdx;
                resolveAttachRefs[numColourAttachments].layout =
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                ++attachmentIdx;
                usesResolveAttachments = true;
            }
            else
            {
                resolveAttachRefs[numColourAttachments].attachment = VK_ATTACHMENT_UNUSED;
                resolveAttachRefs[numColourAttachments].layout = VK_IMAGE_LAYOUT_UNDEFINED;
            }
            ++numColourAttachments;

            sanitizeMsaaResolve( i );
        }

        if( mDepth.texture )
        {
            mClearValues[attachmentIdx].depthStencil.depth = static_cast<float>( mDepth.clearDepth );
            mClearValues[attachmentIdx].depthStencil.stencil = mStencil.clearStencil;

            setupDepthAttachment( attachments[attachmentIdx] );
            depthAttachRef.attachment = attachmentIdx;
            depthAttachRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            ++attachmentIdx;
        }

        VkSubpassDescription subpass;
        memset( &subpass, 0, sizeof( subpass ) );
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.inputAttachmentCount = 0u;
        subpass.colorAttachmentCount = numColourAttachments;
        subpass.pColorAttachments = colourAttachRefs;
        subpass.pResolveAttachments = usesResolveAttachments ? resolveAttachRefs : 0;
        subpass.pDepthStencilAttachment = mDepth.texture ? &depthAttachRef : 0;

        mNumImageViews = attachmentIdx;

        VkRenderPassCreateInfo renderPassCreateInfo;
        makeVkStruct( renderPassCreateInfo, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO );
        renderPassCreateInfo.attachmentCount = attachmentIdx;
        renderPassCreateInfo.pAttachments = attachments;
        renderPassCreateInfo.subpassCount = 1u;
        renderPassCreateInfo.pSubpasses = &subpass;
        vkCreateRenderPass( mQueue->mDevice, &renderPassCreateInfo, 0, &mRenderPass );

        VkFramebufferCreateInfo fbCreateInfo;
        makeVkStruct( fbCreateInfo, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO );
        fbCreateInfo.renderPass = mRenderPass;
        fbCreateInfo.attachmentCount = attachmentIdx;
        fbCreateInfo.pAttachments = mImageViews;
        fbCreateInfo.width = mTargetWidth;
        fbCreateInfo.height = mTargetHeight;
        fbCreateInfo.layers = 1u;

        const size_t numFramebuffers = std::max<size_t>( mWindowImageViews.size(), 1u );
        mFramebuffers.resize( numFramebuffers );
        for( size_t i = 0u; i < numFramebuffers; ++i )
        {
            if( !mWindowImageViews.empty() )
                mImageViews[windowAttachmentIdx] = mWindowImageViews[i];
            vkCreateFramebuffer( mQueue->mDevice, &fbCreateInfo, 0, &mFramebuffers[i] );
            if( !mWindowImageViews.empty() )
                mImageViews[windowAttachmentIdx] = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void VulkanRenderPassDescriptor::destroyFbo( void )
    {
        {
            FastArray<VkFramebuffer>::const_iterator itor = mFramebuffers.begin();
            FastArray<VkFramebuffer>::const_iterator endt = mFramebuffers.end();
            while( itor != endt )
                vkDestroyFramebuffer( mQueue->mDevice, *itor++, 0 );
            mFramebuffers.clear();
        }

        {
            FastArray<VkImageView>::const_iterator itor = mWindowImageViews.begin();
            FastArray<VkImageView>::const_iterator endt = mWindowImageViews.end();

            while( itor != endt )
                vkDestroyImageView( mQueue->mDevice, *itor++, 0 );
            mWindowImageViews.clear();
        }

        for( size_t i = 0u; i < mNumImageViews; ++i )
        {
            if( mImageViews[i] )
            {
                vkDestroyImageView( mQueue->mDevice, mImageViews[i], 0 );
                mImageViews[i] = 0;
            }
        }
        mNumImageViews = 0u;
    }
    //-----------------------------------------------------------------------------------
    void VulkanRenderPassDescriptor::entriesModified( uint32 entryTypes )
    {
        RenderPassDescriptor::entriesModified( entryTypes );

        checkRenderWindowStatus();

        TextureGpu *anyTargetTexture = 0;
        const uint8 numColourEntries = mNumColourEntries;
        for( int i = 0; i < numColourEntries && !anyTargetTexture; ++i )
            anyTargetTexture = mColour[i].texture;
        if( !anyTargetTexture )
            anyTargetTexture = mDepth.texture;
        if( !anyTargetTexture )
            anyTargetTexture = mStencil.texture;

        mTargetWidth = 0u;
        mTargetHeight = 0u;
        if( anyTargetTexture )
        {
            mTargetWidth = anyTargetTexture->getWidth();
            mTargetHeight = anyTargetTexture->getHeight();
        }

        if( entryTypes & RenderPassDescriptor::All )
            updateFbo();
    }
    //-----------------------------------------------------------------------------------
    void VulkanRenderPassDescriptor::setClearColour( uint8 idx, const ColourValue &clearColour )
    {
        RenderPassDescriptor::setClearColour( idx, clearColour );

        size_t attachmentIdx = 0u;
        for( size_t i = 0u; i < idx; ++i )
        {
            ++attachmentIdx;
            if( mColour->resolveTexture )
                ++attachmentIdx;
        }

        mClearValues[attachmentIdx].color =
            getClearColour( clearColour, mColour[idx].texture->getPixelFormat() );
    }
    //-----------------------------------------------------------------------------------
    void VulkanRenderPassDescriptor::setClearDepth( Real clearDepth )
    {
        RenderPassDescriptor::setClearDepth( clearDepth );
        if( mDepth.texture )
        {
            size_t attachmentIdx = mNumImageViews - 1u;
            mClearValues[attachmentIdx].depthStencil.depth = static_cast<float>( clearDepth );
        }
    }
    //-----------------------------------------------------------------------------------
    void VulkanRenderPassDescriptor::setClearStencil( uint32 clearStencil )
    {
        RenderPassDescriptor::setClearStencil( clearStencil );
        if( mDepth.texture || mStencil.texture )
        {
            size_t attachmentIdx = mNumImageViews - 1u;
            mClearValues[attachmentIdx].depthStencil.stencil = clearStencil;
        }
    }
    //-----------------------------------------------------------------------------------
    void VulkanRenderPassDescriptor::setClearColour( const ColourValue &clearColour )
    {
        const size_t numColourEntries = mNumColourEntries;
        size_t attachmentIdx = 0u;
        for( size_t i = 0u; i < numColourEntries; ++i )
        {
            mColour[i].clearColour = clearColour;
            mClearValues[attachmentIdx].color =
                getClearColour( clearColour, mColour[i].texture->getPixelFormat() );
            ++attachmentIdx;
            if( mColour->resolveTexture )
                ++attachmentIdx;
        }
    }
#if VULKAN_DISABLED
    //-----------------------------------------------------------------------------------
    uint32 VulkanRenderPassDescriptor::checkForClearActions( VulkanRenderPassDescriptor *other ) const
    {
        uint32 entriesToFlush = 0;

        assert( this->mSharedFboItor == other->mSharedFboItor );
        assert( this->mNumColourEntries == other->mNumColourEntries );

        const RenderSystemCapabilities *capabilities = mRenderSystem->getCapabilities();
        const bool isTiler = capabilities->hasCapability( RSC_IS_TILER );

        for( size_t i = 0; i < mNumColourEntries; ++i )
        {
            // this->mColour[i].allLayers doesn't need to be analyzed
            // because it requires a different FBO.
            if( other->mColour[i].loadAction == LoadAction::Clear ||
                ( isTiler && mColour[i].loadAction == LoadAction::ClearOnTilers ) )
            {
                entriesToFlush |= RenderPassDescriptor::Colour0 << i;
            }
        }

        if( other->mDepth.loadAction == LoadAction::Clear ||
            ( isTiler && mDepth.loadAction == LoadAction::ClearOnTilers ) )
        {
            entriesToFlush |= RenderPassDescriptor::Depth;
        }

        if( other->mStencil.loadAction == LoadAction::Clear ||
            ( isTiler && mStencil.loadAction == LoadAction::ClearOnTilers ) )
        {
            entriesToFlush |= RenderPassDescriptor::Stencil;
        }

        return entriesToFlush;
    }
    //-----------------------------------------------------------------------------------
    uint32 VulkanRenderPassDescriptor::willSwitchTo( VulkanRenderPassDescriptor *newDesc,
                                                     bool warnIfRtvWasFlushed ) const
    {
        uint32 entriesToFlush = 0;

        if( !newDesc || this->mSharedFboItor != newDesc->mSharedFboItor || this->mInformationOnly ||
            newDesc->mInformationOnly )
        {
            entriesToFlush = RenderPassDescriptor::All;
        }
        else
            entriesToFlush |= checkForClearActions( newDesc );

        if( warnIfRtvWasFlushed )
            newDesc->checkWarnIfRtvWasFlushed( entriesToFlush );

        return entriesToFlush;
    }
    //-----------------------------------------------------------------------------------
    bool VulkanRenderPassDescriptor::cannotInterruptRendering( void ) const
    {
        bool cannotInterrupt = false;

        for( size_t i = 0; i < mNumColourEntries && !cannotInterrupt; ++i )
        {
            if( mColour[i].storeAction != StoreAction::Store &&
                mColour[i].storeAction != StoreAction::StoreAndMultisampleResolve )
            {
                cannotInterrupt = true;
            }
        }

        cannotInterrupt |= ( mDepth.texture && mDepth.storeAction != StoreAction::Store &&
                             mDepth.storeAction != StoreAction::StoreAndMultisampleResolve ) ||
                           ( mStencil.texture && mStencil.storeAction != StoreAction::Store &&
                             mStencil.storeAction != StoreAction::StoreAndMultisampleResolve );

        return cannotInterrupt;
    }
#endif
    //-----------------------------------------------------------------------------------
    void VulkanRenderPassDescriptor::performLoadActions( void )
    {
        if( mInformationOnly )
            return;

        VkCommandBuffer cmdBuffer = mQueue->mCurrentCmdBuffer;

        size_t fboIdx = 0u;
        if( !mWindowImageViews.empty() )
        {
            VulkanTextureGpuWindow *textureVulkan =
                static_cast<VulkanTextureGpuWindow *>( mColour[0].texture );
            fboIdx = textureVulkan->getCurrentSwapchainIdx();

            VkSemaphore semaphore = textureVulkan->getImageAcquiredSemaphore();
            if( semaphore )
            {
                // We cannot start executing color attachment commands until the semaphore says so
                VkImageMemoryBarrier imageBarrier;
                makeVkStruct( imageBarrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER );
                imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                imageBarrier.dstAccessMask =
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                imageBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                imageBarrier.image = textureVulkan->getFinalTextureName();

                if( mColour[0].loadAction == LoadAction::DontCare ||
                    mColour[0].loadAction == LoadAction::Clear )
                {
                    // Undefined means we don't care, which means existing contents may not be preserved
                    imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                }
                else
                    imageBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                imageBarrier.subresourceRange = textureVulkan->getFullSubresourceRange();

                vkCmdPipelineBarrier(
                    mQueue->mCurrentCmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0u, 0u, 0, 0u, 0, 1u, &imageBarrier );
                mQueue->addWindowToWaitFor( semaphore );
            }
        }

        VkRenderPassBeginInfo passBeginInfo;
        makeVkStruct( passBeginInfo, VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO );
        passBeginInfo.renderPass = mRenderPass;
        passBeginInfo.framebuffer = mFramebuffers[fboIdx];
        passBeginInfo.renderArea.offset.x = 0;
        passBeginInfo.renderArea.offset.y = 0;
        passBeginInfo.renderArea.extent.width = mTargetWidth;
        passBeginInfo.renderArea.extent.height = mTargetHeight;
        passBeginInfo.clearValueCount = sizeof( mClearValues ) / sizeof( mClearValues[0] );
        passBeginInfo.pClearValues = mClearValues;

        vkCmdBeginRenderPass( cmdBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
    }
    //-----------------------------------------------------------------------------------
    void VulkanRenderPassDescriptor::performStoreActions( void )
    {
        if( mInformationOnly )
            return;

        vkCmdEndRenderPass( mQueue->mCurrentCmdBuffer );
    }
#if VULKAN_DISABLED
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    VulkanFrameBufferDescValue::VulkanFrameBufferDescValue() : refCount( 0 ) {}
#endif
}  // namespace Ogre
