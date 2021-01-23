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

#include "OgreEGLWindow.h"

#include "OgreDepthBuffer.h"
#include "OgreEGLGLSupport.h"
#include "OgreException.h"
#include "OgreGL3PlusRenderSystem.h"
#include "OgreGL3PlusTextureGpuManager.h"
#include "OgreGL3PlusTextureGpuWindow.h"
#include "OgreLogManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreRoot.h"
#include "OgreStringConverter.h"
#include "OgreTextureGpuListener.h"
#include "OgreWindowEventUtilities.h"

#include "OgreProfiler.h"
#include "OgreString.h"

#include <sys/time.h>
#include <algorithm>
#include <climits>
#include <iostream>

#include "/home/matias/Projects/renderdoc/renderdoc/api/app/renderdoc_app.h"

RENDERDOC_API_1_4_1 *rdoc_api = NULL;


#define TODO_depth_buffer_bits

namespace Ogre
{
    EGLWindow::EGLWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode,
                          PixelFormatGpu depthStencilFormat, const NameValuePairList *miscParams,
                          EGLGLSupport *glsupport ) :
        Window( title, width, height, fullscreenMode ),
        mClosed( false ),
        mVisible( true ),
        mHidden( false ),
        mIsTopLevel( false ),
        mIsExternal( false ),
        mIsExternalGLControl( false ),
        mGLSupport( glsupport ),
        mContext( 0 ),
        mEglConfig( 0 ),
        mEglSurface( 0 )
    {
        assert( depthStencilFormat == PFG_NULL || depthStencilFormat == PFG_UNKNOWN ||
                PixelFormatGpuUtils::isDepth( depthStencilFormat ) );

        create( depthStencilFormat, miscParams );
    }
    //-----------------------------------------------------------------------------------
    EGLWindow::~EGLWindow()
    {
#ifdef OGRE_EGL_UNPORTED
        Display *xDisplay = mGLSupport->getXDisplay();
#endif

        destroy();

#ifdef OGRE_EGL_UNPORTED
        if( mWindow )
        {
            XDestroyWindow( xDisplay, mWindow );
            XSync( xDisplay, false );
        }
#endif

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

        mContext = 0;
    }
    //-----------------------------------------------------------------------------------
    void EGLWindow::create( PixelFormatGpu depthStencilFormat, const NameValuePairList *miscParams )
    {
        EGLDisplay display = mGLSupport->getGLDisplay();
        uint8 samples = 0;
        bool hidden = false;
        bool vSync = false;
        uint32 vSyncInterval = 1u;
        bool gamma = 0;
        ::EGLContext eglContext = 0;
        ::EGLSurface eglDrawable = 0;

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
                if( !eglGetCurrentContext() )
                {
                    OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                                 "currentGLContext was specified with no current GL context",
                                 "EGLWindow::create" );
                }

                eglContext = eglGetCurrentContext();
                // Trying to reuse the drawable here will actually
                // cause the context sharing to break so it is hidden
                // behind this extra option
                if( ( opt = miscParams->find( "currentGLDrawable" ) ) != end &&
                    StringConverter::parseBool( opt->second ) )
                {
                    eglDrawable = eglGetCurrentSurface( EGL_DRAW );
                }
            }

            // Note: Some platforms support AA inside ordinary windows
            if( ( opt = miscParams->find( "FSAA" ) ) != end )
                samples = StringConverter::parseUnsignedInt( opt->second );
            if( ( opt = miscParams->find( "MSAA" ) ) != end )
                samples = StringConverter::parseUnsignedInt( opt->second );

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

#ifdef OGRE_EGL_UNPORTED
            if( ( opt = miscParams->find( "left" ) ) != end )
                left = StringConverter::parseInt( opt->second );

            if( ( opt = miscParams->find( "top" ) ) != end )
                top = StringConverter::parseInt( opt->second );
#endif

            if( ( opt = miscParams->find( "externalGLControl" ) ) != end )
                mIsExternalGLControl = StringConverter::parseBool( opt->second );

#ifdef OGRE_EGL_UNPORTED
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
                    "EGLWindow::create: The externalWindowHandle parameter is deprecated.\n"
                    "Use the parentWindowHandle or currentGLContext parameter instead." );

                if( tokens.size() == 3 )
                {
                    // Old display:screen:xid format
                    // The old EGL code always created a "parent" window in this case:
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
#endif

            if( ( opt = miscParams->find( "border" ) ) != end )
                border = opt->second;
        }

#ifdef OGRE_EGL_UNPORTED
        // Validate externalWindowHandle

        if( externalWindow != 0 )
        {
            XWindowAttributes windowAttrib;

            if( !XGetWindowAttributes( xDisplay, externalWindow, &windowAttrib ) ||
                windowAttrib.root != DefaultRootWindow( xDisplay ) )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Invalid externalWindowHandle (wrong server or screen)",
                             "EGLWindow::create" );
            }
            eglDrawable = externalWindow;
        }
