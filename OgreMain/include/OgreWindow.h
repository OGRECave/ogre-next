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

#ifndef _OgreWindow_H_
#define _OgreWindow_H_

#include "OgreTextureGpu.h"

#include "Vao/OgreBufferPacked.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    class _OgreExport Window : public RenderSysAlloc
    {
    protected:
        String      mTitle;
        TextureGpu  *mTexture;
        TextureGpu  *mDepthBuffer;
        TextureGpu  *mStencilBuffer;

        /** 0/0 is legal and will be interpreted as 0/1.
            0/anything is interpreted as zero.
            If you are representing a whole number, the denominator should be 1.
        */
        uint32  mFrequencyNumerator;
        uint32  mFrequencyDenominator;

        uint32  mRequestedWidth;
        uint32  mRequestedHeight;

        bool    mFullscreenMode;
        bool    mRequestedFullscreenMode;
        bool    mBorderless;

        bool    mFocused;
        bool    mIsPrimary;

        bool    mVSync;
        uint32  mVSyncInterval;

        int32 mLeft;
        int32 mTop;

        void setFinalResolution( uint32 width, uint32 height );

    public:
        Window( const String &title, uint32 width, uint32 height, bool fullscreenMode );
        virtual ~Window();

        virtual void destroy(void) = 0;

        virtual void setTitle( const String &title );
        const String& getTitle(void) const;

        virtual void reposition( int32 left, int32 top ) = 0;

        /// Requests a change in resolution. Change is not immediate.
        /// Use getRequestedWidth & getRequestedHeight if you need to know
        /// what you've requested, but beware you may not get that resolution,
        /// and once we get word from the OS, getRequestedWidth/Height will
        /// change again so that getWidth == getRequestedWidth.
        virtual void requestResolution( uint32 width, uint32 height );

        /** Requests to toggle between fullscreen and windowed mode.
        @remarks
            Use wantsToGoFullscreen & wantsToGoWindowed to know what you've requested.
            Same remarks as requestResolution apply:
                If we request to go fullscreen, wantsToGoFullscreen will return true.
                But if get word from OS saying we stay in windowed mode,
                wantsToGoFullscreen will start returning false.
        @param goFullscreen
            True to go fullscreen, false to go windowed mode.
        @param borderless
            Whether to be borderless. Only useful if goFullscreen == false;
        @param monitorIdx
        @param width
            New width. Leave 0 if you don't care.
        @param height
            New height. Leave 0 if you don't care.
        @param frequencyNumerator
            New frequency (fullscreen only). Leave 0 if you don't care.
        @param frequencyDenominator
            New frequency (fullscreen only). Leave 0 if you don't care.
        */
        virtual void requestFullscreenSwitch( bool goFullscreen, bool borderless, uint32 monitorIdx,
                                              uint32 width, uint32 height,
                                              uint32 frequencyNumerator, uint32 frequencyDenominator );

        virtual void setVSync( bool vSync, uint32 vSyncInterval );
        bool getVSync(void) const;
        uint32 getVSyncInterval(void) const;

        virtual void setBorderless( bool borderless );
        bool getBorderless(void) const;

        uint32 getWidth(void) const;
        uint32 getHeight(void) const;
        PixelFormatGpu getPixelFormat(void) const;
        uint8 getMsaa(void) const;
        MsaaPatterns::MsaaPatterns getMsaaPatterns(void) const;

        uint32 getFrequencyNumerator(void) const;
        uint32 getFrequencyDenominator(void) const;

        uint32 getRequestedWidth(void) const;
        uint32 getRequestedHeight(void) const;

        /// Returns true if we are currently in fullscreen mode.
        bool isFullscreen(void) const;
        /// Returns true if we are in windowed mode right now, but want to go fullscreen.
        bool wantsToGoFullscreen(void) const;
        /// Returns true if we are in fullscreen mode right now, but want to go windowed mode.
        bool wantsToGoWindowed(void) const;

        /** Notify that the window has been resized
        @remarks
            You don't need to call this unless you created the window externally.
        */
        virtual void windowMovedOrResized(void) {}

        /// Indicates whether the window has been closed by the user.
        virtual bool isClosed(void) const = 0;

        /// Internal method to notify the window it has been obscured or minimized
        virtual void _setVisible( bool visible ) = 0;
        ////Indicates whether the window is visible (not minimized or fully obscured)
        virtual bool isVisible(void) const = 0;

        /** Hide (or show) the window. If called with hidden=true, this
            will make the window completely invisible to the user.
        @remarks
            Setting a window to hidden is useful to create a dummy primary
            RenderWindow hidden from the user so that you can create and
            recreate your actual RenderWindows without having to recreate
            all your resources.
        */
        virtual void setHidden( bool hidden ) = 0;
        /// Indicates whether the window was set to hidden (not displayed)
        virtual bool isHidden(void) const = 0;

        virtual void setFocused( bool focused );
        bool isFocused(void) const;

        /// Indicates that this is the primary window.
        /// Only to be called by Ogre::Root
        void _setPrimary(void);
        bool isPrimary(void) const;

        virtual void _initialize( TextureGpuManager *textureGpuManager ) = 0;

        /// Overloaded version of getMetrics from RenderTarget, including extra details
        /// specific to windowing systems. Result is in pixels.
        virtual void getMetrics( uint32 &width, uint32 &height, int32 &left, int32 &top ) const;

        /// WARNING: Attempting to change the TextureGpu (e.g. setResolution, setPixelFormat)
        /// is undefined behavior
        TextureGpu* getTexture(void) const;
        TextureGpu* getDepthBuffer(void) const;
        TextureGpu* getStencilBuffer(void) const;

        virtual void getCustomAttribute( IdString name, void* pData ) {}

        virtual void swapBuffers(void) = 0;
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
