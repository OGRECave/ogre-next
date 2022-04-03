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
--------------------------------------------------------------------------*/

#include "OgreEAGL2Window.h"

#include "OgreEAGL2Support.h"
#include "OgreEAGLES2Context.h"

#include "OgreRoot.h"
#include "OgreWindowEventUtilities.h"
#include "OgreGLES2RenderSystem.h"
#include "OgreGLES2PixelFormat.h"
#include "OgrePixelBox.h"
#include "OgreViewport.h"
#include <iomanip>

#import <UIKit/UIWindow.h>
#import <UIKit/UIGraphics.h>
#include "ARCMacros.h"

namespace Ogre {

    struct EAGLContextGuard
    {
        EAGLContextGuard(EAGLContext* ctx) : mPrevContext([EAGLContext currentContext]) { if(ctx != mPrevContext) [EAGLContext setCurrentContext:ctx]; }
        ~EAGLContextGuard() { [EAGLContext setCurrentContext:mPrevContext]; }
    private:
         EAGLContext *mPrevContext;
    };


    EAGL2Window::EAGL2Window(EAGL2Support *glsupport)
        :   mClosed(false),
            mVisible(false),
            mHidden(false),
            mIsExternal(false),
            mUsingExternalView(false),
            mUsingExternalViewController(false),
            mIsContentScalingSupported(false),
            mContentScalingFactor(1.0),
            mGLSupport(glsupport),
            mContext(NULL),
            mWindow(nil),
            mView(nil),
            mViewController(nil)
    {
        mIsFullScreen = true;
        mActive = true;
        mHwGamma = false;

        // Check for content scaling.  iOS 4 or later
        if(mGLSupport->getCurrentOSVersion() >= 4.0)
            mIsContentScalingSupported = true;
    }

    EAGL2Window::~EAGL2Window()
    {
        destroy();

        if (mContext != NULL)
        {
            OGRE_DELETE mContext;
        }

        mContext = NULL;
    }

    void EAGL2Window::destroy(void)
    {
        if (mClosed)
        {
            return;
        }

        mClosed = true;
        mActive = false;

        if (!mIsExternal)
        {
            WindowEventUtilities::_removeRenderWindow(this);
        
            SAFE_ARC_RELEASE(mWindow);
            mWindow = nil;
        }

        if(!mUsingExternalViewController)
            SAFE_ARC_RELEASE(mViewController);
    }

    void EAGL2Window::setHidden(bool hidden)
    {
        mHidden = hidden;
        if (!mIsExternal)
        {
            [mWindow setHidden:hidden];
        }
    }

    void EAGL2Window::setFullscreen(bool fullscreen, uint widthPt, uint heightPt)
    {
    }

    void EAGL2Window::reposition(int leftPt, int topPt)
	{
	}
    
	void EAGL2Window::resize(unsigned int widthPt, unsigned int heightPt)
	{
        if(!mWindow) return;

        Real widthPx = _getPixelFromPoint(widthPt);
        Real heightPx = _getPixelFromPoint(heightPt);

        // Check if the window size really changed
        if(mWidth == widthPx && mHeight == heightPx)
            return;
        
        // Destroy and recreate the framebuffer with new dimensions
        EAGLContextGuard ctx_guard(mContext->getContext());
        
        mContext->destroyFramebuffer();
        
        mWidth = widthPx;
        mHeight = heightPx;
        
        mContext->createFramebuffer();

        for (ViewportList::iterator it = mViewportList.begin(); it != mViewportList.end(); ++it)
        {
            (*it)->_updateDimensions();
        }
	}
    
	void EAGL2Window::windowMovedOrResized()
	{
        CGRect frame = [mView frame];
        CGFloat width = _getPixelFromPoint(frame.size.width);
        CGFloat height = _getPixelFromPoint(frame.size.height);
        CGFloat left = _getPixelFromPoint(frame.origin.x);
        CGFloat top = _getPixelFromPoint(frame.origin.y);
        
        if(mWidth == width && mHeight == height && mLeft == left && mTop == top)
            return;
        
        EAGLContextGuard ctx_guard(mContext->getContext());
        mContext->destroyFramebuffer();

        mWidth  = width;
        mHeight = height;
        mLeft   = left;
        mTop    = top;

        mContext->createFramebuffer();

        for (ViewportList::iterator it = mViewportList.begin(); it != mViewportList.end(); ++it)
        {
            (*it)->_updateDimensions();
        }
	}

