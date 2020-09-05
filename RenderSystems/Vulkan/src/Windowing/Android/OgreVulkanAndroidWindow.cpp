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

#include "Windowing/Android/OgreVulkanAndroidWindow.h"

#include "OgreVulkanDevice.h"
#include "OgreVulkanTextureGpu.h"
#include "OgreVulkanTextureGpuManager.h"
#include "OgreVulkanUtils.h"

#include "OgreTextureGpuListener.h"

#include "OgreDepthBuffer.h"
#include "OgreException.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreString.h"
#include "OgreWindowEventUtilities.h"

#include "vulkan/vulkan_android.h"
#include "vulkan/vulkan_core.h"

namespace Ogre
{
    VulkanAndroidWindow::VulkanAndroidWindow( const String &title, uint32 width, uint32 height,
                                              bool fullscreenMode ) :
        VulkanWindow( title, width, height, fullscreenMode ),
        mNativeWindow( 0 ),
        mVisible( true ),
        mHidden( false ),
        mIsTopLevel( true ),
        mIsExternal( false )
    {
    }
    //-------------------------------------------------------------------------
    VulkanAndroidWindow::~VulkanAndroidWindow()
    {
        destroy();

        if( mTexture )
        {
            mTexture->notifyAllListenersTextureChanged( TextureGpuListener::Deleted );
            OGRE_DELETE mTexture;
            mTexture = 0;
        }
        if( mStencilBuffer && mStencilBuffer != mDepthBuffer )
        {
            mStencilBuffer->notifyAllListenersTextureChanged( TextureGpuListener::Deleted );
            OGRE_DELETE mStencilBuffer;
            mStencilBuffer = 0;
        }
        if( mDepthBuffer )
        {
            mDepthBuffer->notifyAllListenersTextureChanged( TextureGpuListener::Deleted );
            OGRE_DELETE mDepthBuffer;
            mDepthBuffer = 0;
            mStencilBuffer = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    const char *VulkanAndroidWindow::getRequiredExtensionName( void )
    {
        return VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
    }
    //-----------------------------------------------------------------------------------
    void VulkanAndroidWindow::destroy( void )
    {
        VulkanWindow::destroy();

        if( mClosed )
            return;

        mClosed = true;
        mFocused = false;

        WindowEventUtilities::_removeRenderWindow( this );

        if( mFullscreenMode )
        {
            // switchFullScreen( false );
            mRequestedFullscreenMode = false;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanAndroidWindow::_initialize( TextureGpuManager *textureGpuManager,
                                           const NameValuePairList *miscParams )
    {
        destroy();

        mFocused = true;
        mClosed = false;
        mHwGamma = false;

        if( miscParams )
        {
            NameValuePairList::const_iterator opt;
            NameValuePairList::const_iterator end = miscParams->end();
            parseSharedParams( miscParams );
        }

        setHidden( false );

        WindowEventUtilities::_addRenderWindow( this );

        VkAndroidSurfaceCreateInfoKHR andrSurfCreateInfo;
        makeVkStruct( andrSurfCreateInfo, VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR );
        andrSurfCreateInfo.window = mNativeWindow;
        VkResult result =
            vkCreateAndroidSurfaceKHR( mDevice->mInstance, &andrSurfCreateInfo, 0, &mSurfaceKHR );
        checkVkResult( result, "vkCreateAndroidSurfaceKHR" );

        VulkanTextureGpuManager *textureManager =
            static_cast<VulkanTextureGpuManager *>( textureGpuManager );

        mTexture = textureManager->createTextureGpuWindow( this );
        mDepthBuffer = textureManager->createWindowDepthBuffer();
        mStencilBuffer = 0;

        setFinalResolution( mRequestedWidth, mRequestedHeight );
        mTexture->setPixelFormat( chooseSurfaceFormat( mHwGamma ) );
        mDepthBuffer->setPixelFormat( DepthBuffer::DefaultDepthBufferFormat );
        if( PixelFormatGpuUtils::isStencil( mDepthBuffer->getPixelFormat() ) )
            mStencilBuffer = mDepthBuffer;

        mTexture->setSampleDescription( mRequestedSampleDescription );
        mDepthBuffer->setSampleDescription( mRequestedSampleDescription );
        mSampleDescription = mRequestedSampleDescription;

        if( mDepthBuffer )
        {
            mTexture->_setDepthBufferDefaults( DepthBuffer::POOL_NON_SHAREABLE, false,
                                               mDepthBuffer->getPixelFormat() );
        }
        else
        {
            mTexture->_setDepthBufferDefaults( DepthBuffer::POOL_NO_DEPTH, false, PFG_NULL );
        }

        mTexture->_transitionTo( GpuResidency::Resident, (uint8 *)0 );
        mDepthBuffer->_transitionTo( GpuResidency::Resident, (uint8 *)0 );

        createSwapchain();
    }
    //-----------------------------------------------------------------------------------
    void VulkanAndroidWindow::reposition( int32 left, int32 top )
    {
        // Unsupported. Does nothing
    }
    //-----------------------------------------------------------------------------------
    void VulkanAndroidWindow::requestResolution( uint32 width, uint32 height )
    {
        if( mClosed )
            return;

        if( mTexture && mTexture->getWidth() == width && mTexture->getHeight() == height )
            return;

        Window::requestResolution( width, height );

        if( width != 0 && height != 0 )
        {
            setFinalResolution( width, height );
        }
    }
    //-----------------------------------------------------------------------------------
    void VulkanAndroidWindow::windowMovedOrResized( void )
    {
        if( mClosed || !mNativeWindow )
            return;

        mDevice->stall();

        destroySwapchain();

        // Depth & Stencil buffer are normal textures; thus they need to be reeinitialized normally
        if( mDepthBuffer )
            mDepthBuffer->_transitionTo( GpuResidency::OnStorage, (uint8 *)0 );
        if( mStencilBuffer && mStencilBuffer != mDepthBuffer )
            mStencilBuffer->_transitionTo( GpuResidency::OnStorage, (uint8 *)0 );

        VkSurfaceCapabilitiesKHR surfaceCaps;
        VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR( mDevice->mPhysicalDevice,
                                                                     mSurfaceKHR, &surfaceCaps );
        checkVkResult( result, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR" );

        setFinalResolution( surfaceCaps.currentExtent.width, surfaceCaps.currentExtent.height );

        if( mDepthBuffer )
            mDepthBuffer->_transitionTo( GpuResidency::Resident, (uint8 *)0 );
        if( mStencilBuffer && mStencilBuffer != mDepthBuffer )
            mStencilBuffer->_transitionTo( GpuResidency::Resident, (uint8 *)0 );

        createSwapchain();
    }
    //-------------------------------------------------------------------------
    void VulkanAndroidWindow::_setVisible( bool visible ) { mVisible = visible; }
    //-------------------------------------------------------------------------
    bool VulkanAndroidWindow::isVisible( void ) const { return mVisible; }
    //-------------------------------------------------------------------------
    void VulkanAndroidWindow::setHidden( bool hidden )
    {
        mHidden = hidden;

        // ignore for external windows as these should handle
        // this externally
        if( mIsExternal )
            return;
    }
    //-------------------------------------------------------------------------
    bool VulkanAndroidWindow::isHidden( void ) const { return false; }
    //-------------------------------------------------------------------------
    void VulkanAndroidWindow::getCustomAttribute( IdString name, void *pData )
    {
        if( name == "ANativeWindow" )
        {
            *static_cast<ANativeWindow **>( pData ) = mNativeWindow;
            return;
        }
    }
}  // namespace Ogre
