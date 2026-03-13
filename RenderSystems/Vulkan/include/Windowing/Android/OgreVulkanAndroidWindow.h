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

#ifndef _OgreVulkanAndroidWindow_H_
#define _OgreVulkanAndroidWindow_H_

#include "OgreVulkanPrerequisites.h"

#include "OgreVulkanWindow.h"

#include "OgreStringVector.h"

#include "OgreHeaderPrefix.h"

struct ANativeWindow;

struct _JNIEnv;
class _jobject;
typedef _JNIEnv JNIEnv;
typedef _jobject *jobject;

namespace Ogre
{
    class _OgreVulkanExport AndroidJniProvider
    {
    public:
        /** User must override this function.
        @param env [out]
            The JNI class that is assumed to be from AttachCurrentThread().
        @param activity [out]
            NativeActivity object handle, used for JNI.
        */
        virtual void acquire( JNIEnv **env, jobject *activity ) = 0;

        /** Notifies the implementation that we're done using the env object.
            In case the implementation wants to call DetachCurrentThread().
        @param env
            The JNI class that was retrieved via acquire().
        */
        virtual void release( JNIEnv *env ) = 0;
    };

    class _OgreVulkanExport VulkanAndroidWindow final : public VulkanWindowSwapChainBased
    {
    public:
        enum FramePacingSwappyModes
        {
            /// Try to honour the vSyncInterval set via Window::setVSync.
            /// Pipelining is always on. This is the same behavior as Desktop APIs
            /// (e.g. D3D11, Vulkan & GL on Windows and Linux).
            PipelineForcedOn,

            /// Autocalculate vSyncInterval (see Window::setVSync).
            /// The autocalculated vSyncInterval should be in the range [vSyncInterval; inf) where
            /// vSyncInterval is the value passed to Window::setVSync.
            ///
            /// While this sounds convenient, beware that Swappy will often downgrade vSyncInterval
            /// until it finds something that can be met & sustained.
            /// That means if your game runs between 40fps and 60fps on a 60hz screen, after some time
            /// Swappy will downgrade vSyncInterval to 2 so that the game renders at perfect 30fps.
            ///
            /// This may result in a better experience considering framerates jump a lot due to
            /// thermal throttling on phones. But it may also cause undesired or unexplainable
            /// "locked to 30fps for no aparent reason for a considerable time".
            ///
            /// Pipelining is always on.
            AutoVSyncInterval_PipelineForcedOn,

            /// Autocalculate vSyncInterval (see Window::setVSync).
            /// See AutoSwapInterval_PipelineForcedOn documentation.
            ///
            /// Use this mode when you know you have extremely fast render times (e.g. CPU + GPU time is
            /// below half the monitor's refresh time, e.g. it takes <= 8ms to render in a 60hz screen)
            /// which means Swappy will eventually disable pipelining.
            ///
            /// A disabled pipeline minimizes latency because the whole thing presented is ASAP by the
            /// time the next VBLANK interval arrives.
            AutoVSyncInterval_AutoPipeline,
        };

    private:
        ANativeWindow *mNativeWindow;

#ifdef OGRE_VULKAN_USE_SWAPPY
        AndroidJniProvider *mJniProvider;
        uint64 mRefreshDuration;

        // At least on one device (Redmi 4X, custom ROM), Swappy got into a resize loop:
        //
        // 1. Swappy is initialized
        // 2. I get a resize event
        // 3. Recreate the swapchain
        // 4. Swappy is reinitialized
        // 5. Go to step 2
        //
        // This causes framerate to drop catastrophically (~7 fps)
        // because every frame recreates the swapchain.
        //
        // Thus if we've recreated the swapchain N times in a second, we
        // consider Swappy busted on this device and disable it.
        int64 mFirstRecreateTimestamp;  // In milliseconds.
        uint8 mRecreateCount;

        static FramePacingSwappyModes msFramePacingSwappyMode;
#endif

        bool mVisible;
        bool mHidden;
        bool mIsExternal;

#ifdef OGRE_VULKAN_USE_SWAPPY
        void initSwappy();

        static void setFramePacingSwappyAutoMode();
#endif

        void createSurface() override;
        void createSwapchain() override;
        void destroySwapchain( bool finalDestruction = false ) override;

    public:
        VulkanAndroidWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode );
        ~VulkanAndroidWindow() override;

        static const char *getRequiredExtensionName();

        void setVSync( bool vSync, uint32 vSyncInterval ) override;

        void destroy() override;
        void _initialize( TextureGpuManager *textureGpuManager,
                          const NameValuePairList *miscParams ) override;

        void reposition( int32 left, int32 top ) override;
        void requestResolution( uint32 width, uint32 height ) override;
        void windowMovedOrResized() override;

        void _setVisible( bool visible ) override;
        bool isVisible() const override;
        void setHidden( bool hidden ) override;
        bool isHidden() const override;

        /** Sets Swappy auto swap interval and auto pipeline modes.
            See https://developer.android.com/games/sdk/frame-pacing
        @remarks
            This function is static, because it affects all Windows.
        @param bAutoSwapInterval
            Set to let Swappy autocalculate the vSyncInterval. i.e. See Window::setVSync.
            False to always try to honour the vSyncInterval set in Window::setVSync.
        */
        static void setFramePacingSwappyAutoMode( FramePacingSwappyModes mode );

#ifdef OGRE_VULKAN_USE_SWAPPY
        void _notifySwappyToggled();
#endif

        /// If the ANativeWindow changes, allows to set a new one.
        void setNativeWindow( ANativeWindow *nativeWindow );

        /// Informs us that the main looper thread received a APP_CMD_INIT_WINDOW/APP_CMD_TERM_WINDOW
        /// event and thus we should expect the swapchain to become potentially invalid (it could return
        /// VK_ERROR_OUT_OF_DATE_KHR) and thus we should avoid submitting.
        /// We expect the graphics thread to call flushQueuedNativeWindowChanges() with the new
        /// window (or with a nullptr if the window is being destroyed).
        ///
        /// Can be called from other threads.
        void requestNativeWindowChange();

        /** Must be called from main thread.
        @param nativeWindow
            Can be nullptr.
        */
        void flushQueuedNativeWindowChanges( ANativeWindow *nativeWindow );

        /// User must call this function before initializing if built with OGRE_VULKAN_USE_SWAPPY.
        ///
        /// We don't ask for the JNIEnv directly (but rather through the provider) because the dev user
        /// may chose to call DetachCurrentThread often instead of keeping the JNIEnv around.
        ///
        /// We only need to call AndroidJniProvider::get when recreating the swapchain.
        void setJniProvider( AndroidJniProvider *provider );

        void getCustomAttribute( IdString name, void *pData ) override;
    };

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
