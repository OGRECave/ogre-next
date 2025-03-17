/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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
#include "OgreStringConverter.h"

namespace Ogre
{
    Window::Window( const String &title, uint32 widthPt, uint32 heightPt, bool fullscreenMode ) :
        mTitle( title ),
        mTexture( 0 ),
        mDepthBuffer( 0 ),
        mStencilBuffer( 0 ),
        mFrequencyNumerator( 0 ),
        mFrequencyDenominator( 0 ),
        mRequestedWidth( widthPt ),
        mRequestedHeight( heightPt ),
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
    void Window::setFinalResolution( uint32 widthPx, uint32 heightPx )
    {
        if( mTexture )
            mTexture->setResolution( widthPx, heightPx, 1u );
        if( mDepthBuffer )
            mDepthBuffer->setResolution( widthPx, heightPx, 1u );
        if( mStencilBuffer )
            mStencilBuffer->setResolution( widthPx, heightPx, 1u );
    }
    //-------------------------------------------------------------------------
    bool Window::requestedMemoryless( const NameValuePairList *ogre_nullable miscParams )
    {
        if( !miscParams )
            return false;

        NameValuePairList::const_iterator opt;
        NameValuePairList::const_iterator end = miscParams->end();

        opt = miscParams->find( "memoryless_depth_buffer" );
        if( opt != end )
            return StringConverter::parseBool( opt->second );
        return false;
    }
    //-----------------------------------------------------------------------------------
    void Window::setTitle( const String &title ) { mTitle = title; }
    //-----------------------------------------------------------------------------------
    const String &Window::getTitle() const { return mTitle; }
    //-----------------------------------------------------------------------------------
    void Window::requestResolution( uint32 widthPt, uint32 heightPt )
    {
        mRequestedWidth = widthPt;
        mRequestedHeight = heightPt;
    }
    //-----------------------------------------------------------------------------------
    void Window::requestFullscreenSwitch( bool goFullscreen, bool borderless, uint32 monitorIdx,
                                          uint32 widthPt, uint32 heightPt, uint32 frequencyNumerator,
                                          uint32 frequencyDenominator )
    {
        mRequestedFullscreenMode = goFullscreen;
        mRequestedWidth = widthPt;
        mRequestedHeight = heightPt;
        mFrequencyNumerator = frequencyNumerator;
        mFrequencyDenominator = frequencyDenominator;
        mBorderless = borderless;
    }
    //-----------------------------------------------------------------------------------
    void Window::setVSync( bool vSync, uint32 vSyncInterval )
    {
        mVSync = vSync;
        mVSyncInterval = vSyncInterval & 0x7FFFFFFF;
    }
    //-----------------------------------------------------------------------------------
    bool Window::getVSync() const { return mVSync; }
    //-----------------------------------------------------------------------------------
    uint32 Window::getVSyncInterval() const { return mVSyncInterval; }
    //-----------------------------------------------------------------------------------
    void Window::setBorderless( bool borderless ) { mBorderless = borderless; }
    //-----------------------------------------------------------------------------------
    void Window::setManualSwapRelease( bool ) {}
    //-----------------------------------------------------------------------------------
    void Window::performManualRelease() {}
    //-----------------------------------------------------------------------------------
    void Window::setWantsToDownload( bool ) {}
    //-----------------------------------------------------------------------------------
    bool Window::canDownloadData() const { return true; }
    //-----------------------------------------------------------------------------------
    bool Window::getBorderless() const { return mBorderless; }
    //-----------------------------------------------------------------------------------
    uint32 Window::getWidth() const { return mTexture->getWidth(); }
    //-----------------------------------------------------------------------------------
    uint32 Window::getHeight() const { return mTexture->getHeight(); }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu Window::getPixelFormat() const { return mTexture->getPixelFormat(); }
    //-----------------------------------------------------------------------------------
    SampleDescription Window::getSampleDescription() const { return mTexture->getSampleDescription(); }
    //-----------------------------------------------------------------------------------
    bool Window::isMultisample() const { return mTexture->isMultisample(); }
    //-----------------------------------------------------------------------------------
    uint32 Window::getFrequencyNumerator() const { return mFrequencyNumerator; }
    //-----------------------------------------------------------------------------------
    uint32 Window::getFrequencyDenominator() const { return mFrequencyDenominator; }
    //-----------------------------------------------------------------------------------
    uint32 Window::getRequestedWidthPt() const { return mRequestedWidth; }
    //-----------------------------------------------------------------------------------
    uint32 Window::getRequestedHeightPt() const { return mRequestedHeight; }
    //-----------------------------------------------------------------------------------
    bool Window::isFullscreen() const { return mFullscreenMode; }
    //-----------------------------------------------------------------------------------
    bool Window::wantsToGoFullscreen() const
    {
        return mRequestedFullscreenMode && mRequestedFullscreenMode != mFullscreenMode;
    }
    //-----------------------------------------------------------------------------------
    bool Window::wantsToGoWindowed() const
    {
        return !mRequestedFullscreenMode && mRequestedFullscreenMode != mFullscreenMode;
    }
    //-----------------------------------------------------------------------------------
    void Window::setFocused( bool focused ) { mFocused = focused; }
    //-----------------------------------------------------------------------------------
    bool Window::isFocused() const { return mFocused; }
    //-----------------------------------------------------------------------------------
    void Window::_setPrimary() { mIsPrimary = true; }
    //-----------------------------------------------------------------------------------
    bool Window::isPrimary() const { return mIsPrimary; }
    //-----------------------------------------------------------------------------------
    void Window::getMetrics( uint32 &width, uint32 &height, int32 &left, int32 &top ) const
    {
        left = mLeft;
        top = mTop;
        if( mTexture )
        {
            width = mTexture->getWidth();
            height = mTexture->getHeight();
        }
        else
        {
            float scale = getViewPointToPixelScale();
            width = (uint32)floorf( float( mRequestedWidth ) * scale + 0.5f );
            height = (uint32)floorf( float( mRequestedHeight ) * scale + 0.5f );
        }
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *Window::getTexture() const { return mTexture; }
    //-----------------------------------------------------------------------------------
    TextureGpu *Window::getDepthBuffer() const { return mDepthBuffer; }
    //-----------------------------------------------------------------------------------
    TextureGpu *Window::getStencilBuffer() const { return mStencilBuffer; }
}  // namespace Ogre
