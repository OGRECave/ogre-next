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

#include "Windowing/X11/OgreVulkanXcbWindow.h"

#include "OgreVulkanDevice.h"
#include "OgreVulkanTextureGpu.h"
#include "OgreVulkanTextureGpuManager.h"

#include "OgreTextureGpuListener.h"

#include "OgreDepthBuffer.h"
#include "OgreException.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreString.h"
#include "OgreWindowEventUtilities.h"

#include <X11/Xlib-xcb.h>
#include <xcb/randr.h>
#include <xcb/xcb.h>
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_xcb.h"

namespace Ogre
{
    static xcb_intern_atom_cookie_t intern_atom_cookie( xcb_connection_t *c, const std::string &s )
    {
        return xcb_intern_atom( c, false, static_cast<uint16>( s.size() ), s.c_str() );
    }

    static xcb_atom_t intern_atom( xcb_connection_t *c, xcb_intern_atom_cookie_t cookie )
    {
        xcb_atom_t atom = XCB_ATOM_NONE;
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply( c, cookie, 0 );
        if( reply )
        {
            atom = reply->atom;
            free( reply );
        }
        return atom;
    }

    VulkanXcbWindow::VulkanXcbWindow( const String &title, uint32 width, uint32 height,
                                      bool fullscreenMode ) :
        VulkanWindowSwapChainBased( title, width, height, fullscreenMode ),
        mConnection( 0 ),
        mScreen( 0 ),
        mXcbWindow( 0 ),
        mWmProtocols( 0 ),
        mWmDeleteWindow( 0 ),
        mWmNetState( 0 ),
        mWmFullscreen( 0 ),
        mVisible( true ),
        mHidden( false ),
        mIsTopLevel( true ),
        mIsExternal( false )
    {
    }
    //-------------------------------------------------------------------------
    VulkanXcbWindow::~VulkanXcbWindow()
    {
        destroy();

        if( !mIsExternal )
        {
            xcb_destroy_window( mConnection, mXcbWindow );
            xcb_flush( mConnection );
            xcb_disconnect( mConnection );
        }
        else
        {
            xcb_flush( mConnection );
        }

        mConnection = 0;
    }
    //-----------------------------------------------------------------------------------
    const char *VulkanXcbWindow::getRequiredExtensionName() { return VK_KHR_XCB_SURFACE_EXTENSION_NAME; }
    //-----------------------------------------------------------------------------------
    void VulkanXcbWindow::destroy()
    {
        VulkanWindowSwapChainBased::destroy();

        if( mClosed )
            return;

        mClosed = true;
        mFocused = false;

        if( !mIsExternal )
            WindowEventUtilities::_removeRenderWindow( this );

        if( mFullscreenMode )
        {
            switchFullScreen( false );
            mRequestedFullscreenMode = false;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanXcbWindow::_initialize( TextureGpuManager *textureGpuManager,
                                       const NameValuePairList *miscParams )
    {
        destroy();

        mFocused = true;
        mClosed = false;
        mHwGamma = false;

        if( miscParams )
        {
            NameValuePairList::const_iterator opt;
            NameValuePairList::const_iterator end = miscParams->end();

            opt = miscParams->find( "SDL2x11" );
            if( opt != end )
            {
                struct SDLx11
                {
                    Display *display; /**< The X11 display */
                    ::Window window;  /**< The X11 window */
                };

                SDLx11 *sdlHandles =
                    reinterpret_cast<SDLx11 *>( StringConverter::parseUnsignedLong( opt->second ) );
                mIsExternal = true;
                mConnection = XGetXCBConnection( sdlHandles->display );
                mXcbWindow = (xcb_window_t)sdlHandles->window;

                XWindowAttributes windowAttrib;
                XGetWindowAttributes( sdlHandles->display, sdlHandles->window, &windowAttrib );

                int scr = DefaultScreen( sdlHandles->display );

                const xcb_setup_t *setup = xcb_get_setup( mConnection );
                xcb_screen_iterator_t iter = xcb_setup_roots_iterator( setup );
                while( scr-- > 0 )
                    xcb_screen_next( &iter );

                mScreen = iter.data;
            }

            parseSharedParams( miscParams );
        }

        if( !mXcbWindow )
        {
            initConnection();  // TODO: Connection must be shared by ALL windows
            createWindow( mTitle, mRequestedWidth, mRequestedHeight, miscParams );
            setHidden( false );
        }

        if( mRequestedFullscreenMode )
        {
            switchMode( mRequestedWidth, mRequestedHeight, mFrequencyNumerator, mFrequencyDenominator );
            mFullscreenMode = true;
        }

        PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR get_xcb_presentation_support =
            (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR)vkGetInstanceProcAddr(
                mDevice->mInstance->mVkInstance, "vkGetPhysicalDeviceXcbPresentationSupportKHR" );

        if( !get_xcb_presentation_support( mDevice->mPhysicalDevice, mDevice->mGraphicsQueue.mFamilyIdx,
                                           mConnection, mScreen->root_visual ) )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "Vulkan not supported on given X11 window",
                         "VulkanXcbWindow::_initialize" );
        }

        createSurface();

        VulkanTextureGpuManager *textureManager =
            static_cast<VulkanTextureGpuManager *>( textureGpuManager );

        mTexture = textureManager->createTextureGpuWindow( this );
        if( DepthBuffer::DefaultDepthBufferFormat != PFG_NULL )
        {
            const bool bMemoryLess = requestedMemoryless( miscParams );
            mDepthBuffer = textureManager->createWindowDepthBuffer( bMemoryLess );
        }
        mStencilBuffer = 0;

        setFinalResolution( mRequestedWidth, mRequestedHeight );

        createSwapchain();
    }
    //-------------------------------------------------------------------------
    void VulkanXcbWindow::createSurface()
    {
        PFN_vkCreateXcbSurfaceKHR create_xcb_surface = (PFN_vkCreateXcbSurfaceKHR)vkGetInstanceProcAddr(
            mDevice->mInstance->mVkInstance, "vkCreateXcbSurfaceKHR" );

        VkXcbSurfaceCreateInfoKHR xcbSurfCreateInfo;
        memset( &xcbSurfCreateInfo, 0, sizeof( xcbSurfCreateInfo ) );
        xcbSurfCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        xcbSurfCreateInfo.connection = mConnection;
        xcbSurfCreateInfo.window = mXcbWindow;
        VkResult result =
            create_xcb_surface( mDevice->mInstance->mVkInstance, &xcbSurfCreateInfo, 0, &mSurfaceKHR );
        checkVkResult( mDevice, result, "vkCreateXcbSurfaceKHR" );
    }
    //-------------------------------------------------------------------------
    void VulkanXcbWindow::initConnection()
    {
        int scr = 0;

        mConnection = xcb_connect( 0, &scr );
        if( !mConnection || xcb_connection_has_error( mConnection ) )
        {
            xcb_disconnect( mConnection );
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "failed to connect to the display server",
                         "VulkanXcbWindow::initConnection" );
        }

