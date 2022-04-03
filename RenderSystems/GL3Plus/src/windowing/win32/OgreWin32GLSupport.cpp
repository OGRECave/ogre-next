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

#include "OgreWin32GLSupport.h"

#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreRoot.h"
#include "OgreStringConverter.h"

#include <algorithm>

#include "OgreWin32Window.h"

#include <GL/wglext.h>

#include <sstream>

using namespace Ogre;

namespace Ogre
{
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = 0;

    Win32GLSupport::Win32GLSupport() :
        mInitialWindow( 0 ),
        mHasPixelFormatARB( false ),
        mHasMultisample( false ),
        mHasHardwareGamma( false ),
        mWglChoosePixelFormat( 0 )
    {
        // immediately test WGL_ARB_pixel_format and FSAA support
        // so we can set configuration options appropriately
        initialiseWGL();
#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        mStereoMode = SMT_NONE;
#endif
    }

    template <class C>
    void remove_duplicates( C &c )
    {
        std::sort( c.begin(), c.end() );
        typename C::iterator p = std::unique( c.begin(), c.end() );
        c.erase( p, c.end() );
    }

    void Win32GLSupport::addConfig()
    {
        // TODO: EnumDisplayDevices http://msdn.microsoft.com/library/en-us/gdi/devcons_2303.asp
        /*vector<string> DisplayDevices;
        DISPLAY_DEVICE DisplayDevice;
        DisplayDevice.cb = sizeof(DISPLAY_DEVICE);
        DWORD i=0;
        while (EnumDisplayDevices(NULL, i++, &DisplayDevice, 0) {
            DisplayDevices.push_back(DisplayDevice.DeviceName);
        }*/

        ConfigOption optFullScreen;
        ConfigOption optVideoMode;
        ConfigOption optColourDepth;
        ConfigOption optDisplayFrequency;
        ConfigOption optVSync;
        ConfigOption optVSyncInterval;
        ConfigOption optFSAA;
        ConfigOption optRTTMode;
        ConfigOption optSRGB;
#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        ConfigOption optStereoMode;
#endif

        // FS setting possibilities
        optFullScreen.name = "Full Screen";
        optFullScreen.possibleValues.push_back( "Yes" );
        optFullScreen.possibleValues.push_back( "No" );
        optFullScreen.currentValue = "Yes";
        optFullScreen.immutable = false;

        // Video mode possibilities
        DEVMODE DevMode;
        DevMode.dmSize = sizeof( DEVMODE );
        optVideoMode.name = "Video Mode";
        optVideoMode.immutable = false;
        for( DWORD i = 0; EnumDisplaySettings( NULL, i, &DevMode ); ++i )
        {
            if( DevMode.dmBitsPerPel < 16 || DevMode.dmPelsHeight < 480 )
                continue;
            mDevModes.push_back( DevMode );
            StringStream str;
            str << DevMode.dmPelsWidth << " x " << DevMode.dmPelsHeight;
            optVideoMode.possibleValues.push_back( str.str() );
        }
        remove_duplicates( optVideoMode.possibleValues );
        optVideoMode.currentValue = optVideoMode.possibleValues.front();

        optColourDepth.name = "Colour Depth";
        optColourDepth.immutable = false;
        optColourDepth.currentValue.clear();

        optDisplayFrequency.name = "Display Frequency";
        optDisplayFrequency.immutable = false;
        optDisplayFrequency.currentValue.clear();

        optVSync.name = "VSync";
        optVSync.immutable = false;
        optVSync.possibleValues.push_back( "No" );
        optVSync.possibleValues.push_back( "Yes" );
        optVSync.currentValue = "No";

        optVSyncInterval.name = "VSync Interval";
        optVSyncInterval.immutable = false;
        optVSyncInterval.possibleValues.push_back( "1" );
        optVSyncInterval.possibleValues.push_back( "2" );
        optVSyncInterval.possibleValues.push_back( "3" );
        optVSyncInterval.possibleValues.push_back( "4" );
        optVSyncInterval.currentValue = "1";

        optFSAA.name = "FSAA";
        optFSAA.immutable = false;
        optFSAA.possibleValues.push_back( "1" );
        for( vector<int>::type::iterator it = mFSAALevels.begin(); it != mFSAALevels.end(); ++it )
        {
            String val = StringConverter::toString( *it );
            optFSAA.possibleValues.push_back( val );
            /* not implementing CSAA in GL for now
            if (*it >= 8)
                optFSAA.possibleValues.push_back(val + " [Quality]");
            */
        }
        optFSAA.currentValue = "1";

        optRTTMode.name = "RTT Preferred Mode";
        optRTTMode.possibleValues.push_back( "FBO" );
        optRTTMode.possibleValues.push_back( "PBuffer" );
        optRTTMode.possibleValues.push_back( "Copy" );
        optRTTMode.currentValue = "FBO";
        optRTTMode.immutable = false;

        // SRGB on auto window
        optSRGB.name = "sRGB Gamma Conversion";
        optSRGB.possibleValues.push_back( "Yes" );
        optSRGB.possibleValues.push_back( "No" );
        optSRGB.currentValue = "Yes";
        optSRGB.immutable = false;

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        optStereoMode.name = "Stereo Mode";
        optStereoMode.possibleValues.push_back( StringConverter::toString( SMT_NONE ) );
        optStereoMode.possibleValues.push_back( StringConverter::toString( SMT_FRAME_SEQUENTIAL ) );
        optStereoMode.currentValue = optStereoMode.possibleValues[0];
        optStereoMode.immutable = false;

        mOptions[optStereoMode.name] = optStereoMode;
#endif

        mOptions[optFullScreen.name] = optFullScreen;
        mOptions[optVideoMode.name] = optVideoMode;
        mOptions[optColourDepth.name] = optColourDepth;
        mOptions[optDisplayFrequency.name] = optDisplayFrequency;
        mOptions[optVSync.name] = optVSync;
        mOptions[optVSyncInterval.name] = optVSyncInterval;
        mOptions[optFSAA.name] = optFSAA;
        mOptions[optRTTMode.name] = optRTTMode;
        mOptions[optSRGB.name] = optSRGB;

        refreshConfig();
    }

