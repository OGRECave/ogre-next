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

#ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0502
#endif
#include "OgreWin32Window.h"
#include "OgreDepthBuffer.h"
#include "OgreException.h"
#include "OgreGL3PlusTextureGpuManager.h"
#include "OgreLogManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreProfiler.h"
#include "OgreRenderSystem.h"
#include "OgreRoot.h"
#include "OgreStringConverter.h"
#include "OgreTextureGpuListener.h"
#include "OgreViewport.h"
#include "OgreWin32Context.h"
#include "OgreWin32GLSupport.h"
#include "OgreWindowEventUtilities.h"

#include <sstream>

#define TODO_notify_listeners

namespace Ogre
{
#define _MAX_CLASS_NAME_ 128
    bool Win32Window::mClassRegistered = false;

    Win32Window::Win32Window( const String &title, uint32 width, uint32 height, bool fullscreenMode,
                              PixelFormatGpu depthStencilFormat, const NameValuePairList *miscParams,
                              Win32GLSupport &glsupport ) :
        Window( title, width, height, fullscreenMode ),
        mGLSupport( glsupport ),
        mHwnd( 0 ),
        mHDC( 0 ),
        mGlrc( 0 ),
        mColourDepth( 32 ),
        mIsExternal( false ),
        mDeviceName( 0 ),
        mIsExternalGLControl( false ),
        mOwnsGLContext( true ),
        mSizing( false ),
        mClosed( false ),
        mHidden( false ),
        mVisible( true ),
        mHwGamma( false ),
        mContext( 0 ),
        mWindowedWinStyle( 0 ),
        mFullscreenWinStyle( 0 )
    {
        create( depthStencilFormat, miscParams );
    }
    //-----------------------------------------------------------------------------------
    Win32Window::~Win32Window() { destroy(); }
    //-----------------------------------------------------------------------------------
    void Win32Window::updateWindowRect()
    {
        RECT rc;
        BOOL result;
        result = GetWindowRect( mHwnd, &rc );
        if( result == FALSE )
        {
            mTop = 0;
            mLeft = 0;
            setFinalResolution( 0, 0 );
            return;
        }

        mTop = rc.top;
        mLeft = rc.left;
        result = GetClientRect( mHwnd, &rc );
        if( result == FALSE )
        {
            mTop = 0;
            mLeft = 0;
            setFinalResolution( 0, 0 );
            return;
        }
        uint32 width = static_cast<uint32>( rc.right - rc.left );
        uint32 height = static_cast<uint32>( rc.bottom - rc.top );
        if( width != getWidth() || height != getHeight() )
        {
            mRequestedWidth = static_cast<uint32>( rc.right - rc.left );
            mRequestedHeight = static_cast<uint32>( rc.bottom - rc.top );
            setFinalResolution( mRequestedWidth, mRequestedHeight );
            notifyResolutionChanged();
        }
    }
    //-----------------------------------------------------------------------------------
    void Win32Window::adjustWindow( uint32 clientWidth, uint32 clientHeight, uint32 *outDrawableWidth,
                                    uint32 *outDrawableHeight )
    {
        RECT rc;
        SetRect( &rc, 0, 0, clientWidth, clientHeight );
        AdjustWindowRect( &rc, getWindowStyle( mRequestedFullscreenMode ), false );
        *outDrawableWidth = rc.right - rc.left;
        *outDrawableHeight = rc.bottom - rc.top;
    }
    //-----------------------------------------------------------------------------------
    DWORD Win32Window::getWindowStyle( bool fullScreen ) const
    {
        return fullScreen ? mFullscreenWinStyle : mWindowedWinStyle;
    }
    //-----------------------------------------------------------------------------------
    void Win32Window::notifyResolutionChanged() { TODO_notify_listeners; }
    //-----------------------------------------------------------------------------------
    void Win32Window::create( PixelFormatGpu depthStencilFormat, const NameValuePairList *miscParams )
    {
        // destroy current window, if any
        if( mHwnd )
            destroy();

        mClosed = false;
        mColourDepth = mRequestedFullscreenMode ? 32 : GetDeviceCaps( GetDC( 0 ), BITSPIXEL );
        int left = -1;  // Defaults to screen center
        int top = -1;   // Defaults to screen center
        HWND parentHwnd = 0;
        String border;
        bool outerSize = false;
        mHwGamma = false;
        bool enableDoubleClick = false;
        int monitorIndex = -1;
        HMONITOR hMonitor = NULL;

        if( miscParams )
        {
            // Get variable-length params
            NameValuePairList::const_iterator opt;
            NameValuePairList::const_iterator end = miscParams->end();

            opt = miscParams->find( "title" );
            if( opt != end )
                mTitle = opt->second;
            opt = miscParams->find( "left" );
            if( opt != end )
                left = StringConverter::parseInt( opt->second );
            opt = miscParams->find( "top" );
            if( opt != end )
                top = StringConverter::parseInt( opt->second );
            opt = miscParams->find( "vsync" );
            if( opt != end )
                mVSync = StringConverter::parseBool( opt->second );
            opt = miscParams->find( "hidden" );
            if( opt != end )
                mHidden = StringConverter::parseBool( opt->second );
            opt = miscParams->find( "vsyncInterval" );
            if( opt != end )
                mVSyncInterval = StringConverter::parseUnsignedInt( opt->second );
            opt = miscParams->find( "FSAA" );
            if( opt != end )
                mRequestedSampleDescription.parseString( opt->second );
            opt = miscParams->find( "gamma" );
            if( opt != end )
                mHwGamma = StringConverter::parseBool( opt->second );

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
            opt = miscParams->find( "stereoMode" );
            if( opt != end )
            {
                StereoModeType stereoMode = StringConverter::parseStereoMode( opt->second );
                if( SMT_NONE != stereoMode )
                    mStereoEnabled = true;
            }
#endif
            opt = miscParams->find( "externalWindowHandle" );
            if( opt != end )
            {
                mHwnd = (HWND)StringConverter::parseSizeT( opt->second );

                if( IsWindow( mHwnd ) && mHwnd )
                {
                    mIsExternal = true;
                    mRequestedFullscreenMode = false;
                }

                opt = miscParams->find( "externalGLControl" );
                if( opt != end )
                    mIsExternalGLControl = StringConverter::parseBool( opt->second );
            }
            opt = miscParams->find( "currentGLContext" );
            if( opt != end )
            {
                if( StringConverter::parseBool( opt->second ) )
                {
                    mGlrc = wglGetCurrentContext();

                    if( mGlrc )
                    {
                        mOwnsGLContext = false;
                    }
                    else
                    {
                        OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                                     "currentGLContext was specified with no current GL context",
                                     "Win32Window::create" );
                    }
                }
            }

            opt = miscParams->find( "externalGLContext" );
            if( opt != end )
            {
                mGlrc = (HGLRC)StringConverter::parseSizeT( opt->second );

                if( mGlrc )
                {
                    mOwnsGLContext = false;
                }
                else
                {
                    OGRE_EXCEPT(
                        Exception::ERR_RENDERINGAPI_ERROR,
                        "parsing the value of 'externalGLContext' failed: " + translateWGLError(),
                        "Win32Window::create" );
                }
            }

            // window border style
            opt = miscParams->find( "border" );
            if( opt != miscParams->end() )
                border = opt->second;
            // set outer dimensions?
            opt = miscParams->find( "outerDimensions" );
            if( opt != miscParams->end() )
                outerSize = StringConverter::parseBool( opt->second );

            // only available with fullscreen
            opt = miscParams->find( "displayFrequency" );
            if( opt != end )
                mFrequencyNumerator = StringConverter::parseUnsignedInt( opt->second );

            opt = miscParams->find( "colourDepth" );
            if( opt != end )
            {
                mColourDepth = StringConverter::parseUnsignedInt( opt->second );
                if( !mRequestedFullscreenMode )
                {
                    // make sure we don't exceed desktop colour depth
                    if( (int)mColourDepth > GetDeviceCaps( GetDC( 0 ), BITSPIXEL ) )
                        mColourDepth = GetDeviceCaps( GetDC( 0 ), BITSPIXEL );
                }
            }

            // incompatible with fullscreen
            opt = miscParams->find( "parentWindowHandle" );
            if( opt != end )
                parentHwnd = (HWND)StringConverter::parseSizeT( opt->second );

            // monitor index
            opt = miscParams->find( "monitorIndex" );
            if( opt != end )
                monitorIndex = StringConverter::parseInt( opt->second );

            // monitor handle
            opt = miscParams->find( "monitorHandle" );
            if( opt != end )
                hMonitor = (HMONITOR)StringConverter::parseInt( opt->second );

            // enable double click messages
            opt = miscParams->find( "enableDoubleClick" );
            if( opt != end )
                enableDoubleClick = StringConverter::parseBool( opt->second );
        }

