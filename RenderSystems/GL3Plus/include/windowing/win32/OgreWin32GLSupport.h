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
#ifndef __OgreWin32GLSupport_H__
#define __OgreWin32GLSupport_H__

#include "OgreGL3PlusRenderSystem.h"
#include "OgreGL3PlusSupport.h"
#include "OgreWin32Prerequisites.h"

namespace Ogre
{
    class _OgreGL3PlusExport Win32GLSupport : public GL3PlusSupport
    {
    public:
        Win32GLSupport();
        /**
         * Add any special config values to the system.
         * Must have a "Full Screen" value that is a bool and a "Video Mode" value
         * that is a string in the form of wxhxb
         */
        void addConfig();

        void setConfigOption( const String &name, const String &value );

        /**
         * Make sure all the extra options are valid
         */
        String validateConfig();

        virtual Window *createWindow( bool autoCreateWindow, GL3PlusRenderSystem *renderSystem,
                                      const String &windowTitle = "OGRE Render Window" );

        /// @copydoc RenderSystem::_createRenderWindow
        virtual Window *newWindow( const String &name, uint32 width, uint32 height, bool fullScreen,
                                   const NameValuePairList *miscParams = 0 );

        /**
         * Start anything special
         */
        void start();
        /**
         * Stop anything special
         */
        void stop();

        /**
         * Get the address of a function
         */
        void *getProcAddress( const char *procname ) const;

        /**
         * Initialise extensions
         */
        virtual void initialiseExtensions();

        bool selectPixelFormat( HDC hdc, int colourDepth, int multisample,
                                PixelFormatGpu depthStencilFormat, bool hwGamma );

        virtual unsigned int getDisplayMonitorCount() const;

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        void setStereoModeType( StereoModeType stereoMode ) { mStereoMode = stereoMode; }
#endif

    private:
        // Allowed video modes
        vector<DEVMODE>::type mDevModes;
        Win32Window          *mInitialWindow;
        vector<int>::type     mFSAALevels;
        bool                  mHasPixelFormatARB;
        bool                  mHasMultisample;
        bool                  mHasHardwareGamma;

        PFNWGLCHOOSEPIXELFORMATARBPROC mWglChoosePixelFormat;

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        StereoModeType mStereoMode;
#endif

        struct DisplayMonitorInfo
        {
            HMONITOR      hMonitor;
            MONITORINFOEX monitorInfoEx;
        };

        typedef vector<DisplayMonitorInfo>::type DisplayMonitorInfoList;
        typedef DisplayMonitorInfoList::iterator DisplayMonitorInfoIterator;

        DisplayMonitorInfoList mMonitorInfoList;

        void                    refreshConfig();
        void                    initialiseWGL();
        static LRESULT CALLBACK dummyWndProc( HWND hwnd, UINT umsg, WPARAM wp, LPARAM lp );
        static BOOL CALLBACK    sCreateMonitorsInfoEnumProc( HMONITOR hMonitor, HDC hdcMonitor,
                                                             LPRECT lprcMonitor, LPARAM dwData );
    };

    extern PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
}  // namespace Ogre

#endif