    void Win32GLSupport::refreshConfig()
    {
        ConfigOptionMap::iterator optVideoMode = mOptions.find( "Video Mode" );
        ConfigOptionMap::iterator moptColourDepth = mOptions.find( "Colour Depth" );
        ConfigOptionMap::iterator moptDisplayFrequency = mOptions.find( "Display Frequency" );
        if( optVideoMode == mOptions.end() || moptColourDepth == mOptions.end() ||
            moptDisplayFrequency == mOptions.end() )
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Can't find mOptions!",
                         "Win32GLSupport::refreshConfig" );
        ConfigOption *optColourDepth = &moptColourDepth->second;
        ConfigOption *optDisplayFrequency = &moptDisplayFrequency->second;

        const String &val = optVideoMode->second.currentValue;
        String::size_type pos = val.find( 'x' );
        if( pos == String::npos )
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Invalid Video Mode provided",
                         "Win32GLSupport::refreshConfig" );
        DWORD width = StringConverter::parseUnsignedInt( val.substr( 0, pos ) );
        DWORD height = StringConverter::parseUnsignedInt( val.substr( pos + 1, String::npos ) );

        for( vector<DEVMODE>::type::const_iterator i = mDevModes.begin(); i != mDevModes.end(); ++i )
        {
            if( i->dmPelsWidth != width || i->dmPelsHeight != height )
                continue;
            optColourDepth->possibleValues.push_back(
                StringConverter::toString( (unsigned int)i->dmBitsPerPel ) );
            optDisplayFrequency->possibleValues.push_back(
                StringConverter::toString( (unsigned int)i->dmDisplayFrequency ) );
        }
        remove_duplicates( optColourDepth->possibleValues );
        remove_duplicates( optDisplayFrequency->possibleValues );
        optColourDepth->currentValue = optColourDepth->possibleValues.back();
        bool freqValid =
            std::find( optDisplayFrequency->possibleValues.begin(),
                       optDisplayFrequency->possibleValues.end(),
                       optDisplayFrequency->currentValue ) != optDisplayFrequency->possibleValues.end();

        if( ( optDisplayFrequency->currentValue != "N/A" ) && !freqValid )
            optDisplayFrequency->currentValue = optDisplayFrequency->possibleValues.front();
    }

    void Win32GLSupport::setConfigOption( const String &name, const String &value )
    {
        ConfigOptionMap::iterator it = mOptions.find( name );

        // Update
        if( it != mOptions.end() )
            it->second.currentValue = value;
        else
        {
            StringStream str;
            str << "Option named '" << name << "' does not exist.";
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, str.str(), "Win32GLSupport::setConfigOption" );
        }

        if( name == "Video Mode" )
            refreshConfig();

        if( name == "Full Screen" )
        {
            it = mOptions.find( "Display Frequency" );
            if( value == "No" )
            {
                it->second.currentValue = "N/A";
                it->second.immutable = true;
            }
            else
            {
                if( it->second.currentValue.empty() || it->second.currentValue == "N/A" )
                    it->second.currentValue = it->second.possibleValues.front();
                it->second.immutable = false;
            }
        }
    }

    String Win32GLSupport::validateConfig()
    {
        // TODO, DX9
        return BLANKSTRING;
    }

    Window *Win32GLSupport::createWindow( bool autoCreateWindow, GL3PlusRenderSystem *renderSystem,
                                          const String &windowTitle )
    {
        if( autoCreateWindow )
        {
            ConfigOptionMap::iterator opt = mOptions.find( "Full Screen" );
            if( opt == mOptions.end() )
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Can't find full screen options!",
                             "Win32GLSupport::createWindow" );
            bool fullscreen = ( opt->second.currentValue == "Yes" );

            opt = mOptions.find( "Video Mode" );
            if( opt == mOptions.end() )
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Can't find video mode options!",
                             "Win32GLSupport::createWindow" );
            String val = opt->second.currentValue;
            String::size_type pos = val.find( 'x' );
            if( pos == String::npos )
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Invalid Video Mode provided",
                             "Win32GLSupport::createWindow" );

            unsigned int w = StringConverter::parseUnsignedInt( val.substr( 0, pos ) );
            unsigned int h = StringConverter::parseUnsignedInt( val.substr( pos + 1 ) );

            // Parse optional parameters
            NameValuePairList winOptions;
            opt = mOptions.find( "Colour Depth" );
            if( opt == mOptions.end() )
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Can't find Colour Depth options!",
                             "Win32GLSupport::createWindow" );
            unsigned int colourDepth = StringConverter::parseUnsignedInt( opt->second.currentValue );
            winOptions["colourDepth"] = StringConverter::toString( colourDepth );

            opt = mOptions.find( "VSync" );
            if( opt == mOptions.end() )
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Can't find VSync options!",
                             "Win32GLSupport::createWindow" );
            bool vsync = ( opt->second.currentValue == "Yes" );
            winOptions["vsync"] = StringConverter::toString( vsync );

            opt = mOptions.find( "VSync Interval" );
            if( opt == mOptions.end() )
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Can't find VSync Interval options!",
                             "Win32GLSupport::createWindow" );
            winOptions["vsyncInterval"] = opt->second.currentValue;

            opt = mOptions.find( "Display Frequency" );
            if( opt != mOptions.end() )
            {
                unsigned int displayFrequency =
                    StringConverter::parseUnsignedInt( opt->second.currentValue );
                winOptions["displayFrequency"] = StringConverter::toString( displayFrequency );
            }

            opt = mOptions.find( "FSAA" );
            if( opt == mOptions.end() )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Can't find FSAA options!",
                             "Win32GLSupport::createWindow" );
            }
            String fsaa = opt->second.currentValue;
            winOptions["FSAA"] = fsaa;

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
            opt = mOptions.find( "Stereo Mode" );
            if( opt == mOptions.end() )
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Can't find stereo enabled options!",
                             "Win32GLSupport::createWindow" );
            winOptions["stereoMode"] = opt->second.currentValue;
            mStereoMode = StringConverter::parseStereoMode( opt->second.currentValue );
