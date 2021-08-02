/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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

#import "OgreOSXCocoaWindow.h"
#import "OgreRoot.h"
#import "OgreLogManager.h"
#import "OgreStringConverter.h"
#import "OgreWindowEventUtilities.h"
#import "OgreGL3PlusPixelFormat.h"
#import "OgrePixelBox.h"

#import "OgreGL3PlusRenderSystem.h"
#import <AppKit/NSScreen.h>
#import <AppKit/NSOpenGLView.h>
#import <QuartzCore/CVDisplayLink.h>
#import "OgreViewport.h"
#import <iomanip>

@implementation OgreGL3PlusWindow

- (BOOL)canBecomeKeyWindow
{
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

@end

namespace Ogre {
    
    struct NSOpenGLContextGuard
    {
        NSOpenGLContextGuard(NSOpenGLContext* ctx) : mPrevContext([NSOpenGLContext currentContext]) { if(ctx != mPrevContext) [ctx makeCurrentContext]; }
        ~NSOpenGLContextGuard() { [mPrevContext makeCurrentContext]; }
    private:
         NSOpenGLContext *mPrevContext;
    };


    CocoaWindow::CocoaWindow() : mWindow(nil), mView(nil), mGLContext(nil), mGLPixelFormat(nil), mWindowOriginPt(NSZeroPoint),
        mWindowDelegate(NULL), mActive(false), mClosed(false), mHidden(false), mVSync(true), mHasResized(false), mIsExternal(false), mWindowTitle(""),
        mUseOgreGLView(true), mContentScalingFactor(1.0)
    {
        // Set vsync by default to save battery and reduce tearing
    }

    CocoaWindow::~CocoaWindow()
    {
		[mGLContext clearDrawable];

        destroy();
        
        if(mView && mUseOgreGLView)
        {
            [(OgreGL3PlusView*)mView setOgreWindow:NULL];
        }
    }
	
	void CocoaWindow::create(const String& name, unsigned int widthPt, unsigned int heightPt,
	            bool fullScreen, const NameValuePairList *miscParams)
    {
        @autoreleasepool {
            NSApplicationLoad();

            /*
             ***Key: "title" Description: The title of the window that will appear in the title bar
             Values: string Default: RenderTarget name
             
             ***Key: "colourDepth" Description: Colour depth of the resulting rendering window;
             only applies if fullScreen is set. Values: 16 or 32 Default: desktop depth Notes: [W32 specific]
             
             ***Key: "left" Description: screen x coordinate from left Values: positive integers
             Default: 'center window on screen' Notes: Ignored in case of full screen
             
             ***Key: "top" Description: screen y coordinate from top Values: positive integers
             Default: 'center window on screen' Notes: Ignored in case of full screen
             
             ***Key: "depthBuffer" [DX9 specific] Description: Use depth buffer Values: false or true Default: true
             
             ***Key: "externalWindowHandle" [API specific] Description: External window handle, for embedding the
             OGRE context Values: positive integer for W32 (HWND handle) poslong:posint:poslong (display*:screen:windowHandle)
             or poslong:posint:poslong:poslong (display*:screen:windowHandle:XVisualInfo*) for GLX Default: 0 (None)
             
             ***Key: "FSAA" Description: Full screen antialiasing factor Values: 0,2,4,6,... Default: 0
             
             ***Key: "displayFrequency" Description: Display frequency rate, for fullscreen mode Values: 60...?
             Default: Desktop vsync rate
             
             ***Key: "vsync" Description: Synchronize buffer swaps to vsync Values: true, false Default: 0
             */
            
            BOOL hasDepthBuffer = YES;
            int fsaa_samples = 0;
            bool hidden = false;
            NSString *windowTitle = [NSString stringWithCString:name.c_str() encoding:NSUTF8StringEncoding];
            int winxPt = 0, winyPt = 0;
            int colourDepth = 32;
            NSOpenGLContext *externalGLContext = nil;
            NSObject* externalWindowHandle = nil; // NSOpenGLView, NSView or NSWindow
            NameValuePairList::const_iterator opt;
            bool useCurrentGLContext = false;

            mIsFullScreen = fullScreen;
            
            if(miscParams)
            {
                opt = miscParams->find("title");
                if(opt != miscParams->end())
                    windowTitle = [NSString stringWithCString:opt->second.c_str() encoding:NSUTF8StringEncoding];
                
                opt = miscParams->find("left");
                if(opt != miscParams->end())
                    winxPt = StringConverter::parseUnsignedInt(opt->second);
                
                opt = miscParams->find("top");
                if(opt != miscParams->end())
                    winyPt = (int)NSHeight([[NSScreen mainScreen] frame]) - StringConverter::parseUnsignedInt(opt->second) - heightPt;
                
                opt = miscParams->find("hidden");
                if (opt != miscParams->end())
                    hidden = StringConverter::parseBool(opt->second);
                
                opt = miscParams->find("depthBuffer");
                if(opt != miscParams->end())
                    hasDepthBuffer = StringConverter::parseBool(opt->second);
                
                opt = miscParams->find("FSAA");
                if(opt != miscParams->end())
                    fsaa_samples = StringConverter::parseUnsignedInt(opt->second);
                
                opt = miscParams->find("gamma");
                if(opt != miscParams->end())
                    mHwGamma = StringConverter::parseBool(opt->second);
                
                opt = miscParams->find("vsync");
                if(opt != miscParams->end())
                    mVSync = StringConverter::parseBool(opt->second);
                
                opt = miscParams->find("colourDepth");
                if(opt != miscParams->end())
                    colourDepth = StringConverter::parseUnsignedInt(opt->second);
                
                opt = miscParams->find("Full Screen");
                if(opt != miscParams->end())
                    fullScreen = StringConverter::parseBool(opt->second);
                
                opt = miscParams->find("contentScalingFactor");
                if(opt != miscParams->end())
                    mContentScalingFactor = StringConverter::parseReal(opt->second);
                
                opt = miscParams->find("currentGLContext");
                if(opt != miscParams->end())
                    useCurrentGLContext = StringConverter::parseBool(opt->second);
                
                opt = miscParams->find("externalGLContext");
                if(opt != miscParams->end())
                    externalGLContext = (__bridge NSOpenGLContext *)(void*)StringConverter::parseSizeT(opt->second);
                
                opt = miscParams->find("externalWindowHandle");
                if(opt != miscParams->end())
                    externalWindowHandle = (__bridge NSObject*)(void*)StringConverter::parseSizeT(opt->second);

                // TODO: implement proper parentWindowHandle support as in Metal RS, emulate it now for OgreSamplesCommon.
                if(!externalWindowHandle) 
                {
                    opt = miscParams->find("parentWindowHandle");
                    if(opt != miscParams->end())
                        externalWindowHandle = (__bridge NSObject*)(void*)StringConverter::parseSizeT(opt->second);
                }
                
#if OGRE_NO_QUAD_BUFFER_STEREO == 0
                opt = miscParams->find("stereoMode");
                if (opt != miscParams->end())
                {
                    StereoModeType stereoMode = StringConverter::parseStereoMode(opt->second);
                    if (SMT_NONE != stereoMode)
                        mStereoEnabled = true;
                }
#endif
            }
            
            if(externalGLContext)
            {
                mGLContext = externalGLContext;
                mGLPixelFormat = externalGLContext.pixelFormat;
            }
            else if(useCurrentGLContext)
            {
                CGLContextObj currentGLContext = CGLGetCurrentContext();
                mGLContext = [[NSOpenGLContext alloc] initWithCGLContextObj:currentGLContext];
                mGLPixelFormat = mGLContext.pixelFormat;
            }
            else
            {
                NSOpenGLPixelFormatAttribute attribs[30];
                int i = 0;
                
                // Specify that we want to use the OpenGL 3.2 Core profile
                attribs[i++] = NSOpenGLPFAOpenGLProfile;
                attribs[i++] = NSOpenGLProfileVersion3_2Core;
                
                // Specifying "NoRecovery" gives us a context that cannot fall back to the software renderer.
                // This makes the View-based context a compatible with the fullscreen context, enabling us to use
                // the "shareContext" feature to share textures, display lists, and other OpenGL objects between the two.
                attribs[i++] = NSOpenGLPFANoRecovery;
                
                attribs[i++] = NSOpenGLPFAAccelerated;
                attribs[i++] = NSOpenGLPFADoubleBuffer;
                
                attribs[i++] = NSOpenGLPFAColorSize;
                attribs[i++] = (NSOpenGLPixelFormatAttribute) colourDepth;
                
                attribs[i++] = NSOpenGLPFAAlphaSize;
                attribs[i++] = (NSOpenGLPixelFormatAttribute) 8;
                
                attribs[i++] = NSOpenGLPFAStencilSize;
                attribs[i++] = (NSOpenGLPixelFormatAttribute) 8;
                
                attribs[i++] = NSOpenGLPFADepthSize;
                attribs[i++] = (NSOpenGLPixelFormatAttribute) (hasDepthBuffer? 16 : 0);
                
                if(fsaa_samples > 0)
                {
                    attribs[i++] = NSOpenGLPFAMultisample;
                    attribs[i++] = NSOpenGLPFASampleBuffers;
                    attribs[i++] = (NSOpenGLPixelFormatAttribute) 1;
                    
                    attribs[i++] = NSOpenGLPFASamples;
                    attribs[i++] = (NSOpenGLPixelFormatAttribute) fsaa_samples;
                }
                
#if OGRE_NO_QUAD_BUFFER_STEREO == 0
                if (mStereoEnabled)
                    attribs[i++] = NSOpenGLPFAStereo;
#endif
                
                attribs[i++] = (NSOpenGLPixelFormatAttribute) 0;
                
                mGLPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
                
                GL3PlusRenderSystem *rs = static_cast<GL3PlusRenderSystem*>(Root::getSingleton().getRenderSystem());
                CocoaContext *mainContext = (CocoaContext*)rs->_getMainContext();
                NSOpenGLContext *shareContext = mainContext == 0 ? nil : mainContext->getContext();
                mGLContext = [[NSOpenGLContext alloc] initWithFormat:mGLPixelFormat shareContext:shareContext];
            }
            
            // Set vsync
            GLint swapInterval = (GLint)mVSync;
            [mGLContext setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];
            
            // Make active
            setHidden(hidden);
            mActive = true;
            mClosed = false;
            mName = [windowTitle cStringUsingEncoding:NSUTF8StringEncoding];
            mWidth = _getPixelFromPoint(widthPt);
            mHeight = _getPixelFromPoint(heightPt);
            mFSAA = fsaa_samples;
            
            if(!externalWindowHandle)
            {
                createNewWindow(widthPt, heightPt, [windowTitle cStringUsingEncoding:NSUTF8StringEncoding]);
            }
            else
            {
                if([externalWindowHandle isKindOfClass:[NSWindow class]])
                {
                    mView = [(NSWindow*)externalWindowHandle contentView];
                    mUseOgreGLView = [mView isKindOfClass:[OgreGL3PlusView class]];
                    LogManager::getSingleton().logMessage(mUseOgreGLView ?
                        "Mac Cocoa Window: Rendering on an external NSWindow with nested OgreGL3PlusView" :
                        "Mac Cocoa Window: Rendering on an external NSWindow with nested NSView");
                }
                else
                {
                    assert([externalWindowHandle isKindOfClass:[NSView class]]);
                    mView = (NSView*)externalWindowHandle;
                    mUseOgreGLView = [mView isKindOfClass:[OgreGL3PlusView class]];
                    LogManager::getSingleton().logMessage(mUseOgreGLView ?
                        "Mac Cocoa Window: Rendering on an external OgreGL3PlusView" :
                        "Mac Cocoa Window: Rendering on an external NSView");
                }
                
                if(mUseOgreGLView)
                {
                    [(OgreGL3PlusView*)mView setOgreWindow:this];
                }
                
                NSRect b = [mView bounds];
                mWidth = _getPixelFromPoint((int)b.size.width);
                mHeight = _getPixelFromPoint((int)b.size.height);
                
                mWindow = [mView window];
                mIsExternal = true;
                
                // Add our window to the window event listener class
                WindowEventUtilities::_addRenderWindow(this);
            }
            
            // Create register the context with the rendersystem and associate it with this window
            mContext = OGRE_NEW CocoaContext(mGLContext, mGLPixelFormat);
            
            // Create the window delegate instance to handle window resizing and other window events
            mWindowDelegate = [[CocoaWindowDelegate alloc] initWithNSWindow:mWindow ogreWindow:this];
            
            if(mContentScalingFactor > 1.0)
                [mView setWantsBestResolutionOpenGLSurface:YES];
            
            CGLLockContext((CGLContextObj)[mGLContext CGLContextObj]);
            
            // At least NSOpenGLView responds to both setPixelFormat: and setOpenGLContext:,
            // call it to avoid creation of the unused internal copy
            if([mView respondsToSelector:@selector(setPixelFormat:)])
                [(id)mView setPixelFormat:mGLPixelFormat];
            if([mView respondsToSelector:@selector(setOpenGLContext:)])
                [(id)mView setOpenGLContext:mGLContext];
            
            // Repeat what -[NSOpenGLView setOpenGLContext:] does in case mView is not NSOpenGLView
            // Must be called on the main (UI) thread or else a hard crash occurs
            // on macOS Catalina and later.
            if([mGLContext view] != mView)
            {
                if ([NSThread isMainThread])
                {
                    [mGLContext setView:mView];
                }
                else
                {
                    OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "NSOpenGLContext setView must be called on main (UI) thread.",
                        "CocoaWindow::create");
                }
            }
            [mGLContext makeCurrentContext];
            
#if OGRE_DEBUG_MODE
            // Crash on functions that have been removed from the API
            CGLEnable((CGLContextObj)[mGLContext CGLContextObj], kCGLCECrashOnRemovedFunctions);
#endif
            
            // Enable GL multithreading
            CGLEnable((CGLContextObj)[mGLContext CGLContextObj], kCGLCEMPEngine);
            
            [mGLContext update];
            
            //        rs->clearFrameBuffer(FBT_COLOUR);
            
            //[mGLContext flushBuffer];
            CGLUnlockContext((CGLContextObj)[mGLContext CGLContextObj]);
        }

        StringStream ss;
        ss  << "Cocoa: Window created " << widthPt << " x " << heightPt
        << " with backing store size " << mWidth << " x " << mHeight
        << " using content scaling factor " << std::fixed << std::setprecision(1) << getViewPointToPixelScale();
        LogManager::getSingleton().logMessage(ss.str());
    }

    unsigned int CocoaWindow::getWidth() const
    {
        // keep mWidth in sync with reality
        assert(mView == nil || int(mWidth) == _getPixelFromPoint([mView frame].size.width));
        
        return mWidth;
    }

    unsigned int CocoaWindow::getHeight() const
    {
        // keep mHeight in sync with reality
        assert(mView == nil || int(mHeight) == _getPixelFromPoint([mView frame].size.height));
        
        return mHeight;
    }

    void CocoaWindow::destroy(void)
    {
        if(!mIsFullScreen)
        {
            // Unregister and destroy OGRE GL3PlusContext
            OGRE_DELETE mContext;
            
            if(!mIsExternal)
            {
                // Remove the window from the Window listener
                WindowEventUtilities::_removeRenderWindow(this);
            }

            if(mWindow && !mIsExternal)
            {
                [mWindow performClose:nil];
            }
		}
    }

    bool CocoaWindow::isActive() const
    {
        return mActive;
    }

    bool CocoaWindow::isClosed() const
    {
        return false;
    }

    void CocoaWindow::setHidden(bool hidden)
    {
        mHidden = hidden;
        if (!mIsExternal)
        {
            if (hidden)
                [mWindow orderOut:nil];
            else
                [mWindow makeKeyAndOrderFront:nil];
        }
    }

	void CocoaWindow::setVSyncEnabled(bool vsync)
	{
        mVSync = vsync;
        mContext->setCurrent();
        
        GLint vsyncInterval = mVSync ? 1 : 0;
        [mGLContext setValues:&vsyncInterval forParameter:NSOpenGLCPSwapInterval];

        mContext->endCurrent();
        
        if(mGLContext != [NSOpenGLContext currentContext])
            [mGLContext makeCurrentContext];
	}
    
	bool CocoaWindow::isVSyncEnabled() const
	{
        return mVSync;
	}

    void CocoaWindow::copyContentsToMemory(const Box& src, const PixelBox &dst, FrameBuffer buffer)
    {
        if(src.right > mWidth || src.bottom > mHeight || src.front != 0 || src.back != 1
        || dst.getWidth() != src.getWidth() || dst.getHeight() != src.getHeight() || dst.getDepth() != 1)
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Invalid box.", "CocoaWindow::copyContentsToMemory");
        }
        
        if (buffer == FB_AUTO)
        {
            buffer = mIsFullScreen? FB_FRONT : FB_BACK;
        }
        
        GLenum format = GL3PlusPixelUtil::getGLOriginFormat(dst.format);
        GLenum type = GL3PlusPixelUtil::getGLOriginDataType(dst.format);
        
        if ((format == GL_NONE) || (type == 0))
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Unsupported format.", "CocoaWindow::copyContentsToMemory");
        }
        
        if(dst.getWidth() != dst.rowPitch)
        {
            OGRE_CHECK_GL_ERROR(glPixelStorei(GL_PACK_ROW_LENGTH, static_cast<GLint>(dst.rowPitch)));
        }
        if((dst.getWidth()*Ogre::PixelUtil::getNumElemBytes(dst.format)) & 3)
        {
            // Standard alignment of 4 is not right
            OGRE_CHECK_GL_ERROR(glPixelStorei(GL_PACK_ALIGNMENT, 1));
        }
        
        OGRE_CHECK_GL_ERROR(glReadBuffer((buffer == FB_FRONT)? GL_FRONT : GL_BACK));
        OGRE_CHECK_GL_ERROR(glReadPixels((GLint)src.left, (GLint)(mHeight - src.bottom),
                     (GLsizei)src.getWidth(), (GLsizei)src.getHeight(),
                     format, type, dst.getTopLeftFrontPixelPtr()));
        
        OGRE_CHECK_GL_ERROR(glPixelStorei(GL_PACK_ALIGNMENT, 4));
        OGRE_CHECK_GL_ERROR(glPixelStorei(GL_PACK_ROW_LENGTH, 0));
        
        PixelUtil::bulkPixelVerticalFlip(dst);
    }

    float CocoaWindow::getViewPointToPixelScale()
    {
        return mContentScalingFactor > 1.0f ? mContentScalingFactor : 1.0f;
    }
    
    int CocoaWindow::_getPixelFromPoint(int viewPt) const
    {
        return mContentScalingFactor > 1.0 ? viewPt * mContentScalingFactor : viewPt;
    }
    
    void CocoaWindow::reposition(int leftPt, int topPt)
    {
		if(!mWindow)
            return;

        if(mIsFullScreen)
            return;

		NSRect frame = [mWindow frame];
        NSRect screenFrame = [[NSScreen mainScreen] visibleFrame];
		frame.origin.x = leftPt;
		frame.origin.y = screenFrame.size.height - frame.size.height - topPt;
        mWindowOriginPt = frame.origin;
		[mWindow setFrame:frame display:YES];
    }

    void CocoaWindow::resize(unsigned int widthPt, unsigned int heightPt)
    {
		if(!mWindow)
            return;

        if(mIsFullScreen)
            return;

        unsigned widthPx = _getPixelFromPoint(widthPt);
        unsigned heightPx = _getPixelFromPoint(heightPt);
        
        // Check if the window size really changed
        if(mWidth == widthPx && mHeight == heightPx)
            return;

        mWidth = widthPx;
        mHeight = heightPx;

        if(mIsExternal)
        {
            NSRect viewFrame = [mView frame];
            viewFrame.size.width = widthPt;
            viewFrame.size.height = heightPt;

            NSRect windowFrame = [[mView window] frame];

            CGFloat leftPt = viewFrame.origin.x;
            CGFloat topPt = windowFrame.size.height - (viewFrame.origin.y + viewFrame.size.height);
            mLeft = _getPixelFromPoint((int)leftPt);
            mTop = _getPixelFromPoint((int)topPt);
            mWindowOriginPt = NSMakePoint(leftPt, topPt);

            GLint bufferRect[4];
            bufferRect[0] = mLeft;      // 0 = left edge 
            bufferRect[1] = mTop;       // 0 = bottom edge 
            bufferRect[2] = mWidth;     // width of buffer rect 
            bufferRect[3] = mHeight;    // height of buffer rect 
            CGLContextObj ctx = (CGLContextObj)[mGLContext CGLContextObj];
            CGLSetParameter(ctx, kCGLCPSwapRectangle, bufferRect);
        }
        else
        {
            mWindowOriginPt = [mWindow frame].origin;
            [mWindow setContentSize:NSMakeSize(widthPt, heightPt)];
        }
        //make sure the context is current
        NSOpenGLContextGuard ctx_guard(mGLContext);

		[mGLContext update];
    }

    void CocoaWindow::windowHasResized()
    {
        // Ensure the context is current
        [mGLContext flushBuffer];
    }
    
    void CocoaWindow::windowMovedOrResized()
    {
        mContentScalingFactor =
            ([mView respondsToSelector:@selector(wantsBestResolutionOpenGLSurface)] && [(id)mView wantsBestResolutionOpenGLSurface]) ?
            (mView.window.screen ?: [NSScreen mainScreen]).backingScaleFactor : 1.0f;

        NSRect winFrame = [mWindow frame];
        NSRect viewFrame = [mView frame];
        NSRect screenFrame = [[NSScreen mainScreen] visibleFrame];
        CGFloat leftPt = winFrame.origin.x;
        CGFloat topPt = screenFrame.size.height - winFrame.size.height;
        mWidth = _getPixelFromPoint((unsigned int)viewFrame.size.width);
        mHeight = _getPixelFromPoint((unsigned int)viewFrame.size.height);
        mLeft = _getPixelFromPoint((int)leftPt);
        mTop = _getPixelFromPoint((int)topPt);

        mWindowOriginPt = NSMakePoint(leftPt, topPt);

        //make sure the context is current
        NSOpenGLContextGuard ctx_guard(mGLContext);

        for (ViewportList::iterator it = mViewportList.begin(); it != mViewportList.end(); ++it)
        {
            (*it)->_updateDimensions();
        }
		[mGLContext update];
    }

    void CocoaWindow::swapBuffers()
    {
        CGLLockContext((CGLContextObj)[mGLContext CGLContextObj]);
        [mGLContext makeCurrentContext];

        if([mGLContext view] != mView)
        {
            if ([NSThread isMainThread])
            {
                [mGLContext setView:mView];
            }
            else
            {
                OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                    "NSOpenGLContext setView must be called on main (UI) thread.",
                    "CocoaWindow::swapBuffers");
            }
        }

        [mGLContext flushBuffer];
        CGLUnlockContext((CGLContextObj)[mGLContext CGLContextObj]);

        RenderWindow::swapBuffers();
    }
	
	//-------------------------------------------------------------------------------------------------//
	void CocoaWindow::getCustomAttribute( const String& name, void* pData )
	{
		if( name == "GLCONTEXT" ) 
		{
			*static_cast<CocoaContext**>(pData) = mContext;
			return;
		} 
		else if( name == "WINDOW" ) 
		{
			*(void**)(pData) = (__bridge_retained void*)mWindow;
			return;
		} 
		else if( name == "VIEW" ) 
		{
			*(void**)(pData) = (__bridge_retained void*)mView;
			return;
		}
		else if( name == "NSOPENGLCONTEXT" ) 
		{
			*(void**)(pData) = (__bridge_retained void*)mGLContext;
			return;
		}
		else if( name == "NSOPENGLPIXELFORMAT" ) 
		{
			*(void**)(pData) = (__bridge_retained void*)mGLPixelFormat;
			return;
		}
		
	}

    void CocoaWindow::createNewWindow(unsigned int widthPt, unsigned int heightPt, String title)
    {
        // Get the dimensions of the display. We will use it for the window size but not context resolution
        NSRect windowRect = NSZeroRect;
        if(mIsFullScreen)
        {
            NSRect mainDisplayRect = [[NSScreen mainScreen] visibleFrame];
            windowRect = NSMakeRect(0.0, 0.0, mainDisplayRect.size.width, mainDisplayRect.size.height);
        }
        else
            windowRect = NSMakeRect(0.0, 0.0, widthPt, heightPt);

        @try
        {
            mWindow = [[OgreGL3PlusWindow alloc] initWithContentRect:windowRect
                                                styleMask:mIsFullScreen ? 
                                                    NSWindowStyleMaskBorderless :
                                                    NSWindowStyleMaskResizable|NSWindowStyleMaskTitled
                                                backing:NSBackingStoreBuffered
                                                defer:YES];
        }
        @catch (NSException *exception)
        {
            std::string msg([exception.reason UTF8String]); 
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                "Failed to create window : " + msg,
                "CocoaWindow::createNewWindow");
        }

        [mWindow setTitle:[NSString stringWithCString:title.c_str() encoding:NSUTF8StringEncoding]];
        mWindowTitle = title;

        mView = [[OgreGL3PlusView alloc] initWithGLOSXWindow:this];

        _setWindowParameters(widthPt, heightPt);

