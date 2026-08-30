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

#include "windowing/EGL/Wayland/OgreWaylandEglWindow.h"

#include "OgreDepthBuffer.h"
#include "OgreException.h"
#include "OgreGL3PlusRenderSystem.h"
#include "OgreGL3PlusTextureGpuManager.h"
#include "OgreGL3PlusTextureGpuWindow.h"
#include "OgreLogManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreStringConverter.h"
#include "OgreTextureGpuListener.h"

#include "OgreProfiler.h"
#include "OgreString.h"

namespace Ogre
{
    //-----------------------------------------------------------------------------------
    WaylandEglWindow::WaylandEglWindow( const String &title, uint32 width, uint32 height,
                                         bool fullscreenMode, const NameValuePairList *miscParams,
                                         WaylandEglSupport *glsupport ) :
        Window( title, width, height, fullscreenMode ),
        mClosed( false ),
        mVisible( true ),
        mHidden( false ),
        mHwGamma( false ),
        mIsExternalGLControl( false ),
        mGLSupport( glsupport ),
        mContext( 0 ),
        mWlDisplay( 0 ),
        mWlSurface( 0 ),
        mWlEglWindow( 0 ),
        mEglSurface( EGL_NO_SURFACE )
    {
        create( miscParams );
    }
    //-----------------------------------------------------------------------------------
    WaylandEglWindow::~WaylandEglWindow()
    {
        destroy();

        if( mContext )
        {
            delete mContext;
            mContext = 0;
        }

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
        mStencilBuffer = 0;
    }
    //-----------------------------------------------------------------------------------
    void WaylandEglWindow::create( const NameValuePairList *miscParams )
    {
        bool   hidden = false;
        bool   vSync = false;
        uint32 vSyncInterval = 1u;
        bool   wantCurrentGLContext = false;

        wl_display *externalDisplay = 0;
        wl_surface *externalSurface = 0;

        if( miscParams )
        {
            NameValuePairList::const_iterator opt;
            NameValuePairList::const_iterator end = miscParams->end();

            if( ( opt = miscParams->find( "externalWaylandDisplay" ) ) != end )
            {
                externalDisplay =
                    reinterpret_cast<wl_display *>( StringConverter::parseSizeT( opt->second ) );
            }

            if( ( opt = miscParams->find( "externalWaylandSurface" ) ) != end )
            {
                externalSurface =
                    reinterpret_cast<wl_surface *>( StringConverter::parseSizeT( opt->second ) );
            }

            if( ( opt = miscParams->find( "FSAA" ) ) != end )
                mRequestedSampleDescription.parseString( opt->second );

            if( ( opt = miscParams->find( "gamma" ) ) != end )
                mHwGamma = StringConverter::parseBool( opt->second );

            if( ( opt = miscParams->find( "vsync" ) ) != end )
                vSync = StringConverter::parseBool( opt->second );

            if( ( opt = miscParams->find( "vsyncInterval" ) ) != end )
                vSyncInterval = StringConverter::parseUnsignedInt( opt->second );

            if( ( opt = miscParams->find( "hidden" ) ) != end )
                hidden = StringConverter::parseBool( opt->second );

            if( ( opt = miscParams->find( "currentGLContext" ) ) != end )
                wantCurrentGLContext = StringConverter::parseBool( opt->second );

            if( ( opt = miscParams->find( "externalGLControl" ) ) != end )
                mIsExternalGLControl = StringConverter::parseBool( opt->second );
        }

        if( !externalDisplay || !externalSurface )
        {
            // v1 only supports embedding into a host-owned wl_display/wl_surface
            // (e.g. Gazebo embedding into a Qt/QML-owned surface). A self-owned
            // standalone toplevel window mode (own wl_display connection, own
            // xdg_wm_base binding, own xdg_toplevel) is deferred future work
            // and is not implemented here. See the class Doxygen comment.
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "WaylandEglWindow requires both externalWaylandDisplay and "
                         "externalWaylandSurface miscParams (raw wl_display*/wl_surface* "
                         "pointers, stringified via StringConverter::toString of the "
                         "reinterpreted size_t). Self-owned standalone windows are not "
                         "supported yet.",
                         "WaylandEglWindow::create" );
        }

        mWlDisplay = externalDisplay;
        mWlSurface = externalSurface;

        mGLSupport->initialise( mWlDisplay );

