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
#ifndef _OgreMetalWindow_H_
#define _OgreMetalWindow_H_

#include "OgreMetalPrerequisites.h"

#include "OgreWindow.h"

#include "OgreMetalView.h"

#import <QuartzCore/CAMetalLayer.h>

namespace Ogre
{
    class MetalWindow : public Window
    {
        bool mClosed;
        bool mHidden;
        bool mIsExternal;
        bool mHwGamma;
        bool mManualRelease;

        CAMetalLayer       *mMetalLayer;
        id<CAMetalDrawable> mCurrentDrawable;
        OgreMetalView      *mMetalView;
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        NSWindow *mWindow;
#endif
        MetalDevice *mDevice;

        inline void checkLayerSizeChanges();
        void        setResolutionFromView();

    public:
        MetalWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode,
                     const NameValuePairList *miscParams, MetalDevice *ownerDevice );
        ~MetalWindow() override;

        float getViewPointToPixelScale() const override;

        void swapBuffers() override;
        void windowMovedOrResized() override;

        void setManualSwapRelease( bool bManualRelease ) override;
        bool isManualSwapRelease() const override;
        void performManualRelease() override;
        void setWantsToDownload( bool bWantsToDownload ) override;
        bool canDownloadData() const override;

        bool nextDrawable();

        virtual void create( bool fullScreen, const NameValuePairList *miscParams );
        void         destroy() override;

        void _initialize( TextureGpuManager       *textureGpuManager,
                          const NameValuePairList *miscParams ) override;

        void reposition( int32 left, int32 top ) override;
        void requestResolution( uint32 width, uint32 height ) override;

        bool isClosed() const override;
        void _setVisible( bool visible ) override;
        bool isVisible() const override;
        void setHidden( bool hidden ) override;
        bool isHidden() const override;

        void getCustomAttribute( IdString name, void *pData ) override;
    };
}  // namespace Ogre

#endif
