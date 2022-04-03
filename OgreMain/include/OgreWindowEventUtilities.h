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
#ifndef __OgreWindowEventUtils_H__
#define __OgreWindowEventUtils_H__

#include "OgrePrerequisites.h"

#include "OgreCommon.h"
#include "OgrePlatform.h"

#include "ogrestd/map.h"

#include "OgreHeaderPrefix.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#    if !defined( WIN32_LEAN_AND_MEAN )
#        define WIN32_LEAN_AND_MEAN
#    endif
#    if !defined( NOMINMAX ) && defined( _MSC_VER )
#        define NOMINMAX  // required to stop windows.h messing up std::min
#    endif
#    include <windows.h>
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE && !defined( __LP64__ )
#    include <Carbon/Carbon.h>
#endif

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup RenderSystem
     *  @{
     */
    /**
    @remarks
        Callback class used to send out window events to client app
    */
    class _OgreExport WindowEventListener
    {
    public:
        virtual ~WindowEventListener() {}

        /**
        @remarks
            Window has moved position
        @param rw
            The Window which created this events
        */
        virtual void windowMoved( Window *rw ) { (void)rw; }

        /**
        @remarks
            Window has resized
        @param rw
            The Window which created this events
        */
        virtual void windowResized( Window *rw ) { (void)rw; }

        /**
        @remarks
            Window is closing (Only triggered if user pressed the [X] button)
        @param rw
            The Window which created this events
        @return True will close the window(default).
        */
        virtual bool windowClosing( Window *rw )
        {
            (void)rw;
            return true;
        }

        /**
        @remarks
            Window has been closed (Only triggered if user pressed the [X] button)
        @param rw
            The Window which created this events
        @note
            The window has not actually close yet when this event triggers. It's only closed after
            all windowClosed events are triggered. This allows apps to deinitialise properly if they
            have services that needs the window to exist when deinitialising.
        */
        virtual void windowClosed( Window *rw ) { (void)rw; }

        /**
        @remarks
            Window has lost/gained focus
        @param rw
            The Window which created this events
        */
        virtual void windowFocusChange( Window *rw ) { (void)rw; }
    };

    /**
    @remarks
        Utility class to handle Window Events/Pumping/Messages
    */
    class _OgreExport WindowEventUtilities
    {
    public:
        /**
        @remarks
            Call this once per frame if not using Root:startRendering(). This will update all registered
            RenderWindows (If using external Windows, you can optionally register those yourself)
        */
        static void messagePump();

        /**
        @remarks
            Add a listener to listen to renderwindow events (multiple listener's per renderwindow is
        fine) The same listener can listen to multiple windows, as the Window Pointer is sent along with
            any messages.
        @param window
            The RenderWindow you are interested in monitoring
        @param listener
            Your callback listener
        */
        static void addWindowEventListener( Window *window, WindowEventListener *listener );

        /**
        @remarks
            Remove previously added listener
        @param window
            The RenderWindow you registered with
        @param listener
            The listener registered
        */
        static void removeWindowEventListener( Window *window, WindowEventListener *listener );

        /**
        @remarks
            Called by RenderWindows upon creation for Ogre generated windows. You are free to add your
            external windows here too if needed.
        @param window
            The RenderWindow to monitor
        */
        static void _addRenderWindow( Window *window );

        /**
        @remarks
            Called by RenderWindows upon creation for Ogre generated windows. You are free to add your
            external windows here too if needed.
        @param window
            The RenderWindow to remove from list
        */
        static void _removeRenderWindow( Window *window );

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        //! Internal winProc (RenderWindow's use this when creating the Win32 Window)
        static LRESULT CALLBACK _WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE && !defined __OBJC__ && !defined( __LP64__ )
        //! Internal UPP Window Handler (RenderWindow's use this when creating the OS X Carbon Window
        static OSStatus _CarbonWindowHandler( EventHandlerCallRef nextHandler, EventRef event,
                                              void *wnd );
#endif

        // These are public only so GLXProc can access them without adding Xlib headers header
        typedef multimap<Window *, WindowEventListener *>::type WindowEventListeners;

        static WindowEventListeners _msListeners;
        static WindowList           _msWindows;
    };
    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
