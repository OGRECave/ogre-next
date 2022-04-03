/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#ifndef _OgreCompositorPassShadowsDef_H_
#define _OgreCompositorPassShadowsDef_H_

#include "../OgreCompositorPassDef.h"

#include "OgreCommon.h"
#include "OgrePixelFormatGpu.h"

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

    class _OgreExport CompositorPassShadowsDef : public CompositorPassDef
    {
    protected:
        CompositorNodeDef *mParentNodeDef;

    public:
        /// Name of shadow nodes to update.
        FastArray<IdString> mShadowNodes;

        /// @copydoc CompositorPassSceneDef::mCameraName
        IdString mCameraName;
        /// @copydoc CompositorPassSceneDef::mLodCameraName
        IdString mLodCameraName;
        /// @copydoc CompositorPassSceneDef::mCullCameraName
        IdString mCullCameraName;

        /// @copydoc CompositorPassSceneDef::mVisibilityMask
        uint32 mVisibilityMask;

        /// @copydoc CompositorPassSceneDef::mCameraCubemapReorient
        bool mCameraCubemapReorient;

        CompositorPassShadowsDef( CompositorNodeDef   *parentNodeDef,
                                  CompositorTargetDef *parentTargetDef );

        void setVisibilityMask( uint32 visibilityMask );
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
