/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2016 Torus Knot Software Ltd

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

#include "OgreMetalWindow.h"

#include "OgreDepthBuffer.h"
#include "OgreMetalDevice.h"
#include "OgreMetalMappings.h"
#include "OgreMetalTextureGpuManager.h"
#include "OgreMetalTextureGpuWindow.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreRenderSystem.h"
#include "OgreStringConverter.h"
#include "OgreViewport.h"
#include "OgreWindowEventUtilities.h"

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS

#    import <objc/runtime.h>

@interface MetalWinListener : NSObject

@end

@implementation MetalWinListener

- (Ogre::MetalWindow *)getMetalWindow:(NSNotification *)notification
{
    Ogre::MetalWindow *metalWindow = nullptr;
    NSWindow *cocoaWindow = notification.object;
    NSValue *assoc = objc_getAssociatedObject( cocoaWindow, @selector( windowClosed: ) );
    if( assoc != nil )
    {
        metalWindow = (Ogre::MetalWindow *)assoc.pointerValue;
    }
    return metalWindow;
}

- (void)windowMoved:(NSNotification *)notification
{
    Ogre::MetalWindow *metalWindow = [self getMetalWindow:notification];
    if( metalWindow != nil )
    {
        auto foundRange = Ogre::WindowEventUtilities::_msListeners.equal_range( metalWindow );
        for( auto it = foundRange.first; it != foundRange.second; ++it )
        {
            Ogre::WindowEventListener *listener = it->second;
            listener->windowMoved( metalWindow );
        }
    }
}

- (void)windowResized:(NSNotification *)notification
{
    Ogre::MetalWindow *metalWindow = [self getMetalWindow:notification];
    if( metalWindow != nil )
    {
        auto foundRange = Ogre::WindowEventUtilities::_msListeners.equal_range( metalWindow );
        for( auto it = foundRange.first; it != foundRange.second; ++it )
        {
            Ogre::WindowEventListener *listener = it->second;
            listener->windowResized( metalWindow );
        }
    }
}

- (void)windowClosed:(NSNotification *)notification
{
    Ogre::MetalWindow *metalWindow = [self getMetalWindow:notification];
    if( metalWindow != nil )
    {
        auto foundRange = Ogre::WindowEventUtilities::_msListeners.equal_range( metalWindow );
        for( auto it = foundRange.first; it != foundRange.second; ++it )
        {
            Ogre::WindowEventListener *listener = it->second;
            listener->windowClosed( metalWindow );
        }
    }
}
@end

static void SetupMetalWindowListeners( Ogre::MetalWindow *metalWindow, NSWindow *cocoaWindow )
{
    static MetalWinListener *listener = [[MetalWinListener alloc] init];

    // Make it possible to find the MetalWindow from the NSWindow
    objc_setAssociatedObject( cocoaWindow, @selector( windowClosed: ),
                              [NSValue valueWithPointer:metalWindow], OBJC_ASSOCIATION_RETAIN );

    [NSNotificationCenter.defaultCenter addObserver:listener
                                           selector:@selector( windowClosed: )
                                               name:NSWindowWillCloseNotification
                                             object:cocoaWindow];

    [NSNotificationCenter.defaultCenter addObserver:listener
                                           selector:@selector( windowMoved: )
                                               name:NSWindowDidMoveNotification
                                             object:cocoaWindow];

    [NSNotificationCenter.defaultCenter addObserver:listener
                                           selector:@selector( windowResized: )
                                               name:NSWindowDidResizeNotification
                                             object:cocoaWindow];
}
#endif

