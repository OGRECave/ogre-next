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

namespace Ogre
{
#define _MAX_CLASS_NAME_ 128
    bool OgreVulkanWin32Window::mClassRegistered = false;

    OgreVulkanWin32Window::OgreVulkanWin32Window( FastArray<const char *> &inOutRequiredInstanceExts,
                                                  const String &title, uint32 width, uint32 height,
                                                  bool fullscreenMode ) :
        VulkanWindow( title, width, height, fullscreenMode ),
        mHwnd( 0 ),
        mHDC( 0 ),
        mColourDepth( 32 ),
        mIsExternal( false ),
        mDeviceName( 0 ),
        mIsExternalGLControl( false ),
        mOwnsGLContext( true ),
        mSizing( false ),
        mClosed( false ),
        mHidden( false ),
        mVisible( true ),
        mIsTopLevel( true ),
        mWindowedWinStyle( 0 ),
        mFullscreenWinStyle( 0 )
    {
        inOutRequiredInstanceExts.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
    }

    OgreVulkanWin32Window::~OgreVulkanWin32Window()
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

    void OgreVulkanWin32Window::destroy()
    {
        VulkanWindow::destroy();

        if( mClosed )
            return;

        mClosed = true;
        mFocused = false;

        if( !mIsExternal )
            WindowEventUtilities::_removeRenderWindow( this );

        if( mFullscreenMode )
        {
            // switchFullScreen( false );
            mRequestedFullscreenMode = false;
        }
    }

    void OgreVulkanWin32Window::createWindow( const String &windowName, uint32 width, uint32 height,
                                              const NameValuePairList *miscParams )
    {
        mClosed = false;
        mColourDepth = mRequestedFullscreenMode ? 32 : GetDeviceCaps( GetDC( 0 ), BITSPIXEL );
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
        //        uint8 msaaQuality = 0;
        HINSTANCE hInstance = NULL;
        uint32 windowWidth = width;
        uint32 windowHeight = height;

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
                windowWidth = rc.right - rc.left - 1;
                windowHeight = rc.bottom - rc.top - 1;
            }

            // Grab the HINSTANCE by asking the OS what's the hinstance at an address in this process

#ifdef __MINGW32__
#    ifdef OGRE_STATIC_LIB
            hInstance = GetModuleHandle( NULL );
#    else
#        if OGRE_DEBUG_MODE == 1
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
            wcex.lpszClassName = "OgreVulkanWindow";
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
            mHwnd = CreateWindowEx( dwStyleEx, "OgreVulkanWindow", mTitle.c_str(),
                                    getWindowStyle( mRequestedFullscreenMode ), mLeft, mTop, windowWidth,
                                    windowHeight, parentHwnd, 0, hInstance, this );

            WindowEventUtilities::_addRenderWindow( this );

            LogManager::getSingleton().stream()
                << "Created Win32Window '" << mTitle << "' : " << mRequestedWidth << "x"
                << mRequestedHeight << ", " << mColourDepth << "bpp";
        }

        VkWin32SurfaceCreateInfoKHR createInfo;
        makeVkStruct( createInfo, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR );
        createInfo.hwnd = mHwnd;
        createInfo.hinstance = hInstance;

        VkBool32 presentationSupportError = vkGetPhysicalDeviceWin32PresentationSupportKHR(
            mDevice->mPhysicalDevice, mDevice->mGraphicsQueue.mFamilyIdx );

        // if( !presentationSupportError )
        // {
        //     OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "Vulkan not supported on given X11
        //     window",
        //                  "OgreVulkanWin32Window::_initialize" );
        // }

        VkSurfaceKHR surface;

        VkResult createSurfaceResult =
            vkCreateWin32SurfaceKHR( mDevice->mInstance, &createInfo, nullptr, &surface );

        // if( createSurfaceResult != VK_SUCCESS )
        // {
        //     throw std::runtime_error( "failed to create window surface!" );
        // }
        mSurfaceKHR = surface;
    }

    DWORD OgreVulkanWin32Window::getWindowStyle( bool fullScreen ) const
    {
        return fullScreen ? mFullscreenWinStyle : mWindowedWinStyle;
    }

    void OgreVulkanWin32Window::reposition( int32 left, int32 top )
    {
        if( mClosed || !mIsTopLevel )
            return;

        if( mHwnd && !mRequestedFullscreenMode )
        {
            SetWindowPos( mHwnd, 0, top, left, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
        }
    }

    void OgreVulkanWin32Window::_setVisible( bool visible ) { mVisible = visible; }

    bool OgreVulkanWin32Window::isVisible() const { return mVisible; }

    void OgreVulkanWin32Window::setHidden( bool hidden )
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

    bool OgreVulkanWin32Window::isHidden() const { return false; }

    void OgreVulkanWin32Window::_initialize( TextureGpuManager *textureGpuManager,
                                             const NameValuePairList *miscParams )
    {
        destroy();

        mFocused = true;
        mClosed = false;

        createWindow( mTitle, mRequestedWidth, mRequestedHeight, miscParams );

        VulkanTextureGpuManager *textureManager =
            static_cast<VulkanTextureGpuManager *>( textureGpuManager );

        mTexture = textureManager->createTextureGpuWindow( this );
        mDepthBuffer = textureManager->createWindowDepthBuffer();

        setFinalResolution( mRequestedWidth, mRequestedHeight );

        // if( mColourDepth == 16u )
        //     mTexture->setPixelFormat( PFG_B5G5R5A1_UNORM );
        // else
        mTexture->setPixelFormat( chooseSurfaceFormat( mHwGamma ) );
		mDepthBuffer->setPixelFormat( DepthBuffer::DefaultDepthBufferFormat );
        if( PixelFormatGpuUtils::isStencil( mDepthBuffer->getPixelFormat() ) )
            mStencilBuffer = mDepthBuffer;
        
        mTexture->setSampleDescription( mRequestedSampleDescription );
        mDepthBuffer->setSampleDescription( mRequestedSampleDescription );

        // mTexture->setMsaa( mMsaaCount );
        // mDepthBuffer->setMsaa( mMsaaCount );

        if( mDepthBuffer )
        {
            mTexture->_setDepthBufferDefaults( DepthBuffer::POOL_NON_SHAREABLE, false,
                                               mDepthBuffer->getPixelFormat() );
        }
        else
        {
            mTexture->_setDepthBufferDefaults( DepthBuffer::POOL_NO_DEPTH, false, PFG_NULL );
        }

        mSampleDescription = mRequestedSampleDescription;

        setMsaaBackbuffer();

        mTexture->_transitionTo( GpuResidency::Resident, (uint8 *)0 );
        if( mDepthBuffer )
            mDepthBuffer->_transitionTo( GpuResidency::Resident, (uint8 *)0 );

        setHidden( mHidden );

        createSwapchain();
    }
}  // namespace Ogre
