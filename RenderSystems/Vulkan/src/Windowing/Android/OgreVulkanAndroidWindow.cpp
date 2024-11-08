/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#include <android/native_window.h>

#ifdef OGRE_VULKAN_USE_SWAPPY
#    include "swappy/swappyVk.h"
#endif

namespace Ogre
{
#ifdef OGRE_VULKAN_USE_SWAPPY
    // Same default as Swappy.
    VulkanAndroidWindow::FramePacingSwappyModes VulkanAndroidWindow::msFramePacingSwappyMode =
        VulkanAndroidWindow::AutoVSyncInterval_AutoPipeline;
#endif

    VulkanAndroidWindow::VulkanAndroidWindow( const String &title, uint32 width, uint32 height,
                                              bool fullscreenMode ) :
        VulkanWindowSwapChainBased( title, width, height, fullscreenMode ),
        mNativeWindow( 0 ),
#ifdef OGRE_VULKAN_USE_SWAPPY
        mJniProvider( 0 ),
        mRefreshDuration( 0 ),
        mFirstRecreateTimestamp( 0u ),
        mRecreateCount( 255u ),
#endif
        mVisible( true ),
        mHidden( false ),
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
    const char *VulkanAndroidWindow::getRequiredExtensionName()
    {
        return VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
    }
    //-----------------------------------------------------------------------------------
    void VulkanAndroidWindow::destroy()
    {
        if( mNativeWindow )
        {
            // Android is destroying our window. Possibly user pressed the home or power
            // button.
            //
            // We must flush all our references to the old swapchain otherwise when
            // the app goes to foreground again and submit that stale content Mali
            // will return DEVICE_LOST
            mDevice->stall();
        }

        VulkanWindowSwapChainBased::destroy();

        if( mClosed )
            return;

        mClosed = true;
        mFocused = false;

#ifdef OGRE_VULKAN_USE_SWAPPY
        if( mSwapchain && mDevice->mRenderSystem->getSwappyFramePacing() )
            SwappyVk_setWindow( mDevice->mDevice, mSwapchain, mNativeWindow );
#endif

        // WindowEventUtilities::_removeRenderWindow( this );

        if( mFullscreenMode )
        {
            // switchFullScreen( false );
            mRequestedFullscreenMode = false;
        }

        if( mNativeWindow )
        {
            ANativeWindow_release( mNativeWindow );
            mNativeWindow = nullptr;
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

        ANativeWindow *nativeWindow = 0;

        if( miscParams )
        {
            NameValuePairList::const_iterator opt;
            NameValuePairList::const_iterator end = miscParams->end();
            parseSharedParams( miscParams );

            opt = miscParams->find( "ANativeWindow" );
            if( opt != end )
            {
                nativeWindow = reinterpret_cast<ANativeWindow *>(
                    StringConverter::parseUnsignedLong( opt->second ) );
            }

#ifdef OGRE_VULKAN_USE_SWAPPY
            opt = miscParams->find( "AndroidJniProvider" );
            if( opt != end )
            {
                mJniProvider = reinterpret_cast<AndroidJniProvider *>(
                    StringConverter::parseUnsignedLong( opt->second ) );
            }
#endif
        }

        if( !nativeWindow )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "App must provide ANativeWindow via Misc Params!",
                         "VulkanAndroidWindow::_initialize" );
        }

#ifdef OGRE_VULKAN_USE_SWAPPY
        if( !mJniProvider )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "App must provide AndroidJniProvider via Misc Params!",
                         "VulkanAndroidWindow::_initialize" );
        }
#endif

        setHidden( false );

        VulkanTextureGpuManager *textureManager =
            static_cast<VulkanTextureGpuManager *>( textureGpuManager );
        mTexture = textureManager->createTextureGpuWindow( this );
        if( DepthBuffer::DefaultDepthBufferFormat != PFG_NULL )
            mDepthBuffer = textureManager->createWindowDepthBuffer();
        mStencilBuffer = 0;

        setNativeWindow( nativeWindow );
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
#ifdef OGRE_VULKAN_USE_SWAPPY
    static int64_t timeInMilliseconds()
    {
        timespec clockTime;
        clock_gettime( CLOCK_MONOTONIC, &clockTime );
        return clockTime.tv_sec * 1000 + clockTime.tv_nsec / 1000000;
    }
