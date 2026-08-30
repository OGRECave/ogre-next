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

/** Standalone smoke test for the "Wayland EGL Window" GL3PlusSupport
    interface (see WaylandEglWindow). It plays the role of the HOST
    application (e.g. Qt/QML, as Gazebo would use) - it owns the wl_display
    connection, binds wl_compositor/xdg_wm_base, creates its own wl_surface
    and xdg_toplevel, and is solely responsible for dispatching the
    wl_display's event loop - then hands that surface to Ogre via the
    "externalWaylandDisplay"/"externalWaylandSurface" createRenderWindow
    miscParams, exactly as an embedding host is expected to.

    Requires a running Wayland compositor (WAYLAND_DISPLAY set). Not run as
    part of normal builds; only built when OGRE_BUILD_WAYLAND_EGL_SMOKE_TEST
    is explicitly enabled, and must be run manually (or in an explicitly
    opted-in CI job) against a compositor, e.g.:
        weston --backend=headless-backend.so &
        WAYLAND_DISPLAY=<socket> ./Test_GL3PlusWaylandEglSmoke
*/

#include "OgreAbiUtils.h"
#include "OgreGL3PlusContext.h"
#include "OgreGL3PlusPlugin.h"
#include "OgreLogManager.h"
#include "OgreRenderSystem.h"
#include "OgreRoot.h"
#include "OgreStringConverter.h"
#include "OgreWindow.h"

#include <wayland-client.h>

#include "xdg-shell-client-protocol.h"

#include <EGL/egl.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Deliberately not #include <GL/gl.h>: OgreGL3PlusContext.h transitively
// pulls in Ogre's gl3w loader (GL/gl3w.h), whose inline GL entry points
// forward through a `gl3wProcs` global that isn't exported outside
// RenderSystem_GL3Plus - linking against it from here fails. This test only
// needs a handful of core GL 1.x entry points, so they're loaded directly
// via eglGetProcAddress instead, exactly like WaylandEglSupport itself does
// for the real render system's function pointers.
namespace GL
{
    typedef unsigned int  Enum;
    typedef unsigned int  Bitfield;
    typedef float         Float;
    typedef int           Int;
    typedef int           Sizei;
    typedef unsigned char Ubyte;

    typedef void ( *ClearColorFn )( Float, Float, Float, Float );
    typedef void ( *ClearFn )( Bitfield );
    typedef void ( *ViewportFn )( Int, Int, Sizei, Sizei );
    typedef void ( *ReadPixelsFn )( Int, Int, Sizei, Sizei, Enum, Enum, void * );
    typedef void ( *FinishFn )();

    const Bitfield COLOR_BUFFER_BIT = 0x00004000;
    const Enum     RGBA = 0x1908;
    const Enum     UNSIGNED_BYTE = 0x1401;

    ClearColorFn ClearColor = nullptr;
    ClearFn      Clear = nullptr;
    ViewportFn   Viewport = nullptr;
    ReadPixelsFn ReadPixels = nullptr;
    FinishFn     Finish = nullptr;

    bool load()
    {
        ClearColor = reinterpret_cast<ClearColorFn>( eglGetProcAddress( "glClearColor" ) );
        Clear = reinterpret_cast<ClearFn>( eglGetProcAddress( "glClear" ) );
        Viewport = reinterpret_cast<ViewportFn>( eglGetProcAddress( "glViewport" ) );
        ReadPixels = reinterpret_cast<ReadPixelsFn>( eglGetProcAddress( "glReadPixels" ) );
        Finish = reinterpret_cast<FinishFn>( eglGetProcAddress( "glFinish" ) );
        return ClearColor && Clear && Viewport && ReadPixels && Finish;
    }
}  // namespace GL

namespace
{
    struct HostState
    {
        wl_display    *display = nullptr;
        wl_registry   *registry = nullptr;
        wl_compositor *compositor = nullptr;
        xdg_wm_base   *wmBase = nullptr;