        if( !mIsExternal )
        {
            DWORD dwStyleEx = 0;
            MONITORINFOEX monitorInfoEx;

            // If we didn't specified the adapter index, or if it didn't find it
            if( hMonitor == NULL )
            {
                POINT windowAnchorPoint;

                // Fill in anchor point.
                windowAnchorPoint.x = left;
                windowAnchorPoint.y = top;

                // Get the nearest monitor to this window.
                hMonitor = MonitorFromPoint( windowAnchorPoint, MONITOR_DEFAULTTOPRIMARY );
            }

            // Get the target monitor info
            memset( &monitorInfoEx, 0, sizeof( MONITORINFOEX ) );
            monitorInfoEx.cbSize = sizeof( MONITORINFOEX );
            GetMonitorInfo( hMonitor, &monitorInfoEx );

            size_t devNameLen = strlen( monitorInfoEx.szDevice );
            mDeviceName = new char[devNameLen + 1];

            strcpy( mDeviceName, monitorInfoEx.szDevice );

            // Update window style flags.
            mFullscreenWinStyle = ( mHidden ? 0 : WS_VISIBLE ) | WS_CLIPCHILDREN | WS_POPUP;
            mWindowedWinStyle = ( mHidden ? 0 : WS_VISIBLE ) | WS_CLIPCHILDREN;

            if( parentHwnd )
            {
                mWindowedWinStyle |= WS_CHILD;
            }
            else
            {
                if( border == "none" )
                    mWindowedWinStyle |= WS_POPUP;
                else if( border == "fixed" )
                    mWindowedWinStyle |=
                        WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
                else
                    mWindowedWinStyle |= WS_OVERLAPPEDWINDOW;
            }

            uint32 winWidth, winHeight;
            winWidth = mRequestedWidth;
            winHeight = mRequestedHeight;
            {
                // Center window horizontally and/or vertically, on the right monitor.
                uint32 screenw = monitorInfoEx.rcWork.right - monitorInfoEx.rcWork.left;
                uint32 screenh = monitorInfoEx.rcWork.bottom - monitorInfoEx.rcWork.top;
                uint32 outerw = ( winWidth < screenw ) ? winWidth : screenw;
                uint32 outerh = ( winHeight < screenh ) ? winHeight : screenh;
                if( left == INT_MAX )
                    left = monitorInfoEx.rcWork.left + ( screenw - outerw ) / 2;
                else if( monitorIndex != -1 )
                    left += monitorInfoEx.rcWork.left;
                if( top == INT_MAX )
                    top = monitorInfoEx.rcWork.top + ( screenh - outerh ) / 2;
                else if( monitorIndex != -1 )
                    top += monitorInfoEx.rcWork.top;
            }

            mTop = top;
            mLeft = left;

            if( mRequestedFullscreenMode )
            {
                dwStyleEx |= WS_EX_TOPMOST;
                mTop = monitorInfoEx.rcMonitor.top;
                mLeft = monitorInfoEx.rcMonitor.left;
            }
            else
            {
                RECT rc;
                SetRect( &rc, mLeft, mTop, mRequestedWidth, mRequestedHeight );
                if( !outerSize )
                {
                    // User requested "client resolution", we need to grow the rect
                    // for the window's resolution (which is always bigger).
                    AdjustWindowRect( &rc, getWindowStyle( mRequestedFullscreenMode ), false );
                }

                // Clamp to current monitor's size
                if( rc.left < monitorInfoEx.rcWork.left )
                {
                    rc.right += monitorInfoEx.rcWork.left - rc.left;
                    rc.left = monitorInfoEx.rcWork.left;
                }
                if( rc.top < monitorInfoEx.rcWork.top )
                {
                    rc.bottom += monitorInfoEx.rcWork.top - rc.top;
                    rc.top = monitorInfoEx.rcWork.top;
                }
                if( rc.right > monitorInfoEx.rcWork.right )
                    rc.right = monitorInfoEx.rcWork.right;
                if( rc.bottom > monitorInfoEx.rcWork.bottom )
                    rc.bottom = monitorInfoEx.rcWork.bottom;

                mLeft = rc.left;
                mTop = rc.top;
                mRequestedWidth = rc.right - rc.left;
                mRequestedHeight = rc.bottom - rc.top;
            }

            // Grab the HINSTANCE by asking the OS what's the hinstance at an address in this process
            HINSTANCE hInstance = NULL;
#ifdef __MINGW32__
#    ifdef OGRE_STATIC_LIB
            hInstance = GetModuleHandle( NULL );
#    else
#        if OGRE_DEBUG_MODE
            hInstance = GetModuleHandle( "RenderSystem_GL3Plus_d.dll" );
#        else
            hInstance = GetModuleHandle( "RenderSystem_GL3Plus.dll" );
#        endif
#    endif
#else
            static const TCHAR staticVar;
            GetModuleHandleEx(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                &staticVar, &hInstance );
#endif

            // register class and create window
            WNDCLASSEX wcex;
            wcex.cbSize = sizeof( WNDCLASSEX );
            wcex.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
            wcex.lpfnWndProc = WindowEventUtilities::_WndProc;
            wcex.cbClsExtra = 0;
            wcex.cbWndExtra = 0;
            wcex.hInstance = hInstance;
            wcex.hIcon = LoadIcon( (HINSTANCE)0, (LPCTSTR)IDI_APPLICATION );
            wcex.hCursor = LoadCursor( (HINSTANCE)0, IDC_ARROW );
            wcex.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
            wcex.lpszMenuName = 0;
            wcex.lpszClassName = "OgreGLWindow";
            wcex.hIconSm = 0;

            if( enableDoubleClick )
                wcex.style |= CS_DBLCLKS;

            if( !mClassRegistered )
            {
                if( !RegisterClassEx( &wcex ) )
                {
                    OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                                 "RegisterClassEx failed! Cannot create window", "Win32Window::create" );
                }
                mClassRegistered = true;
            }

