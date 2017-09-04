/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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
-----------------------------------------------------------------------------
*/

#ifndef __CompositorPassClearDef_H__
#define __CompositorPassClearDef_H__

#include "OgreHeaderPrefix.h"

#include "../OgreCompositorPassDef.h"
#include "OgreCommon.h"
#include "OgreColourValue.h"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Effects
    *  @{
    */

    class _OgreExport CompositorPassClearDef : public CompositorPassDef
    {
    public:
        /// Only execute this pass on non-tilers
        bool                    mNonTilersOnly;

        /** By default clear all buffers. Stencil needs to be cleared because it hinders Fast Z Clear
            on GPU architectures that don't separate the stencil from depth and the program requested
            a Z Buffer with stencil (even if we never use it)
        */
        CompositorPassClearDef( CompositorTargetDef *parentTargetDef ) :
            CompositorPassDef( PASS_CLEAR, parentTargetDef ),
            mNonTilersOnly( false )
        {
            //Override so that it only gets executed on the first execution on the
            //whole screen (i.e. clear the whole viewport during the left eye pass)
            mExecutionMask          = 0x01;
            mViewportModifierMask   = 0x00;

            for( int i=0; i<OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
                mLoadActionColour[i] = LoadAction::Clear;

            mLoadActionDepth    = LoadAction::Clear;
            mLoadActionStencil  = LoadAction::Clear;

            setAllStoreActions( StoreAction::StoreAndMultisampleResolve );
        }

        /** Sets which buffers you want to clear for each attachment. Replaces
            'mClearBufferFlags' from previous versions of Ogre.
        @remarks
            Manually setting load actions to anything other than
            LoadAction::Clear or LoadAction::Load when there is a valid buffer
            is undefined behavior.
        @param buffersToClear
            Bitmask. See RenderPassDescriptor::EntryTypes
        */
        void setBuffersToClear( uint32 buffersToClear )
        {
            for( size_t i=0; i<OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
            {
                if( buffersToClear & (RenderPassDescriptor::Colour0 << i) )
                    mLoadActionColour[i] = LoadAction::Clear;
                else
                    mLoadActionColour[i] = LoadAction::Load;
            }

            if( buffersToClear & RenderPassDescriptor::Depth )
                mLoadActionDepth = LoadAction::Clear;
            else
                mLoadActionDepth = LoadAction::Load;

            if( buffersToClear & RenderPassDescriptor::Stencil )
                mLoadActionStencil = LoadAction::Clear;
            else
                mLoadActionStencil = LoadAction::Load;
        }
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
