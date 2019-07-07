/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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
#include "OgreMetalMappings.h"
#include "OgreMetalTextureGpuWindow.h"
#include "OgreMetalTextureGpuManager.h"
#include "OgreMetalDevice.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreDepthBuffer.h"
#include "OgreViewport.h"
#include "OgreStringConverter.h"

namespace Ogre
{
    MetalWindow::MetalWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode,
                              const NameValuePairList *miscParams, MetalDevice *ownerDevice ) :
        Window( title, width, height, fullscreenMode ),
        mClosed( false ),
        mHwGamma( true ),
        mMsaa( 1u ),
        mMetalLayer( 0 ),
        mCurrentDrawable( 0 ),
        mDevice( ownerDevice )
    {
        create( fullscreenMode, miscParams );
    }
    //-------------------------------------------------------------------------
    MetalWindow::~MetalWindow()
    {
        destroy();
    }
    //-------------------------------------------------------------------------
    inline void MetalWindow::checkLayerSizeChanges(void)
    {
        // Handle display changes here
        if( mMetalView.layerSizeDidUpdate )
        {
            // set the metal layer to the drawable size in case orientation or size changes
            CGSize drawableSize = CGSizeMake( mMetalView.bounds.size.width,
                                              mMetalView.bounds.size.height );
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS  
            drawableSize.width  *= mMetalView.layer.contentsScale;
            drawableSize.height *= mMetalView.layer.contentsScale;
#else
            NSScreen* screen = mMetalView.window.screen ?: [NSScreen mainScreen];
            drawableSize.width *= screen.backingScaleFactor;
            drawableSize.height *= screen.backingScaleFactor;
#endif
            mMetalLayer.drawableSize = drawableSize;

            // Resize anything if needed
            this->windowMovedOrResized();

            mMetalView.layerSizeDidUpdate = NO;
        }
    }
    //-------------------------------------------------------------------------
    void MetalWindow::setResolutionFromView(void)
    {
        const uint32 newWidth = static_cast<uint32>( mMetalLayer.drawableSize.width );
        const uint32 newHeight = static_cast<uint32>( mMetalLayer.drawableSize.height );

        if( newWidth > 0 && newHeight > 0 && mTexture )
        {
            assert( dynamic_cast<MetalTextureGpuWindow*>( mTexture ) );
            MetalTextureGpuWindow *texWindow = static_cast<MetalTextureGpuWindow*>( mTexture );
            texWindow->_setMsaaBackbuffer( 0 );

            if( mMsaa > 1u )
            {
                MTLTextureDescriptor* desc = [MTLTextureDescriptor
                                             texture2DDescriptorWithPixelFormat:
                                             MetalMappings::get( mTexture->getPixelFormat() )
                                             width: newWidth height: newHeight mipmapped: NO];
                desc.textureType = MTLTextureType2DMultisample;
                desc.sampleCount = mMsaa;
                desc.usage       = MTLTextureUsageRenderTarget;
                desc.storageMode = MTLStorageModePrivate;

                id<MTLTexture> msaaTex = [mDevice->mDevice newTextureWithDescriptor: desc];
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
                mDepthBuffer->_transitionTo( GpuResidency::OnStorage, (uint8*)0 );
                wasResident = true;
            }

            setFinalResolution( newWidth, newHeight );

            if( mDepthBuffer && wasResident )
                mDepthBuffer->_transitionTo( GpuResidency::Resident, (uint8*)0 );
        }
    }
    //-------------------------------------------------------------------------
    void MetalWindow::swapBuffers(void)
    {
        if( !mDevice->mFrameAborted )
        {
            // Schedule a present once rendering to the framebuffer is complete
            const CFTimeInterval presentationTime = mMetalView.presentationTime;

            if( presentationTime < 0 )
            {
                [mDevice->mCurrentCommandBuffer presentDrawable:mCurrentDrawable];
            }
            else
            {
                [mDevice->mCurrentCommandBuffer presentDrawable:mCurrentDrawable
                                                         atTime:presentationTime];
            }
        }
        mCurrentDrawable = 0;
    }
    //-------------------------------------------------------------------------
    void MetalWindow::windowMovedOrResized(void)
    {
        if( mRequestedWidth != mMetalLayer.drawableSize.width ||
            mRequestedHeight != mMetalLayer.drawableSize.height )
        {
            mRequestedWidth  = mMetalLayer.drawableSize.width;
            mRequestedHeight = mMetalLayer.drawableSize.height;

            setResolutionFromView();
        }
    }
    //-------------------------------------------------------------------------
    bool MetalWindow::nextDrawable(void)
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
                    //We're unable to render. Skip frame.
                    //dispatch_semaphore_signal( _inflight_semaphore );
    
                    mDevice->mFrameAborted |= true;
                    isSuccess = false;
                }
                else
                {
                    assert( dynamic_cast<MetalTextureGpuWindow*>( mTexture ) );
                    MetalTextureGpuWindow *texWindow = static_cast<MetalTextureGpuWindow*>( mTexture );

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

        mMsaa       = 1u;
        mHwGamma    = true;

        if( miscParams )
        {
            NameValuePairList::const_iterator opt;
            NameValuePairList::const_iterator end = miscParams->end();

            opt = miscParams->find("MSAA");
            if( opt != end )
                mMsaa = std::max( 1u, StringConverter::parseUnsignedInt( opt->second ) );

            opt = miscParams->find("gamma");
            if( opt != end )
                mHwGamma = StringConverter::parseBool( opt->second );
            
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
            opt = miscParams->find("macAPICocoaUseNSView");
            if( opt != end )
            {
                LogManager::getSingleton().logMessage("Mac Cocoa Window: Rendering on an external plain NSView*");
                opt = miscParams->find("parentWindowHandle");
                NSView *nsview = (__bridge NSView*)reinterpret_cast<void*>(StringConverter::parseSizeT(opt->second));
                assert( nsview &&
                       "Unable to get a pointer to the parent NSView."
                       "Was the 'parentWindowHandle' parameter set correctly in the call to createRenderWindow()?");
                mWindow = [nsview window];
            }
#endif
        }

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        CGRect frame;
#else
        NSRect frame;
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        frame.origin.x = 0;
        frame.origin.y = 0;
        frame.size.width = mRequestedWidth;
        frame.size.height = mRequestedHeight;
#else
        frame = [mWindow.contentView bounds];
#endif
        mMetalView = [[OgreMetalView alloc] initWithFrame:frame];

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        NSView *view = mWindow.contentView;
        [view addSubview:mMetalView];
        mResizeObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSWindowDidResizeNotification object:mWindow queue:nil usingBlock:^(NSNotification *){
          mMetalView.frame = [mWindow.contentView bounds];
        }];
