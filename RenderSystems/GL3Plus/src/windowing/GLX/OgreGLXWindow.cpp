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

#include "OgreRoot.h"

#include "OgreGLXWindow.h"

#include "OgreDepthBuffer.h"
#include "OgreException.h"
#include "OgreGL3PlusRenderSystem.h"
#include "OgreGL3PlusTextureGpuManager.h"
#include "OgreGL3PlusTextureGpuWindow.h"
#include "OgreGLXGLSupport.h"
#include "OgreGLXUtils.h"
#include "OgreLogManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreStringConverter.h"
#include "OgreTextureGpuListener.h"
#include "OgreWindowEventUtilities.h"

#include "OgreProfiler.h"
#include "OgreString.h"

#include <sys/time.h>
#include <algorithm>
#include <climits>
#include <iostream>

#include <X11/Xlib.h>
#include <X11/keysym.h>

extern "C" {
int safeXErrorHandler( Display *display, XErrorEvent *event )
{
    // Ignore all XErrorEvents
    return 0;
}
int ( *oldXErrorHandler )( Display *, XErrorEvent * );
}

namespace Ogre
{
    GLXWindow::GLXWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode,
                          PixelFormatGpu depthStencilFormat, const NameValuePairList *miscParams,
                          GLXGLSupport *glsupport ) :
        Window( title, width, height, fullscreenMode ),
        mClosed( false ),
        mVisible( true ),
        mHidden( false ),
        mIsTopLevel( false ),
        mIsExternal( false ),
        mIsExternalGLControl( false ),
        mGLSupport( glsupport ),
        mWindow( 0 ),
        mContext( 0 )
    {
        assert( depthStencilFormat == PFG_NULL || depthStencilFormat == PFG_UNKNOWN ||
                PixelFormatGpuUtils::isDepth( depthStencilFormat ) );

        create( depthStencilFormat, miscParams );
    }
    //-----------------------------------------------------------------------------------
    GLXWindow::~GLXWindow()
    {
        Display *xDisplay = mGLSupport->getXDisplay();

        destroy();

        // Ignore fatal XErrorEvents from stale handles.
        oldXErrorHandler = XSetErrorHandler( safeXErrorHandler );

        if( mWindow )
        {
            XDestroyWindow( xDisplay, mWindow );
            XSync( xDisplay, false );
        }

        if( mContext )
        {
            delete mContext;
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
        // OGRE_DELETE mStencilBuffer;
        mStencilBuffer = 0;

        XSetErrorHandler( oldXErrorHandler );

        mContext = 0;
        mWindow = 0;
    }
    //-----------------------------------------------------------------------------------
    void GLXWindow::create( PixelFormatGpu depthStencilFormat, const NameValuePairList *miscParams )
    {
        Display *xDisplay = mGLSupport->getXDisplay();
        uint8 samples = 0;
        bool hidden = false;
        bool vSync = false;
        uint32 vSyncInterval = 1u;
        bool gamma = 0;
        ::GLXContext glxContext = 0;
        ::GLXDrawable glxDrawable = 0;
        ::Window externalWindow = 0;
        ::Window parentWindow = DefaultRootWindow( xDisplay );
        int left = DisplayWidth( xDisplay, DefaultScreen( xDisplay ) ) / 2 -
                   static_cast<int>( mRequestedWidth / 2u );
        int top = DisplayHeight( xDisplay, DefaultScreen( xDisplay ) ) / 2 -
                  static_cast<int>( mRequestedHeight / 2u );
        String border;

        if( miscParams )
        {
            NameValuePairList::const_iterator opt;
            NameValuePairList::const_iterator end = miscParams->end();

            // NB: Do not try to implement the externalGLContext option.
            //
            //   Accepting a non-current context would expose us to the
            //   risk of segfaults when we made it current. Since the
            //   application programmers would be responsible for these
            //   segfaults, they are better discovering them in their code.

            if( ( opt = miscParams->find( "currentGLContext" ) ) != end &&
                StringConverter::parseBool( opt->second ) )
            {
                if( !glXGetCurrentContext() )
                {
                    OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                                 "currentGLContext was specified with no current GL context",
                                 "GLXWindow::create" );
                }

                glxContext = glXGetCurrentContext();
                // Trying to reuse the drawable here will actually
                // cause the context sharing to break so it is hidden
                // behind this extra option
                if( ( opt = miscParams->find( "currentGLDrawable" ) ) != end &&
                    StringConverter::parseBool( opt->second ) )
                {
                    glxDrawable = glXGetCurrentDrawable();
                }
            }

            // Note: Some platforms support AA inside ordinary windows
            if( ( opt = miscParams->find( "FSAA" ) ) != end )
                samples = (uint8)StringConverter::parseUnsignedInt( opt->second );
            if( ( opt = miscParams->find( "MSAA" ) ) != end )
                samples = (uint8)StringConverter::parseUnsignedInt( opt->second );

            if( ( opt = miscParams->find( "displayFrequency" ) ) != end && opt->second != "N/A" )
                mFrequencyNumerator = StringConverter::parseUnsignedInt( opt->second );

            if( ( opt = miscParams->find( "vsync" ) ) != end )
                vSync = StringConverter::parseBool( opt->second );

            if( ( opt = miscParams->find( "hidden" ) ) != end )
                hidden = StringConverter::parseBool( opt->second );

            if( ( opt = miscParams->find( "vsyncInterval" ) ) != end )
                vSyncInterval = StringConverter::parseUnsignedInt( opt->second );

            if( ( opt = miscParams->find( "gamma" ) ) != end )
                gamma = StringConverter::parseBool( opt->second );

            if( ( opt = miscParams->find( "left" ) ) != end )
                left = StringConverter::parseInt( opt->second );

            if( ( opt = miscParams->find( "top" ) ) != end )
                top = StringConverter::parseInt( opt->second );

            if( ( opt = miscParams->find( "externalGLControl" ) ) != end )
                mIsExternalGLControl = StringConverter::parseBool( opt->second );

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
            if( ( opt = miscParams->find( "stereoMode" ) ) != end )
            {
                StereoModeType stereoMode = StringConverter::parseStereoMode( opt->second );
                if( SMT_NONE != stereoMode )
                    mStereoEnabled = true;
            }
#endif

            if( ( opt = miscParams->find( "parentWindowHandle" ) ) != end )
            {
                vector<String>::type tokens = StringUtil::split( opt->second, " :" );

                if( tokens.size() == 3 )
                {
                    // deprecated display:screen:xid format
                    parentWindow = StringConverter::parseUnsignedLong( tokens[2] );
                }
                else
                {
                    // xid format
                    parentWindow = StringConverter::parseUnsignedLong( tokens[0] );
                }
            }
            else if( ( opt = miscParams->find( "externalWindowHandle" ) ) != end )
            {
                vector<String>::type tokens = StringUtil::split( opt->second, " :" );

                LogManager::getSingleton().logMessage(
                    "GLXWindow::create: The externalWindowHandle parameter is deprecated.\n"
                    "Use the parentWindowHandle or currentGLContext parameter instead." );

                if( tokens.size() == 3 )
                {
                    // Old display:screen:xid format
                    // The old GLX code always created a "parent" window in this case:
                    parentWindow = StringConverter::parseUnsignedLong( tokens[2] );
                }
                else if( tokens.size() == 4 )
                {
                    // Old display:screen:xid:visualinfo format
                    externalWindow = StringConverter::parseUnsignedLong( tokens[2] );
                }
                else
                {
                    // xid format
                    externalWindow = StringConverter::parseUnsignedLong( tokens[0] );
                }
            }

            if( ( opt = miscParams->find( "border" ) ) != end )
                border = opt->second;
        }

        // Ignore fatal XErrorEvents during parameter validation:
        oldXErrorHandler = XSetErrorHandler( safeXErrorHandler );
        // Validate parentWindowHandle

        if( parentWindow != DefaultRootWindow( xDisplay ) )
        {
            XWindowAttributes windowAttrib;

            if( !XGetWindowAttributes( xDisplay, parentWindow, &windowAttrib ) ||
                windowAttrib.root != DefaultRootWindow( xDisplay ) )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Invalid parentWindowHandle (wrong server or screen)",
                             "GLXWindow::create" );
            }
        }

        // Validate externalWindowHandle

        if( externalWindow != 0 )
        {
            XWindowAttributes windowAttrib;

            if( !XGetWindowAttributes( xDisplay, externalWindow, &windowAttrib ) ||
                windowAttrib.root != DefaultRootWindow( xDisplay ) )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Invalid externalWindowHandle (wrong server or screen)",
                             "GLXWindow::create" );
            }
            glxDrawable = externalWindow;
        }

        // Derive fbConfig

        ::GLXFBConfig fbConfig = 0;

        if( glxDrawable )
        {
            unsigned int width = mRequestedWidth;
            unsigned int height = mRequestedHeight;
            fbConfig = mGLSupport->getFBConfigFromDrawable( glxDrawable, &width, &height );
            setFinalResolution( width, height );
        }

        if( !fbConfig && glxContext )
            fbConfig = mGLSupport->getFBConfigFromContext( glxContext );

        mIsExternal = ( glxDrawable != 0 );

        XSetErrorHandler( oldXErrorHandler );

        if( !fbConfig )
        {
            const int MAX_ATTRIB_COUNT = 64;
            int minAttribs[MAX_ATTRIB_COUNT];
            memset( minAttribs, 0, sizeof( minAttribs ) );
            int attrib = 0;
            minAttribs[attrib++] = GLX_DRAWABLE_TYPE;
            minAttribs[attrib++] = GLX_WINDOW_BIT;
            minAttribs[attrib++] = GLX_RENDER_TYPE;
            minAttribs[attrib++] = GLX_RGBA_BIT;
            minAttribs[attrib++] = GLX_RED_SIZE;
            minAttribs[attrib++] = 1;
            minAttribs[attrib++] = GLX_GREEN_SIZE;
            minAttribs[attrib++] = 1;
            minAttribs[attrib++] = GLX_BLUE_SIZE;
            minAttribs[attrib++] = 1;
#if OGRE_NO_QUAD_BUFFER_STEREO == 0
            minAttribs[attrib++] = GLX_STEREO;
            minAttribs[attrib++] = mStereoEnabled ? True : False;
#endif
            if( depthStencilFormat == PFG_NULL )
            {
                minAttribs[attrib++] = GLX_DEPTH_SIZE;
                minAttribs[attrib++] = 0;
            }
            else
            {
                minAttribs[attrib++] = GLX_DEPTH_SIZE;
                minAttribs[attrib++] = 24;
            }
            minAttribs[attrib++] = GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT;
            minAttribs[attrib++] = gamma == true;
            minAttribs[attrib++] = None;

            attrib = 0;

            int maxAttribs[MAX_ATTRIB_COUNT];
            memset( maxAttribs, 0, sizeof( maxAttribs ) );
            maxAttribs[attrib++] = GLX_SAMPLES;
            maxAttribs[attrib++] = static_cast<int>( samples );
            maxAttribs[attrib++] = GLX_DOUBLEBUFFER;
            maxAttribs[attrib++] = 1;
            if( depthStencilFormat == PFG_D24_UNORM_S8_UINT || depthStencilFormat == PFG_D24_UNORM )
            {
                // If user wants a 24-bit format, we'll restrict it.
                maxAttribs[attrib++] = GLX_DEPTH_SIZE;
                maxAttribs[attrib++] = 24;
            }
            else if( depthStencilFormat == PFG_NULL )
            {
                // User wants no depth buffer, restrict it even more.
                maxAttribs[attrib++] = GLX_DEPTH_SIZE;
                maxAttribs[attrib++] = 0;
            }
            if( PixelFormatGpuUtils::isStencil( depthStencilFormat ) ||
                depthStencilFormat == PFG_UNKNOWN )
            {
                maxAttribs[attrib++] = GLX_STENCIL_SIZE;
                maxAttribs[attrib++] = INT_MAX;
            }
            else
            {
                maxAttribs[attrib++] = GLX_STENCIL_SIZE;
                maxAttribs[attrib++] = 0;
            }
            maxAttribs[attrib++] = GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT;
            maxAttribs[attrib++] = gamma == true;
            maxAttribs[attrib++] = None;

            assert( attrib < MAX_ATTRIB_COUNT );

            fbConfig = mGLSupport->selectFBConfig( minAttribs, maxAttribs );
        }

        if( !fbConfig )
        {
            // This should never happen.
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Unexpected failure to determine a GLXFBConfig", "GLXWindow::create" );
        }

        mIsTopLevel = !mIsExternal && parentWindow == DefaultRootWindow( xDisplay );

        if( !mIsTopLevel )
        {
            mFullscreenMode = false;
            mRequestedFullscreenMode = false;
            left = top = 0;
        }

        if( mRequestedFullscreenMode )
        {
            mGLSupport->switchMode( mRequestedWidth, mRequestedHeight,
                                    static_cast<short>( mFrequencyNumerator ) );
            mFullscreenMode = true;
        }

        if( !mIsExternal )
        {
            XSetWindowAttributes attr;
            ulong mask;
            XVisualInfo *visualInfo = mGLSupport->getVisualFromFBConfig( fbConfig );

            attr.background_pixel = 0;
            attr.border_pixel = 0;
            attr.colormap = XCreateColormap( xDisplay, DefaultRootWindow( xDisplay ), visualInfo->visual,
                                             AllocNone );
            attr.event_mask = StructureNotifyMask | VisibilityChangeMask | FocusChangeMask;
            mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

            if( mFullscreenMode && mGLSupport->mAtomFullScreen == None )
            {
                mRequestedFullscreenMode = false;
                mFullscreenMode = false;
                LogManager::getSingleton().logMessage(
                    "GLXWindow::switchFullScreen: Your WM has no fullscreen support" );

                // A second best approach for outdated window managers
                attr.backing_store = NotUseful;
                attr.save_under = False;
                attr.override_redirect = True;
                mask |= CWSaveUnder | CWBackingStore | CWOverrideRedirect;
                left = top = 0;
            }

            // Create window on server
            mWindow =
                XCreateWindow( xDisplay, parentWindow, left, top, mRequestedWidth, mRequestedHeight, 0,
                               visualInfo->depth, InputOutput, visualInfo->visual, mask, &attr );

            XFree( visualInfo );

            if( !mWindow )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "Unable to create an X Window",
                             "GLXWindow::create" );
            }

            if( mIsTopLevel )
            {
                XWMHints *wmHints;
                XSizeHints *sizeHints;

                if( ( wmHints = XAllocWMHints() ) != NULL )
                {
                    wmHints->initial_state = NormalState;
                    wmHints->input = True;
                    wmHints->flags = StateHint | InputHint;

                    int depth = DisplayPlanes( xDisplay, DefaultScreen( xDisplay ) );

                    // Check if we can give it an icon
                    if( depth == 24 || depth == 32 )
                    {
                        if( mGLSupport->loadIcon( "GLX_icon.png", &wmHints->icon_pixmap,
                                                  &wmHints->icon_mask ) )
                        {
                            wmHints->flags |= IconPixmapHint | IconMaskHint;
                        }
                    }
                }

                // Is this really necessary ? Which broken WM might need it?
                if( ( sizeHints = XAllocSizeHints() ) != NULL )
                {
                    // Is this really necessary ? Which broken WM might need it?
                    sizeHints->flags = USPosition;

                    if( !mRequestedFullscreenMode && border == "fixed" )
                    {
                        sizeHints->min_width = sizeHints->max_width =
                            static_cast<int>( mRequestedWidth );
                        sizeHints->min_height = sizeHints->max_height =
                            static_cast<int>( mRequestedHeight );
                        sizeHints->flags |= PMaxSize | PMinSize;
                    }
                }

                XTextProperty titleprop;
                char *lst = const_cast<char *>( mTitle.c_str() );
                XStringListToTextProperty( (char **)&lst, 1, &titleprop );
                XSetWMProperties( xDisplay, mWindow, &titleprop, NULL, NULL, 0, sizeHints, wmHints,
                                  NULL );

                XFree( titleprop.value );
                XFree( wmHints );
                XFree( sizeHints );

                XSetWMProtocols( xDisplay, mWindow, &mGLSupport->mAtomDeleteWindow, 1 );

                XWindowAttributes windowAttrib;

                XGetWindowAttributes( xDisplay, mWindow, &windowAttrib );

                left = windowAttrib.x;
                top = windowAttrib.y;
                setFinalResolution( static_cast<uint32>( windowAttrib.width ),
                                    static_cast<uint32>( windowAttrib.height ) );
            }

            glxDrawable = mWindow;

            // setHidden takes care of mapping or unmapping the window
            // and also calls setFullScreen if appropriate.
            setHidden( hidden );
            XSync( xDisplay, False );

            WindowEventUtilities::_addRenderWindow( this );
        }

        mContext = new GLXContext( mGLSupport, fbConfig, glxDrawable, glxContext );

        setVSync( vSync, vSyncInterval );

        int fbConfigID;

        mGLSupport->getFBConfigAttrib( fbConfig, GLX_FBCONFIG_ID, &fbConfigID );

        LogManager::getSingleton().logMessage( "GLXWindow::create used FBConfigID = " +
                                               StringConverter::toString( fbConfigID ) );

        mLeft = left;
        mTop = top;
        mFocused = true;
        mClosed = false;
    }
    //-----------------------------------------------------------------------------------
    void GLXWindow::_initialize( TextureGpuManager *_textureManager, const NameValuePairList * )
    {
        GL3PlusTextureGpuManager *textureManager =
            static_cast<GL3PlusTextureGpuManager *>( _textureManager );

        mTexture = textureManager->createTextureGpuWindow( mContext, this );

        mTexture->setPixelFormat( PFG_RGBA8_UNORM );

        ::GLXFBConfig fbConfig = mContext->_getFbConfig();

        // Now check the actual supported fsaa value
        GLint maxSamples;
        mGLSupport->getFBConfigAttrib( fbConfig, GLX_SAMPLES, &maxSamples );
        SampleDescription sampleDesc( std::max<uint8>( 1u, static_cast<uint8>( maxSamples ) ) );
        mTexture->setSampleDescription( sampleDesc );

        GLint depthSupport = 0, stencilSupport = 0;
        mGLSupport->getFBConfigAttrib( fbConfig, GLX_DEPTH_SIZE, &depthSupport );
        mGLSupport->getFBConfigAttrib( fbConfig, GLX_STENCIL_SIZE, &stencilSupport );
        if( depthSupport != 0 )
        {
            mDepthBuffer = textureManager->createTextureGpuWindow( mContext, this );
            mDepthBuffer->setSampleDescription( sampleDesc );

            if( depthSupport == 24 )
            {
                mDepthBuffer->setPixelFormat( stencilSupport == 0 ? PFG_D24_UNORM
                                                                  : PFG_D24_UNORM_S8_UINT );
            }
            else
            {
                mDepthBuffer->setPixelFormat( stencilSupport == 0 ? PFG_D32_FLOAT
                                                                  : PFG_D32_FLOAT_S8X24_UINT );
            }

            if( stencilSupport != 0 )
                mStencilBuffer = mDepthBuffer;
        }

        GLint gammaSupport = 0;
        int result =
            mGLSupport->getFBConfigAttrib( fbConfig, GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT, &gammaSupport );
        if( result != Success )
            gammaSupport = 0;

        if( gammaSupport )
            mTexture->setPixelFormat( PFG_RGBA8_UNORM_SRGB );

        setFinalResolution( mRequestedWidth, mRequestedHeight );

        if( mDepthBuffer )
        {
            mTexture->_setDepthBufferDefaults( DepthBuffer::NO_POOL_EXPLICIT_RTV, false,
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
    void GLXWindow::destroy()
    {
        if( mClosed )
            return;

        mClosed = true;
        mFocused = false;

        if( !mIsExternal )
            WindowEventUtilities::_removeRenderWindow( this );

        if( mFullscreenMode )
        {
            mGLSupport->switchMode();
            switchFullScreen( false );
            mRequestedFullscreenMode = false;
        }
    }

    //-----------------------------------------------------------------------------------
    void GLXWindow::requestFullscreenSwitch( bool goFullscreen, bool borderless, uint32 monitorIdx,
                                             uint32 width, uint32 height, uint32 frequencyNumerator,
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

        if( goFullscreen && mGLSupport->mAtomFullScreen == None )
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
            mGLSupport->switchMode( width, height, (short)frequencyNumerator );
        }
        else
        {
            mGLSupport->switchMode();
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
    bool GLXWindow::isClosed() const { return mClosed; }
    //-----------------------------------------------------------------------------------
    bool GLXWindow::isVisible() const { return mVisible; }
    //-----------------------------------------------------------------------------------
    void GLXWindow::_setVisible( bool visible ) { mVisible = visible; }
    //-----------------------------------------------------------------------------------
    void GLXWindow::setHidden( bool hidden )
    {
        mHidden = hidden;
        // ignore for external windows as these should handle
        // this externally
        if( mIsExternal )
            return;

        if( hidden )
        {
            XUnmapWindow( mGLSupport->getXDisplay(), mWindow );
        }
        else
        {
            XMapWindow( mGLSupport->getXDisplay(), mWindow );
            if( mFullscreenMode )
                switchFullScreen( true );
        }
    }
    //-----------------------------------------------------------------------------------
    void GLXWindow::setVSync( bool vSync, uint32 vSyncInterval )
    {
        Window::setVSync( vSync, vSyncInterval );

        // we need to make our context current to set vsync
        // store previous context to restore when finished.
        ::GLXDrawable oldDrawable = glXGetCurrentDrawable();
        ::GLXContext oldContext = glXGetCurrentContext();

        mContext->setCurrent();

        PFNGLXSWAPINTERVALEXTPROC _glXSwapIntervalEXT;
        _glXSwapIntervalEXT =
            (PFNGLXSWAPINTERVALEXTPROC)mGLSupport->getProcAddress( "glXSwapIntervalEXT" );
        PFNGLXSWAPINTERVALMESAPROC _glXSwapIntervalMESA;
        _glXSwapIntervalMESA =
            (PFNGLXSWAPINTERVALMESAPROC)mGLSupport->getProcAddress( "glXSwapIntervalMESA" );
        PFNGLXSWAPINTERVALSGIPROC _glXSwapIntervalSGI;
        _glXSwapIntervalSGI =
            (PFNGLXSWAPINTERVALSGIPROC)mGLSupport->getProcAddress( "glXSwapIntervalSGI" );

        if( !mIsExternalGLControl )
        {
            if( _glXSwapIntervalMESA )
                _glXSwapIntervalMESA( mVSync ? mVSyncInterval : 0 );
            else if( _glXSwapIntervalEXT )
            {
                _glXSwapIntervalEXT( mGLSupport->getGLDisplay(), mContext->mDrawable,
                                     mVSync ? static_cast<int>( mVSyncInterval ) : 0 );
            }
            else
                _glXSwapIntervalSGI( mVSync ? static_cast<int>( mVSyncInterval ) : 0 );
        }

        mContext->endCurrent();

        glXMakeCurrent( mGLSupport->getGLDisplay(), oldDrawable, oldContext );
    }
    //-----------------------------------------------------------------------------------
    void GLXWindow::reposition( int32 left, int32 top )
    {
        if( mClosed || !mIsTopLevel )
            return;

        XMoveWindow( mGLSupport->getXDisplay(), mWindow, left, top );
    }
    //-----------------------------------------------------------------------------------
    void GLXWindow::requestResolution( uint32 width, uint32 height )
    {
        if( mClosed )
            return;

        if( mTexture && mTexture->getWidth() == width && mTexture->getHeight() == height )
            return;

        Window::requestResolution( width, height );

        if( width != 0 && height != 0 )
        {
            if( !mIsExternal )
                XResizeWindow( mGLSupport->getXDisplay(), mWindow, width, height );
            else
                setFinalResolution( width, height );
        }
    }
    //-----------------------------------------------------------------------------------
    void GLXWindow::windowMovedOrResized()
    {
        if( mClosed || !mWindow )
            return;

        Display *xDisplay = mGLSupport->getXDisplay();
        XWindowAttributes windowAttrib;

        if( mIsTopLevel && !mFullscreenMode )
        {
            ::Window parent, root, *children;
            uint nChildren;

            XQueryTree( xDisplay, mWindow, &root, &parent, &children, &nChildren );

            if( children )
                XFree( children );

            XGetWindowAttributes( xDisplay, parent, &windowAttrib );

            mLeft = windowAttrib.x;
            mTop = windowAttrib.y;
        }

        XGetWindowAttributes( xDisplay, mWindow, &windowAttrib );

        setFinalResolution( static_cast<uint32>( windowAttrib.width ),
                            static_cast<uint32>( windowAttrib.height ) );
    }
    //-----------------------------------------------------------------------------------
    void GLXWindow::swapBuffers()
    {
        if( mClosed || mIsExternalGLControl )
            return;

        OgreProfileBeginDynamic( ( "SwapBuffers: " + mTitle ).c_str() );
        OgreProfileGpuBeginDynamic( "SwapBuffers: " + mTitle );

        glXSwapBuffers( mGLSupport->getGLDisplay(), mContext->mDrawable );

        OgreProfileEnd( "SwapBuffers: " + mTitle );
        OgreProfileGpuEnd( "SwapBuffers: " + mTitle );
    }
    //-----------------------------------------------------------------------------------
    void GLXWindow::getCustomAttribute( IdString name, void *pData )
    {
        if( name == "DISPLAY NAME" )
        {
            *static_cast<String *>( pData ) = mGLSupport->getDisplayName();
            return;
        }
        else if( name == "DISPLAY" )
        {
            *static_cast<Display **>( pData ) = mGLSupport->getGLDisplay();
            return;
        }
        else if( name == "GLCONTEXT" )
        {
            *static_cast<GLXContext **>( pData ) = mContext;
            return;
        }
        else if( name == "RENDERDOC_DEVICE" )
        {
            *static_cast< ::GLXContext *>( pData ) = mContext->mContext;
            return;
        }
        else if( name == "RENDERDOC_WINDOW" )
        {
            *static_cast< ::GLXDrawable *>( pData ) = mContext->mDrawable;
            return;
        }
        else if( name == "XDISPLAY" )
        {
            *static_cast<Display **>( pData ) = mGLSupport->getXDisplay();
            return;
        }
        else if( name == "ATOM" )
        {
            *static_cast< ::Atom *>( pData ) = mGLSupport->mAtomDeleteWindow;
            return;
        }
        else if( name == "WINDOW" )
        {
            *static_cast< ::Window *>( pData ) = mWindow;
            return;
        }
    }
    //-----------------------------------------------------------------------------------
    void GLXWindow::switchFullScreen( bool fullscreen )
    {
        if( mGLSupport->mAtomFullScreen != None )
        {
            Display *xDisplay = mGLSupport->getXDisplay();
            XClientMessageEvent xMessage;

            xMessage.type = ClientMessage;
            xMessage.serial = 0;
            xMessage.send_event = True;
            xMessage.window = mWindow;
            xMessage.message_type = mGLSupport->mAtomState;
            xMessage.format = 32;
            xMessage.data.l[0] = ( fullscreen ? 1 : 0 );
            xMessage.data.l[1] = static_cast<long>( mGLSupport->mAtomFullScreen );
            xMessage.data.l[2] = 0;

            XSendEvent( xDisplay, DefaultRootWindow( xDisplay ), False,
                        SubstructureRedirectMask | SubstructureNotifyMask, (XEvent *)&xMessage );

            mFullscreenMode = fullscreen;
        }
    }
}  // namespace Ogre
