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
#include "OgreNULLWindow.h"

#include "OgreNULLTextureGpu.h"
#include "OgreNULLTextureGpuManager.h"

namespace Ogre
{
    NULLWindow::NULLWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode ) :
        Window( title, width, height, fullscreenMode )
    {
        mFocused = true;
        mClosed = false;
    }
    //-------------------------------------------------------------------------
    NULLWindow::~NULLWindow() { destroy(); }
    //-------------------------------------------------------------------------
    void NULLWindow::destroy()
    {
        mFocused = false;
        mClosed = true;

        OGRE_DELETE mTexture;
        mTexture = 0;
        OGRE_DELETE mDepthBuffer;
        mDepthBuffer = 0;
        mStencilBuffer = 0;
    }
    //-------------------------------------------------------------------------
    void NULLWindow::reposition( int32 left, int32 top )
    {
        mLeft = left;
        mTop = top;
    }
    //-------------------------------------------------------------------------
    void NULLWindow::requestResolution( uint32 width, uint32 height )
    {
        Window::requestResolution( width, height );
        setFinalResolution( width, height );
    }
    //-------------------------------------------------------------------------
    void NULLWindow::requestFullscreenSwitch( bool goFullscreen, bool borderless, uint32 monitorIdx,
                                              uint32 width, uint32 height, uint32 frequencyNumerator,
                                              uint32 frequencyDenominator )
    {
        Window::requestFullscreenSwitch( goFullscreen, borderless, monitorIdx, width, height,
                                         frequencyNumerator, frequencyDenominator );
        setFinalResolution( width, height );
        mFullscreenMode = goFullscreen;
    }
    //-------------------------------------------------------------------------
    bool NULLWindow::isClosed() const { return mClosed; }
    //-------------------------------------------------------------------------
    void NULLWindow::_setVisible( bool visible ) {}
    //-------------------------------------------------------------------------
    bool NULLWindow::isVisible() const { return true; }
    //-------------------------------------------------------------------------
    void NULLWindow::setHidden( bool hidden ) {}
    //-------------------------------------------------------------------------
    bool NULLWindow::isHidden() const { return false; }
    //-------------------------------------------------------------------------
    void NULLWindow::_initialize( TextureGpuManager *textureGpuManager, const NameValuePairList * )
    {
        destroy();

        mFocused = true;
        mClosed = false;

        NULLTextureGpuManager *textureManager =
            static_cast<NULLTextureGpuManager *>( textureGpuManager );

        mTexture = textureManager->createTextureGpuWindow();
        mDepthBuffer = textureManager->createTextureGpuWindow();
        mStencilBuffer = mDepthBuffer;

        setFinalResolution( mRequestedWidth, mRequestedHeight );
        mTexture->setPixelFormat( PFG_RGBA8_UNORM );
        mDepthBuffer->setPixelFormat( PFG_D32_FLOAT_S8X24_UINT );

        mTexture->_transitionTo( GpuResidency::Resident, (uint8 *)0 );
        mDepthBuffer->_transitionTo( GpuResidency::Resident, (uint8 *)0 );
    }
    //-------------------------------------------------------------------------
    void NULLWindow::swapBuffers() {}
}  // namespace Ogre