            if( mRequestedFullscreenMode )
            {
                DEVMODE displayDeviceMode;

                memset( &displayDeviceMode, 0, sizeof( displayDeviceMode ) );
                displayDeviceMode.dmSize = sizeof( DEVMODE );
                displayDeviceMode.dmBitsPerPel = mColourDepth;
                displayDeviceMode.dmPelsWidth = mRequestedWidth;
                displayDeviceMode.dmPelsHeight = mRequestedHeight;
                displayDeviceMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

                if( mFrequencyNumerator )
                {
                    displayDeviceMode.dmDisplayFrequency = mFrequencyNumerator;
                    displayDeviceMode.dmFields |= DM_DISPLAYFREQUENCY;
                    LONG displayChangeResult = ChangeDisplaySettingsEx(
                        mDeviceName, &displayDeviceMode, NULL, CDS_FULLSCREEN | CDS_TEST, NULL );
                    if( displayChangeResult != DISP_CHANGE_SUCCESSFUL )
                    {
                        LogManager::getSingleton().logMessage(
                            "ChangeDisplaySettings with user display frequency failed" );
                        displayDeviceMode.dmFields ^= DM_DISPLAYFREQUENCY;
                    }
                }

                LONG displayChangeResult = ChangeDisplaySettingsEx( mDeviceName, &displayDeviceMode,
                                                                    NULL, CDS_FULLSCREEN, NULL );
                if( displayChangeResult != DISP_CHANGE_SUCCESSFUL )
                {
                    LogManager::getSingleton().logMessage( LML_CRITICAL,
                                                           "ChangeDisplaySettings failed" );
                    mRequestedFullscreenMode = false;
                }
            }

