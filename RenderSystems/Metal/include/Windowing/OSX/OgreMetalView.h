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

#ifndef _OgreMetalView_OSX_H_
#define _OgreMetalView_OSX_H_

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include "OgreMetalPrerequisites.h"

_OgreMetalExport @interface OgreMetalView : NSView

// view has a handle to the metal device when created
@property( nonatomic, readonly ) id<MTLDevice> device;

@property( nonatomic, readwrite ) BOOL layerSizeDidUpdate;

/// The value of presentationTime will be passed to
/// MTLCommandBuffer::presentDrawable atTime:presentationTime
/// When negative, it means to present immediately (Ogre will call presentDrawable overload)
/// This value is very important if you want to render at e.g. 30 fps:
/// Calling CADisplayLink.frameInterval = 2 is not enough; as the CPU timer will be fired
/// once every two VSync intervals, but if the GPU completes its job too soon, it will
/// present immediately (i.e. in the next 16ms instead of waiting 33ms).
/// In that case you'll want to set presentationTime = displayLink.timestamp+(1.0/fps)
/// See
/// https://developer.apple.com/library/prerelease/content/documentation/3DDrawing/Conceptual/MTLBestPracticesGuide/FrameRate.html
@property( nonatomic ) CFTimeInterval presentationTime;

@end

#endif