#endif

    void VulkanAndroidWindow::windowMovedOrResized()
    {
        if( mClosed || !mNativeWindow )
            return;

        const uint32 newWidth = static_cast<uint32>( ANativeWindow_getWidth( mNativeWindow ) );
        const uint32 newHeight = static_cast<uint32>( ANativeWindow_getHeight( mNativeWindow ) );
        if( newWidth == getWidth() && newHeight == getHeight() && !mRebuildingSwapchain )
            return;

        mDevice->stall();

#ifdef OGRE_VULKAN_USE_SWAPPY
        // Code to detect buggy devices (if it's buggy, we disable Swappy).
        if( mRecreateCount == 255u )
        {
            mRecreateCount = 0u;
            mFirstRecreateTimestamp = timeInMilliseconds();
        }
        else
        {
            int64 currTimestamp = timeInMilliseconds();
            if( currTimestamp - mFirstRecreateTimestamp <= 1500 )
            {
                ++mRecreateCount;
                if( mRecreateCount >= 5u )
                {
                    // We're disabling Swappy. It's likely buggy.
                    LogManager::getSingleton().logMessage(
                        "Swapchain recreated too many times within one second. "
                        "Disabling Swappy Frame Pacing.",
                        LML_CRITICAL );

                    mDevice->mRenderSystem->setSwappyFramePacing( false, this );
                }
            }
            else
            {
                mRecreateCount = 255u;
            }
        }
#endif

        destroySwapchain();

        // Depth & Stencil buffer are normal textures; thus they need to be reeinitialized normally
        if( mDepthBuffer && mDepthBuffer->getResidencyStatus() != GpuResidency::OnStorage )
            mDepthBuffer->_transitionTo( GpuResidency::OnStorage, (uint8 *)0 );
        if( mStencilBuffer && mStencilBuffer != mDepthBuffer &&
            mStencilBuffer->getResidencyStatus() != GpuResidency::OnStorage )
        {
            mStencilBuffer->_transitionTo( GpuResidency::OnStorage, (uint8 *)0 );
        }

        setFinalResolution( newWidth, newHeight );

        createSwapchain();
    }
    //-------------------------------------------------------------------------
    void VulkanAndroidWindow::_setVisible( bool visible ) { mVisible = visible; }
    //-------------------------------------------------------------------------
    bool VulkanAndroidWindow::isVisible() const { return mVisible; }
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
    bool VulkanAndroidWindow::isHidden() const { return false; }
    //-------------------------------------------------------------------------
    void VulkanAndroidWindow::setFramePacingSwappyAutoMode( FramePacingSwappyModes mode )
    {
#ifdef OGRE_VULKAN_USE_SWAPPY
        msFramePacingSwappyMode = mode;
        setFramePacingSwappyAutoMode();
#endif
    }
    //-------------------------------------------------------------------------
#ifdef OGRE_VULKAN_USE_SWAPPY
    void VulkanAndroidWindow::setFramePacingSwappyAutoMode()
    {
        switch( msFramePacingSwappyMode )
        {
        case AutoVSyncInterval_AutoPipeline:
            SwappyVk_setAutoSwapInterval( true );
            SwappyVk_setAutoPipelineMode( true );
            break;
        case AutoVSyncInterval_PipelineForcedOn:
            SwappyVk_setAutoSwapInterval( true );
            SwappyVk_setAutoPipelineMode( false );
            break;
        case PipelineForcedOn:
            SwappyVk_setAutoSwapInterval( false );
            SwappyVk_setAutoPipelineMode( false );
            break;
            // Note:
            //  SwappyVk_setAutoSwapInterval( false );
            //  SwappyVk_setAutoPipelineMode( true );
            // Is ignored by Swappy as it makes no sense.
        }
    }
#endif
    //-------------------------------------------------------------------------