        const xcb_setup_t *setup = xcb_get_setup( mConnection );
        xcb_screen_iterator_t iter = xcb_setup_roots_iterator( setup );
        while( scr-- > 0 )
            xcb_screen_next( &iter );

        mScreen = iter.data;
    }
    //-------------------------------------------------------------------------
    void VulkanXcbWindow::createWindow( const String &windowName, uint32 width, uint32 height,
                                        const NameValuePairList *miscParams )
    {
        mXcbWindow = xcb_generate_id( mConnection );

        uint32_t value_mask, value_list[32];
        value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
        value_list[0] = mScreen->black_pixel;
        value_list[1] = /*XCB_EVENT_MASK_KEY_PRESS |*/ XCB_EVENT_MASK_STRUCTURE_NOTIFY;

        xcb_create_window( mConnection, XCB_COPY_FROM_PARENT, mXcbWindow, mScreen->root, 0, 0,
                           static_cast<uint16>( width ), static_cast<uint16>( height ), 10u,
                           XCB_WINDOW_CLASS_INPUT_OUTPUT, mScreen->root_visual, value_mask, value_list );

        xcb_intern_atom_cookie_t utf8_string_cookie = intern_atom_cookie( mConnection, "UTF8_STRING" );
        xcb_intern_atom_cookie_t wm_name_cookie = intern_atom_cookie( mConnection, "WM_NAME" );
        xcb_intern_atom_cookie_t mWmProtocolscookie = intern_atom_cookie( mConnection, "WM_PROTOCOLS" );
        xcb_intern_atom_cookie_t mWmDeleteWindowcookie =
            intern_atom_cookie( mConnection, "WM_DELETE_WINDOW" );
        xcb_intern_atom_cookie_t mWmNetStatecookie = intern_atom_cookie( mConnection, "_NET_WM_STATE" );
        xcb_intern_atom_cookie_t mWmFullscreencookie =
            intern_atom_cookie( mConnection, "_NET_WM_STATE_FULLSCREEN" );

        // set title
        xcb_atom_t utf8_string = intern_atom( mConnection, utf8_string_cookie );
        xcb_atom_t wm_name = intern_atom( mConnection, wm_name_cookie );
        xcb_change_property( mConnection, XCB_PROP_MODE_REPLACE, mXcbWindow, wm_name, utf8_string, 8u,
                             static_cast<uint32>( windowName.size() ), windowName.c_str() );

        // advertise WM_DELETE_WINDOW
        mWmProtocols = intern_atom( mConnection, mWmProtocolscookie );
        mWmDeleteWindow = intern_atom( mConnection, mWmDeleteWindowcookie );
        xcb_change_property( mConnection, XCB_PROP_MODE_REPLACE, mXcbWindow, mWmProtocols, XCB_ATOM_ATOM,
                             32u, 1u, &mWmDeleteWindow );

        mWmNetState = intern_atom( mConnection, mWmNetStatecookie );
        mWmFullscreen = intern_atom( mConnection, mWmFullscreencookie );
        if( mWmNetState != XCB_ATOM_NONE && mWmFullscreen != XCB_ATOM_NONE && mRequestedFullscreenMode )
        {
            xcb_change_property( mConnection, XCB_PROP_MODE_REPLACE, mXcbWindow, mWmNetState,
                                 XCB_ATOM_ATOM, 32u, 1u, &mWmFullscreen );
            xcb_flush( mConnection );
        }

        WindowEventUtilities::_addRenderWindow( this );
    }
    //-----------------------------------------------------------------------------------
    void VulkanXcbWindow::switchMode( uint32 width, uint32 height, uint32 frequencyNum,
                                      uint32 frequencyDen )
    {
        // TODO: This ignores frequency and plays really bad with multimonitor setups
        // (just like GL3's version). SDL2 gets this right
        LogManager::getSingleton().logMessage( "xcb_randr: Requesting resolution change " +
                                               StringConverter::toString( width ) + "x" +
                                               StringConverter::toString( height ) );
        {
            xcb_randr_query_version_reply_t *replyRrandVersion = xcb_randr_query_version_reply(
                mConnection, xcb_randr_query_version( mConnection, 1, 1 ), 0 );
            if( !replyRrandVersion )
            {
                LogManager::getSingleton().logMessage(
                    "RandR version 1.1 required to switch resolutions" );
                return;
            }
            else
            {
                free( replyRrandVersion );
                replyRrandVersion = 0;
            }
        }

        xcb_randr_get_screen_info_reply_t *replyScreenInfoGet = xcb_randr_get_screen_info_reply(
            mConnection, xcb_randr_get_screen_info( mConnection, mScreen->root ), NULL );
        if( replyScreenInfoGet == NULL )
        {
            LogManager::getSingleton().logMessage(
                "Cannot get screen sizes. Resolution change won't happen" );
            return;
        }

        const xcb_randr_screen_size_t *screenSizes =
            xcb_randr_get_screen_info_sizes( replyScreenInfoGet );
        const size_t numScreenSizes = replyScreenInfoGet->nSizes;

        uint32 chosenId = std::numeric_limits<uint32>::max();

        for( size_t i = 0u; i < numScreenSizes; ++i )
        {
            if( width == screenSizes[i].width && height == screenSizes[i].height )
            {
                chosenId = static_cast<uint32>( i );
                break;
            }
        }

        if( chosenId == std::numeric_limits<uint32_t>::max() )
        {
            free( replyScreenInfoGet );
            LogManager::getSingleton().logMessage( "Requested resolution is not provided by monitor." );
            return;
        }

        xcb_randr_set_screen_config_cookie_t cookieConfigSet = xcb_randr_set_screen_config(
            mConnection, mScreen->root, XCB_CURRENT_TIME, replyScreenInfoGet->config_timestamp,
            static_cast<uint16>( chosenId ), replyScreenInfoGet->rotation, 0 );

        xcb_generic_error_t *error = 0;
        xcb_randr_set_screen_config_reply_t *replySetConfig =
            xcb_randr_set_screen_config_reply( mConnection, cookieConfigSet, &error );

        if( replySetConfig )
            free( replySetConfig );
        else
            LogManager::getSingleton().logMessage( "Failed to set resolution" );
    }
    //-----------------------------------------------------------------------------------
    void VulkanXcbWindow::switchFullScreen( const bool bFullscreen )
    {
#define _NET_WM_STATE_REMOVE 0  // remove/unset property
#define _NET_WM_STATE_ADD 1     // add/set property
#define _NET_WM_STATE_TOGGLE 2  // toggle property

        xcb_client_message_event_t event;
        memset( &event, 0, sizeof( event ) );
        event.response_type = XCB_CLIENT_MESSAGE;
        event.format = 32;
        event.sequence = 0;
        event.window = mXcbWindow;
        event.type = mWmNetState;
        event.data.data32[0] = bFullscreen ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
        event.data.data32[1] = mWmFullscreen;
        event.data.data32[2] = 0;
        event.data.data32[3] = 0;
        event.data.data32[4] = 0;
        xcb_send_event( mConnection, 0, mScreen->root,
                        XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
                        (const char *)&event );
        xcb_flush( mConnection );
    }
    //-----------------------------------------------------------------------------------
    void VulkanXcbWindow::reposition( int32 left, int32 top )
    {
        if( mClosed || !mIsTopLevel )
            return;

        if( !mRequestedFullscreenMode )
        {
            const int32 values[2] = { left, top };
            xcb_configure_window( mConnection, mXcbWindow, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                                  values );
        }
    }
    //-----------------------------------------------------------------------------------
    void VulkanXcbWindow::requestFullscreenSwitch( bool goFullscreen, bool borderless, uint32 monitorIdx,
                                                   uint32 width, uint32 height,
                                                   uint32 frequencyNumerator,
                                                   uint32 frequencyDenominator )
    {
        if( mClosed || !mIsTopLevel )
            return;

        if( goFullscreen == mFullscreenMode && width == mRequestedWidth && height == mRequestedHeight &&
            mTexture->getWidth() == width && mTexture->getHeight() == height &&
            mFrequencyNumerator == frequencyNumerator )
        {
            mRequestedFullscreenMode = mFullscreenMode;
            return;
        }

        if( goFullscreen && !mWmFullscreen )
        {
            // Without WM support it is best to give up.
            LogManager::getSingleton().logMessage(
                "GLXWindow::switchFullScreen: Your WM has no fullscreen support" );
            mRequestedFullscreenMode = false;
            mFullscreenMode = false;
            return;
        }

        Window::requestFullscreenSwitch( goFullscreen, borderless, monitorIdx, width, height,
                                         frequencyNumerator, frequencyDenominator );

        if( goFullscreen )
        {
            switchMode( width, height, frequencyNumerator, frequencyDenominator );
        }
        else
        {
            // switchMode( width, height, frequencyNumerator, frequencyDenominator );
        }

        if( mFullscreenMode != goFullscreen )
            switchFullScreen( goFullscreen );

        if( !mFullscreenMode )
        {
            requestResolution( width, height );
            reposition( mLeft, mTop );
        }
    }
    //-----------------------------------------------------------------------------------
    void VulkanXcbWindow::requestResolution( uint32 width, uint32 height )
    {
        if( mClosed )
            return;

        if( mTexture && mTexture->getWidth() == width && mTexture->getHeight() == height )
            return;

        Window::requestResolution( width, height );

        if( width != 0 && height != 0 )
        {
            if( !mIsExternal )
            {
                const uint32_t values[] = { width, height };
                xcb_configure_window( mConnection, mXcbWindow,
                                      XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values );
            }
            else
            {
                setFinalResolution( width, height );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void VulkanXcbWindow::windowMovedOrResized()
    {
        if( mClosed || !mXcbWindow )
            return;

        xcb_get_geometry_cookie_t geomCookie = xcb_get_geometry( mConnection, mXcbWindow );
        xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply( mConnection, geomCookie, NULL );

        if( mIsTopLevel && !mFullscreenMode )
        {
            mLeft = geom->x;
            mTop = geom->y;
        }

        auto newWidth = geom->width;
        auto newHeight = geom->height;

        free( geom );

        if( newWidth == getWidth() && newHeight == getHeight() && !mRebuildingSwapchain )
            return;

        mDevice->stall();

        destroySwapchain();
        setFinalResolution( newWidth, newHeight );
        createSwapchain();
    }
    //-------------------------------------------------------------------------
    void VulkanXcbWindow::_setVisible( bool visible ) { mVisible = visible; }
    //-------------------------------------------------------------------------
    bool VulkanXcbWindow::isVisible() const { return mVisible; }
    //-------------------------------------------------------------------------
    void VulkanXcbWindow::setHidden( bool hidden )
    {
        mHidden = hidden;

        // ignore for external windows as these should handle
        // this externally
        if( mIsExternal )
            return;

        if( !hidden )
            xcb_map_window( mConnection, mXcbWindow );
        else
            xcb_unmap_window( mConnection, mXcbWindow );
        xcb_flush( mConnection );
    }
    //-------------------------------------------------------------------------
    bool VulkanXcbWindow::isHidden() const { return mHidden; }
    //-------------------------------------------------------------------------
    void VulkanXcbWindow::getCustomAttribute( IdString name, void *pData )
    {
        if( name == "xcb_connection_t" )
        {
            *static_cast<xcb_connection_t **>( pData ) = mConnection;
            return;
        }
        else if( name == "mWmProtocols" )
        {
            *static_cast<xcb_atom_t *>( pData ) = mWmProtocols;
            return;
        }
        else if( name == "mWmDeleteWindow" )
        {
            *static_cast<xcb_atom_t *>( pData ) = mWmDeleteWindow;
            return;
        }
        else if( name == "xcb_window_t" || name == "RENDERDOC_WINDOW" )
        {
            *static_cast<xcb_window_t *>( pData ) = mXcbWindow;
            return;
        }
        else
        {
            VulkanWindowSwapChainBased::getCustomAttribute( name, pData );
        }
    }
}  // namespace Ogre