    void EAGL2Window::createNativeWindow(uint widthPt, uint heightPt, const NameValuePairList *miscParams)
    {
        // This method is called from within create() and after parameters have been parsed.
        // If the window, view or view controller objects are nil at this point, it is safe
        // to assume that external handles are either not being used or are invalid and
        // we can create our own.
        SAFE_ARC_AUTORELEASE_POOL_START()
        
        // Set us up with an external window, or create our own.
        if(!mIsExternal)
        {
            mWindow = [[UIWindow alloc] initWithFrame:CGRectMake(0, 0, widthPt, heightPt)];
        }
        
        OgreAssert(mWindow || mUsingExternalViewController, "EAGL2Window: Failed to obtain required native window");
        
        // Set up the view
        if(!mUsingExternalView)
        {
            mView = [[EAGL2View alloc] initWithFrame:CGRectMake(0, 0, widthPt, heightPt)];
            mView.opaque = YES;

            // Use the default scale factor of the screen
            // See Apple's documentation on supporting high resolution devices for more info
            mView.contentScaleFactor = mContentScalingFactor;
        }
    
        OgreAssert(mView != nil, "EAGL2Window: Failed to create view");
        
        [mView setMWindowName:mName];

        OgreAssert([mView.layer isKindOfClass:[CAEAGLLayer class]], "EAGL2Window: View's Core Animation layer is not a CAEAGLLayer. This is a requirement for using OpenGL ES for drawing.");
        
        CAEAGLLayer *eaglLayer = (CAEAGLLayer *)mView.layer;
        OgreAssert(eaglLayer != nil, "EAGL2Window: Failed to retrieve a pointer to the view's Core Animation layer");

        BOOL retainedBacking = NO;
        NameValuePairList::const_iterator option;
        if ((option = miscParams->find("retainedBacking")) != miscParams->end())
        {
            retainedBacking = StringConverter::parseBool(option->second);
        }

        eaglLayer.opaque = YES;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:retainedBacking], kEAGLDrawablePropertyRetainedBacking,
                                        (mHwGamma ? kEAGLColorFormatSRGBA8 : kEAGLColorFormatRGBA8), kEAGLDrawablePropertyColorFormat,
                                        nil];
        // Set up the view controller
        if(!mUsingExternalViewController)
        {
            mViewController = [[EAGL2ViewController alloc] init];
        }
        
        OgreAssert(mViewController != nil, "EAGL2Window: Failed to create view controller");
        
        if(mViewController.view != mView)
            mViewController.view = mView;

        if(eaglLayer)
        {
            EAGLSharegroup *group = nil;
            
            if ((option = miscParams->find("externalSharegroup")) != miscParams->end())
            {
                group = (__bridge EAGLSharegroup *)(void*)StringConverter::parseSizeT(option->second);
                LogManager::getSingleton().logMessage("iOS: Using an external EAGLSharegroup");
            }
            else
            {
                GLES2RenderSystem *rs = static_cast<GLES2RenderSystem*>(Root::getSingleton().getRenderSystem());
                if(EAGLES2Context* mainContext = (EAGLES2Context*)rs->_getMainContext())
                    group = mainContext->getContext().sharegroup;
            }

            mContext = mGLSupport->createNewContext(eaglLayer, group);

            mContext->mIsMultiSampleSupported = true;
            mContext->mNumSamples = mFSAA;
        }
        
        OgreAssert(mContext != nil, "EAGL2Window: Failed to create OpenGL ES context");

        mViewController.mGLSupport = mGLSupport;
        
        if(!mUsingExternalViewController)
        {
            [mWindow addSubview:mViewController.view];
            mWindow.rootViewController = mViewController;
            [mWindow makeKeyAndVisible];
        }
        
        if(!mUsingExternalView)
            SAFE_ARC_RELEASE(mView);
        
        // Obtain effective view size and scale
        CGSize sz = mView.frame.size;
        mContentScalingFactor = mView.contentScaleFactor;
        mWidth = _getPixelFromPoint(sz.width);
        mHeight = _getPixelFromPoint(sz.height);
        
        mContext->createFramebuffer();
        
        // If content scaling is supported, the window size will be smaller than the GL pixel buffer
        // used to render.  Report the buffer size for reference.
        StringStream ss;
            
        ss  << "iOS: Window created " << widthPt << " x " << heightPt
            << " with backing store size " << mContext->mBackingWidth << " x " << mContext->mBackingHeight
            << " using content scaling factor " << std::fixed << std::setprecision(1) << getViewPointToPixelScale();
        LogManager::getSingleton().logMessage(ss.str());
        
        SAFE_ARC_AUTORELEASE_POOL_END()
    }
    
    void EAGL2Window::create(const String& name, uint widthPt, uint heightPt,
                                bool fullScreen, const NameValuePairList *miscParams)
    {
        short frequency = 0;
        bool vsync = false;
		int left = 0;
		int top  = 0;
        
        mIsFullScreen = fullScreen;
        mName = name;

        // Check the configuration. This may be overridden later by the value sent via miscParams
        ConfigOptionMap::const_iterator configOpt;
        ConfigOptionMap::const_iterator configEnd = mGLSupport->getConfigOptions().end();
        if ((configOpt = mGLSupport->getConfigOptions().find("Content Scaling Factor")) != configEnd)
        {
            mContentScalingFactor = StringConverter::parseReal(configOpt->second.currentValue);
        }

        if (miscParams)
        {
            NameValuePairList::const_iterator opt;
            NameValuePairList::const_iterator end = miscParams->end();

            // Note: Some platforms support AA inside ordinary windows
            if ((opt = miscParams->find("FSAA")) != end)
            {
                mFSAA = StringConverter::parseUnsignedInt(opt->second);
            }
            
            if ((opt = miscParams->find("gamma")) != end)
            {
                mHwGamma = StringConverter::parseBool(opt->second);
            }
                
            if ((opt = miscParams->find("displayFrequency")) != end)
            {
                frequency = (short)StringConverter::parseInt(opt->second);
            }

            if ((opt = miscParams->find("contentScalingFactor")) != end)
            {
                mContentScalingFactor = StringConverter::parseReal(opt->second);
            }
            
            if ((opt = miscParams->find("vsync")) != end)
            {
                vsync = StringConverter::parseBool(opt->second);
            }
            
            if ((opt = miscParams->find("left")) != end)
            {
                left = StringConverter::parseInt(opt->second);
            }
            
            if ((opt = miscParams->find("top")) != end)
            {
                top = StringConverter::parseInt(opt->second);
            }
            
            if ((opt = miscParams->find("title")) != end)
            {
                mName = opt->second;
            }

            if ((opt = miscParams->find("externalWindowHandle")) != end)
            {
                mWindow = (__bridge UIWindow *)(void*)StringConverter::parseSizeT(opt->second);
                mIsExternal = true;
                LogManager::getSingleton().logMessage("iOS: Using an external window handle");
            }
        
            if ((opt = miscParams->find("externalViewHandle")) != end)
            {
                mView = (__bridge EAGL2View *)(void*)StringConverter::parseSizeT(opt->second);
                mUsingExternalView = true;
                LogManager::getSingleton().logMessage("iOS: Using an external view handle");
            }
        
            if ((opt = miscParams->find("externalViewControllerHandle")) != end)
            {
                mViewController = (__bridge EAGL2ViewController *)(void*)StringConverter::parseSizeT(opt->second);
                if(mViewController.view != nil)
                    mView = (EAGL2View *)mViewController.view;
                mUsingExternalViewController = true;
                LogManager::getSingleton().logMessage("iOS: Using an external view controller handle");
            }
		}
        
        createNativeWindow(widthPt, heightPt, miscParams);

        left = top = 0;
        mLeft = left;
		mTop = top;

		mActive = true;
		mVisible = true;
		mClosed = false;
    }

    void EAGL2Window::swapBuffers()
    {
        RenderWindow::swapBuffers();

        if (mClosed)
        {
            return;
        }

        unsigned int attachmentCount = 0;
        GLenum attachments[3];
        GLES2RenderSystem *rs =
            static_cast<GLES2RenderSystem*>(Root::getSingleton().getRenderSystem());
        unsigned int buffers = 0; // rs->getDiscardBuffers();
        
        if(buffers & FBT_COLOUR)
        {
            attachments[attachmentCount++] = GL_COLOR_ATTACHMENT0;
        }
        if(buffers & FBT_DEPTH)
        {
            attachments[attachmentCount++] = GL_DEPTH_ATTACHMENT;
        }
        if(buffers & FBT_STENCIL)
        {
            attachments[attachmentCount++] = GL_STENCIL_ATTACHMENT;
        }
        if(mContext->mIsMultiSampleSupported && mContext->mNumSamples > 0)
        {
#if OGRE_NO_GLES3_SUPPORT == 1
            OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER_APPLE, mContext->mViewFramebuffer));
            OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_READ_FRAMEBUFFER_APPLE, mContext->mSampleFramebuffer));
            OGRE_CHECK_GL_ERROR(glResolveMultisampleFramebufferAPPLE());
            OGRE_CHECK_GL_ERROR(glDiscardFramebufferEXT(GL_READ_FRAMEBUFFER_APPLE, attachmentCount, attachments));
