/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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
#ifndef _OgreVulkanWindow_H_
#define _OgreVulkanWindow_H_

#include "OgreVulkanPrerequisites.h"

#include "OgreVulkanDeviceResource.h"
#include "OgreWindow.h"

namespace Ogre
{
#ifdef OGRE_PLATFORM_ANDROID
#    define OGRE_ANDROID_SURFACE_LOCK \
        std::lock_guard<std::recursive_mutex> androidLock( mAndroidSurfaceMutex )
#    define OGRE_ANDROID_SURFACE_PREVENT_USE \
        std::lock_guard<std::recursive_mutex> androidLock( mAndroidSurfaceMutex ); \
        if( mNativeWindowChangeRequested ) \
        { \
            LogManager::getSingleton().logMessage( \
                "[VulkanWindow*] Preventing surface access due to requested change." ); \
            return; \
        } \
        void()
#else
#    define OGRE_ANDROID_SURFACE_LOCK void()
#    define OGRE_ANDROID_SURFACE_PREVENT_USE void()
#endif

    OGRE_ASSUME_NONNULL_BEGIN

    class VulkanWindow : public Window
    {
    protected:
        VulkanDevice *mDevice;

        bool mHwGamma;

    public:
        VulkanWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode );

        void _setDevice( VulkanDevice *device );
    };

    class VulkanWindowNull : public VulkanWindow
    {
    public:
        VulkanWindowNull( const String &title, uint32 width, uint32 height, bool fullscreenMode );
        ~VulkanWindowNull() override;

        void destroy() override;
        void reposition( int32 leftPt, int32 topPt ) override;
        bool isClosed() const override;
        void _setVisible( bool visible ) override;
        bool isVisible() const override;
        void setHidden( bool hidden ) override;
        bool isHidden() const override;
        void _initialize( TextureGpuManager *textureGpuManager,
                          const NameValuePairList *ogre_nullable miscParams ) override;
        void swapBuffers() override;
    };

    class VulkanWindowSwapChainBased : public VulkanWindow, protected VulkanDeviceResource
    {
    public:
        enum Backend
        {
            BackendX11 = 1u << 0u
        };
        enum SwapchainStatus
        {
            /// We already called VulkanWindowSwapChainBased::acquireNextSwapchain.
            ///
            /// Can only go into this state if we're coming from SwapchainReleased
            SwapchainAcquired,
            /// We already called VulkanWindowSwapChainBased::getImageAcquiredSemaphore.
            /// Further calls to getImageAcquiredSemaphore will return null.
            /// Ogre is rendering or intends to into this swapchain.
            ///
            /// Can only go into this state if we're coming from SwapchainAcquired
            SwapchainUsedInRendering,
            /// We've come from SwapchainUsedInRendering and are waiting for
            /// VulkanDevice::commitAndNextCommandBuffer to present us
            SwapchainPendingSwap,
            /// We don't own a swapchain. Ogre cannot render to this window.
            ///
            /// This status should not last long unless we're not initialized yet.
            SwapchainReleased
        };

        bool mLowestLatencyVSync;
        bool mEnablePreTransform;
        bool mClosed;
        bool mCanDownloadData;

        VkSurfaceKHR mSurfaceKHR;
        VkSwapchainKHR mSwapchain;
        FastArray<VkImage> mSwapchainImages;
        /// Note: We need a semaphore per frame, not per swapchain.
        ///
        /// Makes Queue execution wait until the acquired image is done presenting
        VkSemaphore mSwapchainSemaphore;
        SwapchainStatus mSwapchainStatus;
        bool mRebuildingSwapchain;
        bool mSuboptimal;

#ifdef OGRE_PLATFORM_ANDROID
        // A bit ugly but here goes: Android will trigger ANRs if we spend too much in
        // onSurfaceDestroyed. Our original solution was to wait for the render thread to
        // be done rendering and release the surface, then return from onSurfaceDestroyed.
        //
        // However this causes ANRs higher than Google's threshold on slow phones, so the solution is to
        // release the surface asynchronously and return from onSurfaceDestroyed immediately. Once we get
        // a release request, we must not use the surface again.
        //
        // To avoid placing virtuals anywhere, this code is injected into VulkanWindowSwapChainBased
        // using compile-time macros instead of moving everything to VulkanAndroidWindow.
        //
        // Old drivers crash when we do this, so we use OGRE_VK_WORKAROUND_SYNC_WINDOW_INIT for them.
        std::recursive_mutex mAndroidSurfaceMutex;
        bool mNativeWindowChangeRequested;  // GUARDED_BY( mAndroidSurfaceMutex )
#endif

        void parseSharedParams( const NameValuePairList *miscParams );

        PixelFormatGpu chooseSurfaceFormat( bool hwGamma );
        virtual void createSurface() = 0;
        virtual void destroySurface();
        virtual void createSwapchain();
        virtual void destroySwapchain( bool finalDestruction = false );

        void notifyDeviceLost() override;
        void notifyDeviceRestored( unsigned pass ) override;

    public:
        void acquireNextSwapchain();

    public:
        VulkanWindowSwapChainBased( const String &title, uint32 width, uint32 height,
                                    bool fullscreenMode );
        ~VulkanWindowSwapChainBased() override;

        void destroy() override;

        /// Returns null if getImageAcquiredSemaphore has already been called during this frame
        VkSemaphore getImageAcquiredSemaphore();

        size_t getNumSwapchains() const { return mSwapchainImages.size(); }
        VkImage getSwapchainImage( size_t idx ) const { return mSwapchainImages[idx]; }

        bool isClosed() const override;

        void setVSync( bool vSync, uint32 vSyncInterval ) override;

        void setWantsToDownload( bool bWantsToDownload ) override;

        bool canDownloadData() const override;

        /// Tells our VulkanDevice that the next commitAndNextCommandBuffer call should present us
        /// Calling swapBuffers during the command buffer that is rendering to us is key for
        /// good performance; otherwise Ogre may split the commands that render to this window
        /// and the command that presents this window into two queue submissions.
        void swapBuffers() override;

        /** Actually performs present. Called by VulkanDevice::commitAndNextCommandBuffer
        @param queueFinishSemaphore
            Makes our present request wait until the Queue is done executing before we can present
        @return
            The swapchain index so that queueFinishSemaphore can be scheduled for recycling properly.
        */
        uint32 _swapBuffers( VkSemaphore queueFinishSemaphore );

        void getCustomAttribute( IdString name, void *pData ) override;
    };

    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre

#endif