#ifdef OGRE_VULKAN_USE_SWAPPY
    void VulkanAndroidWindow::_notifySwappyToggled()
    {
        if( !mDevice->mRenderSystem->getSwappyFramePacing() )
        {
            // Disabling
            if( mSwapchain )
            {
                SwappyVk_setWindow( mDevice->mDevice, mSwapchain, nullptr );
                SwappyVk_destroySwapchain( mDevice->mDevice, mSwapchain );
            }
        }
        else
        {
            // Enabling
            if( mSwapchain )
                initSwappy();
        }
    }
#endif
    //-------------------------------------------------------------------------
    void VulkanAndroidWindow::setNativeWindow( ANativeWindow *nativeWindow )
    {
        destroy();

        // Depth & Stencil buffer are normal textures; thus they need to be reeinitialized normally
        if( mDepthBuffer && mDepthBuffer->getResidencyStatus() != GpuResidency::OnStorage )
            mDepthBuffer->_transitionTo( GpuResidency::OnStorage, (uint8 *)0 );
        if( mStencilBuffer && mStencilBuffer != mDepthBuffer &&
            mStencilBuffer->getResidencyStatus() != GpuResidency::OnStorage )
        {
            mStencilBuffer->_transitionTo( GpuResidency::OnStorage, (uint8 *)0 );
        }

        if( mNativeWindow != nativeWindow )
        {
            if( mNativeWindow )
                ANativeWindow_release( mNativeWindow );
            mNativeWindow = nativeWindow;
            if( mNativeWindow )
                ANativeWindow_acquire( mNativeWindow );
        }

        if( !mNativeWindow )
            return;

        mClosed = false;
        mFocused = true;
        // WindowEventUtilities::_addRenderWindow( this );

        VkAndroidSurfaceCreateInfoKHR andrSurfCreateInfo;
        makeVkStruct( andrSurfCreateInfo, VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR );
        andrSurfCreateInfo.window = mNativeWindow;
        VkResult result =
            vkCreateAndroidSurfaceKHR( mDevice->mInstance, &andrSurfCreateInfo, 0, &mSurfaceKHR );
        checkVkResult( result, "vkCreateAndroidSurfaceKHR" );

        const uint32 newWidth = static_cast<uint32>( ANativeWindow_getWidth( mNativeWindow ) );
        const uint32 newHeight = static_cast<uint32>( ANativeWindow_getHeight( mNativeWindow ) );

        mRequestedWidth = newWidth;
        mRequestedHeight = newHeight;

        setFinalResolution( mRequestedWidth, mRequestedHeight );

        // mTexture is in OnStorage only once ever: at startup. Set these parameters once
        if( mTexture->getResidencyStatus() == GpuResidency::OnStorage )
        {
            mTexture->setPixelFormat( chooseSurfaceFormat( mHwGamma ) );
            if( mDepthBuffer )
            {
                mDepthBuffer->setPixelFormat( DepthBuffer::DefaultDepthBufferFormat );
                if( PixelFormatGpuUtils::isStencil( mDepthBuffer->getPixelFormat() ) )
                    mStencilBuffer = mDepthBuffer;
            }

            mSampleDescription = mDevice->mRenderSystem->validateSampleDescription(
                mRequestedSampleDescription, mTexture->getPixelFormat(),
                TextureFlags::NotTexture | TextureFlags::RenderWindowSpecific );
            mTexture->_setSampleDescription( mRequestedSampleDescription, mSampleDescription );
            if( mDepthBuffer )
                mDepthBuffer->_setSampleDescription( mRequestedSampleDescription, mSampleDescription );

            if( mDepthBuffer )
            {
                mTexture->_setDepthBufferDefaults( DepthBuffer::NO_POOL_EXPLICIT_RTV, false,
                                                   mDepthBuffer->getPixelFormat() );
            }
            else
            {
                mTexture->_setDepthBufferDefaults( DepthBuffer::POOL_NO_DEPTH, false, PFG_NULL );
            }
        }

        createSwapchain();
    }
    //-------------------------------------------------------------------------
    void VulkanAndroidWindow::setJniProvider( AndroidJniProvider *provider )
    {
#ifdef OGRE_VULKAN_USE_SWAPPY
        mJniProvider = provider;
#endif
    }
    //-------------------------------------------------------------------------
    void VulkanAndroidWindow::setVSync( bool vSync, uint32 vSyncInterval )
    {
#ifdef OGRE_VULKAN_USE_SWAPPY
        const bool bSwapchainWillbeRecreated = mVSync != vSync;
#endif
        VulkanWindowSwapChainBased::setVSync( vSync, vSyncInterval );

#ifdef OGRE_VULKAN_USE_SWAPPY
        if( !bSwapchainWillbeRecreated && mSwapchain && mDevice->mRenderSystem->getSwappyFramePacing() )
        {
            SwappyVk_setSwapIntervalNS( mDevice->mDevice, mSwapchain,
                                        mRefreshDuration * mVSyncInterval );
        }
#endif
    }
