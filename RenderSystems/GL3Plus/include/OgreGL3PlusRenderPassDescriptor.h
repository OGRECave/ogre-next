/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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

#ifndef _OgreGL3PlusRenderPassDescriptor_H_
#define _OgreGL3PlusRenderPassDescriptor_H_

#include "OgreGL3PlusPrerequisites.h"
#include "OgreRenderPassDescriptor.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    struct FrameBufferDescKey
    {
        uint8                   numColourEntries;
        bool                    allLayers[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        RenderPassTargetBase    colour[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        RenderPassTargetBase    depth;
        RenderPassTargetBase    stencil;

        FrameBufferDescKey();
        FrameBufferDescKey( const RenderPassDescriptor &desc );

        bool operator < ( const FrameBufferDescKey &other ) const;
    };
    struct FrameBufferDescValue
    {
        GLuint  fboName;
        uint16  refCount;
        FrameBufferDescValue();
    };

    typedef map<FrameBufferDescKey, FrameBufferDescValue>::type FrameBufferDescMap;

    /** GL3+ will share FBO handles between all GL3PlusRenderPassDescriptor that share the
        same FBO setup. This doesn't mean these RenderPassDescriptor are exactly the
        same, as they may have different clear, loadAction or storeAction values.
    */
    class _OgreGL3PlusExport GL3PlusRenderPassDescriptor : public RenderPassDescriptor
    {
    protected:
        GLuint  mFboName;
        GLuint  mFboMsaaResolve;
        bool    mAllClearColoursSetAndIdentical;
        bool    mAnyColourLoadActionsSetToClear;
        bool    mHasRenderWindow;

        FrameBufferDescMap::iterator mSharedFboItor;

        GL3PlusRenderSystem *mRenderSystem;

        void checkRenderWindowStatus(void);
        void switchToRenderWindow(void);
        void switchToFBO(void);
        /// Sets mAllClearColoursSetAndIdentical & mAnyColourLoadActionsSetToClear
        /// which can be used for quickly taking fast paths during rendering.
        void analyzeClearColour(void);

        virtual void updateColourFbo( uint8 lastNumColourEntries );
        virtual void updateDepthFbo(void);
        virtual void updateStencilFbo(void);

        /// Returns a mask of RenderPassDescriptor::EntryTypes bits set that indicates
        /// if 'other' wants to perform clears on colour, depth and/or stencil values.
        /// If using MRT, each colour is evaluated independently (only the ones marked
        /// as clear will be cleared).
        uint32 checkForClearActions( GL3PlusRenderPassDescriptor *other ) const;

    public:
        GL3PlusRenderPassDescriptor( GL3PlusRenderSystem *renderSystem );
        virtual ~GL3PlusRenderPassDescriptor();

        GLuint getFboName(void) const       { return mFboName; }

        virtual void entriesModified( uint32 entryTypes );

        virtual void setClearColour( uint8 idx, const ColourValue &clearColour );
        virtual void setClearColour( const ColourValue &clearColour );

        uint32 willSwitchTo( GL3PlusRenderPassDescriptor *newDesc, bool warnIfRtvWasFlushed ) const;

        void performLoadActions( uint8 blendChannelMask, bool depthWrite, uint32 stencilWriteMask,
                                 uint32 entriesToFlush );
        void performStoreActions( bool hasArbInvalidateSubdata, uint32 entriesToFlush );
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