namespace Ogre
{
    MetalWindow::MetalWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode,
                              const NameValuePairList *miscParams, MetalDevice *ownerDevice ) :
        Window( title, width, height, fullscreenMode ),
        mClosed( false ),
        mHidden( false ),
        mIsExternal( false ),
        mHwGamma( true ),
        mManualRelease( false ),
        mMetalLayer( 0 ),
        mCurrentDrawable( 0 ),
        mDevice( ownerDevice )
    {
        create( fullscreenMode, miscParams );
    }
    //-------------------------------------------------------------------------
    MetalWindow::~MetalWindow() { destroy(); }
    //-------------------------------------------------------------------------
    float MetalWindow::getViewPointToPixelScale() const
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        return (float)mMetalLayer.contentsScale;
#else
        NSScreen *screen = mMetalView.window.screen ?: [NSScreen mainScreen];
        return (float)screen.backingScaleFactor;
#endif
    }
    //-------------------------------------------------------------------------
    inline void MetalWindow::checkLayerSizeChanges( void )
    {
        // Handle display changes here
        if( mMetalView.layerSizeDidUpdate )
        {
            // Resize anything if needed
            this->windowMovedOrResized();

            mMetalView.layerSizeDidUpdate = NO;
        }
    }
    //-------------------------------------------------------------------------
    void MetalWindow::setResolutionFromView( void )
    {
        // update drawable size to match layer and thus view size
        float scale = getViewPointToPixelScale();
        CGSize sizePt = mMetalLayer.frame.size;
        const uint32 widthPx = std::max( 1u, (uint32)floor( sizePt.width * scale + 0.5 ) );
        const uint32 heightPx = std::max( 1u, (uint32)floor( sizePt.height * scale + 0.5 ) );
        mMetalLayer.drawableSize = CGSizeMake( widthPx, heightPx );

        if( mTexture )
        {
            assert( dynamic_cast<MetalTextureGpuWindow *>( mTexture ) );
            MetalTextureGpuWindow *texWindow = static_cast<MetalTextureGpuWindow *>( mTexture );
            texWindow->_setMsaaBackbuffer( 0 );

            if( isMultisample() )
            {
                MTLTextureDescriptor *desc = [MTLTextureDescriptor
                    texture2DDescriptorWithPixelFormat:MetalMappings::get( mTexture->getPixelFormat(),
                                                                           mDevice )
                                                 width:widthPx
                                                height:heightPx
                                             mipmapped:NO];
                desc.textureType = MTLTextureType2DMultisample;
                desc.sampleCount = mSampleDescription.getColourSamples();
                desc.usage = MTLTextureUsageRenderTarget;
                desc.storageMode = MTLStorageModePrivate;

                id<MTLTexture> msaaTex = [mDevice->mDevice newTextureWithDescriptor:desc];
                if( !msaaTex )
                {
                    OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                                 "Out of GPU memory or driver refused.",
                                 "MetalWindow::setResolutionFromView" );
                }
                texWindow->_setMsaaBackbuffer( msaaTex );
            }

            bool wasResident = false;
            if( mDepthBuffer && mDepthBuffer->getResidencyStatus() != GpuResidency::OnStorage )
            {
                mDepthBuffer->_transitionTo( GpuResidency::OnStorage, (uint8 *)0 );
                wasResident = true;
            }

            setFinalResolution( widthPx, heightPx );

            if( mDepthBuffer && wasResident )
                mDepthBuffer->_transitionTo( GpuResidency::Resident, (uint8 *)0 );
        }
    }
    //-------------------------------------------------------------------------
    void MetalWindow::swapBuffers()
    {
        if( !mDevice->mFrameAborted )
        {
            // Schedule a present once rendering to the framebuffer is complete
            const CFTimeInterval presentationTime = mMetalView.presentationTime;

            if( mMetalLayer.presentsWithTransaction )
            {
                id<MTLCommandBuffer> commandBuffer = mDevice->mCurrentCommandBuffer;
                mDevice->commitAndNextCommandBuffer();
                [commandBuffer waitUntilScheduled];
                [mCurrentDrawable present];
            }
            else if( presentationTime < 0 )
            {
                [mDevice->mCurrentCommandBuffer presentDrawable:mCurrentDrawable];
            }
            else
            {
                [mDevice->mCurrentCommandBuffer presentDrawable:mCurrentDrawable
                                                         atTime:presentationTime];
            }
        }

        if( !mManualRelease )
            mCurrentDrawable = 0;
    }
    //-------------------------------------------------------------------------
    void MetalWindow::windowMovedOrResized( void )
    {
        CGSize sizePt = mMetalLayer.frame.size;
        if( mRequestedWidth != (uint32)sizePt.width || mRequestedHeight != (uint32)sizePt.height )
        {
            // Resolution is a float. How...?
            mRequestedWidth = (uint32)sizePt.width;
            mRequestedHeight = (uint32)sizePt.height;

            setResolutionFromView();
        }
    }
    //-------------------------------------------------------------------------
    void MetalWindow::setManualSwapRelease( bool bManualRelease ) { mManualRelease = bManualRelease; }
    //-------------------------------------------------------------------------
    bool MetalWindow::isManualSwapRelease() const { return mManualRelease; }
    //-------------------------------------------------------------------------
    void MetalWindow::performManualRelease()
    {
        OGRE_ASSERT_LOW( mManualRelease );

        if( mManualRelease )
        {
            mCurrentDrawable = 0;

            OGRE_ASSERT_HIGH( dynamic_cast<MetalTextureGpuWindow *>( mTexture ) );
            MetalTextureGpuWindow *texWindow = static_cast<MetalTextureGpuWindow *>( mTexture );
            texWindow->_setBackbuffer( 0 );
        }
    }
    //-------------------------------------------------------------------------
    void MetalWindow::setWantsToDownload( bool bWantsToDownload )
    {
        mMetalLayer.framebufferOnly = bWantsToDownload ? NO : YES;
    }
    //-------------------------------------------------------------------------
    bool MetalWindow::canDownloadData() const
    {
        return mCurrentDrawable != 0 && !mMetalLayer.framebufferOnly;
    }
    //-------------------------------------------------------------------------
    bool MetalWindow::nextDrawable( void )
    {
        bool isSuccess = true;

        @autoreleasepool
        {
            if( !mCurrentDrawable )
            {
                if( mMetalView.layerSizeDidUpdate )
                    checkLayerSizeChanges();

                // do not retain current drawable beyond the frame.
                // There should be no strong references to this object outside of this view class
                mCurrentDrawable = [mMetalLayer nextDrawable];
                if( !mCurrentDrawable )
                {
                    LogManager::getSingleton().logMessage( "Metal ERROR: Failed to get a drawable!",
                                                           LML_CRITICAL );
                    // We're unable to render. Skip frame.
                    // dispatch_semaphore_signal( _inflight_semaphore );

                    mDevice->mFrameAborted |= true;
                    isSuccess = false;
                }
                else
                {
                    assert( dynamic_cast<MetalTextureGpuWindow *>( mTexture ) );
                    MetalTextureGpuWindow *texWindow = static_cast<MetalTextureGpuWindow *>( mTexture );

                    texWindow->_setBackbuffer( mCurrentDrawable.texture );
                }
            }
        }

        return isSuccess;
    }
    //-------------------------------------------------------------------------
    void MetalWindow::create( bool fullScreen, const NameValuePairList *miscParams )
    {
        destroy();

        mClosed = false;
        mHidden = false;
        mHwGamma = true;
        NSObject *externalWindowHandle;  // OgreMetalView, NSView or NSWindow
        bool presentsWithTransaction = false;

        if( miscParams )
        {
            NameValuePairList::const_iterator opt;
            NameValuePairList::const_iterator endt = miscParams->end();

            opt = miscParams->find( "hidden" );
            if( opt != endt )
                mHidden = StringConverter::parseBool( opt->second );

            opt = miscParams->find( "FSAA" );
            if( opt != endt )
                mRequestedSampleDescription.parseString( opt->second );

            opt = miscParams->find( "gamma" );
            if( opt != endt )
                mHwGamma = StringConverter::parseBool( opt->second );

            opt = miscParams->find( "externalWindowHandle" );
            if( opt != endt )
                externalWindowHandle =
                    (__bridge NSObject *)(void *)StringConverter::parseSizeT( opt->second );

            if( !externalWindowHandle )
            {
                opt = miscParams->find( "parentWindowHandle" );
                if( opt != endt )
                    externalWindowHandle =
                        (__bridge NSObject *)(void *)StringConverter::parseSizeT( opt->second );
            }

            opt = miscParams->find( "presentsWithTransaction" );
            if( opt != endt )
                presentsWithTransaction = StringConverter::parseBool( opt->second );
        }

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        if( externalWindowHandle && [externalWindowHandle isKindOfClass:[OgreMetalView class]] )
        {
            mMetalView = (OgreMetalView *)externalWindowHandle;
            mIsExternal = true;
        }
        else
        {
            CGRect frame = CGRectMake( 0.0, 0.0, mRequestedWidth, mRequestedHeight );
            mMetalView = [[OgreMetalView alloc] initWithFrame:frame];
        }
#else
        // create window if nothing was provided
        if( !externalWindowHandle )
        {
            NSRect frame = NSMakeRect( 0.0, 0.0, mRequestedWidth, mRequestedHeight );
            if( mRequestedWidth == 1 )
            {
                // just make it big enough to see
                frame.size.width = frame.size.height = 500.0;
            }
            NSWindowStyleMask style =
                NSWindowStyleMaskResizable | NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
            if( mRequestedFullscreenMode )
            {
                frame.size = NSScreen.mainScreen.visibleFrame.size;
                style = NSWindowStyleMaskBorderless;
            }
            NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                                           styleMask:style
                                                             backing:NSBackingStoreBuffered
                                                               defer:YES];
            window.title = @( mTitle.c_str() );
            window.contentView = [[OgreMetalView alloc] initWithFrame:frame];

            externalWindowHandle = window;
            SetupMetalWindowListeners( this, window );
            if( !mHidden )
                [window orderFront:nil];
        }
        else
        {
            mIsExternal = true;
        }

        NSView *externalView;
        if( [externalWindowHandle isKindOfClass:[NSWindow class]] )
        {
            mWindow = (NSWindow *)externalWindowHandle;
            externalView = mWindow.contentView;
        }
        else
        {
            assert( [externalWindowHandle isKindOfClass:[NSView class]] );
            externalView = (NSView *)externalWindowHandle;
            mWindow = externalView.window;
        }

        if( [externalView isKindOfClass:[OgreMetalView class]] )
            mMetalView = (OgreMetalView *)externalView;
        else
        {
            NSRect frame = externalView.bounds;
            mMetalView = [[OgreMetalView alloc] initWithFrame:frame];
            mMetalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
            [externalView addSubview:mMetalView];
        }
