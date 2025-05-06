#include "Core/wxOgreRenderWindow.h"

#include "Core/wxOgreRenderWindowListener.h"

#include "OgreWindow.h"

#include <sstream>

#ifdef __WXGTK__
#    include <GL/glx.h>
#    include <gdk/gdk.h>
#    include <gdk/gdkx.h>
#    include <gtk/gtk.h>
#endif

IMPLEMENT_CLASS( wxOgreRenderWindow, wxControl )

BEGIN_EVENT_TABLE( wxOgreRenderWindow, wxControl )
#ifndef __WXMSW__
EVT_PAINT( wxOgreRenderWindow::OnPaint )
#endif
EVT_SIZE( wxOgreRenderWindow::OnSize )
EVT_MOUSE_EVENTS( wxOgreRenderWindow::OnMouseEvents )
EVT_KEY_DOWN( wxOgreRenderWindow::OnKeyDown )
EVT_KEY_UP( wxOgreRenderWindow::OnKeyUp )
END_EVENT_TABLE()

Ogre::Root *wxOgreRenderWindow::msOgreRoot = 0;
//------------------------------------------------------------------------------
unsigned int wxOgreRenderWindow::msNextRenderWindowId = 1;
//------------------------------------------------------------------------------
wxOgreRenderWindow::wxOgreRenderWindow( wxWindow *parent, wxWindowID id, const wxPoint &pos,
                                        const wxSize &size, long style, const wxValidator &validator )
{
    Init();
    Create( parent, id, pos, size, style, validator );
}
//------------------------------------------------------------------------------
wxOgreRenderWindow::wxOgreRenderWindow()
{
    Init();
}
//------------------------------------------------------------------------------
bool wxOgreRenderWindow::Create( wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size,
                                 long style, const wxValidator &validator )
{
    // Error creating the base class
    if( !wxControl::Create( parent, id, pos, size, style, validator ) )
        return false;

    // Use the told size or let the sizers pick one.
    SetInitialSize( size );

    SetBackgroundStyle( wxBG_STYLE_CUSTOM );

    if( msOgreRoot )
        CreateRenderWindow();

    return true;
}
//------------------------------------------------------------------------------
wxOgreRenderWindow::~wxOgreRenderWindow()
{
    /*if (mRenderWindow && msOgreRoot)
        msOgreRoot->detachRenderTarget (mRenderWindow);*/

    mRenderWindow = 0;
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::Init()
{
    mRenderWindow = 0;

    // Callbacks
    mRenderWindowListener = 0;
}
//------------------------------------------------------------------------------
inline wxSize wxOgreRenderWindow::DoGetBestSize() const
{
    // return wxSize( -1, -1 );
    return wxSize( 512, 512 );
}
//------------------------------------------------------------------------------
Ogre::Root *wxOgreRenderWindow::GetOgreRoot()
{
    return msOgreRoot;
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::SetOgreRoot( Ogre::Root *root )
{
    msOgreRoot = root;
}
//------------------------------------------------------------------------------
Ogre::Window *wxOgreRenderWindow::GetRenderWindow() const
{
    return mRenderWindow;
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::Update()
{
    // Update will be triggered when creating the Render Window. Do not
    // try to render in such case otherwise bad things can happen.
    if( msOgreRoot && mRenderWindow )
    {
        msOgreRoot->renderOneFrame();
    }
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::setRenderWindowListener( wxOgreRenderWindowListener *listener )
{
    mRenderWindowListener = listener;
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::OnPaint( wxPaintEvent &evt )
{
    Update();
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::OnInternalIdle()
{
    wxControl::OnInternalIdle();
    if( wxUpdateUIEvent::CanUpdate( this ) && IsShown() )
    {
        Refresh();
        Update();
    }
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::OnSize( wxSizeEvent &evt )
{
    if( mRenderWindow )
    {
        // Setting new size;
        int width;
        int height;
        wxSize size = evt.GetSize();
        width = size.GetWidth();
        height = size.GetHeight();

        mRenderWindow->requestResolution( width, height );
        // Letting Ogre know the window has been resized;
        mRenderWindow->windowMovedOrResized();

#if OGRE_VERSION_MAJOR < 2
        Ogre::Viewport *vp = mRenderWindow->getViewport( 0 );
        if( vp )
        {
            vp->getCamera()->setAspectRatio( Ogre::Real( vp->getActualWidth() ) /
                                             Ogre::Real( vp->getActualHeight() ) );
        }
#endif
    }

    Update();
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::OnMouseEvents( wxMouseEvent &evt )
{
    if( mRenderWindowListener )
        mRenderWindowListener->OnMouseEvents( evt );
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::OnKeyDown( wxKeyEvent &evt )
{
    if( mRenderWindowListener )
        mRenderWindowListener->OnKeyDown( evt );
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::OnKeyUp( wxKeyEvent &evt )
{
    if( mRenderWindowListener )
        mRenderWindowListener->OnKeyUp( evt );
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::CreateRenderWindow()
{
    Ogre::NameValuePairList params;
    params["externalWindowHandle"] = GetOgreHandle();
    params["parentWindowHandle"] = GetOgreHandle();
#if defined( __WXOSX__ )
    params["macAPI"] = "cocoa";
    params["macAPICocoaUseNSView"] = "true";
#endif
    // params["useNVPerfHUD"] = "Yes";

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    // Needed by Vulkan.
    struct SDLx11
    {
        Display *display;  // < The X11 display.
        ::Window window;   // < The X11 window.
    };

    GtkWidget *widget = GetHandle();
    gtk_widget_realize( widget );

    GdkWindow *gdkWindow = gtk_widget_get_window( widget );

    SDLx11 sdlHandles;
    sdlHandles.display = GDK_WINDOW_XDISPLAY( gdkWindow );
    sdlHandles.window = gdk_x11_window_get_xid( gdkWindow );

    params.insert(
        std::make_pair( "SDL2x11", Ogre::StringConverter::toString( (uintptr_t)&sdlHandles ) ) );
#endif

    // Enforce vsync to avoid hogging the CPU unnecessarily
    params["vsync"] = "true";
#if OGRE_VERSION_MAJOR >= 2
    params["gamma"] = "true";
#endif

    // Get wx control window size
    int width;
    int height;
    GetSize( &width, &height );
    // Create the render window
    mRenderWindow = msOgreRoot->createRenderWindow(
        Ogre::String( "OgreRenderWindow" ) + Ogre::StringConverter::toString( msNextRenderWindowId++ ),
        width, height, false, &params );

    mRenderWindow->setFocused( true );
    mRenderWindow->setHidden( false );
}
//------------------------------------------------------------------------------
Ogre::String wxOgreRenderWindow::GetOgreHandle() const
{
    Ogre::String handle;

#ifdef __WXMSW__
    // Handle for Windows systems
    handle = Ogre::StringConverter::toString( (size_t)( (HWND)GetHandle() ) );
#elif defined( __WXGTK__ )
    // Handle for GTK-based systems

    GtkWidget *widget = GetHandle();
    gtk_widget_set_double_buffered( widget, FALSE );
    gtk_widget_realize( widget );

    // Window wid = gdk_x11_drawable_get_xid( gtk_widget_get_window( widget ) );
    Window wid = gdk_x11_window_get_xid( gtk_widget_get_window( widget ) );

    std::stringstream str;
    str << wid;

    handle = str.str();
#elif __WXOSX_COCOA__
    handle = Ogre::StringConverter::toString( (size_t)( (NSView *)GetHandle() ) );
#else
    // Any other unsupported system
#    error Not supported on this platform.
#endif

    return handle;
}
