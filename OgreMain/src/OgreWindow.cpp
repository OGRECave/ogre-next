/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

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

#include "OgreWindow.h"
#include "OgreException.h"

namespace Ogre
{
    Window::Window( const String &title, uint32 width, uint32 height, bool fullscreenMode ) :
        mTitle( title ),
        mTexture( 0 ),
        mDepthBuffer( 0 ),
        mStencilBuffer( 0 ),
        mFrequencyNumerator( 0 ),
        mFrequencyDenominator( 0 ),
        mRequestedWidth( width ),
        mRequestedHeight( height ),
        mFullscreenMode( false ),
        mRequestedFullscreenMode( fullscreenMode ),
        mBorderless( false ),
        mFocused( true ),
        mIsPrimary( false ),
        mVSync( false ),
        mVSyncInterval( 1u ),
        mLeft( 0 ),
        mTop( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    Window::~Window()
    {
        assert( !mTexture && "Derived class didn't properly free resources" );
        assert( !mDepthBuffer && "Derived class didn't properly free resources" );
        assert( !mStencilBuffer && "Derived class didn't properly free resources" );
    }
    //-----------------------------------------------------------------------------------
    void Window::setFinalResolution( uint32 width, uint32 height )
    {
        mRequestedWidth = width;
        mRequestedHeight = height;

        if( mTexture )
            mTexture->setResolution( width, height, 1u );
        if( mDepthBuffer )
            mDepthBuffer->setResolution( width, height, 1u );
        if( mStencilBuffer )
            mStencilBuffer->setResolution( width, height, 1u );
    }
    //-----------------------------------------------------------------------------------
    void Window::setTitle( const String &title )
    {
        mTitle = title;
    }
    //-----------------------------------------------------------------------------------
    const String& Window::getTitle(void) const
    {
        return mTitle;
    }
    //-----------------------------------------------------------------------------------
    void Window::requestResolution( uint32 width, uint32 height )
    {
        mRequestedWidth = width;
        mRequestedHeight = height;
    }
    //-----------------------------------------------------------------------------------
    void Window::requestFullscreenSwitch( bool goFullscreen, bool borderless, uint32 monitorIdx,
                                          uint32 width, uint32 height,
                                          uint32 frequencyNumerator, uint32 frequencyDenominator )
    {
        mRequestedFullscreenMode    = goFullscreen;
        mRequestedWidth             = width;
        mRequestedHeight            = height;
        mFrequencyNumerator         = frequencyNumerator;
        mFrequencyDenominator       = frequencyDenominator;
        mBorderless                 = borderless;
    }
    //-----------------------------------------------------------------------------------
    void Window::setVSync( bool vSync, uint32 vSyncInterval )
    {
        mVSync = vSync;
        mVSyncInterval = vSyncInterval;
    }
    //-----------------------------------------------------------------------------------
    bool Window::getVSync(void) const
    {
        return mVSync;
    }
    //-----------------------------------------------------------------------------------
    uint32 Window::getVSyncInterval(void) const
    {
        return mVSyncInterval;
    }
    //-----------------------------------------------------------------------------------
    void Window::setBorderless( bool borderless )
    {
        mBorderless = borderless;
    }
    //-----------------------------------------------------------------------------------
    bool Window::getBorderless(void) const
    {
        return mBorderless;
    }
    //-----------------------------------------------------------------------------------
    uint32 Window::getWidth(void) const
    {
        return mTexture->getWidth();
    }
    //-----------------------------------------------------------------------------------
    uint32 Window::getHeight(void) const
    {
        return mTexture->getHeight();
    }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu Window::getPixelFormat(void) const
    {
        return mTexture->getPixelFormat();
    }
    //-----------------------------------------------------------------------------------
    uint8 Window::getMsaa(void) const
    {
        return mTexture->getMsaa();
    }
    //-----------------------------------------------------------------------------------
    MsaaPatterns::MsaaPatterns Window::getMsaaPatterns(void) const
    {
        return mTexture->getMsaaPattern();
    }
    //-----------------------------------------------------------------------------------
    uint32 Window::getFrequencyNumerator(void) const
    {
        return mFrequencyNumerator;
    }
    //-----------------------------------------------------------------------------------
    uint32 Window::getFrequencyDenominator(void) const
    {
        return mFrequencyDenominator;
    }
    //-----------------------------------------------------------------------------------
    uint32 Window::getRequestedWidth(void) const
    {
        return mRequestedWidth;
    }
    //-----------------------------------------------------------------------------------
    uint32 Window::getRequestedHeight(void) const
    {
        return mRequestedHeight;
    }
    //-----------------------------------------------------------------------------------
    bool Window::isFullscreen(void) const
    {
        return mFullscreenMode;
    }
    //-----------------------------------------------------------------------------------
    bool Window::wantsToGoFullscreen(void) const
    {
        return mRequestedFullscreenMode && mRequestedFullscreenMode != mFullscreenMode;
    }
    //-----------------------------------------------------------------------------------
    bool Window::wantsToGoWindowed(void) const
    {
        return !mRequestedFullscreenMode && mRequestedFullscreenMode != mFullscreenMode;
    }
    //-----------------------------------------------------------------------------------
    void Window::setFocused( bool focused )
    {
        mFocused = focused;
    }
    //-----------------------------------------------------------------------------------
    bool Window::isFocused(void) const
    {
        return mFocused;
    }
    //-----------------------------------------------------------------------------------
    void Window::_setPrimary(void)
    {
        mIsPrimary = true;
    }
    //-----------------------------------------------------------------------------------
    bool Window::isPrimary(void) const
    {
        return mIsPrimary;
    }
    //-----------------------------------------------------------------------------------
    void Window::getMetrics( uint32 &width, uint32 &height, int32 &left, int32 &top ) const
    {
        left    = mLeft;
        top     = mTop;
        width   = mTexture ? mTexture->getWidth() : mRequestedWidth;
        height  = mTexture ? mTexture->getHeight() : mRequestedHeight;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* Window::getTexture(void) const
    {
        return mTexture;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* Window::getDepthBuffer(void) const
    {
        return mDepthBuffer;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* Window::getStencilBuffer(void) const
    {
        return mStencilBuffer;
    }
}