            // Pass pointer to self as WM_CREATE parameter
            mHwnd = CreateWindowEx( dwStyleEx, "OgreGLWindow", mTitle.c_str(),
                                    getWindowStyle( mRequestedFullscreenMode ), mLeft, mTop,
                                    mRequestedWidth, mRequestedHeight, parentHwnd, 0, hInstance, this );

            WindowEventUtilities::_addRenderWindow( this );

            LogManager::getSingleton().stream()
                << "Created Win32Window '" << mTitle << "' : " << mRequestedWidth << "x"
                << mRequestedHeight << ", " << mColourDepth << "bpp";
        }

        HDC oldHdc = wglGetCurrentDC();
        HGLRC oldContext = wglGetCurrentContext();

        RECT rc;
        // top and left represent outer window coordinates
        GetWindowRect( mHwnd, &rc );
        mTop = rc.top;
        mLeft = rc.left;
        // width and height represent interior drawable area
        GetClientRect( mHwnd, &rc );
        setFinalResolution( rc.right - rc.left, rc.bottom - rc.top );

        mHDC = GetDC( mHwnd );

        if( !mIsExternalGLControl )
        {
            // TODO: move into GL3PlusRenderSystem::validateSampleDescription
            unsigned msaaCount = mRequestedSampleDescription.getColourSamples();
            int testFsaa = msaaCount > 1u ? msaaCount : 0;
            bool testHwGamma = mHwGamma;
            bool formatOk = mGLSupport.selectPixelFormat( mHDC, mColourDepth, testFsaa,
                                                          depthStencilFormat, testHwGamma );
            if( !formatOk )
            {
                if( msaaCount > 1u )
                {
                    // try without FSAA
                    testFsaa = 0;
                    formatOk = mGLSupport.selectPixelFormat( mHDC, mColourDepth, testFsaa,
                                                             depthStencilFormat, testHwGamma );
                }

                if( !formatOk && mHwGamma )
                {
                    // try without sRGB
                    testHwGamma = false;
                    testFsaa = msaaCount > 1u ? msaaCount : 0;
                    formatOk = mGLSupport.selectPixelFormat( mHDC, mColourDepth, testFsaa,
                                                             depthStencilFormat, testHwGamma );
                }

                if( !formatOk && mHwGamma && msaaCount > 1u )
                {
                    // try without both
                    testHwGamma = false;
                    testFsaa = 0;
                    formatOk = mGLSupport.selectPixelFormat( mHDC, mColourDepth, testFsaa,
                                                             depthStencilFormat, testHwGamma );
                }

                if( !formatOk )
                {
                    OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "selectPixelFormat failed",
                                 "Win32Window::create" );
                }
            }

            // record what gamma option we used in the end
            // this will control enabling of sRGB state flags when used
            mHwGamma = testHwGamma;
            mSampleDescription = SampleDescription( testFsaa ? testFsaa : 1u );
        }

        GLint contextMajor = 4;
        GLint contextMinor = 5;

        if( mOwnsGLContext )
        {
            int attribList[] = {
                WGL_CONTEXT_MAJOR_VERSION_ARB,
                contextMajor,
                WGL_CONTEXT_MINOR_VERSION_ARB,
                contextMinor,
#if OGRE_DEBUG_MODE
                WGL_CONTEXT_FLAGS_ARB,
                WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
                WGL_CONTEXT_PROFILE_MASK_ARB,
                WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                0,
                0
            };

            while( !mGlrc && ( attribList[1] > 3 || ( attribList[1] == 3 && attribList[3] >= 3 ) ) )
            {
                // New context is shared with previous one
                mGlrc = wglCreateContextAttribsARB( mHDC, oldContext, attribList );

                if( !mGlrc )
                {
                    if( attribList[3] == 0 )
                    {
                        contextMajor -= 1;
                        contextMinor = 5;
                        attribList[1] = contextMajor;
                        attribList[3] = contextMinor;
                    }
                    else
                    {
                        contextMinor -= 1;
                        attribList[3] = contextMinor;
                    }
                }
            }

            if( !mGlrc )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "wglCreateContextAttribsARB failed: " + translateWGLError(),
                             "Win32Window::create" );
            }

            LogManager::getSingleton().logMessage(
                "Created GL " + StringConverter::toString( contextMajor ) + "." +
                StringConverter::toString( contextMinor ) + " context" );
        }

        if( !wglMakeCurrent( mHDC, mGlrc ) )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "wglMakeCurrent failed: " + translateWGLError(), "Win32Window::create" );
        }

        // Do not change vsync if the external window has the OpenGL control
        if( !mIsExternalGLControl )
        {
            // Don't use wglew as if this is the first window, we won't have initialised yet
            PFNWGLSWAPINTERVALEXTPROC _wglSwapIntervalEXT =
                (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress( "wglSwapIntervalEXT" );
            if( _wglSwapIntervalEXT )
                _wglSwapIntervalEXT( mVSync ? mVSyncInterval : 0 );
        }

        if( oldContext && oldContext != mGlrc )
        {
            // Restore old context
            if( !wglMakeCurrent( oldHdc, oldContext ) )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "wglMakeCurrent() failed",
                             "Win32Window::create" );
            }
        }

        // Create RenderSystem context
        mContext = new Win32Context( mHDC, mGlrc, static_cast<uint32>( contextMajor ),
                                     static_cast<uint32>( contextMinor ) );

        mFullscreenMode = mRequestedFullscreenMode;
    }
    //-----------------------------------------------------------------------------------
    void Win32Window::_initialize( TextureGpuManager *textureGpuManager )
    {
        GL3PlusTextureGpuManager *textureManager =
            static_cast<GL3PlusTextureGpuManager *>( textureGpuManager );

        mTexture = textureManager->createTextureGpuWindow( mContext, this );
        mDepthBuffer = textureManager->createTextureGpuWindow( mContext, this );

        if( mColourDepth == 16u )
            mTexture->setPixelFormat( PFG_B5G5R5A1_UNORM );
        else
            mTexture->setPixelFormat( mHwGamma ? PFG_RGBA8_UNORM_SRGB : PFG_RGBA8_UNORM );
        mDepthBuffer->setPixelFormat( DepthBuffer::DefaultDepthBufferFormat );
        if( PixelFormatGpuUtils::isStencil( mDepthBuffer->getPixelFormat() ) )
            mStencilBuffer = mDepthBuffer;

        mTexture->setSampleDescription( mSampleDescription );
        mDepthBuffer->setSampleDescription( mSampleDescription );

        RECT rc;
        GetClientRect( mHwnd, &rc );  // width and height represent interior drawable area
        setFinalResolution( rc.right - rc.left, rc.bottom - rc.top );

        if( mDepthBuffer )
        {
            mTexture->_setDepthBufferDefaults( DepthBuffer::NO_POOL_EXPLICIT_RTV, false,
                                               mDepthBuffer->getPixelFormat() );
        }
        else
        {
            mTexture->_setDepthBufferDefaults( DepthBuffer::POOL_NO_DEPTH, false, PFG_NULL );
        }

        mTexture->_transitionTo( GpuResidency::Resident, (uint8 *)0 );
        if( mDepthBuffer )
            mDepthBuffer->_transitionTo( GpuResidency::Resident, (uint8 *)0 );
    }
    //-----------------------------------------------------------------------------------
    void Win32Window::destroy()
    {
        if( mTexture )
        {
            mTexture->notifyAllListenersTextureChanged( TextureGpuListener::Deleted );
            OGRE_DELETE mTexture;
            mTexture = 0;
        }
        if( mDepthBuffer )
        {
            mDepthBuffer->notifyAllListenersTextureChanged( TextureGpuListener::Deleted );
            OGRE_DELETE mDepthBuffer;
            mDepthBuffer = 0;
        }

        // Depth & Stencil buffers are the same pointer
        // OGRE_DELETE mStencilBuffer;
        mStencilBuffer = 0;

        if( !mHwnd )
            return;

        // Unregister and destroy OGRE GL3PlusContext
        delete mContext;
        mContext = 0;

        if( mOwnsGLContext )
        {
            wglDeleteContext( mGlrc );
            mGlrc = 0;
        }
        if( !mIsExternal )
        {
            WindowEventUtilities::_removeRenderWindow( this );

            if( mFullscreenMode )
                ChangeDisplaySettingsEx( mDeviceName, NULL, NULL, 0, NULL );
            DestroyWindow( mHwnd );
        }
        else
        {
            // just release the DC
            ReleaseDC( mHwnd, mHDC );
        }

        mFocused = false;
        mClosed = true;
        mHDC = 0;  // no release thanks to CS_OWNDC wndclass style
        mHwnd = 0;

        if( mClassRegistered )
        {
            UnregisterClassA( "OgreGLWindow", nullptr );
        }

        if( mDeviceName != NULL )
        {
            delete[] mDeviceName;
            mDeviceName = NULL;
        }
    }
    //-----------------------------------------------------------------------------------
    void Win32Window::reposition( int32 left, int32 top )
    {
        if( mHwnd && !mRequestedFullscreenMode )
        {
            SetWindowPos( mHwnd, 0, left, top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
        }
    }
    //-----------------------------------------------------------------------------------
    void Win32Window::requestResolution( uint32 width, uint32 height )
    {
        if( !mIsExternal )
        {
            if( mHwnd && !mRequestedFullscreenMode )
            {
                uint32 winWidth, winHeight;
                adjustWindow( width, height, &winWidth, &winHeight );
                SetWindowPos( mHwnd, 0, 0, 0, winWidth, winHeight,
                              SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
            }
        }
        else
            updateWindowRect();
    }
    //-----------------------------------------------------------------------------------
    void Win32Window::requestFullscreenSwitch( bool goFullscreen, bool borderless, uint32 monitorIdx,
                                               uint32 width, uint32 height, uint32 frequencyNumerator,
                                               uint32 frequencyDenominator )
    {
        if( goFullscreen != mRequestedFullscreenMode || width != mRequestedWidth ||
            height != mRequestedHeight )
        {
            mRequestedFullscreenMode = goFullscreen;
            mFrequencyNumerator = frequencyNumerator;

            if( mRequestedFullscreenMode )
            {
                DEVMODE displayDeviceMode;

                memset( &displayDeviceMode, 0, sizeof( displayDeviceMode ) );
                displayDeviceMode.dmSize = sizeof( DEVMODE );
                displayDeviceMode.dmBitsPerPel = mColourDepth;
                displayDeviceMode.dmPelsWidth = width;
                displayDeviceMode.dmPelsHeight = height;
                displayDeviceMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
                if( mFrequencyNumerator )
                {
                    displayDeviceMode.dmDisplayFrequency = mFrequencyNumerator;
                    displayDeviceMode.dmFields |= DM_DISPLAYFREQUENCY;

                    LONG displayChangeResult = ChangeDisplaySettingsEx(
                        mDeviceName, &displayDeviceMode, NULL, CDS_FULLSCREEN | CDS_TEST, NULL );
                    if( displayChangeResult != DISP_CHANGE_SUCCESSFUL )
                    {
                        LogManager::getSingleton().logMessage(
                            "ChangeDisplaySettings with user display frequency failed" );
                        displayDeviceMode.dmFields ^= DM_DISPLAYFREQUENCY;
                    }
                }
                else
                {
                    // try a few
                    displayDeviceMode.dmDisplayFrequency = 100;
                    displayDeviceMode.dmFields |= DM_DISPLAYFREQUENCY;
                    LONG displayChangeResult = ChangeDisplaySettingsEx(
                        mDeviceName, &displayDeviceMode, NULL, CDS_FULLSCREEN | CDS_TEST, NULL );
                    if( displayChangeResult != DISP_CHANGE_SUCCESSFUL )
                    {
                        displayDeviceMode.dmDisplayFrequency = 75;
                        displayChangeResult = ChangeDisplaySettingsEx(
                            mDeviceName, &displayDeviceMode, NULL, CDS_FULLSCREEN | CDS_TEST, NULL );
                        if( displayChangeResult != DISP_CHANGE_SUCCESSFUL )
                            displayDeviceMode.dmFields ^= DM_DISPLAYFREQUENCY;
                    }
                }

                // move window to 0,0 before display switch
                SetWindowPos( mHwnd, HWND_TOPMOST, 0, 0, mRequestedWidth, mRequestedHeight,
                              SWP_NOACTIVATE );

                LONG displayChangeResult = ChangeDisplaySettingsEx( mDeviceName, &displayDeviceMode,
                                                                    NULL, CDS_FULLSCREEN, NULL );

                if( displayChangeResult != DISP_CHANGE_SUCCESSFUL )
                {
                    LogManager::getSingleton().logMessage( LML_CRITICAL,
                                                           "ChangeDisplaySettings failed" );
                    mRequestedFullscreenMode = false;
                }

                // Get the nearest monitor to this window.
                HMONITOR hMonitor = MonitorFromWindow( mHwnd, MONITOR_DEFAULTTONEAREST );

                // Get monitor info
                MONITORINFO monitorInfo;

                memset( &monitorInfo, 0, sizeof( MONITORINFO ) );
                monitorInfo.cbSize = sizeof( MONITORINFO );
                GetMonitorInfo( hMonitor, &monitorInfo );

                mTop = monitorInfo.rcMonitor.top;
                mLeft = monitorInfo.rcMonitor.left;

                SetWindowLongPtr( mHwnd, GWL_STYLE, getWindowStyle( mRequestedFullscreenMode ) );
                SetWindowPos( mHwnd, HWND_TOPMOST, mLeft, mTop, width, height, SWP_NOACTIVATE );
                setFinalResolution( width, height );
            }
            else
            {
                // drop out of fullscreen
                ChangeDisplaySettingsEx( mDeviceName, NULL, NULL, 0, NULL );

                // calculate overall dimensions for requested client area
                uint32 winWidth, winHeight;
                adjustWindow( width, height, &winWidth, &winHeight );

                // deal with centering when switching down to smaller resolution
                HMONITOR hMonitor = MonitorFromWindow( mHwnd, MONITOR_DEFAULTTONEAREST );
                MONITORINFO monitorInfo;
                memset( &monitorInfo, 0, sizeof( MONITORINFO ) );
                monitorInfo.cbSize = sizeof( MONITORINFO );
                GetMonitorInfo( hMonitor, &monitorInfo );

                LONG screenw = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
                LONG screenh = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;

                int left = ( screenw > (int)winWidth ) ? ( ( screenw - (int)winWidth ) / 2 ) : 0;
                int top = ( screenh > (int)winHeight ) ? ( ( screenh - (int)winHeight ) / 2 ) : 0;

                SetWindowLongPtr( mHwnd, GWL_STYLE, getWindowStyle( mRequestedFullscreenMode ) );
                SetWindowPos( mHwnd, HWND_NOTOPMOST, left, top, winWidth, winHeight,
                              SWP_DRAWFRAME | SWP_FRAMECHANGED | SWP_NOACTIVATE );
                mLeft = left;
                mTop = top;
                setFinalResolution( width, height );

                windowMovedOrResized();
            }

            mFullscreenMode = mRequestedFullscreenMode;
        }
    }
    //-----------------------------------------------------------------------------------
    void Win32Window::windowMovedOrResized()
    {
        if( !mHwnd || IsIconic( mHwnd ) )
            return;

        updateWindowRect();
    }
    //-----------------------------------------------------------------------------------
    bool Win32Window::isClosed() const { return mClosed; }
    //-----------------------------------------------------------------------------------
    void Win32Window::_setVisible( bool visible ) { mVisible = visible; }
    //-----------------------------------------------------------------------------------
    bool Win32Window::isVisible() const
    {
        bool visible = mVisible && !mHidden;

        // Window minimized or fully obscured (we got notified via _setVisible/WM_PAINT messages)
        if( !visible )
            return visible;

        {
            HWND currentWindowHandle = mHwnd;
            while( ( visible = ( IsIconic( currentWindowHandle ) == false ) ) &&
                   ( GetWindowLongPtr( currentWindowHandle, GWL_STYLE ) & WS_CHILD ) != 0 )
            {
                currentWindowHandle = GetParent( currentWindowHandle );
            }

            // Window is minimized
            if( !visible )
                return visible;
        }

        /*
         *  Poll code to see if we're fully obscured.
         * Not needed since we do it via WM_PAINT messages.
         *
        HDC hdc = GetDC( mHwnd );
        if( hdc )
        {
            RECT rcClip, rcClient;
            switch( GetClipBox( hdc, &rcClip ) )
            {
            case NULLREGION:
                // Completely covered
                visible = false;
                break;
            case SIMPLEREGION:
                GetClientRect(hwnd, &rcClient);
                if( EqualRect( &rcClient, &rcClip ) )
                {
                    // Completely uncovered
                    visible = true;
                }
                else
                {
                    // Partially covered
                    visible = true;
                }
                break;
            case COMPLEXREGION:
                // Partially covered
                visible = true;
                break;
            default:
                // Error
                visible = true;
                break;
            }

            // If we wanted, we could also use RectVisible
            // or PtVisible - or go totally overboard by
            // using GetClipRgn
            ReleaseDC( mHwnd, hdc );
        }*/

        return visible;
    }
    //-----------------------------------------------------------------------------------
    void Win32Window::setHidden( bool hidden )
    {
        mHidden = hidden;
        if( !mIsExternal )
        {
            if( hidden )
                ShowWindow( mHwnd, SW_HIDE );
            else
                ShowWindow( mHwnd, SW_SHOWNORMAL );
        }
    }
    //-----------------------------------------------------------------------------------
    bool Win32Window::isHidden() const { return mHidden; }
    //-----------------------------------------------------------------------------------
    void Win32Window::setVSync( bool vSync, uint32 vSyncInterval )
    {
        Window::setVSync( vSync, vSyncInterval );

        HDC oldHdc = wglGetCurrentDC();
        HGLRC oldContext = wglGetCurrentContext();
        if( !wglMakeCurrent( mHDC, mGlrc ) )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "wglMakeCurrent",
                         "Win32Window::setVSyncEnabled" );
        }

        // Do not change vsync if the external window has the OpenGL control
        if( !mIsExternalGLControl )
        {
            // Don't use wglew as if this is the first window, we won't have initialised yet
            PFNWGLSWAPINTERVALEXTPROC _wglSwapIntervalEXT =
                (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress( "wglSwapIntervalEXT" );
            if( _wglSwapIntervalEXT )
                _wglSwapIntervalEXT( mVSync ? mVSyncInterval : 0 );
        }

        if( oldContext && oldContext != mGlrc )
        {
            // Restore old context
            if( !wglMakeCurrent( oldHdc, oldContext ) )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "wglMakeCurrent() failed",
                             "Win32Window::setVSyncEnabled" );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void Win32Window::swapBuffers()
    {
        if( !mIsExternalGLControl )
        {
            OgreProfileBeginDynamic( ( "SwapBuffers: " + mTitle ).c_str() );
            OgreProfileGpuBeginDynamic( "SwapBuffers: " + mTitle );
            SwapBuffers( mHDC );
            OgreProfileEnd( "SwapBuffers: " + mTitle );
            OgreProfileGpuEnd( "SwapBuffers: " + mTitle );
        }
    }
    //-----------------------------------------------------------------------------------
    void Win32Window::setFocused( bool focused )
    {
        if( mDeviceName != NULL && focused == false )
        {
            HWND hActiveWindow = GetActiveWindow();
            char classNameSrc[_MAX_CLASS_NAME_ + 1];
            char classNameDst[_MAX_CLASS_NAME_ + 1];

            GetClassName( mHwnd, classNameSrc, _MAX_CLASS_NAME_ );
            GetClassName( hActiveWindow, classNameDst, _MAX_CLASS_NAME_ );

            if( strcmp( classNameDst, classNameSrc ) == 0 )
            {
                focused = true;
            }
        }

        Window::setFocused( focused );

        if( mRequestedFullscreenMode )
        {
            if( focused == false )
            {  // Restore Desktop
                ChangeDisplaySettingsEx( mDeviceName, NULL, NULL, 0, NULL );
                ShowWindow( mHwnd, SW_SHOWMINNOACTIVE );
            }
            else
            {  // Restore App
                ShowWindow( mHwnd, SW_SHOWNORMAL );

                mRequestedFullscreenMode = false;
                requestFullscreenSwitch( true, mBorderless, -1, mRequestedWidth, mRequestedHeight,
                                         mFrequencyNumerator, mFrequencyDenominator );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void Win32Window::getCustomAttribute( IdString name, void *pData )
    {
        if( name == "GLCONTEXT" )
        {
            *static_cast<GL3PlusContext **>( pData ) = mContext;
            return;
        }
        else if( name == "WINDOW" || name == "RENDERDOC_WINDOW" )
        {
            HWND *pHwnd = (HWND *)pData;
            *pHwnd = getWindowHandle();
            return;
        }
        else if( name == "RENDERDOC_DEVICE" )
        {
            *static_cast<HGLRC *>( pData ) = mContext->getHGLRC();
            return;
        }
    }
}  // namespace Ogre