#endif

        // Derive fbConfig

        // if( !eglDrawable )
        {
            const EGLint configAttribs[] = { EGL_SURFACE_TYPE,
                                             EGL_PBUFFER_BIT,
                                             EGL_BLUE_SIZE,
                                             8,
                                             EGL_GREEN_SIZE,
                                             8,
                                             EGL_RED_SIZE,
                                             8,
                                             EGL_DEPTH_SIZE,
                                             24,
                                             EGL_RENDERABLE_TYPE,
                                             EGL_OPENGL_BIT,
                                             EGL_NONE };

            EGLint numConfigs;
            EGLBoolean configRet = eglChooseConfig( mGLSupport->getGLDisplay(), configAttribs,
                                                    &mEglConfig, 1, &numConfigs );

            if( configRet == EGL_FALSE )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "eglChooseConfig failed", __FUNCTION__ );
            }

            const EGLint pbufferAttribs[] = { EGL_WIDTH, static_cast<EGLint>( mRequestedWidth ),
                                              EGL_HEIGHT, static_cast<EGLint>( mRequestedHeight ),
                                              EGL_NONE };

            mEglSurface =
                eglCreatePbufferSurface( mGLSupport->getGLDisplay(), mEglConfig, pbufferAttribs );

            // Beware ES must not ask EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR
            // eglBindAPI( EGL_OPENGL_ES_API );
            eglBindAPI( EGL_OPENGL_API );

            unsigned int width = mRequestedWidth;
            unsigned int height = mRequestedHeight;

            setFinalResolution( width, height );
        }

        mIsExternal = ( eglDrawable != 0 );

#ifdef OGRE_EGL_UNPORTED
        mIsTopLevel = !mIsExternal && parentWindow == DefaultRootWindow( xDisplay );
#endif

        if( !mIsTopLevel )
        {
            mFullscreenMode = false;
            mRequestedFullscreenMode = false;
#ifdef OGRE_EGL_UNPORTED
            left = top = 0;
#endif
        }

        if( mRequestedFullscreenMode )
        {
#ifdef OGRE_EGL_UNPORTED
            mGLSupport->switchMode( mRequestedWidth, mRequestedHeight,
                                    static_cast<short>( mFrequencyNumerator ) );
#endif
            mFullscreenMode = true;
        }

        if( !mIsExternal )
        {
            // setHidden takes care of mapping or unmapping the window
            // and also calls setFullScreen if appropriate.
            setHidden( hidden );

#ifdef OGRE_EGL_UNPORTED
            WindowEventUtilities::_addRenderWindow( this );
#endif
        }

        mContext = new EGLContext( mGLSupport->getGLDisplay(), mGLSupport, mEglConfig, mEglSurface,
                                   eglContext );

        setVSync( vSync, vSyncInterval );

#ifdef OGRE_EGL_UNPORTED
        mLeft = left;
        mTop = top;