//        GL3PlusRenderSystem *rs = static_cast<GL3PlusRenderSystem*>(Root::getSingleton().getRenderSystem());
//        rs->clearFrameBuffer(FBT_COLOUR);

        // Show window
        setHidden(mHidden);

        // Add our window to the window event listener class
        WindowEventUtilities::_addRenderWindow(this);
    }

    void CocoaWindow::createWindowFromExternal(NSView *viewRef)
    {
        LogManager::getSingleton().logMessage("Creating external window");

        NSRect viewBounds = [mView convertRectToBacking:[mView bounds]];

        mWindow = [viewRef window];

        mView = viewRef;

        GLint bufferRect[4];
        bufferRect[0] = viewBounds.origin.x;      // 0 = left edge 
        bufferRect[1] = viewBounds.origin.y;      // 0 = bottom edge 
        bufferRect[2] = viewBounds.size.width;    // width of buffer rect 
        bufferRect[3] = viewBounds.size.height;   // height of buffer rect 
        CGLContextObj ctx = (CGLContextObj)[mGLContext CGLContextObj];
        CGLSetParameter(ctx, kCGLCPSwapRectangle, bufferRect);
        
        mIsExternal = true;
    }

    void CocoaWindow::_setWindowParameters(unsigned int widthPt, unsigned int heightPt)
    {
        if(mWindow)
        {
            if(mIsFullScreen)
            {
                // Set the backing store size to the viewport dimensions
                // This ensures that it will scale to the full screen size
                NSRect mainDisplayRect = [[NSScreen mainScreen] frame];
                NSRect backingRect = NSZeroRect;
                if(mContentScalingFactor > 1.0)
                    backingRect = [[NSScreen mainScreen] convertRectToBacking:mainDisplayRect];
                else
                    backingRect = mainDisplayRect;

                GLint backingStoreDimensions[2] = { static_cast<GLint>(backingRect.size.width), static_cast<GLint>(backingRect.size.height) };
                CGLSetParameter((CGLContextObj)[mGLContext CGLContextObj], kCGLCPSurfaceBackingSize, backingStoreDimensions);
                CGLEnable((CGLContextObj)[mGLContext CGLContextObj], kCGLCESurfaceBackingSize);

                NSRect windowRect = NSMakeRect(0.0, 0.0, mainDisplayRect.size.width, mainDisplayRect.size.height);
                [mWindow setFrame:windowRect display:YES];
                [mView setFrame:windowRect];

                // Set window properties for full screen and save the origin in case the window has been moved
                [mWindow setStyleMask:NSBorderlessWindowMask];
                [mWindow setOpaque:YES];
                [mWindow setHidesOnDeactivate:YES];
                [mWindow setContentView:mView];
                [mWindow setFrameOrigin:NSZeroPoint];
                [mWindow setLevel:NSMainMenuWindowLevel+1];
                
                mWindowOriginPt = mWindow.frame.origin;
                mLeft = mTop = 0;
            }
            else
            {
                // Reset and disable the backing store in windowed mode
                GLint backingStoreDimensions[2] = { 0, 0 };
                CGLSetParameter((CGLContextObj)[mGLContext CGLContextObj], kCGLCPSurfaceBackingSize, backingStoreDimensions);
                CGLDisable((CGLContextObj)[mGLContext CGLContextObj], kCGLCESurfaceBackingSize);

                NSRect viewRect = NSMakeRect(mWindowOriginPt.x, mWindowOriginPt.y, widthPt, heightPt);
                [mWindow setFrame:viewRect display:YES];
                [mView setFrame:viewRect];
                [mWindow setStyleMask:NSResizableWindowMask|NSTitledWindowMask];
                [mWindow setOpaque:YES];
                [mWindow setHidesOnDeactivate:NO];
                [mWindow setContentView:mView];
                [mWindow setLevel:NSNormalWindowLevel];
                [mWindow center];

                // Set the drawable, and current context
                // If you do this last, there is a moment before the rendering window pops-up
                [mGLContext makeCurrentContext];
            }
            
            [mGLContext update];
            
            // Even though OgreCocoaView doesn't accept first responder, it will get passed onto the next in the chain
            [mWindow makeFirstResponder:mView];
            [NSApp activateIgnoringOtherApps:YES];
        }
    }

    void CocoaWindow::setFullscreen(bool fullScreen, unsigned int widthPt, unsigned int heightPt)
    {
        unsigned widthPx = _getPixelFromPoint(widthPt);
        unsigned heightPx = _getPixelFromPoint(heightPt);
        if (mIsFullScreen != fullScreen || widthPx != mWidth || heightPx != mHeight)
        {
            // Set the full screen flag
			mIsFullScreen = fullScreen;

            // Create a window if we haven't already, existence check is done within the functions
            if(!mWindow)
            {
                if(mIsExternal)
                    createWindowFromExternal(mView);
                else
                    createNewWindow(widthPt, heightPt, mWindowTitle);
            }

            _setWindowParameters(widthPt, heightPt);

            mWidth = widthPx;
            mHeight = heightPx;
        }
    }
}
