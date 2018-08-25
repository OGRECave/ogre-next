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
#ifndef _OgreMetalWindow_H_
#define _OgreMetalWindow_H_

#include "OgreMetalPrerequisites.h"
#include "OgreWindow.h"

#include "OgreMetalRenderTargetCommon.h"

#include "OgreMetalView.h"

#import <QuartzCore/CAMetalLayer.h>

namespace Ogre
{
    class MetalWindow : public Window
    {
        bool    mClosed;
        bool    mHwGamma;
        uint8   mMsaa;

        CAMetalLayer        *mMetalLayer;
        id<CAMetalDrawable> mCurrentDrawable;
        OgreMetalView       *mMetalView;
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        NSWindow            *mWindow;
        id                  mResizeObserver;
#endif
        MetalDevice         *mDevice;

        inline void checkLayerSizeChanges(void);
        void setResolutionFromView(void);
    public:
        MetalWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode,
                     const NameValuePairList *miscParams, MetalDevice *ownerDevice );
        virtual ~MetalWindow();

        virtual void swapBuffers(void);
        virtual void windowMovedOrResized(void);

        virtual bool nextDrawable(void);

        virtual void create( bool fullScreen, const NameValuePairList *miscParams );
        virtual void destroy(void);

        void _initialize( TextureGpuManager *textureGpuManager );

        virtual void reposition( int32 left, int32 top );
        virtual void requestResolution( uint32 width, uint32 height );

        virtual bool isClosed(void) const;
        virtual void _setVisible( bool visible );
        virtual bool isVisible(void) const;
        virtual void setHidden( bool hidden );
        virtual bool isHidden(void) const;

        virtual void getCustomAttribute( IdString name, void* pData );
    };
}

#endif

