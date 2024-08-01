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
    OGRE_ASSUME_NONNULL_BEGIN

    class _OgreExport Window : public OgreAllocatedObj
    {
    protected:
        String                    mTitle;
        TextureGpu *ogre_nullable mTexture;
        TextureGpu *ogre_nullable mDepthBuffer;
        TextureGpu *ogre_nullable mStencilBuffer;

        /** 0/0 is legal and will be interpreted as 0/1.
            0/anything is interpreted as zero.
            If you are representing a whole number, the denominator should be 1.
        */
        uint32 mFrequencyNumerator;
        uint32 mFrequencyDenominator;

        uint32 mRequestedWidth;   // in view points
        uint32 mRequestedHeight;  // in view points

        SampleDescription mRequestedSampleDescription;  // requested FSAA mode
        SampleDescription mSampleDescription;  // effective FSAA mode, limited by hardware capabilities

        bool mFullscreenMode;
        bool mRequestedFullscreenMode;
        bool mBorderless;

        bool mFocused;
        bool mIsPrimary;

        bool   mVSync;
        uint32 mVSyncInterval;

        int32 mLeft;  // in pixels
        int32 mTop;   // in pixels

        void setFinalResolution( uint32 widthPx, uint32 heightPx );

    public:
        Window( const String &title, uint32 widthPt, uint32 heightPt, bool fullscreenMode );
        virtual ~Window();

        virtual void destroy() = 0;

        virtual void  setTitle( const String &title );
        const String &getTitle() const;

        /** Many windowing systems that support HiDPI displays use special points to specify
            size of the windows and controls, so that windows and controls with hardcoded
            sizes does not become too small on HiDPI displays. Such points have constant density
            ~ 100 points per inch (probably 96 on Windows and 72 on Mac), that is independent
            of pixel density of real display, and are used through the all windowing system.

            Sometimes, such view points are choosen bigger for output devices that are viewed
            from larger distances, like 30" TV comparing to 30" monitor, therefore maintaining
            constant points angular density rather than constant linear density.

            In any case, all such windowing system provides the way to convert such view points
            to pixels, be it DisplayProperties::LogicalDpi on WinRT or backingScaleFactor on MacOSX.
            We use pixels consistently through the Ogre, but window/view management functions
            takes view points for convenience, as does the rest of windowing system. Such parameters
            are named using xxxxPt pattern, and should not be mixed with pixels without being
            converted using getViewPointToPixelScale() function.

            Sometimes such scale factor can change on-the-fly, for example if window is dragged
            to monitor with different DPI. In such situation, window size in view points is usually
            preserved by windowing system, and Ogre should adjust pixel size of RenderWindow.
        */
        virtual float getViewPointToPixelScale() const { return 1.0f; }

        virtual void reposition( int32 leftPt, int32 topPt ) = 0;

        /// Requests a change in resolution. Change is not immediate.
        /// Use getRequestedWidthPt & getRequestedHeightPt if you need to know
        /// what you've requested, but beware you may not get that resolution,
        /// and once we get word from the OS, getRequested{Width/Height}Pt will
        /// change again so that getWidth == getRequestedWidthPt * getViewPointToPixelScale.
        virtual void requestResolution( uint32 widthPt, uint32 heightPt );

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
        @param widthPt
            New width. Leave 0 if you don't care.
        @param heightPt
            New height. Leave 0 if you don't care.
        @param frequencyNumerator
            New frequency (fullscreen only). Leave 0 if you don't care.
        @param frequencyDenominator
            New frequency (fullscreen only). Leave 0 if you don't care.
        */
        virtual void requestFullscreenSwitch( bool goFullscreen, bool borderless, uint32 monitorIdx,
                                              uint32 widthPt, uint32 heightPt, uint32 frequencyNumerator,
                                              uint32 frequencyDenominator );

        /** Turns VSync on/off
        @param vSync
        @param vSyncInterval
            When true, specifies how often the screen should be updated.
            e.g. at 60hz:
                vSyncInterval = 1 then update at 60hz
                vSyncInterval = 2 then update at 30hz
                vSyncInterval = 3 then update at 15hz
                vSyncInterval = 4 then update at 7.5hz

            If the 31st bit is set, i.e. 0x80000000, then lowest latency mode, aka mailbox, will
            be used (which doesn't limit the framerate)
        */
        virtual void setVSync( bool vSync, uint32 vSyncInterval );
        bool         getVSync() const;
        uint32       getVSyncInterval() const;

        virtual void setBorderless( bool borderless );
        bool         getBorderless() const;

        /** Metal doesn't want us to hold on to a drawable after presenting

            If you want to take a screenshot capture of the window this is a
            problem because we no longer have a pointer of the backbuffer to
            download from.

            You can either take your screenshot before swapBuffers() gets called,
            or, if you intend to take a screenshot, do the following:

            @code
                window->setWantsToDownload( true );
                window->setManualSwapRelease( true );
                mRoot->renderOneFrame();

                if( window->canDownloadData() )
                {
                    Ogre::Image2 img;
                    Ogre::TextureGpu *texture = window->getTexture();
                    img.convertFromTexture( texture, 0u, texture->getNumMipmaps() - 1u );
                }

                window->performManualRelease();
                window->setManualSwapRelease( false );
            @endcode

            Technically you can do setManualSwapRelease( true ) and leave it like that,
            but then you MUST call performManualRelease and that's bug prone.

        @remarks
            Incorrect usage of this functionality may result in crashes or leaks

            Alternatively you can avoid setManualSwapRelease by taking pictures
            before calling Window::swapBuffers.

            To do that use FrameListener::frameRenderingQueued listener,
            *but* you still have to call setWantsToDownload( true ) and
            check canDownloadData returns true.
        */
        virtual void setManualSwapRelease( bool bManualRelease );

        /// Returns the value set by setManualSwapRelease when supported
        virtual bool isManualSwapRelease() const { return false; }

        /// See Window::setManualSwapRelease
        virtual void performManualRelease();

        /// On Metal you must call this function and set it to true in order
        /// to take pictures.
        ///
        /// If you no longer need that functionality,
        /// set it to false to improve performance
        virtual void setWantsToDownload( bool bWantsToDownload );

        /// Returns true if you can download to CPU (i.e. transfer it via AsyncTextureTicket)
        /// If it returns false, attempting to do so could result in a crash
        ///
        /// See Window::setWantsToDownload
        /// See Window::setManualSwapRelease
        virtual bool canDownloadData() const;

        uint32 getWidth() const;
        uint32 getHeight() const;

        PixelFormatGpu getPixelFormat() const;

        /** Set the FSAA mode to be used if hardware support it.
            This option will be ignored if the hardware does not support it
            or setting can not be changed on the fly on per-target level.
            @param fsaa Requesed FSAA mode (@see Root::createRenderWindow)
        */
        virtual void      setFsaa( const String &fsaa ) {}
        SampleDescription getSampleDescription() const;
        bool              isMultisample() const;

        uint32 getFrequencyNumerator() const;
        uint32 getFrequencyDenominator() const;

        uint32 getRequestedWidthPt() const;
        uint32 getRequestedHeightPt() const;

        /// Returns true if we are currently in fullscreen mode.
        bool isFullscreen() const;
        /// Returns true if we are in windowed mode right now, but want to go fullscreen.
        bool wantsToGoFullscreen() const;
        /// Returns true if we are in fullscreen mode right now, but want to go windowed mode.
        bool wantsToGoWindowed() const;

        /** Notify that the window has been resized
        @remarks
            You don't need to call this unless you created the window externally.
        */
        virtual void windowMovedOrResized() {}

        /// Indicates whether the window has been closed by the user.
        virtual bool isClosed() const = 0;

        /// Internal method to notify the window it has been obscured or minimized
        virtual void _setVisible( bool visible ) = 0;
        ////Indicates whether the window is visible (not minimized or fully obscured)
        virtual bool isVisible() const = 0;

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
        virtual bool isHidden() const = 0;

        virtual void setFocused( bool focused );
        bool         isFocused() const;

        /// Indicates that this is the primary window.
        /// Only to be called by Ogre::Root
        void _setPrimary();
        bool isPrimary() const;

        virtual void _initialize( TextureGpuManager *textureGpuManager ) = 0;

        /// Overloaded version of getMetrics from RenderTarget, including extra details
        /// specific to windowing systems. Result is in pixels.
        virtual void getMetrics( uint32 &width, uint32 &height, int32 &left, int32 &top ) const;

        /// WARNING: Attempting to change the TextureGpu (e.g. setResolution, setPixelFormat)
        /// is undefined behavior
        TextureGpu *ogre_nullable getTexture() const;
        TextureGpu *ogre_nullable getDepthBuffer() const;
        TextureGpu *ogre_nullable getStencilBuffer() const;

        virtual void getCustomAttribute( IdString name, void *pData ) {}

        virtual void swapBuffers() = 0;
    };

    OGRE_ASSUME_NONNULL_END
    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
