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

    class _OgreExport GL3PlusRenderPassDescriptor : RenderPassDescriptor
    {
    protected:
        GLuint  mFboName;
        GLuint  mFboMsaaResolve;
        bool    mAllClearColoursSetAndIdentical;
        bool    mAnyColourLoadActionsSetToClear;

        RenderSystem    *mRenderSystem;

        void analyzeClearColour(void);

    public:
        GL3PlusRenderPassDescriptor( RenderSystem *renderSystem );
        virtual ~GL3PlusRenderPassDescriptor();

        /** Call this when you're done modified mEntries. Note there must be no gaps,
            e.g. mEntries[1] is empty but mEntries[0] & mEntries[2] are not.
        @remarks
            Values that are modified by calling setClearColour et al don't need to call
            entriesModified.
            Prefer changing those values using those calls since it's faster.
        */
        virtual void colourEntriesModified(void);
        virtual void depthModified(void);
        virtual void stencilModified(void);

        virtual void setClearColour( uint8 idx, const ColourValue &clearColour );
        virtual void setClearColour( const ColourValue &clearColour );

        void performLoadActions( uint8 blendChannelMask, bool depthWrite, uint32 stencilWriteMask );
        void performStoreActions( bool hasArbInvalidateSubdata );
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