//-------------------------------------------------------------------------
#ifdef OGRE_VULKAN_USE_SWAPPY
    void VulkanAndroidWindow::initSwappy()
    {
        if( !mDevice->mRenderSystem->getSwappyFramePacing() )
        {
            mFrequencyNumerator = 0u;
            mFrequencyDenominator = 0u;
            return;
        }

        if( !mJniProvider )
        {
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "VulkanAndroidWindow::setJniProvider not called or called with nullptr. This "
                         "function is mandatory to set if built with Swappy support for Android.",
                         "VulkanAndroidWindow::createSwapchain" );
        }

        {
            JNIEnv *jni = 0;
            jobject nativeActivityClass = 0;
            mJniProvider->acquire( &jni, &nativeActivityClass );
            SwappyVk_initAndGetRefreshCycleDuration( jni, nativeActivityClass, mDevice->mPhysicalDevice,
                                                     mDevice->mDevice, mSwapchain, &mRefreshDuration );
            mJniProvider->release( jni );
        }

        // Swappy wants to know the mNativeWindow every time the Swapchain changes.
        // If we try to set mNativeWindow without a valid mSwapchain yet, it won't work correctly.
        // So we must do this here, every time the Swapchain gets recreated
        // (Swappy inverts the relationship: It works as if mNativeWindow depended on Swapchains).
        OGRE_ASSERT_LOW( mSwapchain );  // should've thrown by now if mSwapchain creation failed.
        SwappyVk_setWindow( mDevice->mDevice, mSwapchain, mNativeWindow );

        if( mRefreshDuration <= std::numeric_limits<uint32>::max() )
        {
            mFrequencyDenominator = static_cast<uint32>( mRefreshDuration );
            mFrequencyNumerator = 1000000000u;
        }
        else
        {
            mFrequencyNumerator = 0u;
            mFrequencyDenominator = 0u;
        }
        SwappyVk_setSwapIntervalNS( mDevice->mDevice, mSwapchain, mRefreshDuration * mVSyncInterval );

        setFramePacingSwappyAutoMode();
    }
#endif
    //-------------------------------------------------------------------------
    void VulkanAndroidWindow::createSwapchain()
    {
        VulkanWindowSwapChainBased::createSwapchain();
#ifdef OGRE_VULKAN_USE_SWAPPY
        initSwappy();
#endif
    }
    //-------------------------------------------------------------------------
    void VulkanAndroidWindow::destroySwapchain()
    {
#ifdef OGRE_VULKAN_USE_SWAPPY
        // Swappy has a bug where calling SwappyVk_destroySwapchain will leak the mNativeWindow.
        // So we must call SwappyVk_setWindow() ourselves.
        if( mNativeWindow )
            SwappyVk_setWindow( mDevice->mDevice, mSwapchain, mNativeWindow );
        SwappyVk_destroySwapchain( mDevice->mDevice, mSwapchain );
#endif
        VulkanWindowSwapChainBased::destroySwapchain();
    }
    //-------------------------------------------------------------------------
    void VulkanAndroidWindow::getCustomAttribute( IdString name, void *pData )
    {
        if( name == "ANativeWindow" || name == "RENDERDOC_WINDOW" )
        {
            *static_cast<ANativeWindow **>( pData ) = mNativeWindow;
            return;
        }
        else
        {
            VulkanWindowSwapChainBased::getCustomAttribute( name, pData );
        }
    }
}  // namespace Ogre
