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

    class _OgreGL3PlusExport GL3PlusRenderPassDescriptor : public RenderPassDescriptor
    {
    protected:
        GLuint  mFboName;
        GLuint  mFboMsaaResolve;
        bool    mAllClearColoursSetAndIdentical;
        bool    mAnyColourLoadActionsSetToClear;
        bool    mHasRenderWindow;

        RenderSystem    *mRenderSystem;

        void checkRenderWindowStatus(void);
        void switchToRenderWindow(void);
        void switchToFBO(void);
        void analyzeClearColour(void);

        virtual void updateColourFbo( uint8 lastNumColourEntries );
        virtual void updateDepthFbo(void);
        virtual void updateStencilFbo(void);

        uint32 clearValuesMatch( GL3PlusRenderPassDescriptor *other ) const;

    public:
        GL3PlusRenderPassDescriptor( RenderSystem *renderSystem );
        virtual ~GL3PlusRenderPassDescriptor();

        virtual void entriesModified( uint32 entryTypes );

        virtual void setClearColour( uint8 idx, const ColourValue &clearColour );
        virtual void setClearColour( const ColourValue &clearColour );

        uint32 willSwitchTo( GL3PlusRenderPassDescriptor *newDesc, bool viewportChanged ) const;

        void performLoadActions( uint8 blendChannelMask, bool depthWrite, uint32 stencilWriteMask,
                                 uint32 entriesToFlush );
        void performStoreActions( bool hasArbInvalidateSubdata, uint32 x, uint32 y,
                                  uint32 width, uint32 height, uint32 entriesToFlush );
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