        // If requested, capture whatever EGLContext is current on this
        // thread now (mirrors GLXWindow's "currentGLContext" handling) -
        // must happen before any of our own EGL calls below, which don't
        // change the current context but keep the intent explicit and
        // fail fast if nothing is current.
        ::EGLContext adoptedContext = EGL_NO_CONTEXT;
        if( wantCurrentGLContext )
        {
            adoptedContext = eglGetCurrentContext();
            if( adoptedContext == EGL_NO_CONTEXT )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "currentGLContext was specified with no current GL context",
                             "WaylandEglWindow::create" );
            }
        }

        mWlEglWindow = wl_egl_window_create( mWlSurface, static_cast<int>( mRequestedWidth ),
                                              static_cast<int>( mRequestedHeight ) );
        if( !mWlEglWindow )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "wl_egl_window_create failed",
                         "WaylandEglWindow::create" );
        }

        EGLDisplay eglDisplay = mGLSupport->getEglDisplay();
        // When adopting an external context, the surface must be created
        // with the EXACT EGLConfig that context was created with (mixing
        // configs between a context and the surface it's made current
        // against is undefined behaviour) - derive it instead of using our
        // own independently-chosen mGLSupport->getEglConfig().
        EGLConfig eglConfig = ( adoptedContext != EGL_NO_CONTEXT )
                                  ? mGLSupport->getEglConfigFromContext( adoptedContext )
                                  : mGLSupport->getEglConfig();

        // Must match whichever entry point produced mEglDisplay (see
        // WaylandEglSupport::PlatformMode) - mixing a core/EXT-obtained
        // EGLDisplay with the wrong surface-creation entry point is either
        // undefined behaviour or an unresolved symbol on drivers that only
        // implement one of the two.
        switch( mGLSupport->getPlatformMode() )
        {
        case WaylandEglSupport::PM_CORE_1_5:
            mEglSurface = eglCreatePlatformWindowSurface( eglDisplay, eglConfig, mWlEglWindow, 0 );
            break;
        case WaylandEglSupport::PM_EXT:
        {
            PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC _eglCreatePlatformWindowSurfaceEXT =
                (PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC)mGLSupport->getProcAddress(
                    "eglCreatePlatformWindowSurfaceEXT" );
            if( _eglCreatePlatformWindowSurfaceEXT )
            {
                mEglSurface =
                    _eglCreatePlatformWindowSurfaceEXT( eglDisplay, eglConfig, mWlEglWindow, 0 );
            }
            break;
        }
        case WaylandEglSupport::PM_LEGACY:
            mEglSurface = eglCreateWindowSurface( eglDisplay, eglConfig,
                                                   (EGLNativeWindowType)mWlEglWindow, 0 );
            break;
        }

        if( mEglSurface == EGL_NO_SURFACE )
        {
            wl_egl_window_destroy( mWlEglWindow );
            mWlEglWindow = 0;
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "eglCreatePlatformWindowSurface(EXT)/eglCreateWindowSurface failed",
                         "WaylandEglWindow::create" );
        }

        try
        {
            mContext = new WaylandEglContext( mGLSupport, mEglSurface, adoptedContext );
        }
        catch( ... )
        {
            // mContext was never successfully constructed, so ~WaylandEglWindow
            // won't run and won't clean these up either - do it here.
            eglDestroySurface( eglDisplay, mEglSurface );
            mEglSurface = EGL_NO_SURFACE;
            wl_egl_window_destroy( mWlEglWindow );
            mWlEglWindow = 0;
            throw;
        }

        setFinalResolution( mRequestedWidth, mRequestedHeight );

        setHidden( hidden );
        setVSync( vSync, vSyncInterval );

        mFocused = true;
        mClosed = false;

        LogManager::getSingleton().logMessage(
            "WaylandEglWindow::create: embedded into host-owned wl_surface (" +
            StringConverter::toString( mRequestedWidth ) + "x" +
            StringConverter::toString( mRequestedHeight ) + ")" );
    }
    //-----------------------------------------------------------------------------------
    void WaylandEglWindow::_initialize( TextureGpuManager *_textureManager )
    {
        GL3PlusTextureGpuManager *textureManager =
            static_cast<GL3PlusTextureGpuManager *>( _textureManager );

        mTexture = textureManager->createTextureGpuWindow( mContext, this );
        mTexture->setPixelFormat( PFG_RGBA8_UNORM );

        EGLDisplay eglDisplay = mGLSupport->getEglDisplay();
        EGLConfig  eglConfig = mGLSupport->getEglConfig();

        EGLint samples = 1;
        eglGetConfigAttrib( eglDisplay, eglConfig, EGL_SAMPLES, &samples );
        SampleDescription sampleDesc( std::max<uint8>( 1u, static_cast<uint8>( samples ) ) );
        mTexture->setSampleDescription( sampleDesc );

        EGLint depthSize = 0, stencilSize = 0;
        eglGetConfigAttrib( eglDisplay, eglConfig, EGL_DEPTH_SIZE, &depthSize );
        eglGetConfigAttrib( eglDisplay, eglConfig, EGL_STENCIL_SIZE, &stencilSize );

        if( depthSize != 0 )
        {
            mDepthBuffer = textureManager->createTextureGpuWindow( mContext, this );
            mDepthBuffer->setSampleDescription( sampleDesc );

            if( depthSize == 24 )
            {
                mDepthBuffer->setPixelFormat( stencilSize == 0 ? PFG_D24_UNORM
                                                                 : PFG_D24_UNORM_S8_UINT );
            }
            else
            {
                mDepthBuffer->setPixelFormat( stencilSize == 0 ? PFG_D32_FLOAT
                                                                 : PFG_D32_FLOAT_S8X24_UINT );
            }

            if( stencilSize != 0 )
                mStencilBuffer = mDepthBuffer;
        }

        if( mHwGamma )
            mTexture->setPixelFormat( PFG_RGBA8_UNORM_SRGB );

        setFinalResolution( mRequestedWidth, mRequestedHeight );

        if( mDepthBuffer )
        {
            mTexture->_setDepthBufferDefaults( DepthBuffer::POOL_NON_SHAREABLE, false,
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
    void WaylandEglWindow::destroy()
    {
        if( mClosed )
            return;

        mClosed = true;
        mFocused = false;

        if( mEglSurface != EGL_NO_SURFACE )
        {
            // If mEglSurface is still the current thread's bound draw/read
            // surface, detach it first (surfaceless current, keeping
            // whatever context is current still current) rather than
            // destroying a live-bound surface out from under it. This
            // matters most for an externally-adopted context (see
            // "currentGLContext"/mIsExternalGLControl): initialiseContext()
            // binds the adopted context to this window's surface via
            // setCurrent(), and if that context/window creation then fails
            // (e.g. during a gz-rendering retry loop with different FSAA
            // params) this destroy() runs on the still-current surface.
            // On at least NVIDIA's proprietary driver, destroying a
            // currently-bound EGLSurface here was observed to silently
            // drop the context out of "current" entirely (not just
            // unbind the surface) - which, for an adopted context Ogre
            // doesn't own, corrupts state for the caller (e.g. Qt) and
            // for every subsequent retry, which relies on that same
            // context still being current. Rebinding to
            // EGL_NO_SURFACE/EGL_NO_SURFACE while keeping the context
            // (relies on EGL_KHR_surfaceless_context, near-universally
            // supported) avoids this entirely.
            EGLDisplay eglDisplay = mGLSupport->getEglDisplay();
            if( eglGetCurrentSurface( EGL_DRAW ) == mEglSurface )
            {
                ::EGLContext stillCurrentContext = eglGetCurrentContext();
                eglMakeCurrent( eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, stillCurrentContext );
            }

            eglDestroySurface( eglDisplay, mEglSurface );
            mEglSurface = EGL_NO_SURFACE;
        }

        if( mWlEglWindow )
        {
            wl_egl_window_destroy( mWlEglWindow );
            mWlEglWindow = 0;
        }

        // mWlSurface/mWlDisplay are host-owned: never destroyed here.
        mWlSurface = 0;
        mWlDisplay = 0;
    }
    //-----------------------------------------------------------------------------------
    void WaylandEglWindow::requestFullscreenSwitch( bool goFullscreen, bool borderless,
                                                      uint32 monitorIdx, uint32 width, uint32 height,
                                                      uint32 frequencyNumerator,
                                                      uint32 frequencyDenominator )
    {
        // The host owns window chrome/state in embedded mode; nothing to do here.
        if( mClosed )
            return;
    }
    //-----------------------------------------------------------------------------------
    bool WaylandEglWindow::isClosed() const { return mClosed; }
    //-----------------------------------------------------------------------------------
    bool WaylandEglWindow::isVisible() const { return mVisible; }
    //-----------------------------------------------------------------------------------
    void WaylandEglWindow::_setVisible( bool visible ) { mVisible = visible; }
    //-----------------------------------------------------------------------------------
    void WaylandEglWindow::setHidden( bool hidden ) { mHidden = hidden; }
    //-----------------------------------------------------------------------------------
    void WaylandEglWindow::setVSync( bool vSync, uint32 vSyncInterval )
    {
        Window::setVSync( vSync, vSyncInterval );

        // The caller owns presentation when externalGLControl is set (e.g.
        // Qt/gz-rendering already manages its own swap interval) - mirrors
        // GLXWindow's identical guard.
        if( mIsExternalGLControl )
            return;

        EGLSurface   oldSurface = eglGetCurrentSurface( EGL_DRAW );
        ::EGLContext oldContext = eglGetCurrentContext();

        mContext->setCurrent();

        eglSwapInterval( mGLSupport->getEglDisplay(),
                          mVSync ? static_cast<EGLint>( mVSyncInterval ) : 0 );

        mContext->endCurrent();

        eglMakeCurrent( mGLSupport->getEglDisplay(), oldSurface, oldSurface, oldContext );
    }
    //-----------------------------------------------------------------------------------
    void WaylandEglWindow::reposition( int32 left, int32 top )
    {
        // The host owns window chrome/state in embedded mode; nothing to do here.
    }
    //-----------------------------------------------------------------------------------
    void WaylandEglWindow::requestResolution( uint32 width, uint32 height )
    {
        if( mClosed )
            return;

        if( mTexture && mTexture->getWidth() == width && mTexture->getHeight() == height )
            return;

        Window::requestResolution( width, height );

        if( width != 0 && height != 0 )
        {
            if( mWlEglWindow )
            {
                wl_egl_window_resize( mWlEglWindow, static_cast<int>( width ),
                                       static_cast<int>( height ), 0, 0 );
            }
            setFinalResolution( width, height );
        }
    }
    //-----------------------------------------------------------------------------------
    void WaylandEglWindow::windowMovedOrResized()
    {
        // Wayland has no synchronous "query current geometry" call; resolution
        // changes come from the host calling requestResolution() (driven by
        // whatever mechanism the host uses to learn of a resize, e.g. its own
        // xdg_toplevel::configure handling). Nothing to poll here.
    }
    //-----------------------------------------------------------------------------------
    void WaylandEglWindow::swapBuffers()
    {
        if( mClosed || mIsExternalGLControl )
            return;

        OgreProfileBeginDynamic( ( "SwapBuffers: " + mTitle ).c_str() );
        OgreProfileGpuBeginDynamic( "SwapBuffers: " + mTitle );

        // Deliberately does NOT call wl_surface_commit() or any
        // wl_display_dispatch*()/wl_display_flush(): eglSwapBuffers() on a
        // wl_egl_window-backed EGLSurface internally performs the
        // attach+commit+damage sequence for the buffer it owns, which is
        // sufficient to present per the EGL Wayland platform spec. The host
        // application is solely responsible for driving the wl_display event
        // loop (see class Doxygen comment).
        if( eglSwapBuffers( mGLSupport->getEglDisplay(), mEglSurface ) == EGL_FALSE )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "eglSwapBuffers failed with EGL error 0x" +
                             StringConverter::toString( (uint32)eglGetError(), 0, ' ', std::ios::hex ),
                         "WaylandEglWindow::swapBuffers" );
        }

        OgreProfileEnd( "SwapBuffers: " + mTitle );
        OgreProfileGpuEnd( "SwapBuffers: " + mTitle );
    }
    //-----------------------------------------------------------------------------------
    void WaylandEglWindow::getCustomAttribute( IdString name, void *pData )
    {
        if( name == "GLCONTEXT" )
        {
            *static_cast<GL3PlusContext **>( pData ) = mContext;
            return;
        }
        else if( name == "RENDERDOC_DEVICE" )
        {
            *static_cast< ::EGLContext *>( pData ) = mContext->getEglContext();
            return;
        }
        else if( name == "RENDERDOC_WINDOW" )
        {
            *static_cast<EGLSurface *>( pData ) = mEglSurface;
            return;
        }
        else if( name == "EGLDISPLAY" )
        {
            *static_cast<EGLDisplay *>( pData ) = mGLSupport->getEglDisplay();
            return;
        }
        else if( name == "EGLCONTEXT" )
        {
            *static_cast< ::EGLContext *>( pData ) = mContext->getEglContext();
            return;
        }
        else if( name == "EGLSURFACE" )
        {
            *static_cast<EGLSurface *>( pData ) = mEglSurface;
            return;
        }
        else if( name == "WAYLAND_DISPLAY" )
        {
            *static_cast<wl_display **>( pData ) = mWlDisplay;
            return;
        }
        else if( name == "WAYLAND_SURFACE" )
        {
            // Also doubles as the sentinel WindowEventUtilities::messagePump()
            // probes to skip this window (it never registers for pumping).
            *static_cast<wl_surface **>( pData ) = mWlSurface;
            return;
        }
        else if( name == "WAYLAND_EGL_WINDOW" )
        {
            *static_cast<wl_egl_window **>( pData ) = mWlEglWindow;
            return;
        }
    }
}  // namespace Ogre
