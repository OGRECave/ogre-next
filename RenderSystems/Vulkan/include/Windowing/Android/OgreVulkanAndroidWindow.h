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
            The JNI class that is assumed to be from AttachCurrentThread.
        @param activity [out]
            NativeActivity object handle, used for JNI.
        */
        virtual void get( JNIEnv **env, jobject *activity ) = 0;
    };

    class _OgreVulkanExport VulkanAndroidWindow final : public VulkanWindowSwapChainBased
    {
        ANativeWindow *mNativeWindow;
#ifdef OGRE_USE_VK_SWAPPY
        AndroidJniProvider *mJniProvider;
#endif

        bool mVisible;
        bool mHidden;
        bool mIsExternal;

        void createSwapchain() override;
        void destroySwapchain() override;

    public:
        VulkanAndroidWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode );
        ~VulkanAndroidWindow() override;

        static const char *getRequiredExtensionName();

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

        /// If the ANativeWindow changes, allows to set a new one.
        void setNativeWindow( ANativeWindow *nativeWindow );

        /// User must call this function before initializing if built with OGRE_USE_VK_SWAPPY.
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