#endif

            opt = mOptions.find( "sRGB Gamma Conversion" );
            if( opt == mOptions.end() )
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Can't find sRGB options!",
                             "Win32GLSupport::createWindow" );
            bool hwGamma = ( opt->second.currentValue == "Yes" );
            winOptions["gamma"] = StringConverter::toString( hwGamma );

            return renderSystem->_createRenderWindow( windowTitle, w, h, fullscreen, &winOptions );
        }
        else
        {
            // XXX What is the else?
            return NULL;
        }
    }

    BOOL CALLBACK Win32GLSupport::sCreateMonitorsInfoEnumProc(
        HMONITOR hMonitor,   // handle to display monitor
        HDC hdcMonitor,      // handle to monitor DC
        LPRECT lprcMonitor,  // monitor intersection rectangle
        LPARAM dwData        // data
    )
    {
        DisplayMonitorInfoList *pArrMonitorsInfo = (DisplayMonitorInfoList *)dwData;

        // Get monitor info
        DisplayMonitorInfo displayMonitorInfo;

        displayMonitorInfo.hMonitor = hMonitor;

        memset( &displayMonitorInfo.monitorInfoEx, 0, sizeof( MONITORINFOEX ) );
        displayMonitorInfo.monitorInfoEx.cbSize = sizeof( MONITORINFOEX );
        GetMonitorInfo( hMonitor, &displayMonitorInfo.monitorInfoEx );

        pArrMonitorsInfo->push_back( displayMonitorInfo );

        return TRUE;
    }

    Window *Win32GLSupport::newWindow( const String &name, uint32 width, uint32 height, bool fullScreen,
                                       const NameValuePairList *miscParams )
    {
        Win32Window *window =
            OGRE_NEW Win32Window( name, width, height, fullScreen, PFG_UNKNOWN, miscParams, *this );
        NameValuePairList newParams;

        if( miscParams != NULL )
        {
            newParams = *miscParams;
            miscParams = &newParams;

            NameValuePairList::const_iterator monitorIndexIt = miscParams->find( "monitorIndex" );
            HMONITOR hMonitor = NULL;
            int monitorIndex = -1;

            // If monitor index found, try to assign the monitor handle based on it.
            if( monitorIndexIt != miscParams->end() )
            {
                if( mMonitorInfoList.empty() )
                    EnumDisplayMonitors( NULL, NULL, sCreateMonitorsInfoEnumProc,
                                         (LPARAM)&mMonitorInfoList );

                monitorIndex = StringConverter::parseInt( monitorIndexIt->second );
                if( monitorIndex < (int)mMonitorInfoList.size() )
                {
                    hMonitor = mMonitorInfoList[monitorIndex].hMonitor;
                }
            }
            // If we didn't specified the monitor index, or if it didn't find it
            if( hMonitor == NULL )
            {
                POINT windowAnchorPoint;

                NameValuePairList::const_iterator opt;
                int left = -1;
                int top = -1;

                if( ( opt = newParams.find( "left" ) ) != newParams.end() )
                    left = StringConverter::parseInt( opt->second );

                if( ( opt = newParams.find( "top" ) ) != newParams.end() )
                    top = StringConverter::parseInt( opt->second );

                // Fill in anchor point.
                windowAnchorPoint.x = left;
                windowAnchorPoint.y = top;

                // Get the nearest monitor to this window.
                hMonitor = MonitorFromPoint( windowAnchorPoint, MONITOR_DEFAULTTOPRIMARY );
            }

            newParams["monitorHandle"] = StringConverter::toString( (size_t)hMonitor );
        }

        if( !mInitialWindow )
            mInitialWindow = window;
        return window;
    }

    void Win32GLSupport::start()
    {
        LogManager::getSingleton().logMessage( "*** Starting Win32GL Subsystem ***" );
    }

    void Win32GLSupport::stop()
    {
        LogManager::getSingleton().logMessage( "*** Stopping Win32GL Subsystem ***" );
        mInitialWindow = 0;  // Since there is no removeWindow, although there should be...
    }

    void Win32GLSupport::initialiseExtensions()
    {
        assert( mInitialWindow );
        // First, initialise the normal extensions
        GL3PlusSupport::initialiseExtensions();

        // Check for W32 specific extensions probe function
        PFNWGLGETEXTENSIONSSTRINGARBPROC _wglGetExtensionsStringARB =
            (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress( "wglGetExtensionsStringARB" );
        if( !_wglGetExtensionsStringARB )
            return;
        const char *wgl_extensions = _wglGetExtensionsStringARB( mInitialWindow->getHDC() );
        LogManager::getSingleton().stream() << "Supported WGL extensions: " << wgl_extensions;
        // Parse them, and add them to the main list
        StringStream ext;
        String instr;
        ext << wgl_extensions;
        while( ext >> instr )
        {
            extensionList.insert( instr );
        }
    }

    void *Win32GLSupport::getProcAddress( const char *procname ) const
    {
        return (void *)wglGetProcAddress( procname );
    }
    void Win32GLSupport::initialiseWGL()
    {
        // wglGetProcAddress does not work without an active OpenGL context,
        // but we need wglChoosePixelFormatARB's address before we can
        // create our main window.  Thank you very much, Microsoft!
        //
        // The solution is to create a dummy OpenGL window first, and then
        // test for WGL_ARB_pixel_format support.  If it is not supported,
        // we make sure to never call the ARB pixel format functions.
        //
        // If is is supported, we call the pixel format functions at least once
        // to initialise them (pointers are stored by glprocs.h).  We can also
        // take this opportunity to enumerate the valid FSAA modes.

        LPCSTR dummyText = "OgreWglDummy";

        HINSTANCE hinst = NULL;
#ifdef __MINGW32__
#    ifdef OGRE_STATIC_LIB
        hinst = GetModuleHandle( NULL );
#    else
#        if OGRE_DEBUG_MODE == 1
        hinst = GetModuleHandle( "RenderSystem_GL3Plus_d.dll" );
#        else
        hinst = GetModuleHandle( "RenderSystem_GL3Plus.dll" );
#        endif
#    endif
#else
        static const TCHAR staticVar;
        GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            &staticVar, &hinst );
#endif

        WNDCLASS dummyClass;
        memset( &dummyClass, 0, sizeof( WNDCLASS ) );
        dummyClass.style = CS_OWNDC;
        dummyClass.hInstance = hinst;
        dummyClass.lpfnWndProc = dummyWndProc;
        dummyClass.lpszClassName = dummyText;
        RegisterClass( &dummyClass );

        HWND hwnd = CreateWindow( dummyText, dummyText, WS_POPUP | WS_CLIPCHILDREN, 0, 0, 32, 32, 0, 0,
                                  hinst, 0 );

        // if a simple CreateWindow fails, then boy are we in trouble...
        if( hwnd == NULL )
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "CreateWindow() failed",
                         "Win32GLSupport::initializeWGL" );

        // no chance of failure and no need to release thanks to CS_OWNDC
        HDC hdc = GetDC( hwnd );

        // assign a simple OpenGL pixel format that everyone supports
        PIXELFORMATDESCRIPTOR pfd;
        memset( &pfd, 0, sizeof( PIXELFORMATDESCRIPTOR ) );
        pfd.nSize = sizeof( PIXELFORMATDESCRIPTOR );
        pfd.nVersion = 1;
        pfd.cColorBits = 16;
        pfd.cDepthBits = 15;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;

        // if these fail, wglCreateContext will also quietly fail
        int format;
        if( ( format = ChoosePixelFormat( hdc, &pfd ) ) != 0 )
            SetPixelFormat( hdc, format, &pfd );

        HGLRC hrc = wglCreateContext( hdc );
        if( hrc )
        {
            HGLRC oldrc = wglGetCurrentContext();
            HDC oldhdc = wglGetCurrentDC();
            // if wglMakeCurrent fails, wglGetProcAddress will return null
            wglMakeCurrent( hdc, hrc );

            wglCreateContextAttribsARB =
                (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress( "wglCreateContextAttribsARB" );

            PFNWGLGETEXTENSIONSSTRINGARBPROC _wglGetExtensionsStringARB =
                (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress( "wglGetExtensionsStringARB" );

            // check for pixel format and multisampling support
            if( _wglGetExtensionsStringARB )
            {
                std::istringstream wglexts( _wglGetExtensionsStringARB( hdc ) );
                std::string ext;
                while( wglexts >> ext )
                {
                    if( ext == "WGL_ARB_pixel_format" )
                        mHasPixelFormatARB = true;
                    else if( ext == "WGL_ARB_multisample" )
                        mHasMultisample = true;
                    else if( ext == "WGL_EXT_framebuffer_sRGB" || ext == "WGL_ARB_framebuffer_sRGB" )
                        mHasHardwareGamma = true;
                }
            }

            if( mHasPixelFormatARB && mHasMultisample )
            {
                // enumerate all formats w/ multisampling
                static const int iattr[] = { WGL_DRAW_TO_WINDOW_ARB, GL_TRUE, WGL_SUPPORT_OPENGL_ARB,
                                             GL_TRUE, WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
                                             WGL_SAMPLE_BUFFERS_ARB, GL_TRUE, WGL_ACCELERATION_ARB,
                                             WGL_FULL_ACCELERATION_ARB,
                                             /* We are no matter about the colour, depth and stencil
                                             buffers here WGL_COLOR_BITS_ARB, 24, WGL_ALPHA_BITS_ARB, 8,
                                             WGL_DEPTH_BITS_ARB, 24,
                                             WGL_STENCIL_BITS_ARB, 8,
                                             */
                                             WGL_SAMPLES_ARB, 2, 0 };
                int formats[256];
                memset( formats, 0, sizeof( formats ) );
                unsigned int count = 0;
                // cheating here.  wglChoosePixelFormatARB procc address needed later on
                // when a valid GL context does not exist and glew is not initialized yet.
                PFNWGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribiv =
                    (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)wglGetProcAddress(
                        "wglGetPixelFormatAttribivARB" );

                mWglChoosePixelFormat =
                    (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress( "wglChoosePixelFormatARB" );

                if( mWglChoosePixelFormat( hdc, iattr, 0, 256, formats, &count ) )
                {
                    // determine what multisampling levels are offered
                    int query = WGL_SAMPLES_ARB, samples;
                    for( unsigned int i = 0; i < count; ++i )
                    {
                        if( wglGetPixelFormatAttribiv( hdc, formats[i], 0, 1, &query, &samples ) )
                        {
                            mFSAALevels.push_back( samples );
                        }
                    }
                    remove_duplicates( mFSAALevels );
                }
            }

            wglMakeCurrent( oldhdc, oldrc );
            wglDeleteContext( hrc );
        }

        // clean up our dummy window and class
        DestroyWindow( hwnd );
        UnregisterClass( dummyText, hinst );

        if( !wglCreateContextAttribsARB )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "WGL_ARB_create_context extension required for OpenGL 3.3."
                         " Update your graphics card driver, or your card doesn't "
                         "support OpenGL 3.3",
                         "Win32GLSupport::initialiseWGL" );
        }
    }

    LRESULT Win32GLSupport::dummyWndProc( HWND hwnd, UINT umsg, WPARAM wp, LPARAM lp )
    {
        return DefWindowProc( hwnd, umsg, wp, lp );
    }

    bool Win32GLSupport::selectPixelFormat( HDC hdc, int colourDepth, int multisample,
                                            PixelFormatGpu depthStencilFormat, bool hwGamma )
    {
        PIXELFORMATDESCRIPTOR pfd;
        memset( &pfd, 0, sizeof( pfd ) );
        pfd.nSize = sizeof( pfd );
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = ( colourDepth > 16 ) ? 24 : colourDepth;
        pfd.cAlphaBits = ( colourDepth > 16 ) ? 8 : 0;
        pfd.cDepthBits = 24;
        pfd.cStencilBits = 8;

        if( depthStencilFormat == PFG_D24_UNORM_S8_UINT || depthStencilFormat == PFG_D24_UNORM ||
            depthStencilFormat == PFG_UNKNOWN )
        {
            pfd.cDepthBits = 24;
        }
        else if( depthStencilFormat == PFG_D32_FLOAT || depthStencilFormat == PFG_D32_FLOAT_S8X24_UINT )
        {
            pfd.cDepthBits = 32;
        }
        else if( depthStencilFormat == PFG_NULL )
        {
            pfd.cDepthBits = 0;
        }

        if( PixelFormatGpuUtils::isStencil( depthStencilFormat ) || depthStencilFormat == PFG_UNKNOWN )
        {
            pfd.cStencilBits = 8;
        }
        else
        {
            pfd.cStencilBits = 0;
        }

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        if( SMT_FRAME_SEQUENTIAL == mStereoMode )
            pfd.dwFlags |= PFD_STEREO;
#endif

        int format = 0;

        int useHwGamma = hwGamma ? GL_TRUE : GL_FALSE;

        if( multisample && ( !mHasMultisample || !mHasPixelFormatARB ) )
            return false;

        if( hwGamma && !mHasHardwareGamma )
            return false;

        if( ( multisample || hwGamma ) && mWglChoosePixelFormat )
        {
            // Use WGL to test extended caps (multisample, sRGB)
            vector<int>::type attribList;
            attribList.push_back( WGL_DRAW_TO_WINDOW_ARB );
            attribList.push_back( GL_TRUE );
            attribList.push_back( WGL_SUPPORT_OPENGL_ARB );
            attribList.push_back( GL_TRUE );
            attribList.push_back( WGL_DOUBLE_BUFFER_ARB );
            attribList.push_back( GL_TRUE );
            attribList.push_back( WGL_SAMPLE_BUFFERS_ARB );
            attribList.push_back( GL_TRUE );
            attribList.push_back( WGL_ACCELERATION_ARB );
            attribList.push_back( WGL_FULL_ACCELERATION_ARB );
            attribList.push_back( WGL_COLOR_BITS_ARB );
            attribList.push_back( pfd.cColorBits );
            attribList.push_back( WGL_ALPHA_BITS_ARB );
            attribList.push_back( pfd.cAlphaBits );
            attribList.push_back( WGL_DEPTH_BITS_ARB );
            attribList.push_back( pfd.cDepthBits );
            attribList.push_back( WGL_STENCIL_BITS_ARB );
            attribList.push_back( pfd.cStencilBits );
            attribList.push_back( WGL_SAMPLES_ARB );
            attribList.push_back( multisample );

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
            if( SMT_FRAME_SEQUENTIAL == mStereoMode )
                attribList.push_back( WGL_STEREO_ARB );
            attribList.push_back( GL_TRUE );
#endif

            if( useHwGamma && mHasHardwareGamma )
            {
                attribList.push_back( WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT );
                attribList.push_back( GL_TRUE );
            }
            // terminator
            attribList.push_back( 0 );

            UINT nformats;
            // ChoosePixelFormatARB proc address was obtained when setting up a dummy GL context in
            // initialiseWGL() since glew hasn't been initialized yet, we have to cheat and use the
            // previously obtained address
            if( !mWglChoosePixelFormat( hdc, &( attribList[0] ), NULL, 1, &format, &nformats ) ||
                nformats <= 0 )
                return false;
        }
        else
        {
            format = ChoosePixelFormat( hdc, &pfd );
        }

        return ( format && SetPixelFormat( hdc, format, &pfd ) );
    }

    unsigned int Win32GLSupport::getDisplayMonitorCount() const
    {
        if( mMonitorInfoList.empty() )
            EnumDisplayMonitors( NULL, NULL, sCreateMonitorsInfoEnumProc, (LPARAM)&mMonitorInfoList );

        return (unsigned int)mMonitorInfoList.size();
    }

    String translateWGLError()
    {
        int winError = GetLastError();
        char *errDesc;
        int i;

        errDesc = new char[255];
        // Try windows errors first
        i = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, winError,
                           MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
                           (LPTSTR)errDesc, 255, NULL );

        return String( errDesc );
    }

}  // namespace Ogre