        wl_surface   *surface = nullptr;
        xdg_surface  *xdgSurface = nullptr;
        xdg_toplevel *toplevel = nullptr;

        bool configured = false;
        // Set by the xdg_toplevel::configure listener when the compositor
        // requests a new size (0 means "no preference / keep current").
        int32_t pendingWidth = 0;
        int32_t pendingHeight = 0;
        bool    resizeConfigured = false;
    };

    void registryGlobal( void *data, wl_registry *registry, uint32_t name, const char *interface,
                          uint32_t version )
    {
        HostState *state = static_cast<HostState *>( data );
        if( strcmp( interface, "wl_compositor" ) == 0 )
        {
            state->compositor = static_cast<wl_compositor *>(
                wl_registry_bind( registry, name, &wl_compositor_interface, 4 ) );
        }
        else if( strcmp( interface, "xdg_wm_base" ) == 0 )
        {
            state->wmBase = static_cast<xdg_wm_base *>(
                wl_registry_bind( registry, name, &xdg_wm_base_interface, 1 ) );
        }
    }
    void registryGlobalRemove( void *, wl_registry *, uint32_t ) {}
    const wl_registry_listener gRegistryListener = { registryGlobal, registryGlobalRemove };

    void wmBasePing( void *, xdg_wm_base *wmBase, uint32_t serial ) { xdg_wm_base_pong( wmBase, serial ); }
    const xdg_wm_base_listener gWmBaseListener = { wmBasePing };

    void toplevelConfigure( void *data, xdg_toplevel *, int32_t width, int32_t height, wl_array * )
    {
        HostState *state = static_cast<HostState *>( data );
        state->pendingWidth = width;
        state->pendingHeight = height;
        state->resizeConfigured = true;
    }
    void toplevelClose( void *, xdg_toplevel * ) {}
    const xdg_toplevel_listener gToplevelListener = { toplevelConfigure, toplevelClose, nullptr,
                                                       nullptr };

    void xdgSurfaceConfigure( void *data, xdg_surface *xdgSurface, uint32_t serial )
    {
        HostState *state = static_cast<HostState *>( data );
        xdg_surface_ack_configure( xdgSurface, serial );
        state->configured = true;
    }
    const xdg_surface_listener gXdgSurfaceListener = { xdgSurfaceConfigure };

    /// Pumps the host's own Wayland connection. This is the "host must
    /// dispatch" contract WaylandEglWindow documents - Ogre never does this.
    void pumpHost( HostState &state )
    {
        wl_display_flush( state.display );
        wl_display_dispatch_pending( state.display );
    }

    bool createHostWindow( HostState &state )
    {
        state.display = wl_display_connect( nullptr );
        if( !state.display )
        {
            fprintf( stderr, "FAIL: wl_display_connect failed (WAYLAND_DISPLAY not set / "
                              "no compositor running?)\n" );
            return false;
        }

        state.registry = wl_display_get_registry( state.display );
        wl_registry_add_listener( state.registry, &gRegistryListener, &state );
        wl_display_roundtrip( state.display );

        if( !state.compositor || !state.wmBase )
        {
            fprintf( stderr, "FAIL: compositor lacks wl_compositor or xdg_wm_base\n" );
            return false;
        }
        xdg_wm_base_add_listener( state.wmBase, &gWmBaseListener, &state );

        state.surface = wl_compositor_create_surface( state.compositor );
        state.xdgSurface = xdg_wm_base_get_xdg_surface( state.wmBase, state.surface );
        xdg_surface_add_listener( state.xdgSurface, &gXdgSurfaceListener, &state );
        state.toplevel = xdg_surface_get_toplevel( state.xdgSurface );
        xdg_toplevel_add_listener( state.toplevel, &gToplevelListener, &state );
        xdg_toplevel_set_title( state.toplevel, "WaylandEglSmokeTest (host)" );
        wl_surface_commit( state.surface );

        while( !state.configured )
        {
            if( wl_display_dispatch( state.display ) < 0 )
            {
                fprintf( stderr, "FAIL: wl_display_dispatch failed while waiting for configure\n" );
                return false;
            }
        }
        return true;
    }

