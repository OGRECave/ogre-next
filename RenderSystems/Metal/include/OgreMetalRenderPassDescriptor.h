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

#ifndef _OgreMetalRenderPassDescriptor_H_
#define _OgreMetalRenderPassDescriptor_H_

#include "OgreMetalPrerequisites.h"

#include "OgreRenderPassDescriptor.h"

#include "OgreCommon.h"

#import <Metal/MTLRenderPass.h>

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */

    struct MetalFrameBufferDescValue
    {
        uint16 refCount;
        MetalFrameBufferDescValue();
    };

    typedef map<FrameBufferDescKey, MetalFrameBufferDescValue>::type MetalFrameBufferDescMap;

    class _OgreMetalExport MetalRenderPassDescriptor final : public RenderPassDescriptor
    {
    protected:
        MTLRenderPassColorAttachmentDescriptor   *mColourAttachment[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        MTLRenderPassDepthAttachmentDescriptor   *mDepthAttachment;
        MTLRenderPassStencilAttachmentDescriptor *mStencilAttachment;

        /// Only used if we need to emulate StoreAndMultisampleResolve
        MTLRenderPassColorAttachmentDescriptor *mResolveColourAttachm[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        bool                                    mRequiresManualResolve;

        MetalFrameBufferDescMap::iterator mSharedFboItor;

        MetalDevice       *mDevice;
        MetalRenderSystem *mRenderSystem;

#if OGRE_DEBUG_MODE
        void  *mCallstackBacktrace[32];
        size_t mNumCallstackEntries;
#endif

        void checkRenderWindowStatus();
        void calculateSharedKey();

        static MTLLoadAction  get( LoadAction::LoadAction action );
        static MTLStoreAction get( StoreAction::StoreAction action );

        void sanitizeMsaaResolve( size_t colourIdx );
        void updateColourRtv( uint8 lastNumColourEntries );
        void updateDepthRtv();
        void updateStencilRtv();

        /// Returns a mask of RenderPassDescriptor::EntryTypes bits set that indicates
        /// if 'other' wants to perform clears on colour, depth and/or stencil values.
        /// If using MRT, each colour is evaluated independently (only the ones marked
        /// as clear will be cleared).
        uint32 checkForClearActions( MetalRenderPassDescriptor *other ) const;
        bool   cannotInterruptRendering() const;

    public:
        MetalRenderPassDescriptor( MetalDevice *device, MetalRenderSystem *renderSystem );
        ~MetalRenderPassDescriptor() override;

        void entriesModified( uint32 entryTypes ) override;

        void setClearColour( uint8 idx, const ColourValue &clearColour ) override;
        void setClearDepth( Real clearDepth ) override;
        void setClearStencil( uint32 clearStencil ) override;

        uint32 willSwitchTo( MetalRenderPassDescriptor *newDesc, bool warnIfRtvWasFlushed ) const;

        void performLoadActions( MTLRenderPassDescriptor *passDesc, bool renderingWasInterrupted );
        void performStoreActions( uint32 entriesToFlush, bool isInterruptingRendering );
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