#endif

        mMetalLayer = (CAMetalLayer *)mMetalView.layer;
        mMetalLayer.device = mDevice->mDevice;
        mMetalLayer.pixelFormat =
            MetalMappings::get( mHwGamma ? PFG_BGRA8_UNORM_SRGB : PFG_BGRA8_UNORM, mDevice );

        // This is the default but if we wanted to perform compute
        // on the final rendering layer we could set this to no
        mMetalLayer.framebufferOnly = YES;

        mMetalLayer.presentsWithTransaction = presentsWithTransaction;

        checkLayerSizeChanges();
        setResolutionFromView();

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        // We'll need to refresh as mMetalView.layer.contentsScale is usually out of date.
        mMetalView.layerSizeDidUpdate = YES;
#endif
    }
    //-------------------------------------------------------------------------
    void MetalWindow::destroy()
    {
        mClosed = true;

        OGRE_DELETE mTexture;
        mTexture = 0;

        OGRE_DELETE mDepthBuffer;
        mDepthBuffer = 0;
        mStencilBuffer = 0;

        mMetalLayer = 0;
        mCurrentDrawable = 0;
        mMetalView = 0;
    }
    //-----------------------------------------------------------------------------------
    void MetalWindow::_initialize( TextureGpuManager *textureGpuManager )
    {
        MetalTextureGpuManager *textureManager =
            static_cast<MetalTextureGpuManager *>( textureGpuManager );

        mTexture = textureManager->createTextureGpuWindow( this );
        if( DepthBuffer::DefaultDepthBufferFormat != PFG_NULL )
            mDepthBuffer = textureManager->createWindowDepthBuffer();

        mTexture->setPixelFormat( mHwGamma ? PFG_BGRA8_UNORM_SRGB : PFG_BGRA8_UNORM );
        if( mDepthBuffer )
        {
            mDepthBuffer->setPixelFormat( DepthBuffer::DefaultDepthBufferFormat );

            if( PixelFormatGpuUtils::isStencil( mDepthBuffer->getPixelFormat() ) )
                mStencilBuffer = mDepthBuffer;
        }

        if( mDepthBuffer )
        {
            mTexture->_setDepthBufferDefaults( DepthBuffer::NO_POOL_EXPLICIT_RTV, false,
                                               mDepthBuffer->getPixelFormat() );
        }
        else
        {
            mTexture->_setDepthBufferDefaults( DepthBuffer::POOL_NO_DEPTH, false, PFG_NULL );
        }

        // We create our MSAA buffer and it is not accessible, thus NotTexture.
        // Same for our depth buffer.
        mSampleDescription = textureManager->getRenderSystem()->validateSampleDescription(
            mRequestedSampleDescription, mTexture->getPixelFormat(), TextureFlags::NotTexture,
            TextureFlags::NotTexture );
        mTexture->_setSampleDescription( mRequestedSampleDescription, mSampleDescription );
        if( mDepthBuffer )
            mDepthBuffer->_setSampleDescription( mRequestedSampleDescription, mSampleDescription );

        setResolutionFromView();
        mTexture->_transitionTo( GpuResidency::Resident, (uint8 *)0 );
        if( mDepthBuffer )
            mDepthBuffer->_transitionTo( GpuResidency::Resident, (uint8 *)0 );
    }
    //-------------------------------------------------------------------------
    void MetalWindow::reposition( int32 left, int32 top )
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        CGRect frame = mMetalView.frame;
        frame.origin.x = left;
        frame.origin.y = top;
        mMetalView.frame = frame;