    void destroyHostWindow( HostState &state )
    {
        if( state.toplevel )
            xdg_toplevel_destroy( state.toplevel );
        if( state.xdgSurface )
            xdg_surface_destroy( state.xdgSurface );
        if( state.surface )
            wl_surface_destroy( state.surface );
        if( state.registry )
            wl_registry_destroy( state.registry );
        if( state.display )
            wl_display_disconnect( state.display );
    }

    bool nearlyEqual( GL::Ubyte a, GL::Ubyte b, int tolerance )
    {
        return std::abs( int( a ) - int( b ) ) <= tolerance;
    }
}  // namespace

int main( int argc, char **argv )
{
    const uint32_t startWidth = 320u;
    const uint32_t startHeight = 240u;
    const int      numFrames = argc > 1 ? atoi( argv[1] ) : 30;

    HostState hostState;
    if( !createHostWindow( hostState ) )
        return 1;
    printf( "OK: host xdg_toplevel created and configured\n" );

    const Ogre::AbiCookie abiCookie = Ogre::generateAbiCookie();
    Ogre::Root            *root = new Ogre::Root( &abiCookie, "", "", "WaylandEglSmokeTest.log" );

    Ogre::GL3PlusPlugin *plugin = new Ogre::GL3PlusPlugin();
    root->installPlugin( plugin, nullptr );

    Ogre::RenderSystem *renderSystem = root->getRenderSystemByName( "OpenGL 3+ Rendering Subsystem" );
    if( !renderSystem )
    {
        fprintf( stderr, "FAIL: OpenGL 3+ Rendering Subsystem not registered\n" );
        return 1;
    }
    root->setRenderSystem( renderSystem );

    // Only meaningful (and only present in mOptions) when GlSwitchableSupport
    // is active, i.e. more than one GLSUPPORT interface is compiled in
    // (which is the normal case: GLX is enabled by default alongside
    // Wayland). If Wayland EGL is the *only* enabled interface, there is no
    // "Interface" option to set - it is already the only choice.
    try
    {
        renderSystem->setConfigOption( "Interface", "Wayland EGL Window" );
        printf( "OK: selected \"Wayland EGL Window\" interface\n" );
    }
    catch( Ogre::Exception & )
    {
        printf( "OK: no \"Interface\" option to set (Wayland EGL Window is the only backend)\n" );
    }

    root->initialise( false, "WaylandEglSmokeTest" );
    printf( "OK: Root::initialise(false) succeeded\n" );

    Ogre::NameValuePairList params;
    params["externalWaylandDisplay"] =
        Ogre::StringConverter::toString( reinterpret_cast<size_t>( hostState.display ) );
    params["externalWaylandSurface"] =
        Ogre::StringConverter::toString( reinterpret_cast<size_t>( hostState.surface ) );

    Ogre::Window *window =
        root->createRenderWindow( "WaylandEglSmokeTest", startWidth, startHeight, false, &params );
    if( !window )
    {
        fprintf( stderr, "FAIL: createRenderWindow returned null\n" );
        return 1;
    }
    printf( "OK: Ogre::Window created, embedded into host's wl_surface\n" );

    Ogre::GL3PlusContext *context = nullptr;
    window->getCustomAttribute( "GLCONTEXT", &context );
    if( !context )
    {
        fprintf( stderr, "FAIL: GLCONTEXT custom attribute not set\n" );
        return 1;
    }

    context->setCurrent();

    if( !GL::load() )
    {
        fprintf( stderr, "FAIL: eglGetProcAddress could not resolve required GL entry points\n" );
        return 1;
    }

    // ---- Render + present a handful of frames, host dispatching between
    // each one (the documented contract: Ogre never dispatches itself). ----
    for( int frame = 0; frame < numFrames; ++frame )
    {
        GL::ClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
        GL::Clear( GL::COLOR_BUFFER_BIT );
        window->swapBuffers();
        pumpHost( hostState );
    }
    printf( "OK: %d frames rendered and presented\n", numFrames );

    // ---- Pixel-readback correctness check: clear to a known color and
    // verify glReadPixels sees it, rather than just checking for crashes. ----
    const GL::Ubyte expectedR = 64, expectedG = 128, expectedB = 192;
    GL::ClearColor( expectedR / 255.0f, expectedG / 255.0f, expectedB / 255.0f, 1.0f );
    GL::Clear( GL::COLOR_BUFFER_BIT );
    GL::Finish();

    GL::Ubyte pixel[4] = { 0, 0, 0, 0 };
    GL::ReadPixels( static_cast<GL::Int>( window->getWidth() / 2u ),
                     static_cast<GL::Int>( window->getHeight() / 2u ), 1, 1, GL::RGBA,
                     GL::UNSIGNED_BYTE, pixel );

    const int tolerance = 2;  // allow for dithering/format rounding
    if( !nearlyEqual( pixel[0], expectedR, tolerance ) || !nearlyEqual( pixel[1], expectedG, tolerance ) ||
        !nearlyEqual( pixel[2], expectedB, tolerance ) )
    {
        fprintf( stderr,
                 "FAIL: pixel readback mismatch. Expected ~(%u,%u,%u), got (%u,%u,%u)\n",
                 expectedR, expectedG, expectedB, pixel[0], pixel[1], pixel[2] );
        return 1;
    }
    printf( "OK: pixel readback matches expected clear color (%u,%u,%u)\n", pixel[0], pixel[1],
            pixel[2] );

    window->swapBuffers();
    pumpHost( hostState );

    // ---- Resize test: host resizes its xdg_toplevel; Ogre must pick it up
    // via requestResolution() the same way a real host (e.g. Qt) would
    // translate xdg_toplevel::configure into a resize call. ----
    xdg_toplevel_set_min_size( hostState.toplevel, 0, 0 );
    xdg_toplevel_set_max_size( hostState.toplevel, 0, 0 );
    wl_surface_commit( hostState.surface );

    const uint32_t resizedWidth = startWidth * 2u;
    const uint32_t resizedHeight = startHeight * 2u;
    window->requestResolution( resizedWidth, resizedHeight );

    GL::Viewport( 0, 0, static_cast<GL::Sizei>( window->getWidth() ),
                  static_cast<GL::Sizei>( window->getHeight() ) );
    GL::ClearColor( 0.2f, 0.6f, 0.2f, 1.0f );
    GL::Clear( GL::COLOR_BUFFER_BIT );
    window->swapBuffers();
    pumpHost( hostState );

    if( window->getWidth() != resizedWidth || window->getHeight() != resizedHeight )
    {
        fprintf( stderr, "FAIL: window resolution after requestResolution is %ux%u, expected %ux%u\n",
                 window->getWidth(), window->getHeight(), resizedWidth, resizedHeight );
        return 1;
    }
    printf( "OK: resize to %ux%u succeeded and swapped without EGL errors\n", window->getWidth(),
            window->getHeight() );

    context->endCurrent();

    // ---- Clean teardown, Ogre's window first, then the host's surface -
    // matching Gazebo's real shutdown order (renderer stops before the
    // QML/Qt-owned surface is torn down). ----
    renderSystem->destroyRenderWindow( window );
    printf( "OK: Ogre::Window destroyed\n" );

    delete root;
    printf( "OK: Ogre::Root destroyed\n" );

    destroyHostWindow( hostState );
    printf( "OK: host teardown complete\n" );

    printf( "\nALL CHECKS PASSED\n" );
    return 0;
}
