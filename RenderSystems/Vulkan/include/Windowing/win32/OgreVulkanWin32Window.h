#pragma once

#include "OgreVulkanWindow.h"

#include <windows.h>

namespace Ogre
{
    class OgreVulkanWin32Window : public VulkanWindow
    {
    private:
        HWND mHwnd;  // Win32 Window handle
        HDC mHDC;
        uint32 mColourDepth;
        bool mIsExternal;
        char *mDeviceName;
        bool mIsExternalGLControl;
        bool mOwnsGLContext;
        bool mSizing;
        bool mClosed;
        bool mHidden;
        bool mVisible;
        bool mHwGamma;
        bool mIsTopLevel;
        uint8 mMsaaCount;
        DWORD mWindowedWinStyle;    // Windowed mode window style flags.
        DWORD mFullscreenWinStyle;  // Fullscreen mode window style flags.

        static bool mClassRegistered;

        /// Return the target window style depending on the fullscreen parameter.
        DWORD getWindowStyle( bool fullScreen ) const;

        void createWindow( const String &windowName, uint32 width, uint32 height,
                           const NameValuePairList *miscParams );

    public:
        OgreVulkanWin32Window( FastArray<const char *> &inOutRequiredInstanceExts, const String &title,
                               uint32 width, uint32 height, bool fullscreenMode );

        virtual ~OgreVulkanWin32Window();

        virtual void reposition( int32 left, int32 top ) override;
        virtual void _setVisible( bool visible ) override;
        virtual bool isVisible() const override;
        virtual void setHidden( bool hidden ) override;
        virtual bool isHidden() const override;
        virtual void _initialize( TextureGpuManager *textureGpuManager,
                                  const NameValuePairList *miscParams ) override;

        virtual void destroy() override;
    };

}  // namespace Ogre