#endif

        mMetalLayer = (CAMetalLayer*)mMetalView.layer;
        mMetalLayer.device      = mDevice->mDevice;
        mMetalLayer.pixelFormat = MetalMappings::get( mHwGamma ? PFG_BGRA8_UNORM_SRGB :
                                                                 PFG_BGRA8_UNORM );

        //This is the default but if we wanted to perform compute
        //on the final rendering layer we could set this to no
        mMetalLayer.framebufferOnly = YES;

        checkLayerSizeChanges();
        setResolutionFromView();

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        //We'll need to refresh as mMetalView.layer.contentsScale is usually out of date.
        mMetalView.layerSizeDidUpdate = YES;
#endif
    }
    //-------------------------------------------------------------------------
    void MetalWindow::destroy()
    {
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        [[NSNotificationCenter defaultCenter] removeObserver:mResizeObserver];
#endif
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
                static_cast<MetalTextureGpuManager*>( textureGpuManager );

        mTexture        = textureManager->createTextureGpuWindow( this );
        mDepthBuffer    = textureManager->createWindowDepthBuffer();

        mTexture->setPixelFormat( mHwGamma ? PFG_BGRA8_UNORM_SRGB : PFG_BGRA8_UNORM );
        mDepthBuffer->setPixelFormat( DepthBuffer::DefaultDepthBufferFormat );

        if( PixelFormatGpuUtils::isStencil( mDepthBuffer->getPixelFormat() ) )
            mStencilBuffer = mDepthBuffer;

        if( mDepthBuffer )
        {
            mTexture->_setDepthBufferDefaults( DepthBuffer::POOL_NON_SHAREABLE,
                                               false, mDepthBuffer->getPixelFormat() );
        }
        else
        {
            mTexture->_setDepthBufferDefaults( DepthBuffer::POOL_NO_DEPTH, false, PFG_NULL );
        }

        mTexture->setMsaa( mMsaa );
        mDepthBuffer->setMsaa( mMsaa );

        setResolutionFromView();
        mTexture->_transitionTo( GpuResidency::Resident, (uint8*)0 );
        mDepthBuffer->_transitionTo( GpuResidency::Resident, (uint8*)0 );
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
        frame.size.width    = width;
        frame.size.height   = height;
        mMetalView.frame = frame;
#else
        mMetalView.frame = [mWindow.contentView bounds];
#endif
        checkLayerSizeChanges();
    }
    //-------------------------------------------------------------------------
    bool MetalWindow::isClosed(void) const
    {
        return mClosed;
    }
    //-------------------------------------------------------------------------
    void MetalWindow::_setVisible( bool visible )
    {
    }
    //-------------------------------------------------------------------------
    bool MetalWindow::isVisible(void) const
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
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        [mWindow setIsMiniaturized:hidden];
#endif
    }
    //-------------------------------------------------------------------------
    bool MetalWindow::isHidden(void) const
    {
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        return mWindow.isMiniaturized;
#else
        return false;
#endif
    }
    //-------------------------------------------------------------------------
    void MetalWindow::getCustomAttribute( IdString name, void* pData )
    {
        if( name == "MetalDevice" )
        {
            *static_cast<MetalDevice**>(pData) = mDevice;
        }
        else if( name == "UIView" )
        {
            *static_cast<void**>(pData) = (void*)CFBridgingRetain( mMetalView );
        }
        else
        {
            Window::getCustomAttribute( name, pData );
        }
    }
}

