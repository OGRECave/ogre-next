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

#include "windowing/EGL/Wayland/OgreWaylandEglSupport.h"

#include "OgreException.h"
#include "OgreGL3PlusRenderSystem.h"
#include "OgreLogManager.h"
#include "OgreRoot.h"
#include "OgreStringConverter.h"

#include "windowing/EGL/Wayland/OgreWaylandEglWindow.h"

#ifndef EGL_PLATFORM_WAYLAND_KHR
#    define EGL_PLATFORM_WAYLAND_KHR 0x31D8
#endif

namespace Ogre
{
    //-------------------------------------------------------------------------
    WaylandEglSupport::WaylandEglSupport() :
        mWlDisplay( 0 ),
        mEglDisplay( EGL_NO_DISPLAY ),
        mEglConfig( 0 ),
        mPlatformMode( PM_LEGACY )
    {
    }
    //-------------------------------------------------------------------------
    WaylandEglSupport::~WaylandEglSupport() { terminate(); }
    //-------------------------------------------------------------------------
    void WaylandEglSupport::terminate()
    {
        // No context is owned here - each WaylandEglContext (created by
        // individual windows) owns and destroys its own EGLContext, exactly
        // like GLXGLSupport/GLXContext. By the time terminate() runs, all
        // windows using this display should already be destroyed (same
        // ordering requirement GLX already has).
        //
        // mWlDisplay is always host-owned (see the class doc on
        // WaylandEglWindow - there is no self-owned wl_display mode), so
        // the EGLDisplay eglGetPlatformDisplay() returned for it is a
        // shared, global-per-connection resource, not something this class
        // owns either. eglTerminate() would mark it uninitialised for the
        // whole process, invalidating any other context still alive on
        // that same display (e.g. the host's own adopted "currentGLContext"
        // context) even though this class never created it. GLXGLSupport
        // has the identical rule for mIsExternalDisplay
        // (OgreGLXGLSupport.cpp: skips XCloseDisplay when the X connection
        // was supplied externally) - mirror it here by simply never
        // terminating.
        mEglDisplay = EGL_NO_DISPLAY;
        mEglConfig = 0;
        mWlDisplay = 0;
    }
    //-------------------------------------------------------------------------
    void WaylandEglSupport::initialise( wl_display *waylandDisplay )
    {
        if( waylandDisplay == mWlDisplay && mEglDisplay != EGL_NO_DISPLAY )
        {
            // Already initialised against this exact wl_display. Nothing to do.
            return;
        }

        if( mEglDisplay != EGL_NO_DISPLAY )
        {
            LogManager::getSingleton().logMessage(
                "WaylandEglSupport::initialise: re-initialising against a different "
                "wl_display; tearing down the previous EGLDisplay/shared context first." );
            terminate();
        }

        mWlDisplay = waylandDisplay;

        PFNEGLGETPLATFORMDISPLAYEXTPROC _eglGetPlatformDisplayEXT =
            (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress( "eglGetPlatformDisplayEXT" );

#if defined( EGL_VERSION_1_5 )
        mEglDisplay =
            eglGetPlatformDisplay( EGL_PLATFORM_WAYLAND_KHR, (void *)mWlDisplay, 0 );
        mPlatformMode = PM_CORE_1_5;
#else
        mEglDisplay = EGL_NO_DISPLAY;
#endif

        if( mEglDisplay == EGL_NO_DISPLAY && _eglGetPlatformDisplayEXT )
        {
            mEglDisplay =
                _eglGetPlatformDisplayEXT( EGL_PLATFORM_WAYLAND_KHR, (void *)mWlDisplay, 0 );
            mPlatformMode = PM_EXT;
        }

        if( mEglDisplay == EGL_NO_DISPLAY )
        {
            // Legacy fallback: some old EGL 1.4 implementations without
            // EGL_EXT_platform_wayland accept a wl_display reinterpreted as
            // EGLNativeDisplayType directly.
            mEglDisplay = eglGetDisplay( (EGLNativeDisplayType)mWlDisplay );
            mPlatformMode = PM_LEGACY;
        }

        if( mEglDisplay == EGL_NO_DISPLAY )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Unable to get an EGLDisplay for the given wl_display",
                         "WaylandEglSupport::initialise" );
        }