#endif
        mFocused = true;
        mClosed = false;
    }
    //-----------------------------------------------------------------------------------
    void EGLWindow::_initialize( TextureGpuManager *_textureManager )
    {
        GL3PlusTextureGpuManager *textureManager =
            static_cast<GL3PlusTextureGpuManager *>( _textureManager );

        mTexture = textureManager->createTextureGpuWindow( mContext, this );

        mTexture->setPixelFormat( PFG_RGBA8_UNORM );

        // Now check the actual supported fsaa value
        GLint maxSamples = 1;
        SampleDescription sampleDesc( std::max<uint8>( 1u, static_cast<uint8>( maxSamples ) ) );
        mTexture->setSampleDescription( sampleDesc );

        GLint depthSupport = 0, stencilSupport = 0;
        depthSupport = 24;
        TODO_depth_buffer_bits;
        // mGLSupport->getFBConfigAttrib( fbConfig, EGL_DEPTH_SIZE, &depthSupport );
        // mGLSupport->getFBConfigAttrib( fbConfig, EGL_STENCIL_SIZE, &stencilSupport );
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

        if( gammaSupport )
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
    void EGLWindow::destroy( void )
    {
        if( mClosed )
            return;

        mClosed = true;
        mFocused = false;

#ifdef OGRE_EGL_UNPORTED
        if( !mIsExternal )
            WindowEventUtilities::_removeRenderWindow( this );
#endif

        if( mFullscreenMode )
        {
#ifdef OGRE_EGL_UNPORTED
            mGLSupport->switchMode();
#endif
            switchFullScreen( false );
            mRequestedFullscreenMode = false;
        }
    }

    //-----------------------------------------------------------------------------------
    void EGLWindow::requestFullscreenSwitch( bool goFullscreen, bool borderless, uint32 monitorIdx,
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

#ifdef OGRE_EGL_UNPORTED
        if( goFullscreen && mGLSupport->mAtomFullScreen == None )
        {
            // Without WM support it is best to give up.
            LogManager::getSingleton().logMessage(
                "EGLWindow::switchFullScreen: Your WM has no fullscreen support" );
            mRequestedFullscreenMode = false;
            mFullscreenMode = false;
            return;
        }
#endif

        Window::requestFullscreenSwitch( goFullscreen, borderless, monitorIdx, width, height,
                                         frequencyNumerator, frequencyDenominator );

#ifdef OGRE_EGL_UNPORTED
        if( goFullscreen )
        {
            mGLSupport->switchMode( width, height, frequencyNumerator );
        }
        else
        {
            mGLSupport->switchMode();
        }
#endif

        if( mFullscreenMode != goFullscreen )
            switchFullScreen( goFullscreen );

        if( !mFullscreenMode )
        {
            requestResolution( width, height );
            reposition( mLeft, mTop );
        }
    }

    //-----------------------------------------------------------------------------------
    bool EGLWindow::isClosed() const { return mClosed; }
    //-----------------------------------------------------------------------------------
    bool EGLWindow::isVisible() const { return mVisible; }
    //-----------------------------------------------------------------------------------
    void EGLWindow::_setVisible( bool visible ) { mVisible = visible; }
    //-----------------------------------------------------------------------------------
    void EGLWindow::setHidden( bool hidden )
    {
        mHidden = hidden;
        // ignore for external windows as these should handle
        // this externally
        if( mIsExternal )
            return;
    }
    //-----------------------------------------------------------------------------------
    void EGLWindow::setVSync( bool vSync, uint32 vSyncInterval )
    {
        Window::setVSync( vSync, vSyncInterval );

        // we need to make our context current to set vsync
        // store previous context to restore when finished.
        ::EGLSurface oldDrawable = eglGetCurrentSurface( EGL_DRAW );
        ::EGLContext oldContext = eglGetCurrentContext();

        mContext->setCurrent();

        if( !mIsExternalGLControl )
        {
            eglSwapInterval( mGLSupport->getGLDisplay(),
                             mVSync ? static_cast<EGLint>( mVSyncInterval ) : 0 );
        }

        mContext->endCurrent();

        eglMakeCurrent( mGLSupport->getGLDisplay(), oldDrawable, oldDrawable, oldContext );
    }
    //-----------------------------------------------------------------------------------
    void EGLWindow::reposition( int32 left, int32 top )
    {
        if( mClosed || !mIsTopLevel )
            return;
    }
    //-----------------------------------------------------------------------------------
    void EGLWindow::requestResolution( uint32 width, uint32 height )
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
            }
            else
                setFinalResolution( width, height );
        }
    }
    //-----------------------------------------------------------------------------------
    void EGLWindow::windowMovedOrResized()
    {
#ifdef OGRE_EGL_UNPORTED
        if( mClosed || !mWindow )
            return;
#else
        if( mClosed )
            return;
#endif
    }
    //-----------------------------------------------------------------------------------
    void EGLWindow::swapBuffers()
    {
        if( mClosed || mIsExternalGLControl )
            return;

        OgreProfileBeginDynamic( ( "SwapBuffers: " + mTitle ).c_str() );
        OgreProfileGpuBeginDynamic( "SwapBuffers: " + mTitle );

        const bool bFirstFrame = rdoc_api == 0;

        void *mod;
        if(!rdoc_api && (mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD)))
        {
            pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
            int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_1, (void **)&rdoc_api);
            assert(ret == 1);
        }

        if(rdoc_api && !bFirstFrame)
            rdoc_api->EndFrameCapture(NULL, NULL);

        eglSwapBuffers( mGLSupport->getGLDisplay(), mContext->mDrawable );

        if(rdoc_api)
            rdoc_api->StartFrameCapture(NULL, NULL);

        OgreProfileEnd( "SwapBuffers: " + mTitle );
        OgreProfileGpuEnd( "SwapBuffers: " + mTitle );
    }
    //-----------------------------------------------------------------------------------
    void EGLWindow::getCustomAttribute( IdString name, void *pData )
    {
#ifdef OGRE_EGL_UNPORTED
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
        else
#endif
            if( name == "GLCONTEXT" )
        {
            *static_cast<EGLContext **>( pData ) = mContext;
            return;
        }
#ifdef OGRE_EGL_UNPORTED
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
#endif
    }
    //-----------------------------------------------------------------------------------
    void EGLWindow::switchFullScreen( bool fullscreen ) {}
}  // namespace Ogre