#else
            OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mContext->mViewFramebuffer));
            OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_READ_FRAMEBUFFER, mContext->mSampleFramebuffer));
			OGRE_CHECK_GL_ERROR(glBlitFramebuffer(0, 0, mWidth, mHeight, 0, 0, mWidth, mHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST));
            OGRE_CHECK_GL_ERROR(glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, attachmentCount, attachments));
#endif
        }
        else
        {
            OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, mContext->mViewFramebuffer));
#if OGRE_NO_GLES3_SUPPORT == 1
            OGRE_CHECK_GL_ERROR(glDiscardFramebufferEXT(GL_FRAMEBUFFER, attachmentCount, attachments));
#else
            OGRE_CHECK_GL_ERROR(glInvalidateFramebuffer(GL_FRAMEBUFFER, attachmentCount, attachments));
#endif
        }

        OGRE_CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, mContext->mViewRenderbuffer));
        if ([mContext->getContext() presentRenderbuffer:GL_RENDERBUFFER] == NO)
        {
            glGetError();
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "Failed to swap buffers in ",
                        __FUNCTION__);
        }
    }

    void EAGL2Window::getCustomAttribute( const String& name, void* pData )
    {
		if( name == "GLCONTEXT" )
		{
			*static_cast<EAGLES2Context**>(pData) = mContext;
			return;
		}

        if( name == "GLFBO" )
        {
            *static_cast<GLuint*>(pData) = (mContext->mIsMultiSampleSupported && mContext->mNumSamples>0) ? mContext->mSampleFramebuffer : mContext->mViewFramebuffer;
            return;
        }

        if( name == "SHAREGROUP" )
		{
            *(void**)(pData) = (__bridge void*)mContext->getContext().sharegroup;
            return;
		}

		if( name == "WINDOW" )
		{
            *(void**)(pData) = (__bridge void*)mWindow;
			return;
		}
        
		if( name == "VIEW" )
		{
            *(void**)(pData) = (__bridge void*)mViewController.view;
            return;
		}

        if( name == "VIEWCONTROLLER" )
		{
            *(void**)(pData) = (__bridge void*)mViewController;
            return;
		}
	}

    void EAGL2Window::copyContentsToMemory(const Box& src, const PixelBox &dst, FrameBuffer buffer)
    {
        if(src.right > mWidth || src.bottom > mHeight || src.front != 0 || src.back != 1
        || dst.getWidth() != src.getWidth() || dst.getHeight() != src.getHeight() || dst.getDepth() != 1
        || dst.getWidth() != dst.rowPitch /* GLES2 does not support GL_PACK_ROW_LENGTH, nor iOS supports GL_NV_pack_subimage */)
		{
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Invalid box.", __FUNCTION__ );
		}

		if (buffer == FB_AUTO)
		{
			buffer = mIsFullScreen ? FB_FRONT : FB_BACK;
		}

		// Switch context if different from current one
		RenderSystem* rsys = Root::getSingleton().getRenderSystem();
		rsys->_setViewport(this->getViewport(0));

        OGRE_CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, mContext->mViewRenderbuffer));

        // The following code is adapted from Apple Technical Q & A QA1704
        // http://developer.apple.com/library/ios/#qa/qa1704/_index.html
        NSInteger width = dst.getWidth(), height = dst.getHeight();
        NSInteger dataLength = width * height * PixelUtil::getComponentCount(dst.format);
        GLubyte *data = (GLubyte*)malloc(dataLength * sizeof(GLubyte));
        GLenum format = GLES2PixelUtil::getGLOriginFormat(dst.format, mHwGamma);
        GLenum type = GLES2PixelUtil::getGLOriginDataType(dst.format);

        GLint currentFBO = 0;
        GLuint sampleFramebuffer = 0;
        OGRE_CHECK_GL_ERROR(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO));
        OGRE_CHECK_GL_ERROR(glGenFramebuffers(1, &sampleFramebuffer));
        
