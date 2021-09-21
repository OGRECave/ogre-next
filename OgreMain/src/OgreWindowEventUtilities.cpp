/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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
#include "OgreStableHeaders.h"
#include "OgreCommon.h"
#include "OgreWindowEventUtilities.h"
#include "OgreLogManager.h"
#include "OgreWindow.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX || OGRE_PLATFORM == OGRE_PLATFORM_FREEBSD
#include <xcb/xcb.h>
#include <X11/Xlib.h>
static void GLXProc( Ogre::Window *win, const XEvent &event );
static void XcbProc( xcb_connection_t *xcbConnection, xcb_generic_event_t *event );

typedef Ogre::map<xcb_window_t, Ogre::Window*>::type XcbWindowMap;
static XcbWindowMap gXcbWindowToOgre;
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#   include "OSX/macUtils.h"
#endif


//using namespace Ogre;

Ogre::WindowEventUtilities::WindowEventListeners Ogre::WindowEventUtilities::_msListeners;
Ogre::WindowList Ogre::WindowEventUtilities::_msWindows;

namespace Ogre {
//--------------------------------------------------------------------------------//
void WindowEventUtilities::messagePump()
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    // Windows Message Loop (NULL means check all HWNDs belonging to this context)
    MSG  msg;
    while( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX || OGRE_PLATFORM == OGRE_PLATFORM_FREEBSD
#    ifndef OGRE_CONFIG_UNIX_NO_X11
    //GLX Message Pump

    xcb_connection_t *xcbConnection = 0;

    if( !_msWindows.empty() )
        _msWindows.front()->getCustomAttribute( "xcb_connection_t", &xcbConnection );

    if( !xcbConnection )
    {
        // Uses the older Xlib
        WindowList::iterator win = _msWindows.begin();
        WindowList::iterator end = _msWindows.end();

        Display *xDisplay = 0;  // same for all windows

        for( ; win != end; win++ )
        {
            XID xid;
            XEvent event;

            if( !xDisplay )
                ( *win )->getCustomAttribute( "XDISPLAY", &xDisplay );

        (*win)->getCustomAttribute("WINDOW", &xid);

            while( XCheckWindowEvent(
                       xDisplay, xid, StructureNotifyMask | VisibilityChangeMask | FocusChangeMask, &event ) )
            {
                GLXProc( *win, event );
            }

            // The ClientMessage event does not appear under any Event Mask
            while( XCheckTypedWindowEvent( xDisplay, xid, ClientMessage, &event ) )
            {
                GLXProc( *win, event );
            }
        }
    }
    else
    {
        // Uses the newer xcb
        xcb_generic_event_t *nextEvent = 0;

        nextEvent = xcb_poll_for_event( xcbConnection );

        while( nextEvent )
        {
            XcbProc( xcbConnection, nextEvent );
            free( nextEvent );
            nextEvent = xcb_poll_for_event( xcbConnection );
        }
    }
#    endif
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE && !defined __OBJC__ && !defined __LP64__
    // OSX Message Pump
    EventRef event = NULL;
    EventTargetRef targetWindow;
    targetWindow = GetEventDispatcherTarget();
    
    // If we are unable to get the target then we no longer care about events.
    if( !targetWindow ) return;
    
    // Grab the next event, process it if it is a window event
    while( ReceiveNextEvent( 0, NULL, kEventDurationNoWait, true, &event ) == noErr )
    {
        // Dispatch the event
        SendEventToEventTarget( event, targetWindow );
        ReleaseEvent( event );
    }
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	mac_dispatchOneEvent();
#endif
}

//--------------------------------------------------------------------------------//
void WindowEventUtilities::addWindowEventListener( Window *window, WindowEventListener* listener )
{
    _msListeners.insert(std::make_pair(window, listener));
}

//--------------------------------------------------------------------------------//
void WindowEventUtilities::removeWindowEventListener( Window* window, WindowEventListener* listener )
{
    WindowEventListeners::iterator i = _msListeners.begin(), e = _msListeners.end();

    for( ; i != e; ++i )
    {
        if( i->first == window && i->second == listener )
        {
            _msListeners.erase(i);
            break;
        }
    }
}

//--------------------------------------------------------------------------------//
void WindowEventUtilities::_addRenderWindow( Window *window )
{
    _msWindows.push_back(window);

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    xcb_window_t windowHandle = 0;
    window->getCustomAttribute( "xcb_window_t", &windowHandle );
    if( windowHandle )
        gXcbWindowToOgre[windowHandle] = window;
#endif
}

//--------------------------------------------------------------------------------//
void WindowEventUtilities::_removeRenderWindow( Window *window )
{
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    xcb_window_t windowHandle = 0;
    window->getCustomAttribute( "xcb_window_t", &windowHandle );
    if( windowHandle )
        gXcbWindowToOgre.erase( windowHandle );
#endif

    WindowList::iterator i = std::find( _msWindows.begin(), _msWindows.end(), window );
    if( i != _msWindows.end() )
        _msWindows.erase( i );
}

}
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
//--------------------------------------------------------------------------------//
namespace Ogre {
LRESULT CALLBACK WindowEventUtilities::_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CREATE)
    {   // Store pointer to Win32Window in user data area
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)(((LPCREATESTRUCT)lParam)->lpCreateParams));
        return 0;
    }

    // look up window instance
    // note: it is possible to get a WM_SIZE before WM_CREATE
    Window *win = (Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (!win)
        return DefWindowProc(hWnd, uMsg, wParam, lParam);

    //LogManager* log = LogManager::getSingletonPtr();
    //Iterator of all listeners registered to this Window
    WindowEventListeners::iterator index,
        start = _msListeners.lower_bound(win),
        end = _msListeners.upper_bound(win);

    switch( uMsg )
    {
    case WM_ACTIVATE:
    {
        bool active = (LOWORD(wParam) != WA_INACTIVE);
        win->setFocused( active );

        for( ; start != end; ++start )
            (start->second)->windowFocusChange(win);
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint( hWnd, &ps );
        win->_setVisible( !IsRectEmpty( &ps.rcPaint ) );
        EndPaint( hWnd, &ps );
        break;
    }
    case WM_SYSKEYDOWN:
        switch( wParam )
        {
        case VK_CONTROL:
        case VK_SHIFT:
        case VK_MENU: //ALT
            //return zero to bypass defProc and signal we processed the message
            return 0;
        }
        break;
    case WM_SYSKEYUP:
        switch( wParam )
        {
        case VK_CONTROL:
        case VK_SHIFT:
        case VK_MENU: //ALT
        case VK_F10:
            //return zero to bypass defProc and signal we processed the message
            return 0;
        }
        break;
    case WM_SYSCHAR:
        // return zero to bypass defProc and signal we processed the message, unless it's an ALT-space
        if (wParam != VK_SPACE)
            return 0;
        break;
    case WM_ENTERSIZEMOVE:
        //log->logMessage("WM_ENTERSIZEMOVE");
        break;
    case WM_EXITSIZEMOVE:
        //log->logMessage("WM_EXITSIZEMOVE");
        break;
    case WM_MOVE:
        //log->logMessage("WM_MOVE");
        win->windowMovedOrResized();
        for(index = start; index != end; ++index)
            (index->second)->windowMoved(win);
        break;
    case WM_DISPLAYCHANGE:
        win->windowMovedOrResized();
        for(index = start; index != end; ++index)
            (index->second)->windowResized(win);
        break;
    case WM_SIZE:
        //log->logMessage("WM_SIZE");
        win->windowMovedOrResized();
        for(index = start; index != end; ++index)
            (index->second)->windowResized(win);
        break;
    case WM_GETMINMAXINFO:
        // Prevent the window from going smaller than some minimu size
        ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 100;
        ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 100;
        break;
    case WM_CLOSE:
    {
        //log->logMessage("WM_CLOSE");
        bool close = true;
        for(index = start; index != end; ++index)
        {
            if (!(index->second)->windowClosing(win))
                close = false;
        }
        if (!close) return 0;

        for(index = _msListeners.lower_bound(win); index != end; ++index)
            (index->second)->windowClosed(win);
        win->destroy();
        return 0;
    }
    }

    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}
}
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX || OGRE_PLATFORM == OGRE_PLATFORM_FREEBSD
//--------------------------------------------------------------------------------//
void GLXProc( Ogre::Window *win, const XEvent &event )
{
    //An iterator for the window listeners
  Ogre::WindowEventUtilities::WindowEventListeners::iterator index,
        start = Ogre::WindowEventUtilities::_msListeners.lower_bound(win),
        end   = Ogre::WindowEventUtilities::_msListeners.upper_bound(win);

    switch(event.type)
    {
    case ClientMessage:
    {
        ::Atom atom;
        win->getCustomAttribute("ATOM", &atom);
        if(event.xclient.format == 32 && event.xclient.data.l[0] == (long)atom)
        {   //Window closed by window manager
            //Send message first, to allow app chance to unregister things that need done before
            //window is shutdown
            bool close = true;
            for(index = start ; index != end; ++index)
            {
                if (!(index->second)->windowClosing(win))
                    close = false;
            }
            if (!close) return;

            for(index = start ; index != end; ++index)
                (index->second)->windowClosed(win);
            win->destroy();
        }
        break;
    }
    case DestroyNotify:
    {
        if (!win->isClosed())
        {
            // Window closed without window manager warning.
            for(index = start ; index != end; ++index)
                (index->second)->windowClosed(win);
            win->destroy();
        }
        break;
    }
    case ConfigureNotify:
    {    
        // This could be slightly more efficient if windowMovedOrResized took arguments:
        Ogre::uint32 oldWidth, oldHeight;
        Ogre::int32 oldLeft, oldTop;
        win->getMetrics(oldWidth, oldHeight, oldLeft, oldTop);
        win->windowMovedOrResized();

        Ogre::uint32 newWidth, newHeight;
        Ogre::int32 newLeft, newTop;
        win->getMetrics(newWidth, newHeight, newLeft, newTop);

        if( newLeft != oldLeft || newTop != oldTop )
        {
            for( index = start; index != end; ++index )
                (index->second)->windowMoved( win );
        }

        if( newWidth != oldWidth || newHeight != oldHeight )
        {
            for( index = start; index != end; ++index )
                (index->second)->windowResized( win );
        }
        break;
    }
    case FocusIn:     // Gained keyboard focus
    case FocusOut:    // Lost keyboard focus
        win->setFocused( event.type == FocusIn );
        for(index = start ; index != end; ++index)
            (index->second)->windowFocusChange(win);
        break;
    case MapNotify:   //Restored
        win->setFocused( true );
        for(index = start ; index != end; ++index)
            (index->second)->windowFocusChange(win);
        break;
    case UnmapNotify: //Minimised
        win->setFocused( false );
        win->_setVisible( false );
        for(index = start ; index != end; ++index)
            (index->second)->windowFocusChange(win);
        break;
    case VisibilityNotify:
        switch(event.xvisibility.state)
        {
        case VisibilityUnobscured:
            win->setFocused( true );
            win->_setVisible( true );
            break;
        case VisibilityPartiallyObscured:
            win->setFocused( true );
            win->_setVisible( true );
            break;
        case VisibilityFullyObscured:
            win->setFocused( false );
            win->_setVisible( false );
            break;
        }
        for(index = start ; index != end; ++index)
            (index->second)->windowFocusChange(win);
        break;
    default:
        break;
    } //End switch event.type
}
//--------------------------------------------------------------------------------//
void XcbProc( xcb_connection_t *xcbConnection, xcb_generic_event_t *e )
{
    XcbWindowMap::const_iterator itWindow;
    Ogre::WindowEventUtilities::WindowEventListeners::iterator index, start, end;

    const Ogre::uint8 responseType = e->response_type & ~0x80;
    switch( responseType )
    {
    case XCB_CLIENT_MESSAGE:
    {
        xcb_client_message_event_t *event = reinterpret_cast<xcb_client_message_event_t *>( e );
        itWindow = gXcbWindowToOgre.find( event->window );
        if( itWindow != gXcbWindowToOgre.end() )
        {
            Ogre::Window *win = itWindow->second;
            if( event->format == 32u )
            {
                xcb_atom_t wmProtocols;
                xcb_atom_t wmDeleteWindow;
                win->getCustomAttribute( "mWmProtocols", &wmProtocols );
                win->getCustomAttribute( "mWmDeleteWindow", &wmDeleteWindow );

                if( event->type == wmProtocols && event->data.data32[0] == wmDeleteWindow )
                {
                    start = Ogre::WindowEventUtilities::_msListeners.lower_bound( win );
                    end = Ogre::WindowEventUtilities::_msListeners.upper_bound( win );

                    // Window closed by window manager
                    // Send message first, to allow app chance to unregister things that need done before
                    // window is shutdown
                    bool close = true;
                    for( index = start; index != end; ++index )
                    {
                        if( !( index->second )->windowClosing( win ) )
                            close = false;
                    }
                    if( !close )
                        return;

                    for( index = start; index != end; ++index )
                        ( index->second )->windowClosed( win );
                    win->destroy();
                }
            }
        }
    }
    break;
    case XCB_FOCUS_IN:   // Gained keyboard focus
    case XCB_FOCUS_OUT:  // Lost keyboard focus
    {
        xcb_focus_in_event_t *event = reinterpret_cast<xcb_focus_in_event_t *>( e );
        itWindow = gXcbWindowToOgre.find( event->event );
        if( itWindow != gXcbWindowToOgre.end() )
        {
            Ogre::Window *win = itWindow->second;
            win->setFocused( responseType == XCB_FOCUS_IN );

            start = Ogre::WindowEventUtilities::_msListeners.lower_bound( win );
            end = Ogre::WindowEventUtilities::_msListeners.upper_bound( win );
            for( index = start; index != end; ++index )
                ( index->second )->windowFocusChange( win );
        }
    }
    break;
    case XCB_MAP_NOTIFY:  // Restored
    {
        xcb_map_notify_event_t *event = reinterpret_cast<xcb_map_notify_event_t *>( e );
        itWindow = gXcbWindowToOgre.find( event->window );
        if( itWindow != gXcbWindowToOgre.end() )
        {
            Ogre::Window *win = itWindow->second;
            win->setFocused( true );

            start = Ogre::WindowEventUtilities::_msListeners.lower_bound( win );
            end = Ogre::WindowEventUtilities::_msListeners.upper_bound( win );
            for( index = start; index != end; ++index )
                ( index->second )->windowFocusChange( win );
        }
    }
    break;
    case XCB_UNMAP_NOTIFY:  // Minimized
    {
        xcb_unmap_notify_event_t *event = reinterpret_cast<xcb_unmap_notify_event_t *>( e );
        itWindow = gXcbWindowToOgre.find( event->window );
        if( itWindow != gXcbWindowToOgre.end() )
        {
            Ogre::Window *win = itWindow->second;
            win->setFocused( false );
            win->_setVisible( false );

            start = Ogre::WindowEventUtilities::_msListeners.lower_bound( win );
            end = Ogre::WindowEventUtilities::_msListeners.upper_bound( win );
            for( index = start; index != end; ++index )
                ( index->second )->windowFocusChange( win );
        }
    }
    break;
    case XCB_VISIBILITY_NOTIFY:
    {
        xcb_visibility_notify_event_t *event = reinterpret_cast<xcb_visibility_notify_event_t *>( e );
        itWindow = gXcbWindowToOgre.find( event->window );
        if( itWindow != gXcbWindowToOgre.end() )
        {
            Ogre::Window *win = itWindow->second;
            xcb_visibility_t visibility = static_cast<xcb_visibility_t>( event->state );
            switch( visibility )
            {
            case XCB_VISIBILITY_UNOBSCURED:
            case XCB_VISIBILITY_PARTIALLY_OBSCURED:
                win->setFocused( true );
                win->_setVisible( true );
                break;
            case XCB_VISIBILITY_FULLY_OBSCURED:
                win->setFocused( false );
                win->_setVisible( false );
                break;
            }

            start = Ogre::WindowEventUtilities::_msListeners.lower_bound( win );
            end = Ogre::WindowEventUtilities::_msListeners.upper_bound( win );
            for( index = start; index != end; ++index )
                ( index->second )->windowFocusChange( win );
        }
    }
    break;
    case XCB_CONFIGURE_NOTIFY:
    {
        xcb_configure_notify_event_t *event = reinterpret_cast<xcb_configure_notify_event_t *>( e );

        itWindow = gXcbWindowToOgre.find( event->window );
        if( itWindow != gXcbWindowToOgre.end() )
        {
            Ogre::Window *win = itWindow->second;

            // This could be slightly more efficient if windowMovedOrResized took arguments:
            Ogre::uint32 oldWidth, oldHeight;
            Ogre::int32 oldLeft, oldTop;
            win->getMetrics( oldWidth, oldHeight, oldLeft, oldTop );
            win->windowMovedOrResized();

            Ogre::uint32 newWidth, newHeight;
            Ogre::int32 newLeft, newTop;
            win->getMetrics( newWidth, newHeight, newLeft, newTop );

            start = Ogre::WindowEventUtilities::_msListeners.lower_bound( win );
            end = Ogre::WindowEventUtilities::_msListeners.upper_bound( win );

            if( newLeft != oldLeft || newTop != oldTop )
            {
                for( index = start; index != end; ++index )
                    ( index->second )->windowMoved( win );
            }

            if( newWidth != oldWidth || newHeight != oldHeight )
            {
                for( index = start; index != end; ++index )
                    ( index->second )->windowResized( win );
            }
        }
    }
    break;
    case XCB_DESTROY_NOTIFY:
    {
        xcb_visibility_notify_event_t *event = reinterpret_cast<xcb_visibility_notify_event_t *>( e );
        itWindow = gXcbWindowToOgre.find( event->window );
        if( itWindow != gXcbWindowToOgre.end() )
        {
            Ogre::Window *win = itWindow->second;
            if( !win->isClosed() )
            {
                start = Ogre::WindowEventUtilities::_msListeners.lower_bound( win );
                end = Ogre::WindowEventUtilities::_msListeners.upper_bound( win );
                for( index = start; index != end; ++index )
                    ( index->second )->windowClosed( win );
                win->destroy();
            }
        }
    }
    break;
    }
}
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE && !defined __OBJC__ && !defined __LP64__
//--------------------------------------------------------------------------------//
namespace Ogre {
OSStatus WindowEventUtilities::_CarbonWindowHandler(EventHandlerCallRef nextHandler, EventRef event, void* wnd)
{
    OSStatus status = noErr;

    // Only events from our window should make it here
    // This ensures that our user data is our WindowRef
    Window* curWindow = (Window*)wnd;
    if(!curWindow) return eventNotHandledErr;
    
    //Iterator of all listeners registered to this Window
    WindowEventListeners::iterator index,
        start = _msListeners.lower_bound(curWindow),
        end = _msListeners.upper_bound(curWindow);
    
    // We only get called if a window event happens
    UInt32 eventKind = GetEventKind( event );

    switch( eventKind )
    {
        case kEventWindowActivated:
            curWindow->setFocused( true );
            for( ; start != end; ++start )
                (start->second)->windowFocusChange(curWindow);
            break;
        case kEventWindowDeactivated:
            curWindow->setFocused( false );

            for( ; start != end; ++start )
                (start->second)->windowFocusChange(curWindow);

            break;
        case kEventWindowShown:
        case kEventWindowExpanded:
            curWindow->setFocused( true );
            curWindow->setVisible( true );
            for( ; start != end; ++start )
                (start->second)->windowFocusChange(curWindow);
            break;
        case kEventWindowHidden:
        case kEventWindowCollapsed:
            curWindow->setFocused( false );
            curWindow->setVisible( false );
            for( ; start != end; ++start )
                (start->second)->windowFocusChange(curWindow);
            break;            
        case kEventWindowDragCompleted:
            curWindow->windowMovedOrResized();
            for( ; start != end; ++start )
                (start->second)->windowMoved(curWindow);
            break;
        case kEventWindowBoundsChanged:
            curWindow->windowMovedOrResized();
            for( ; start != end; ++start )
                (start->second)->windowResized(curWindow);
            break;
        case kEventWindowClose:
        {
            bool close = true;
            for( ; start != end; ++start )
            {
                if (!(start->second)->windowClosing(curWindow))
                    close = false;
            }
            if (close)
                // This will cause event handling to continue on to the standard handler, which calls
                // DisposeWindow(), which leads to the 'kEventWindowClosed' event
                status = eventNotHandledErr;
            break;
        }
        case kEventWindowClosed:
            curWindow->destroy();
            for( ; start != end; ++start )
                (start->second)->windowClosed(curWindow);
            break;
        default:
            status = eventNotHandledErr;
            break;
    }
    
    return status;
}
}
#endif
