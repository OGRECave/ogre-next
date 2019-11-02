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
#include "OgreVulkanWindow.h"

#include "OgreVulkanDevice.h"
#include "OgreVulkanMappings.h"
#include "OgreVulkanRenderSystem.h"
#include "OgreVulkanTextureGpuWindow.h"
#include "OgreVulkanUtils.h"

#include "Vao/OgreVulkanVaoManager.h"

#include "OgreException.h"

#include "vulkan/vulkan_core.h"

#define TODO_allow_users_to_choose_presentModes
#define TODO_handle_VK_ERROR_OUT_OF_DATE_KHR
#define TODO_handleSeparatePresentQueue

namespace Ogre
{
    VulkanWindow::VulkanWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode ) :
        Window( title, width, height, fullscreenMode ),
        mClosed( false ),
        mDevice( 0 ),
        mSurfaceKHR( 0 ),
        mSwapchainStatus( SwapchainReleased )
    {
        mFocused = true;
    }
    //-------------------------------------------------------------------------
    VulkanWindow::~VulkanWindow() {}
    //-------------------------------------------------------------------------
    PixelFormatGpu VulkanWindow::chooseSurfaceFormat( bool hwGamma )
    {
        uint32 numFormats = 0u;
        VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR( mDevice->mPhysicalDevice, mSurfaceKHR,
                                                                &numFormats, 0 );
        checkVkResult( result, "vkGetPhysicalDeviceSurfaceFormatsKHR" );
        if( numFormats == 0 )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "vkGetPhysicalDeviceSurfaceFormatsKHR returned 0 formats!",
                         "VulkanWindow::chooseSurfaceFormat" );
        }

        FastArray<VkSurfaceFormatKHR> formats;
        formats.resize( numFormats );
        result = vkGetPhysicalDeviceSurfaceFormatsKHR( mDevice->mPhysicalDevice, mSurfaceKHR,
                                                       &numFormats, formats.begin() );
        checkVkResult( result, "vkGetPhysicalDeviceSurfaceFormatsKHR" );

        PixelFormatGpu pixelFormat = PFG_UNKNOWN;
        for( size_t i = 0; i < numFormats && pixelFormat == PFG_UNKNOWN; ++i )
        {
            switch( formats[i].format )
            {
            case VK_FORMAT_R8G8B8A8_SRGB:
                if( hwGamma )
                    pixelFormat = PFG_RGBA8_UNORM_SRGB;
                break;
            case VK_FORMAT_B8G8R8A8_SRGB:
                if( hwGamma )
                    pixelFormat = PFG_BGRA8_UNORM_SRGB;
                break;
            case VK_FORMAT_R8G8B8A8_UNORM:
                if( !hwGamma )
                    pixelFormat = PFG_RGBA8_UNORM;
                break;
            case VK_FORMAT_B8G8R8A8_UNORM:
                if( !hwGamma )
                    pixelFormat = PFG_BGRA8_UNORM;
                break;
            default:
                continue;
            }
        }

        if( pixelFormat == PFG_UNKNOWN )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "Could not find a suitable Surface format",
                         "VulkanWindow::chooseSurfaceFormat" );
        }

        return pixelFormat;
    }
    //-------------------------------------------------------------------------
    void VulkanWindow::createSwapchain( void )
    {
        VkSurfaceCapabilitiesKHR surfaceCaps;
        VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR( mDevice->mPhysicalDevice,
                                                                     mSurfaceKHR, &surfaceCaps );
        checkVkResult( result, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR" );

        VkBool32 supported;
        result = vkGetPhysicalDeviceSurfaceSupportKHR(
            mDevice->mPhysicalDevice, mDevice->mGraphicsQueue.mFamilyIdx, mSurfaceKHR, &supported );
        checkVkResult( result, "vkGetPhysicalDeviceSurfaceSupportKHR" );

        if( !supported )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "vkGetPhysicalDeviceSurfaceSupportKHR says our KHR Surface is unsupported!",
                         "VulkanWindow::createSwapchain" );
        }

        TODO_allow_users_to_choose_presentModes;  // Also check which ones are the best
        uint32 numPresentModes = 0u;
        vkGetPhysicalDeviceSurfacePresentModesKHR( mDevice->mPhysicalDevice, mSurfaceKHR,
                                                   &numPresentModes, 0 );
        FastArray<VkPresentModeKHR> presentModes;
        presentModes.resize( numPresentModes );
        vkGetPhysicalDeviceSurfacePresentModesKHR( mDevice->mPhysicalDevice, mSurfaceKHR,
                                                   &numPresentModes, presentModes.begin() );
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        for( size_t i = 0u; i < numPresentModes; ++i )
        {
            if( presentModes[i] == VK_PRESENT_MODE_FIFO_KHR )
            {
                presentMode = VK_PRESENT_MODE_FIFO_KHR;
                break;
            }
        }

        //-----------------------------
        // Create swapchain
        //-----------------------------

        VaoManager *vaoManager = mDevice->mVaoManager;
        const uint8 numDynFrames = vaoManager->getDynamicBufferMultiplier();

        uint32 minImageCount =
            std::max<uint32>( surfaceCaps.minImageCount, vaoManager->getDynamicBufferMultiplier() );
        if( surfaceCaps.maxImageCount != 0u )
            minImageCount = std::min<uint32>( minImageCount, surfaceCaps.maxImageCount );

        VkSwapchainCreateInfoKHR swapchainCreateInfo;
        memset( &swapchainCreateInfo, 0, sizeof( swapchainCreateInfo ) );
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = mSurfaceKHR;
        swapchainCreateInfo.minImageCount = minImageCount;
        swapchainCreateInfo.imageFormat = VulkanMappings::get( mTexture->getPixelFormat() );
        swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        swapchainCreateInfo.imageExtent.width = getWidth();
        swapchainCreateInfo.imageExtent.height = getHeight();
        swapchainCreateInfo.imageArrayLayers = 1u;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0u;
        swapchainCreateInfo.pQueueFamilyIndices = 0;
        // Find a supported composite alpha mode - one of these is guaranteed to be set
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        };
        for( size_t i = 0; i < sizeof( compositeAlphaFlags ) / sizeof( compositeAlphaFlags[0] ); ++i )
        {
            if( surfaceCaps.supportedCompositeAlpha & compositeAlphaFlags[i] )
            {
                swapchainCreateInfo.compositeAlpha = compositeAlphaFlags[i];
                break;
            }
        }
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchainCreateInfo.presentMode = presentMode;

        //-----------------------------
        // Create swapchain images
        //-----------------------------

        result = vkCreateSwapchainKHR( mDevice->mDevice, &swapchainCreateInfo, 0, &mSwapchain );
        checkVkResult( result, "vkCreateSwapchainKHR" );

        uint32 numSwapchainImages = 0u;
        result = vkGetSwapchainImagesKHR( mDevice->mDevice, mSwapchain, &numSwapchainImages, NULL );
        checkVkResult( result, "vkGetSwapchainImagesKHR" );

        OGRE_ASSERT_LOW( numSwapchainImages > 0u );

        mSwapchainImages.resize( numSwapchainImages );
        result = vkGetSwapchainImagesKHR( mDevice->mDevice, mSwapchain, &numSwapchainImages,
                                          mSwapchainImages.begin() );
        checkVkResult( result, "vkGetSwapchainImagesKHR" );

        VkSemaphoreCreateInfo semaphoreCreateInfo;
        makeVkStruct( semaphoreCreateInfo, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO );
        mSwapchainSemaphores.resize( numDynFrames );

        for( size_t i = 0u; i < numDynFrames; ++i )
        {
            result = vkCreateSemaphore( mDevice->mDevice, &semaphoreCreateInfo, NULL,
                                        &mSwapchainSemaphores[i] );
            checkVkResult( result, "vkCreateSemaphore" );
        }

        acquireNextSwapchain();
    }
    //-------------------------------------------------------------------------
    void VulkanWindow::acquireNextSwapchain( void )
    {
        OGRE_ASSERT_LOW( mSwapchainStatus == SwapchainReleased );

        VaoManager *vaoManager = mDevice->mVaoManager;
        const size_t currFrame = vaoManager->waitForTailFrameToFinish();

        uint32 swapchainIdx = 0u;
        VkResult result =
            vkAcquireNextImageKHR( mDevice->mDevice, mSwapchain, UINT64_MAX,
                                   mSwapchainSemaphores[currFrame], VK_NULL_HANDLE, &swapchainIdx );
        if( result != VK_SUCCESS )
        {
            // VK_ERROR_OUT_OF_DATE_KHR means the swapchain changed (e.g. was resized)
            TODO_handle_VK_ERROR_OUT_OF_DATE_KHR;
            LogManager::getSingleton().logMessage(
                "[VulkanWindow::acquireNextSwapchain] vkAcquireNextImageKHR failed VkResult = " +
                StringConverter::toString( result ) );
        }
        else
        {
            mSwapchainStatus = SwapchainAcquired;

            VulkanTextureGpuWindow *vulkanTexture = static_cast<VulkanTextureGpuWindow *>( mTexture );
            vulkanTexture->_setCurrentSwapchain( mSwapchainImages[swapchainIdx], swapchainIdx );
        }
    }
    //-------------------------------------------------------------------------
    void VulkanWindow::destroy( void )
    {
        if( !mSwapchainSemaphores.empty() )
        {
            mDevice->stall();
            VkSemaphoreArray::const_iterator itor = mSwapchainSemaphores.begin();
            VkSemaphoreArray::const_iterator endt = mSwapchainSemaphores.end();

            while( itor != endt )
                vkDestroySemaphore( mDevice->mDevice, *itor++, 0 );
            mSwapchainSemaphores.clear();
        }

        if( mSurfaceKHR )
            vkDestroySurfaceKHR( mDevice->mInstance, mSurfaceKHR, 0 );
    }
    //-------------------------------------------------------------------------
    void VulkanWindow::_setDevice( VulkanDevice *device )
    {
        OGRE_ASSERT_LOW( !mDevice );
        mDevice = device;
    }
    //-------------------------------------------------------------------------
    VkSemaphore VulkanWindow::getImageAcquiredSemaphore( void )
    {
        OGRE_ASSERT_LOW( mSwapchainStatus != SwapchainReleased );
        // It's weird that mSwapchainStatus would be in SwapchainPendingSwap here,
        // however it's not invalid and won't result in race conditions (e.g. swapBuffers was called,
        // then more work was added, but commitAndNextCommandBuffer hasn't yet been called).
        //
        // We assert because it may signify that something weird is going on: if user called
        // swapBuffers(), then he may expect to present everything that has been rendered
        // up until now, without including what came after the swapBuffers call.
        OGRE_ASSERT_MEDIUM( mSwapchainStatus != SwapchainPendingSwap );

        VkSemaphore retVal = 0;
        if( mSwapchainStatus == SwapchainAcquired )
        {
            mSwapchainStatus = SwapchainUsedInRendering;
            const size_t currFrame = mDevice->mVaoManager->waitForTailFrameToFinish();
            retVal = mSwapchainSemaphores[currFrame];
        }
        return retVal;
    }
    //-------------------------------------------------------------------------
    bool VulkanWindow::isClosed( void ) const { return mClosed; }
    //-------------------------------------------------------------------------
    void VulkanWindow::swapBuffers( void )
    {
        if( mSwapchainStatus == SwapchainAcquired || mSwapchainStatus == SwapchainPendingSwap )
        {
            // Ogre never rendered to this window. There's nothing to present.
            // Pass the currently acquired swapchain onto the next frame.
            // Or alternatively, swapBuffers was called twice in a row
            return;
        }

        OGRE_ASSERT_LOW( mSwapchainStatus == SwapchainUsedInRendering );
        mDevice->mGraphicsQueue.mWindowsPendingSwap.push_back( this );
        mSwapchainStatus = SwapchainPendingSwap;
    }
    //-------------------------------------------------------------------------
    void VulkanWindow::_swapBuffers( VkSemaphore queueFinishSemaphore )
    {
        OGRE_ASSERT_LOW( mSwapchainStatus == SwapchainPendingSwap );

        VulkanTextureGpuWindow *vulkanTexture = static_cast<VulkanTextureGpuWindow *>( mTexture );
        const uint32 currentSwapchainIdx = vulkanTexture->getCurrentSwapchainIdx();

        TODO_handleSeparatePresentQueue;
        /*
        if (demo->separate_present_queue) {
            // If we are using separate queues, change image ownership to the
            // present queue before presenting, waiting for the draw complete
            // semaphore and signalling the ownership released semaphore when finished
            VkFence nullFence = VK_NULL_HANDLE;
            pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &demo->draw_complete_semaphores[demo->frame_index];
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers =
                &demo->swapchain_image_resources[demo->current_buffer].graphics_to_present_cmd;
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &demo->image_ownership_semaphores[demo->frame_index];
            err = vkQueueSubmit(demo->present_queue, 1, &submit_info, nullFence);
            assert(!err);
        }*/

        VkPresentInfoKHR present;
        makeVkStruct( present, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR );
        present.swapchainCount = 1u;
        present.pSwapchains = &mSwapchain;
        present.pImageIndices = &currentSwapchainIdx;
        if( queueFinishSemaphore )
        {
            present.waitSemaphoreCount = 1u;
            present.pWaitSemaphores = &queueFinishSemaphore;
        }

        VkResult result = vkQueuePresentKHR( mDevice->mPresentQueue, &present );

        if( result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR )
        {
            LogManager::getSingleton().logMessage(
                "[VulkanWindow::swapBuffers] vkQueuePresentKHR: error presenting VkResult = " +
                StringConverter::toString( result ) );
        }

        mSwapchainStatus = SwapchainReleased;

        acquireNextSwapchain();
    }
}  // namespace Ogre
