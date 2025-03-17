/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

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

#include "OgreD3D11Device.h"
#include "OgreD3D11Mappings.h"
#include "OgreD3D11RenderSystem.h"
#include "OgreD3D11TextureGpuManager.h"
#include "OgreD3D11TextureGpuWindow.h"
#include "OgreD3D11Window.h"
#include "OgreDepthBuffer.h"
#include "OgreOSVersionHelpers.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreStringConverter.h"
#include "OgreWindowEventUtilities.h"

#include "OgreException.h"

#if UNICODE
#    define OGRE_D3D11_WIN_CLASS_NAME L"OgreD3D11Wnd"
#else
#    define OGRE_D3D11_WIN_CLASS_NAME "OgreD3D11Wnd"
#endif

namespace Ogre
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32

    bool D3D11WindowHwnd::mClassRegistered = false;

    typedef vector<HMONITOR>::type DisplayMonitorList;

    D3D11WindowHwnd::D3D11WindowHwnd( const String &title, uint32 width, uint32 height,
                                      bool fullscreenMode, PixelFormatGpu depthStencilFormat,
                                      const NameValuePairList *miscParams, D3D11Device &device,
                                      D3D11RenderSystem *renderSystem ) :
        D3D11WindowSwapChainBased( title, width, height, fullscreenMode, depthStencilFormat, miscParams,
                                   device, renderSystem ),
        mHwnd( 0 ),
        mWindowedWinStyle( 0 ),
        mFullscreenWinStyle( 0 ),
        mLastSwitchingFullscreenCounter( 0 )
    {
        create( fullscreenMode, miscParams );
    }
    //-----------------------------------------------------------------------------------
    D3D11WindowHwnd::~D3D11WindowHwnd() { destroy(); }
    //-----------------------------------------------------------------------------------
    DWORD D3D11WindowHwnd::getWindowStyle( bool fullScreen ) const
    {
        return fullScreen ? mFullscreenWinStyle : mWindowedWinStyle;
    }
    //-----------------------------------------------------------------------------------
    BOOL CALLBACK D3D11WindowHwnd::createMonitorsInfoEnumProc(
        HMONITOR hMonitor,   // handle to display monitor
        HDC hdcMonitor,      // handle to monitor DC
        LPRECT lprcMonitor,  // monitor intersection rectangle
        LPARAM dwData        // data
    )
    {
        DisplayMonitorList *pArrMonitors = reinterpret_cast<DisplayMonitorList *>( dwData );
        pArrMonitors->push_back( hMonitor );
        return TRUE;
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowHwnd::updateWindowRect()
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
            resizeSwapChainBuffers( mRequestedWidth, mRequestedHeight );
            notifyResolutionChanged();
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowHwnd::adjustWindow( uint32 clientWidth, uint32 clientHeight,
                                        uint32 *outDrawableWidth, uint32 *outDrawableHeight )
    {
        RECT rc;
        SetRect( &rc, 0, 0, clientWidth, clientHeight );
        AdjustWindowRect( &rc, getWindowStyle( mRequestedFullscreenMode ), false );
        *outDrawableWidth = rc.right - rc.left;
        *outDrawableHeight = rc.bottom - rc.top;
    }
    //-----------------------------------------------------------------------------------
    template <typename T>
    void D3D11WindowHwnd::setCommonSwapChain( T &sd )
    {
        if( mUseFlipMode )
        {
            sd.SampleDesc.Count = 1u;
            sd.SampleDesc.Quality = 0;
        }
        else
        {
            sd.SampleDesc.Count = mSampleDescription.getColourSamples();
            sd.SampleDesc.Quality = mSampleDescription.getCoverageSamples()
                                        ? mSampleDescription.getCoverageSamples()
                                        : D3D11Mappings::get( mSampleDescription.getMsaaPattern() );
        }

        sd.Flags = _getSwapChainFlags();
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

        sd.BufferCount = _getSwapChainBufferCount();
        sd.SwapEffect = !mUseFlipMode            ? DXGI_SWAP_EFFECT_DISCARD
                        : IsWindows10OrGreater() ? DXGI_SWAP_EFFECT_FLIP_DISCARD
                                                 : DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    }
    //-----------------------------------------------------------------------------------
    HRESULT D3D11WindowHwnd::_createSwapChainImpl()
    {
        mSampleDescription = mRenderSystem->validateSampleDescription(
            mRequestedSampleDescription, _getRenderFormat(),
            TextureFlags::NotTexture | TextureFlags::RenderWindowSpecific );
        HRESULT hr;

        // Create swap chain
        IDXGIFactory2 *dxgiFactory2 = mDevice.GetDXGIFactory2();
        if( dxgiFactory2 )
        {
            // DirectX 11.1 or later
            DXGI_SWAP_CHAIN_DESC1 sd;
            ZeroMemory( &sd, sizeof( sd ) );
            sd.Width = mRequestedWidth;
            sd.Height = mRequestedHeight;
            sd.Format = _getSwapChainFormat();

            setCommonSwapChain( sd );

            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsDesc;
            ZeroMemory( &fsDesc, sizeof( fsDesc ) );
            fsDesc.RefreshRate.Numerator = mFrequencyNumerator;
            fsDesc.RefreshRate.Denominator = mFrequencyDenominator;
            fsDesc.Windowed = !( mRequestedFullscreenMode && !mAlwaysWindowedMode );

            hr = dxgiFactory2->CreateSwapChainForHwnd( mDevice.get(), mHwnd, &sd, &fsDesc, 0,
                                                       mSwapChain1.ReleaseAndGetAddressOf() );
        }
        else
        {
            // DirectX 11.0 systems
            DXGI_SWAP_CHAIN_DESC sd;
            ZeroMemory( &sd, sizeof( sd ) );
            setCommonSwapChain( sd );
            sd.BufferDesc.Width = mRequestedWidth;
            sd.BufferDesc.Height = mRequestedHeight;
            sd.BufferDesc.Format = _getSwapChainFormat();
            sd.BufferDesc.RefreshRate.Numerator = mFrequencyNumerator;
            sd.BufferDesc.RefreshRate.Denominator = mFrequencyDenominator;
            sd.OutputWindow = mHwnd;
            sd.Windowed = !( mRequestedFullscreenMode && !mAlwaysWindowedMode );

            IDXGIFactory1 *dxgiFactory1 = mDevice.GetDXGIFactory();
            hr =
                dxgiFactory1->CreateSwapChain( mDevice.get(), &sd, mSwapChain.ReleaseAndGetAddressOf() );
        }

        return hr;
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowHwnd::create( bool fullscreenMode, const NameValuePairList *miscParams )
    {
        destroy();

        HWND parentHwnd = 0;
        HWND externalHandle = 0;

        int left = INT_MAX;     // Defaults to screen center
        int top = INT_MAX;      // Defaults to screen center
        int monitorIndex = -1;  // Default by detecting the adapter from left / top position

        String border = "";
        bool outerSize = false;
        bool enableDoubleClick = false;

        if( miscParams )
        {
            NameValuePairList::const_iterator opt;
            // left (x)
            opt = miscParams->find( "left" );
            if( opt != miscParams->end() )
                left = StringConverter::parseInt( opt->second );
            // top (y)
            opt = miscParams->find( "top" );
            if( opt != miscParams->end() )
                top = StringConverter::parseInt( opt->second );
            // Window title
            opt = miscParams->find( "title" );
            if( opt != miscParams->end() )
                mTitle = opt->second;
            // parentWindowHandle       -> parentHwnd
            opt = miscParams->find( "parentWindowHandle" );
            if( opt != miscParams->end() )
                parentHwnd = (HWND)StringConverter::parseSizeT( opt->second );
            // externalWindowHandle     -> externalHandle
            opt = miscParams->find( "externalWindowHandle" );
            if( opt != miscParams->end() )
                externalHandle = (HWND)StringConverter::parseSizeT( opt->second );
            // window border style
            opt = miscParams->find( "border" );
            if( opt != miscParams->end() )
                border = opt->second;
            // set outer dimensions?
            opt = miscParams->find( "outerDimensions" );
            if( opt != miscParams->end() )
                outerSize = StringConverter::parseBool( opt->second );
            opt = miscParams->find( "monitorIndex" );
            if( opt != miscParams->end() )
                monitorIndex = StringConverter::parseInt( opt->second );

            // vsync    [parseBool]
            opt = miscParams->find( "vsync" );
            if( opt != miscParams->end() )
                mVSync = StringConverter::parseBool( opt->second );

#    if defined( _WIN32_WINNT_WIN8 )
            // useFlipMode    [parseBool]
            opt = miscParams->find( "useFlipMode" );
            if( opt != miscParams->end() )
            {
                mUseFlipMode =
                    mVSync && IsWindows8OrGreater() && StringConverter::parseBool( opt->second );
            }
#    endif
            // vsyncInterval    [parseUnsignedInt]
            opt = miscParams->find( "vsyncInterval" );
            if( opt != miscParams->end() )
                mVSyncInterval = StringConverter::parseUnsignedInt( opt->second );

            opt = miscParams->find( "alwaysWindowedMode" );
            if( opt != miscParams->end() )
                mAlwaysWindowedMode = StringConverter::parseBool( opt->second );

            // enable double click messages
            opt = miscParams->find( "enableDoubleClick" );
            if( opt != miscParams->end() )
                enableDoubleClick = StringConverter::parseBool( opt->second );
        }

        mRequestedFullscreenMode = fullscreenMode;
        if( mAlwaysWindowedMode )
            mRequestedFullscreenMode = false;

        if( !externalHandle )
        {
            HMONITOR hMonitor = NULL;
            MONITORINFO monitorInfo;

            // If monitor index found, try to assign the monitor handle based on it.
            if( monitorIndex != -1 )
            {
                DisplayMonitorList monitorList;
                EnumDisplayMonitors( NULL, NULL, createMonitorsInfoEnumProc, (LPARAM)&monitorList );
                if( monitorIndex < (int)monitorList.size() )
                    hMonitor = monitorList[monitorIndex];
            }

            if( hMonitor == NULL )
            {
                POINT windowAnchorPoint;
                windowAnchorPoint.x = left == INT_MAX ? 0 : left;
                windowAnchorPoint.y = top == INT_MAX ? 0 : top;

                hMonitor = MonitorFromPoint( windowAnchorPoint, MONITOR_DEFAULTTONEAREST );
            }

            memset( &monitorInfo, 0, sizeof( MONITORINFO ) );
            monitorInfo.cbSize = sizeof( MONITORINFO );
            GetMonitorInfo( hMonitor, &monitorInfo );

            // Setup styles
            mFullscreenWinStyle = WS_CLIPCHILDREN | WS_POPUP;
            mWindowedWinStyle = WS_CLIPCHILDREN;
            if( !mHidden )
            {
                mFullscreenWinStyle |= WS_VISIBLE;
                mWindowedWinStyle |= WS_VISIBLE;
            }
            if( parentHwnd )
            {
                mWindowedWinStyle |= WS_CHILD;
            }
            else
            {
                if( border == "none" || mBorderless )
                    mWindowedWinStyle |= WS_POPUP;
                else if( border == "fixed" )
                {
                    mWindowedWinStyle |=
                        WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
                }
                else
                    mWindowedWinStyle |= WS_OVERLAPPEDWINDOW;
            }

            uint32 winWidth, winHeight;
            winWidth = mRequestedWidth;
            winHeight = mRequestedHeight;
            {
                // Center window horizontally and/or vertically, on the right monitor.
                uint32 screenw = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
                uint32 screenh = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;
                uint32 outerw = ( winWidth < screenw ) ? winWidth : screenw;
                uint32 outerh = ( winHeight < screenh ) ? winHeight : screenh;
                if( left == INT_MAX )
                    left = monitorInfo.rcWork.left + ( screenw - outerw ) / 2;
                else if( monitorIndex != -1 )
                    left += monitorInfo.rcWork.left;
                if( top == INT_MAX )
                    top = monitorInfo.rcWork.top + ( screenh - outerh ) / 2;
                else if( monitorIndex != -1 )
                    top += monitorInfo.rcWork.top;
            }

            mTop = top;
            mLeft = left;
            DWORD dwStyleEx = 0;
            if( fullscreenMode )
            {
                dwStyleEx |= WS_EX_TOPMOST;
                mTop = monitorInfo.rcMonitor.top;
                mLeft = monitorInfo.rcMonitor.left;
            }
            else
            {
                RECT rc;
                SetRect( &rc, mLeft, mTop, mLeft + mRequestedWidth, mTop + mRequestedHeight );
                if( !outerSize )
                {
                    // User requested "client resolution", we need to grow the rect
                    // for the window's resolution (which is always bigger).
                    AdjustWindowRect( &rc, getWindowStyle( fullscreenMode ), false );
                }

                // Clamp to current monitor's size
                if( rc.left < monitorInfo.rcWork.left )
                {
                    rc.right += monitorInfo.rcWork.left - rc.left;
                    rc.left = monitorInfo.rcWork.left;
                }
                if( rc.top < monitorInfo.rcWork.top )
                {
                    rc.bottom += monitorInfo.rcWork.top - rc.top;
                    rc.top = monitorInfo.rcWork.top;
                }
                if( rc.right > monitorInfo.rcWork.right )
                    rc.right = monitorInfo.rcWork.right;
                if( rc.bottom > monitorInfo.rcWork.bottom )
                    rc.bottom = monitorInfo.rcWork.bottom;

                mLeft = rc.left;
                mTop = rc.top;
                mRequestedWidth = rc.right - rc.left;
                mRequestedHeight = rc.bottom - rc.top;
            }

            // Grab the HINSTANCE by asking the OS what's the hinstance at an address in this process
            HINSTANCE hInstance = NULL;
            static TCHAR staticVar;
            GetModuleHandleEx(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                &staticVar, &hInstance );

            WNDCLASSEX wcex;
            wcex.cbSize = sizeof( WNDCLASSEX );
            wcex.style = CS_HREDRAW | CS_VREDRAW;
            wcex.lpfnWndProc = WindowEventUtilities::_WndProc;
            wcex.cbClsExtra = 0;
            wcex.cbWndExtra = 0;
            wcex.hInstance = hInstance;
            wcex.hIcon = LoadIcon( (HINSTANCE)0, (LPCTSTR)IDI_APPLICATION );
            wcex.hCursor = LoadCursor( (HINSTANCE)0, IDC_ARROW );
            wcex.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
            wcex.lpszMenuName = 0;
            wcex.lpszClassName = OGRE_D3D11_WIN_CLASS_NAME;
            wcex.hIconSm = 0;

            if( enableDoubleClick )
                wcex.style |= CS_DBLCLKS;

            if( !mClassRegistered )
            {
                if( !RegisterClassEx( &wcex ) )
                {
                    OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                                 "RegisterClassEx failed! Cannot create window",
                                 "D3D11WindowHwnd::create" );
                }
                mClassRegistered = true;
            }

            mHwnd = CreateWindowEx( dwStyleEx, OGRE_D3D11_WIN_CLASS_NAME, mTitle.c_str(),
                                    getWindowStyle( fullscreenMode ), mLeft, mTop, mRequestedWidth,
                                    mRequestedHeight, parentHwnd, 0, hInstance, this );

            if( !mHwnd )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "CreateWindowEx failed! Cannot create window", "D3D11WindowHwnd::create" );
            }

            WindowEventUtilities::_addRenderWindow( this );

            mIsExternal = false;
        }
        else
        {
            mHwnd = externalHandle;
            mIsExternal = true;
        }

        RECT rc;
        // top and left represent outer window coordinates
        GetWindowRect( mHwnd, &rc );
        mTop = rc.top;
        mLeft = rc.left;
        // width and height represent interior drawable area
        GetClientRect( mHwnd, &rc );
        setFinalResolution( rc.right - rc.left, rc.bottom - rc.top );

        setHidden( mHidden );
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowHwnd::_initialize( TextureGpuManager *textureGpuManager,
                                       const NameValuePairList *miscParams )
    {
        D3D11WindowSwapChainBased::_initialize( textureGpuManager, miscParams );

        IDXGIFactory1 *dxgiFactory1 = mDevice.GetDXGIFactory();
        dxgiFactory1->MakeWindowAssociation( mHwnd,
                                             mAlwaysWindowedMode == true ? DXGI_MWA_NO_ALT_ENTER : 0 );
        setHidden( mHidden );
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowHwnd::destroy()
    {
        D3D11WindowSwapChainBased::destroy();

        if( mHwnd && !mIsExternal )
        {
            WindowEventUtilities::_removeRenderWindow( this );
            DestroyWindow( mHwnd );
        }
        mHwnd = 0;
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowHwnd::reposition( int32 left, int32 top )
    {
        if( mHwnd && !mRequestedFullscreenMode )
        {
            SetWindowPos( mHwnd, 0, left, top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowHwnd::requestResolution( uint32 width, uint32 height )
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
    void D3D11WindowHwnd::requestFullscreenSwitch( bool goFullscreen, bool borderless, uint32 monitorIdx,
                                                   uint32 width, uint32 height,
                                                   uint32 frequencyNumerator,
                                                   uint32 frequencyDenominator )
    {
        if( goFullscreen != mRequestedFullscreenMode || width != mRequestedWidth ||
            height != mRequestedHeight )
        {
            if( goFullscreen != mRequestedFullscreenMode )
                mRenderSystem->addToSwitchingFullscreenCounter();

            DWORD dwStyle = WS_VISIBLE | WS_CLIPCHILDREN;

            bool oldFullscreen = mRequestedFullscreenMode;
            mRequestedFullscreenMode = oldFullscreen;

            if( goFullscreen )
            {
                HMONITOR hMonitor = 0;
                if( monitorIdx != -1 )
                {
                    DisplayMonitorList monitorList;
                    EnumDisplayMonitors( NULL, NULL, createMonitorsInfoEnumProc, (LPARAM)&monitorList );
                    if( monitorIdx < (int)monitorList.size() )
                        hMonitor = monitorList[monitorIdx];
                }
                else
                    hMonitor = MonitorFromWindow( mHwnd, MONITOR_DEFAULTTONEAREST );
                MONITORINFO monitorInfo;
                memset( &monitorInfo, 0, sizeof( MONITORINFO ) );
                monitorInfo.cbSize = sizeof( MONITORINFO );
                GetMonitorInfo( hMonitor, &monitorInfo );
                mTop = monitorInfo.rcMonitor.top;
                mLeft = monitorInfo.rcMonitor.left;

                // need different ordering here
                if( oldFullscreen )
                {
                    // was previously fullscreen, just changing the resolution
                    SetWindowPos( mHwnd, HWND_TOPMOST, 0, 0, width, height, SWP_NOACTIVATE );
                }
                else
                {
                    SetWindowPos( mHwnd, HWND_TOPMOST, 0, 0, width, height, SWP_NOACTIVATE );
                    SetWindowLongPtr( mHwnd, GWL_STYLE, dwStyle );
                    SetWindowPos( mHwnd, 0, 0, 0, 0, 0,
                                  SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER );
                }
            }
            else
            {
                uint32 winWidth, winHeight;
                winWidth = mRequestedWidth;
                winHeight = mRequestedHeight;
                adjustWindow( mRequestedWidth, mRequestedHeight, &winWidth, &winHeight );
                SetWindowLongPtr( mHwnd, GWL_STYLE, getWindowStyle( mRequestedFullscreenMode ) );
                SetWindowPos( mHwnd, HWND_NOTOPMOST, 0, 0, winWidth, winHeight,
                              SWP_DRAWFRAME | SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOACTIVATE );
                updateWindowRect();
            }

            if( ( oldFullscreen && goFullscreen ) || mIsExternal )
            {
                // Notify viewports of resize
                notifyResolutionChanged();
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11WindowHwnd::windowMovedOrResized()
    {
        if( !mHwnd || IsIconic( mHwnd ) || !mSwapChain )
            return;

        updateWindowRect();
        notifyResolutionChanged();
    }
    //-----------------------------------------------------------------------------------
    bool D3D11WindowHwnd::isVisible() const
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
    void D3D11WindowHwnd::setHidden( bool hidden )
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
    void D3D11WindowHwnd::getCustomAttribute( IdString name, void *pData )
    {
        if( name == "WINDOW" || name == "RENDERDOC_WINDOW" )
        {
            HWND *pWnd = (HWND *)pData;
            *pWnd = mHwnd;
        }
        else
        {
            D3D11WindowSwapChainBased::getCustomAttribute( name, pData );
        }
    }
#endif
}  // namespace Ogre
