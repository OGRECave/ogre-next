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

#include "PBuffer/OgreEglPBufferWindow.h"

#include "OgreDepthBuffer.h"
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
#include "PBuffer/OgreEglPBufferSupport.h"

#include "OgreProfiler.h"
#include "OgreString.h"

#include <sys/time.h>
#include <algorithm>
#include <climits>
#include <iostream>

namespace Ogre
{
    EglPBufferWindow::EglPBufferWindow( const String &title, uint32 width, uint32 height,
                                        bool fullscreenMode, const NameValuePairList *miscParams,
                                        EglPBufferSupport *glsupport ) :
        Window( title, width, height, fullscreenMode ),
        mClosed( false ),
        mVisible( true ),
        mHidden( false ),
        mHwGamma( false ),
        mGLSupport( glsupport ),
        mContext( 0 ),
        mEglConfig( 0 ),
        mEglSurface( 0 )
    {
        create( miscParams );
    }
    //-----------------------------------------------------------------------------------
    EglPBufferWindow::~EglPBufferWindow()
    {
        destroy();

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
    void EglPBufferWindow::create( const NameValuePairList *miscParams )
    {
        if( miscParams )
        {
            NameValuePairList::const_iterator opt;
            NameValuePairList::const_iterator end = miscParams->end();

            opt = miscParams->find( "FSAA" );
            if( opt != end )
                mRequestedSampleDescription.parseString( opt->second );

            opt = miscParams->find( "gamma" );
            if( opt != end )
                mHwGamma = StringConverter::parseBool( opt->second );
        }

        setFinalResolution( mRequestedWidth, mRequestedHeight );

        mFullscreenMode = mRequestedFullscreenMode;

        setHidden( false );

        mContext = new EglPBufferContext( mGLSupport );

        setVSync( false, 1u );

        mFocused = true;
        mClosed = false;
    }
    //-----------------------------------------------------------------------------------
    void EglPBufferWindow::_initialize( TextureGpuManager *_textureManager )
    {
        GL3PlusTextureGpuManager *textureManager =
            static_cast<GL3PlusTextureGpuManager *>( _textureManager );

        mTexture = textureManager->createTextureGpuHeadlessWindow( mContext, this );
        mTexture->setResolution( mRequestedWidth, mRequestedHeight );
        mTexture->setPixelFormat( mHwGamma ? PFG_RGBA8_UNORM_SRGB : PFG_RGBA8_UNORM );

        mDepthBuffer = textureManager->createTextureGpuHeadlessWindow( mContext, this );
        mDepthBuffer->setPixelFormat( DepthBuffer::DefaultDepthBufferFormat );
        if( PixelFormatGpuUtils::isStencil( mDepthBuffer->getPixelFormat() ) )
            mStencilBuffer = mDepthBuffer;

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

        mTexture->setSampleDescription( mRequestedSampleDescription );
        mDepthBuffer->setSampleDescription( mRequestedSampleDescription );
        mSampleDescription = mRequestedSampleDescription;

        mTexture->_transitionTo( GpuResidency::Resident, (uint8 *)0 );
        if( mDepthBuffer )
            mDepthBuffer->_transitionTo( GpuResidency::Resident, (uint8 *)0 );
    }
    //-----------------------------------------------------------------------------------
    void EglPBufferWindow::destroy()
    {
        if( mClosed )
            return;

        mClosed = true;
        mFocused = false;

        if( mFullscreenMode )
            mRequestedFullscreenMode = false;
    }
    //-----------------------------------------------------------------------------------
    void EglPBufferWindow::requestFullscreenSwitch( bool goFullscreen, bool borderless,
                                                    uint32 monitorIdx, uint32 width, uint32 height,
                                                    uint32 frequencyNumerator,
                                                    uint32 frequencyDenominator )
    {
        if( mClosed )
            return;

        Window::requestFullscreenSwitch( goFullscreen, borderless, monitorIdx, width, height,
                                         frequencyNumerator, frequencyDenominator );
    }
    //-----------------------------------------------------------------------------------
    bool EglPBufferWindow::isClosed() const { return mClosed; }
    //-----------------------------------------------------------------------------------
    bool EglPBufferWindow::isVisible() const { return mVisible; }
    //-----------------------------------------------------------------------------------
    void EglPBufferWindow::_setVisible( bool visible ) { mVisible = visible; }
    //-----------------------------------------------------------------------------------
    void EglPBufferWindow::setHidden( bool hidden ) { mHidden = hidden; }
    //-----------------------------------------------------------------------------------
    void EglPBufferWindow::setVSync( bool vSync, uint32 vSyncInterval )
    {
        Window::setVSync( vSync, vSyncInterval );

        // we need to make our context current to set vsync
        // store previous context to restore when finished.
        ::EGLSurface oldDrawable = eglGetCurrentSurface( EGL_DRAW );
        ::EGLContext oldContext = eglGetCurrentContext();

        mContext->setCurrent();

        eglSwapInterval( mGLSupport->getGLDisplay(),
                         mVSync ? static_cast<EGLint>( mVSyncInterval ) : 0 );

        mContext->endCurrent();

        eglMakeCurrent( mGLSupport->getGLDisplay(), oldDrawable, oldDrawable, oldContext );
    }
    //-----------------------------------------------------------------------------------
    void EglPBufferWindow::reposition( int32 left, int32 top ) {}
    //-----------------------------------------------------------------------------------
    void EglPBufferWindow::requestResolution( uint32 width, uint32 height )
    {
        if( mClosed )
            return;

        if( mTexture && mTexture->getWidth() == width && mTexture->getHeight() == height )
            return;

        Window::requestResolution( width, height );

        setFinalResolution( width, height );
    }
    //-----------------------------------------------------------------------------------
    void EglPBufferWindow::swapBuffers()
    {
        if( mClosed )
            return;

        OgreProfileBeginDynamic( ( "SwapBuffers: " + mTitle ).c_str() );
        OgreProfileGpuBeginDynamic( "SwapBuffers: " + mTitle );

        eglSwapBuffers( mContext->getDeviceData()->eglDisplay, mContext->getDeviceData()->eglSurf );

        OgreProfileEnd( "SwapBuffers: " + mTitle );
        OgreProfileGpuEnd( "SwapBuffers: " + mTitle );
    }
    //-----------------------------------------------------------------------------------
    void EglPBufferWindow::getCustomAttribute( IdString name, void *pData )
    {
        if( name == "GLCONTEXT" )
        {
            *static_cast<GL3PlusContext **>( pData ) = mContext;
            return;
        }
        else if( name == "RENDERDOC_DEVICE" )
        {
            *static_cast< ::EGLContext *>( pData ) = mContext->getDeviceData()->eglCtx;
            return;
        }
        else if( name == "RENDERDOC_WINDOW" )
        {
            *static_cast< ::EGLSurface *>( pData ) = mContext->getDeviceData()->eglSurf;
            return;
        }
    }
}  // namespace Ogre