#if OGRE_NO_GLES3_SUPPORT == 1
        OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_READ_FRAMEBUFFER_APPLE, sampleFramebuffer));
        OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER_APPLE, currentFBO));
        OGRE_CHECK_GL_ERROR(glResolveMultisampleFramebufferAPPLE());
#else
        OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_READ_FRAMEBUFFER, sampleFramebuffer));
        OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, currentFBO));
        OGRE_CHECK_GL_ERROR(glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST));
#endif
        OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, currentFBO));
        
        // Read pixel data from the framebuffer
        OGRE_CHECK_GL_ERROR(glPixelStorei(GL_PACK_ALIGNMENT, 1));
        OGRE_CHECK_GL_ERROR(glReadPixels((GLint)src.left, (GLint)(mHeight - src.bottom),
                                         (GLsizei)width, (GLsizei)height,
                                         format, type, data));
        OGRE_CHECK_GL_ERROR(glPixelStorei(GL_PACK_ALIGNMENT, 4));
        
        OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, currentFBO));
        OGRE_CHECK_GL_ERROR(glDeleteFramebuffers(1, &sampleFramebuffer));

        // Create a CGImage with the pixel data
        // If your OpenGL ES content is opaque, use kCGImageAlphaNoneSkipLast to ignore the alpha channel
        // otherwise, use kCGImageAlphaPremultipliedLast
        CGDataProviderRef ref = CGDataProviderCreateWithData(NULL, data, dataLength, NULL);
        CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
        CGImageRef iref = CGImageCreate(width, height, 8, PixelUtil::getNumElemBits(dst.format),
                                        width * PixelUtil::getComponentCount(dst.format), colorspace,
                                        kCGBitmapByteOrderDefault,
                                        ref, NULL, YES, kCGRenderingIntentDefault);

        // OpenGL ES measures data in PIXELS
        // Create a graphics context with the target size measured in POINTS
        NSInteger widthInPoints = 0, heightInPoints = 0;

        // Set the scale parameter to your OpenGL ES view's contentScaleFactor
        // so that you get a high-resolution snapshot when its value is greater than 1.0
        CGFloat scale = mView.contentScaleFactor;
        widthInPoints = width / scale;
        heightInPoints = height / scale;
        UIGraphicsBeginImageContextWithOptions(CGSizeMake(widthInPoints, heightInPoints), NO, scale);

        CGContextRef context = UIGraphicsGetCurrentContext();
        CGContextSetBlendMode(context, kCGBlendModeCopy);
        CGContextDrawImage(context, CGRectMake(0.0, 0.0, widthInPoints, heightInPoints), iref);

        // Retrieve the UIImage from the current context
        memcpy(dst.data, CGBitmapContextGetData(context), CGBitmapContextGetBytesPerRow(context) * height); // TODO: support dst.rowPitch != dst.getWidth() case
        UIGraphicsEndImageContext();

        // Clean up
        free(data);
        CFRelease(ref);
        CFRelease(colorspace);
        CGImageRelease(iref);
    }
}