#else
        mMetalView.frame = [mWindow.contentView bounds];
        checkLayerSizeChanges();
#endif
    }
    //-------------------------------------------------------------------------
    void MetalWindow::requestResolution( uint32 width, uint32 height )
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        CGRect frame = mMetalView.frame;
        frame.size.width = width;
        frame.size.height = height;
        mMetalView.frame = frame;
#else
        mMetalView.frame = [mWindow.contentView bounds];
#endif
        checkLayerSizeChanges();
    }
    //-------------------------------------------------------------------------
    bool MetalWindow::isClosed( void ) const { return mClosed; }
    //-------------------------------------------------------------------------
    void MetalWindow::_setVisible( bool visible ) {}
    //-------------------------------------------------------------------------
    bool MetalWindow::isVisible( void ) const
    {
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        return mWindow.isVisible;
#else
        return mMetalView && mMetalView.window;
#endif
    }
    //-------------------------------------------------------------------------
    void MetalWindow::setHidden( bool hidden )
    {
        mHidden = hidden;
        if( !mIsExternal )
        {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            [mMetalView.window setHidden:hidden];
#else
            if( hidden )
                [mWindow orderOut:nil];
            else
                [mWindow makeKeyAndOrderFront:nil];
#endif
        }
    }
    //-------------------------------------------------------------------------
    bool MetalWindow::isHidden( void ) const { return mHidden; }
    //-------------------------------------------------------------------------
    void MetalWindow::getCustomAttribute( IdString name, void *pData )
    {
        if( name == "MetalDevice" )
        {
            *static_cast<MetalDevice **>( pData ) = mDevice;
        }
        else if( name == "UIView" )
        {
            *static_cast<const void **>( pData ) = (const void *)CFBridgingRetain( mMetalView );
        }
        else
        {
            Window::getCustomAttribute( name, pData );
        }
    }
}
