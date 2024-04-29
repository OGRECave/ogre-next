/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2023 Torus Knot Software Ltd

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

#ifndef __CompositorPassWarmUpDef_H__
#define __CompositorPassWarmUpDef_H__

#include "../OgreCompositorPassDef.h"

#include "../PassScene/OgreCompositorPassSceneDef.h"
#include "OgreCommon.h"
#include "OgreVisibilityFlags.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class CompositorNodeDef;

    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Effects
     *  @{
     */

    class _OgreExport CompositorPassWarmUpDef : public CompositorPassDef
    {
    protected:
        CompositorNodeDef *mParentNodeDef;

    public:
        enum Mode : uint8
        {
            /// This pass will collect all shaders and compile them
            CollectAndTrigger = 0x3,
            /// This pass will collect all shaders.
            /// Useful when multiple pass must accumulate many shaders.
            /// Another pass will be the one to compile them.
            Collect = 0x1,
            /// Compile all shaders accumulated by previous passes set to 'Collect'
            Trigger = 0x2,
        };

        /// Viewport's visibility mask while pretending to render our pass
        /// Please don't write to this directly. Use setVisibilityMask()
        uint32 mVisibilityMask;

        IdString                mShadowNode;
        ShadowNodeRecalculation mShadowNodeRecalculation;  // Only valid if mShadowNode is not empty

        /// When empty, uses the default camera.
        IdString mCameraName;

        /// See CompositorPassWarmUpDef::Mode
        Mode mMode;

        /// First Render Queue ID to render. Inclusive
        uint8 mFirstRQ;
        /// Last Render Queue ID to render. Not inclusive
        uint8 mLastRQ;

        /// See CompositorPassSceneDef::mEnableForwardPlus
        /// TODO: Maybe it's best to hold a PassSceneDef child?
        bool mEnableForwardPlus;

        CompositorPassWarmUpDef( CompositorNodeDef   *parentNodeDef,
                                 CompositorTargetDef *parentTargetDef ) :
            CompositorPassDef( PASS_WARM_UP, parentTargetDef ),
            mParentNodeDef( parentNodeDef ),
            mVisibilityMask( VisibilityFlags::RESERVED_VISIBILITY_FLAGS ),
            mShadowNodeRecalculation( SHADOW_NODE_FIRST_ONLY ),
            mMode( CollectAndTrigger ),
            mFirstRQ( 0 ),
            mLastRQ( (uint8)-1 ),
            mEnableForwardPlus( true )
        {
            setAllLoadActions( LoadAction::Clear );
            setAllStoreActions( StoreAction::StoreOrResolve );
        }

        void setVisibilityMask( uint32 visibilityMask )
        {
            mVisibilityMask = visibilityMask & VisibilityFlags::RESERVED_VISIBILITY_FLAGS;
        }
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