        EGLint major = 0, minor = 0;
        if( eglInitialize( mEglDisplay, &major, &minor ) == EGL_FALSE )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "eglInitialize failed",
                         "WaylandEglSupport::initialise" );
        }

        LogManager::getSingleton().logMessage( "WaylandEglSupport: EGL " +
                                                StringConverter::toString( major ) + "." +
                                                StringConverter::toString( minor ) + " initialised" );

        eglBindAPI( EGL_OPENGL_API );

        chooseEglConfig();
    }
    //-------------------------------------------------------------------------
    void WaylandEglSupport::chooseEglConfig()
    {
        // Fallback ladder, most to least preferred. Each tier is tried in
        // turn; the first one eglChooseConfig can actually satisfy wins.
        const EGLint tierRgba8Depth24Stencil8[] = { EGL_SURFACE_TYPE,
                                                     EGL_WINDOW_BIT,
                                                     EGL_RENDERABLE_TYPE,
                                                     EGL_OPENGL_BIT,
                                                     EGL_RED_SIZE,
                                                     8,
                                                     EGL_GREEN_SIZE,
                                                     8,
                                                     EGL_BLUE_SIZE,
                                                     8,
                                                     EGL_ALPHA_SIZE,
                                                     8,
                                                     EGL_DEPTH_SIZE,
                                                     24,
                                                     EGL_STENCIL_SIZE,
                                                     8,
                                                     EGL_NONE };
        const EGLint tierRgba8Depth24[] = { EGL_SURFACE_TYPE,
                                             EGL_WINDOW_BIT,
                                             EGL_RENDERABLE_TYPE,
                                             EGL_OPENGL_BIT,
                                             EGL_RED_SIZE,
                                             8,
                                             EGL_GREEN_SIZE,
                                             8,
                                             EGL_BLUE_SIZE,
                                             8,
                                             EGL_ALPHA_SIZE,
                                             8,
                                             EGL_DEPTH_SIZE,
                                             24,
                                             EGL_NONE };
        const EGLint tierRgb8[] = { EGL_SURFACE_TYPE,
                                     EGL_WINDOW_BIT,
                                     EGL_RENDERABLE_TYPE,
                                     EGL_OPENGL_BIT,
                                     EGL_RED_SIZE,
                                     8,
                                     EGL_GREEN_SIZE,
                                     8,
                                     EGL_BLUE_SIZE,
                                     8,
                                     EGL_NONE };

        struct Tier
        {
            const EGLint *attribs;
            const char   *description;
        };
        const Tier tiers[] = { { tierRgba8Depth24Stencil8, "RGBA8/D24/S8" },
                                { tierRgba8Depth24, "RGBA8/D24" },
                                { tierRgb8, "RGB8" } };

        for( size_t i = 0u; i < sizeof( tiers ) / sizeof( tiers[0] ); ++i )
        {
            EGLint numConfigs = 0;
            if( eglChooseConfig( mEglDisplay, tiers[i].attribs, &mEglConfig, 1, &numConfigs ) &&
                numConfigs > 0 )
            {
                LogManager::getSingleton().logMessage(
                    String( "WaylandEglSupport: chose EGLConfig tier " ) + tiers[i].description );
                return;
            }
        }

        OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                     "eglChooseConfig could not find a suitable EGLConfig",
                     "WaylandEglSupport::chooseEglConfig" );
    }
    //-------------------------------------------------------------------------
    ::EGLContext WaylandEglSupport::createContext( ::EGLContext shareContext ) const
    {
        EGLint contextAttrs[] = { EGL_CONTEXT_MAJOR_VERSION,
                                   4,
                                   EGL_CONTEXT_MINOR_VERSION,
                                   5,
                                   EGL_CONTEXT_OPENGL_PROFILE_MASK,
                                   EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
#if OGRE_DEBUG_MODE
                                   EGL_CONTEXT_FLAGS_KHR,
                                   EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,
#endif
                                   EGL_NONE };

        ::EGLContext context = EGL_NO_CONTEXT;

        while( context == EGL_NO_CONTEXT && contextAttrs[1] >= 3 )
        {
            context = eglCreateContext( mEglDisplay, mEglConfig, shareContext, contextAttrs );

            if( context != EGL_NO_CONTEXT )
            {
                LogManager::getSingleton().logMessage(
                    "WaylandEglSupport: created GL " + StringConverter::toString( contextAttrs[1] ) +
                    "." + StringConverter::toString( contextAttrs[3] ) + " context" +
                    ( shareContext != EGL_NO_CONTEXT ? " (shared)" : " (unshared)" ) );
            }
            else
            {
                if( contextAttrs[3] == 0 )
                {
                    contextAttrs[1] -= 1;
                    contextAttrs[3] = 5;
                }
                else
                {
                    contextAttrs[3] -= 1;
                }
            }
        }

        return context;
    }
    //-------------------------------------------------------------------------
    EGLConfig WaylandEglSupport::getEglConfigFromContext( ::EGLContext context ) const
    {
        EGLint configId = 0;
        if( !eglQueryContext( mEglDisplay, context, EGL_CONFIG_ID, &configId ) )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "eglQueryContext(EGL_CONFIG_ID) failed for the given EGLContext",
                         "WaylandEglSupport::getEglConfigFromContext" );
        }

        const EGLint configAttrs[] = { EGL_CONFIG_ID, configId, EGL_NONE };
        EGLConfig    config = 0;
        EGLint       numConfigs = 0;
        if( !eglChooseConfig( mEglDisplay, configAttrs, &config, 1, &numConfigs ) || numConfigs == 0 )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Unable to resolve the EGLConfig (id " +
                             StringConverter::toString( (int)configId ) +
                             ") the adopted EGLContext was created with",
                         "WaylandEglSupport::getEglConfigFromContext" );
        }

        return config;
    }
    //-------------------------------------------------------------------------
    void WaylandEglSupport::addConfig()
    {
        // v1 has no meaningful FSAA enumeration to expose before a live
        // wl_display exists (initialise() only happens once a window is
        // actually created, per-instance, not at addConfig() time) -
        // register a minimal placeholder instead of leaving "FSAA" absent
        // entirely. Callers reasonably assume every GL3PlusSupport
        // interface exposes an "FSAA" option (GLX and EGL headless both
        // do) and may set it unconditionally after switching interfaces;
        // leaving it unregistered makes that throw ERR_INVALIDPARAMS.
        ConfigOption optFSAA;
        optFSAA.name = "FSAA";
        optFSAA.immutable = false;
        optFSAA.possibleValues.push_back( "0" );
        optFSAA.currentValue = "0";

        mOptions[optFSAA.name] = optFSAA;
    }
    //-------------------------------------------------------------------------
    String WaylandEglSupport::validateConfig() { return BLANKSTRING; }
    //-------------------------------------------------------------------------
    Window *WaylandEglSupport::createWindow( bool autoCreateWindow, GL3PlusRenderSystem *renderSystem,
                                              const String &windowTitle )
    {
        if( autoCreateWindow )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "WaylandEglSupport requires createRenderWindow to be called with "
                         "externalWaylandDisplay/externalWaylandSurface miscParams; "
                         "auto-window creation is not supported.",
                         "WaylandEglSupport::createWindow" );
        }

        return 0;
    }
    //-------------------------------------------------------------------------
    Window *WaylandEglSupport::newWindow( const String &name, uint32 width, uint32 height,
                                           bool fullScreen, const NameValuePairList *miscParams )
    {
        return new WaylandEglWindow( name, width, height, fullScreen, miscParams, this );
    }
    //-------------------------------------------------------------------------
    void WaylandEglSupport::start()
    {
        LogManager::getSingleton().logMessage(
            "*****************************************\n"
            "*** Starting Wayland EGL Window Subsystem ***\n"
            "*****************************************" );
    }
    //-------------------------------------------------------------------------
    void WaylandEglSupport::stop()
    {
        LogManager::getSingleton().logMessage(
            "*****************************************\n"
            "*** Stopping Wayland EGL Window Subsystem ***\n"
            "*****************************************" );

        terminate();
    }
    //-------------------------------------------------------------------------
    void *WaylandEglSupport::getProcAddress( const char *procname ) const
    {
        return (void *)eglGetProcAddress( procname );
    }
}  // namespace Ogre
