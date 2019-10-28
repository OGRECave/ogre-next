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

#include "Vao/OgreVaoManager.h"

#include "OgreException.h"
#include "OgreStringConverter.h"

#include "vulkan/vulkan_core.h"

#define TODO_allow_users_to_choose_presentModes

namespace Ogre
{
    VulkanWindow::VulkanWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode ) :
        Window( title, width, height, fullscreenMode ),
        mClosed( false ),
        mDevice( 0 ),
        mSurfaceKHR( 0 )
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
        if( result != VK_SUCCESS )
        {
            OGRE_VK_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, result,
                            "vkGetPhysicalDeviceSurfaceFormatsKHR failed",
                            "VulkanWindow::chooseSurfaceFormat" );
        }
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
        if( result != VK_SUCCESS )
        {
            OGRE_VK_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, result,
                            "vkGetPhysicalDeviceSurfaceFormatsKHR failed",
                            "VulkanWindow::chooseSurfaceFormat" );
        }

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
        if( result != VK_SUCCESS )
        {
            OGRE_VK_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, result,
                            "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed",
                            "VulkanWindow::createSwapchain" );
        }

        VkBool32 supported;
        result = vkGetPhysicalDeviceSurfaceSupportKHR(
            mDevice->mPhysicalDevice, mDevice->mSelectedQueues[VulkanDevice::Graphics].familyIdx,
            mSurfaceKHR, &supported );

        if( result != VK_SUCCESS )
        {
            OGRE_VK_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, result,
                            "vkGetPhysicalDeviceSurfaceSupportKHR failed",
                            "VulkanWindow::createSwapchain" );
        }

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

        VaoManager *vaoManager = mDevice->mRenderSystem->getVaoManager();
        uint32 minImageCount =
            std::max<uint32>( surfaceCaps.minImageCount, vaoManager->getDynamicBufferMultiplier() );
        if( surfaceCaps.maxImageCount != 0u )
            minImageCount = std::min<uint32>( minImageCount, surfaceCaps.maxImageCount );

        const uint32_t queueFamilyIndices[1] = {
            mDevice->mSelectedQueues[VulkanDevice::Graphics].familyIdx
        };

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
        swapchainCreateInfo.queueFamilyIndexCount = 1u;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
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
        if( result != VK_SUCCESS )
        {
            OGRE_VK_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, result, "vkCreateSwapchainKHR failed",
                            "VulkanWindow::createSwapchain" );
        }

        uint32 numSwapchainImages = 0u;
        result = vkGetSwapchainImagesKHR( mDevice->mDevice, mSwapchain, &numSwapchainImages, NULL );
        if( result != VK_SUCCESS )
        {
            OGRE_VK_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, result, "vkGetSwapchainImagesKHR failed",
                            "VulkanWindow::createSwapchain" );
        }

        OGRE_ASSERT_LOW( numSwapchainImages > 0u );

        mSwapchainImages.resize( numSwapchainImages );
        result = vkGetSwapchainImagesKHR( mDevice->mDevice, mSwapchain, &numSwapchainImages,
                                          mSwapchainImages.begin() );
        if( result != VK_SUCCESS )
        {
            OGRE_VK_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, result, "vkGetSwapchainImagesKHR failed",
                            "VulkanWindow::createSwapchain" );
        }
    }
    //-------------------------------------------------------------------------
    void VulkanWindow::_setDevice( VulkanDevice *device )
    {
        OGRE_ASSERT_LOW( !mDevice );
        mDevice = device;
    }
    //-------------------------------------------------------------------------
    bool VulkanWindow::isClosed( void ) const { return mClosed; }
    //-------------------------------------------------------------------------
    void VulkanWindow::swapBuffers( void ) {}
}  // namespace Ogre
