/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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

#include "Windowing/win32/OgreVulkanWin32Window.h"

#include "OgreVulkanDevice.h"
#include "OgreVulkanTextureGpu.h"
#include "OgreVulkanTextureGpuManager.h"

#include "OgreDepthBuffer.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreTextureGpuListener.h"
#include "OgreVulkanUtils.h"
#include "OgreWindowEventUtilities.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_win32.h"

#include <sstream>

#if defined( UNICODE ) && UNICODE
#    define OGRE_VULKAN_WIN_CLASS_NAME L"OgreVulkanWindow"
#else
#    define OGRE_VULKAN_WIN_CLASS_NAME "OgreVulkanWindow"
#endif

namespace Ogre
{
#define OGRE_MAX_CLASS_NAME 128
    bool VulkanWin32Window::mClassRegistered = false;

    VulkanWin32Window::VulkanWin32Window( const String &title, uint32 width, uint32 height,
                                          bool fullscreenMode ) :
        VulkanWindowSwapChainBased( title, width, height, fullscreenMode ),
        mHwnd( 0 ),
        mHDC( 0 ),
        mColourDepth( 32 ),
        mIsExternal( false ),
        mDeviceName( 0 ),
        mSizing( false ),
        mHidden( false ),
        mVisible( true ),
        mWindowedWinStyle( 0 ),
        mFullscreenWinStyle( 0 )
    {
    }
    //-------------------------------------------------------------------------
    VulkanWin32Window::~VulkanWin32Window()
    {
        destroy();

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
    }
    //-------------------------------------------------------------------------
    const char *VulkanWin32Window::getRequiredExtensionName()
    {
        return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
    }
    //-------------------------------------------------------------------------
    void VulkanWin32Window::updateWindowRect()
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
        }
    }
    //-------------------------------------------------------------------------
    void VulkanWin32Window::destroy()
    {
        VulkanWindowSwapChainBased::destroy();

        if( mClosed )
            return;

        mClosed = true;
        mFocused = false;

        if( mHwnd && !mIsExternal )
        {
            WindowEventUtilities::_removeRenderWindow( this );
            DestroyWindow( mHwnd );
        }
        mHwnd = 0;

        if( mFullscreenMode )
        {
            // switchFullScreen( false );
            mRequestedFullscreenMode = false;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanWin32Window::createWindow( const NameValuePairList *miscParams )
    {
        mClosed = false;
        mColourDepth = mRequestedFullscreenMode
                           ? 32u
                           : static_cast<uint32>( GetDeviceCaps( GetDC( 0 ), BITSPIXEL ) );

        int left = -1;  // Defaults to screen center
        int top = -1;   // Defaults to screen center
        HWND parentHwnd = 0;
        bool hidden = false;
        String border;
        bool outerSize = false;
        mHwGamma = false;
        bool enableDoubleClick = false;
        int monitorIndex = -1;
        HMONITOR hMonitor = NULL;
        // uint8 msaaQuality = 0;
        HINSTANCE hInstance = NULL;

        mFrequencyDenominator = 1u;

        if( miscParams )
        {
            // Get variable-length params
            NameValuePairList::const_iterator opt;
            NameValuePairList::const_iterator end = miscParams->end();

            opt = miscParams->find( "left" );
            if( opt != end )
                left = StringConverter::parseInt( opt->second );
            opt = miscParams->find( "top" );
            if( opt != end )
                top = StringConverter::parseInt( opt->second );
            opt = miscParams->find( "hidden" );
            if( opt != end )
                hidden = StringConverter::parseBool( opt->second );

            parseSharedParams( miscParams );

            opt = miscParams->find( "externalWindowHandle" );
            if( opt != end )
            {
                mHwnd = (HWND)StringConverter::parseSizeT( opt->second );

                if( IsWindow( mHwnd ) && mHwnd )
                {
                    mIsExternal = true;
                    mRequestedFullscreenMode = false;
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
            mFullscreenWinStyle = ( hidden ? 0 : WS_VISIBLE ) | WS_CLIPCHILDREN | WS_POPUP;
            mWindowedWinStyle = ( hidden ? 0 : WS_VISIBLE ) | WS_CLIPCHILDREN;

            if( parentHwnd )
            {
                mWindowedWinStyle |= WS_CHILD;
            }
            else
            {
                if( border == "none" || mBorderless )
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
                uint32 screenw = (uint32)( monitorInfoEx.rcWork.right - monitorInfoEx.rcWork.left );
                uint32 screenh = (uint32)( monitorInfoEx.rcWork.bottom - monitorInfoEx.rcWork.top );
                uint32 outerw = ( winWidth < screenw ) ? winWidth : screenw;
                uint32 outerh = ( winHeight < screenh ) ? winHeight : screenh;
                if( left == INT_MAX )
                    left = monitorInfoEx.rcWork.left + int( ( screenw - outerw ) / 2 );
                else if( monitorIndex != -1 )
                    left += monitorInfoEx.rcWork.left;
                if( top == INT_MAX )
                    top = monitorInfoEx.rcWork.top + int( ( screenh - outerh ) / 2 );
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
                SetRect( &rc, mLeft, mTop,  //
                         static_cast<int>( mRequestedWidth ), static_cast<int>( mRequestedHeight ) );
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
                mRequestedWidth = static_cast<uint32>( rc.right - rc.left );
                mRequestedHeight = static_cast<uint32>( rc.bottom - rc.top );
            }
            // Grab the HINSTANCE by asking the OS what's the hinstance at an address in this process

#ifdef __MINGW32__
#    ifdef OGRE_STATIC_LIB
            hInstance = GetModuleHandle( NULL );
#    else
#        if OGRE_DEBUG_MODE
            hInstance = GetModuleHandle( "RenderSystem_Vulkan_d.dll" );
#        else
            hInstance = GetModuleHandle( "RenderSystem_Vulkan.dll" );
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
            wcex.lpszClassName = OGRE_VULKAN_WIN_CLASS_NAME;
            wcex.hIconSm = 0;

            if( enableDoubleClick )
                wcex.style |= CS_DBLCLKS;

            if( !mClassRegistered )
            {
                if( !RegisterClassEx( &wcex ) )
                {
                    OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                                 "RegisterClassEx failed! Cannot create window",
                                 "VulkanWin32Window::create" );
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
            mHwnd =
                CreateWindowEx( dwStyleEx, "OgreVulkanWindow", mTitle.c_str(),
                                getWindowStyle( mRequestedFullscreenMode ), mLeft, mTop,
                                static_cast<int>( mRequestedWidth ),
                                static_cast<int>( mRequestedHeight ), parentHwnd, 0, hInstance, this );

            WindowEventUtilities::_addRenderWindow( this );

            LogManager::getSingleton().stream()
                << "Created VulkanWin32Window '" << mTitle << "' : " << mRequestedWidth << "x"
                << mRequestedHeight << ", " << mColourDepth << "bpp";
        }

        VkWin32SurfaceCreateInfoKHR createInfo;
        makeVkStruct( createInfo, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR );
        createInfo.hwnd = mHwnd;
        createInfo.hinstance = hInstance;

        VkBool32 presentationSupportError = vkGetPhysicalDeviceWin32PresentationSupportKHR(
            mDevice->mPhysicalDevice, mDevice->mGraphicsQueue.mFamilyIdx );

        if( !presentationSupportError )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Vulkan not supported on given Win32 window ",
                         "VulkanWin32Window::_initialize" );
        }

        VkSurfaceKHR surface;

        VkResult result =
            vkCreateWin32SurfaceKHR( mDevice->mInstance->mVkInstance, &createInfo, 0, &surface );
        checkVkResult( result, "vkCreateWin32SurfaceKHR" );

        mSurfaceKHR = surface;
    }
    //-------------------------------------------------------------------------
    void VulkanWin32Window::adjustWindow( uint32 clientWidth, uint32 clientHeight,
                                          uint32 *outDrawableWidth, uint32 *outDrawableHeight )
    {
        RECT rc;
        SetRect( &rc, 0, 0, static_cast<int>( clientWidth ), static_cast<int>( clientHeight ) );
        AdjustWindowRect( &rc, getWindowStyle( mRequestedFullscreenMode ), false );
        *outDrawableWidth = static_cast<uint32>( rc.right - rc.left );
        *outDrawableHeight = static_cast<uint32>( rc.bottom - rc.top );
    }
    //-------------------------------------------------------------------------
    DWORD VulkanWin32Window::getWindowStyle( bool fullScreen ) const
    {
        return fullScreen ? mFullscreenWinStyle : mWindowedWinStyle;
    }
    //-------------------------------------------------------------------------
    void VulkanWin32Window::_setVisible( bool visible ) { mVisible = visible; }
    //-------------------------------------------------------------------------
    bool VulkanWin32Window::isVisible() const { return mVisible; }
    //-------------------------------------------------------------------------
    void VulkanWin32Window::setHidden( bool hidden )
    {
        mHidden = hidden;

        // ignore for external windows as these should handle
        // this externally
        if( mIsExternal )
            return;

        if( !mIsExternal )
        {
            if( hidden )
                ShowWindow( mHwnd, SW_HIDE );
            else
                ShowWindow( mHwnd, SW_SHOWNORMAL );
        }
    }
    //-------------------------------------------------------------------------
    bool VulkanWin32Window::isHidden() const { return mHidden; }
    //-------------------------------------------------------------------------
    void VulkanWin32Window::_initialize( TextureGpuManager *textureGpuManager,
                                         const NameValuePairList *miscParams )
    {
        destroy();

        mFocused = true;
        mClosed = false;

        createWindow( miscParams );

        VulkanTextureGpuManager *textureManager =
            static_cast<VulkanTextureGpuManager *>( textureGpuManager );

        mTexture = textureManager->createTextureGpuWindow( this );
        if( DepthBuffer::DefaultDepthBufferFormat != PFG_NULL )
        {
            const bool bMemoryLess = requestedMemoryless( miscParams );
            mDepthBuffer = textureManager->createWindowDepthBuffer( bMemoryLess );
        }

        setFinalResolution( mRequestedWidth, mRequestedHeight );

        // if( mColourDepth == 16u )
        //     mTexture->setPixelFormat( PFG_B5G5R5A1_UNORM );
        // else
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
            mTexture->_setDepthBufferDefaults( mDepthBuffer->isTilerMemoryless()
                                                   ? DepthBuffer::POOL_MEMORYLESS
                                                   : DepthBuffer::NO_POOL_EXPLICIT_RTV,
                                               false, mDepthBuffer->getPixelFormat() );
        }
        else
        {
            mTexture->_setDepthBufferDefaults( DepthBuffer::POOL_NO_DEPTH, false, PFG_NULL );
        }

        createSwapchain();

        setHidden( mHidden );
    }
    //-------------------------------------------------------------------------
    void VulkanWin32Window::reposition( int32 left, int32 top )
    {
        if( mHwnd && !mRequestedFullscreenMode )
        {
            SetWindowPos( mHwnd, 0, top, left, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
        }
    }
    //-------------------------------------------------------------------------
    void VulkanWin32Window::requestResolution( uint32 width, uint32 height )
    {
        if( !mIsExternal )
        {
            if( mHwnd && !mRequestedFullscreenMode )
            {
                uint32 winWidth, winHeight;
                adjustWindow( width, height, &winWidth, &winHeight );
                SetWindowPos( mHwnd, 0, 0, 0, static_cast<int>( winWidth ),
                              static_cast<int>( winHeight ),
                              SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
            }
        }
        else
            updateWindowRect();
    }
    //-------------------------------------------------------------------------
    void VulkanWin32Window::requestFullscreenSwitch( bool goFullscreen, bool borderless,
                                                     uint32 monitorIdx, uint32 width, uint32 height,
                                                     uint32 frequencyNumerator,
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
                SetWindowPos( mHwnd, HWND_TOPMOST, 0, 0, static_cast<int>( mRequestedWidth ),
                              static_cast<int>( mRequestedHeight ), SWP_NOACTIVATE );

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

                SetWindowLong( mHwnd, GWL_STYLE, (LONG)getWindowStyle( mRequestedFullscreenMode ) );
                SetWindowPos( mHwnd, HWND_TOPMOST, mLeft, mTop, static_cast<int>( width ),
                              static_cast<int>( height ), SWP_NOACTIVATE );
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

                const LONG screenw = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
                const LONG screenh = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;

                const int left = ( screenw > (int)winWidth ) ? ( ( screenw - (int)winWidth ) / 2 ) : 0;
                const int top = ( screenh > (int)winHeight ) ? ( ( screenh - (int)winHeight ) / 2 ) : 0;

                SetWindowLong( mHwnd, GWL_STYLE, (LONG)getWindowStyle( mRequestedFullscreenMode ) );
                SetWindowPos( mHwnd, HWND_NOTOPMOST, left, top, static_cast<int>( winWidth ),
                              static_cast<int>( winHeight ),
                              SWP_DRAWFRAME | SWP_FRAMECHANGED | SWP_NOACTIVATE );
                mLeft = left;
                mTop = top;
                setFinalResolution( width, height );

                windowMovedOrResized();
            }

            mFullscreenMode = mRequestedFullscreenMode;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanWin32Window::windowMovedOrResized()
    {
        if( !mHwnd || IsIconic( mHwnd ) )
            return;

        updateWindowRect();

        if( mRequestedWidth == getWidth() && mRequestedHeight == getHeight() && !mRebuildingSwapchain )
            return;

        mDevice->stall();

        destroySwapchain();

        // Depth & Stencil buffer are normal textures; thus they need to be reeinitialized normally
        if( mDepthBuffer )
            mDepthBuffer->_transitionTo( GpuResidency::OnStorage, (uint8 *)0 );
        if( mStencilBuffer && mStencilBuffer != mDepthBuffer )
            mStencilBuffer->_transitionTo( GpuResidency::OnStorage, (uint8 *)0 );

        setFinalResolution( mRequestedWidth, mRequestedHeight );

        createSwapchain();
    }
    //-------------------------------------------------------------------------
    void VulkanWin32Window::setFocused( bool focused )
    {
        if( mDeviceName != NULL && focused == false )
        {
            HWND hActiveWindow = GetActiveWindow();
            char classNameSrc[OGRE_MAX_CLASS_NAME + 1];
            char classNameDst[OGRE_MAX_CLASS_NAME + 1];

            GetClassName( mHwnd, classNameSrc, OGRE_MAX_CLASS_NAME );
            GetClassName( hActiveWindow, classNameDst, OGRE_MAX_CLASS_NAME );

            if( strcmp( classNameDst, classNameSrc ) == 0 )
                focused = true;
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
                requestFullscreenSwitch( true, mBorderless, std::numeric_limits<uint32>::max(),
                                         mRequestedWidth, mRequestedHeight, mFrequencyNumerator,
                                         mFrequencyDenominator );
            }
        }
    }
    //-------------------------------------------------------------------------
    void VulkanWin32Window::getCustomAttribute( IdString name, void *pData )
    {
        if( name == "WINDOW" || name == "RENDERDOC_WINDOW" )
        {
            HWND *pHwnd = (HWND *)pData;
            *pHwnd = mHwnd;
            return;
        }
        else
        {
            VulkanWindowSwapChainBased::getCustomAttribute( name, pData );
        }
    }
}  // namespace Ogre
