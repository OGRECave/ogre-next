/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE-Next
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org

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

#ifndef _OgreMetalDevice_H_
#define _OgreMetalDevice_H_

#include "OgreMetalPrerequisites.h"

#import <dispatch/dispatch.h>

namespace Ogre
{
    class MetalDeviceItem;

    struct _OgreMetalExport MetalDevice
    {
        /// Once a frame is aborted, MTLRenderCommandEncoder are no longer created (ignored).
        bool                         mFrameAborted;
        id<MTLDevice>                mDevice;
        id<MTLCommandQueue>          mMainCommandQueue;
        id<MTLCommandBuffer>         mCurrentCommandBuffer;
        id<MTLBlitCommandEncoder>    mBlitEncoder;
        id<MTLComputeCommandEncoder> mComputeEncoder;
        id<MTLRenderCommandEncoder>  mRenderEncoder;
        MetalRenderSystem           *mRenderSystem;
        dispatch_semaphore_t         mStallSemaphore;

        MetalDevice( MetalRenderSystem *renderSystem );
        ~MetalDevice();

        void init( const MetalDeviceItem *device );

        void endBlitEncoder();
        void endRenderEncoder( bool endRenderPassDesc = true );
        void endComputeEncoder();

        void endAllEncoders( bool endRenderPassDesc = true );

        // Ends all encoders, calls commit and grabs a new mMainCommandBuffer
        void commitAndNextCommandBuffer();

        /** Gets current blit encoder. If none is current, ends all other
            encoders and creates a new blit encoder.
        @remarks
            Use __unsafe_unretained to avoid unnecessary ARC overhead; unless
            you really need to hold on to the returned variable.
            i.e.
            __unsafe_unretained id<MTLBlitCommandEncoder> blitEncoder = mDevice->getBlitEncoder();
        */
        id<MTLBlitCommandEncoder> getBlitEncoder();

        /// See getBlitEncoder.
        id<MTLComputeCommandEncoder> getComputeEncoder();

        /// Waits for the GPU to finish all pending commands.
        void stall();
    };

    class _OgreMetalExport MetalDeviceItem
    {
        id<MTLDevice> mDevice;
        unsigned      mSameNameAdapterIndex;

    public:
        MetalDeviceItem();
        MetalDeviceItem( id<MTLDevice> device, unsigned sameNameIndex );

        String        getDescription() const;
        id<MTLDevice> getMTLDevice() const;
    };

    class _OgreMetalExport MetalDeviceList
    {
        vector<MetalDeviceItem>::type mItems;

    public:
        void refresh();
        void clear();

        size_t count() const;

        const MetalDeviceItem *item( size_t index ) const;
        const MetalDeviceItem *item( const String &name ) const;
    };
}  // namespace Ogre

#endif
