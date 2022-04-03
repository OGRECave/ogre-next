/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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
#ifndef _Ogre_ShadowCameraSetupConcentric_H_
#define _Ogre_ShadowCameraSetupConcentric_H_

#include "OgrePrerequisites.h"
#include "OgreShadowCameraSetup.h"

#include "OgreAxisAlignedBox.h"
#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Scene
     *  @{
     */
    /** Implements the concentric shadow mapping algorithm
    @remarks
    */
    class _OgreExport ConcentricShadowCamera : public DefaultShadowCameraSetup
    {
    public:
        /** Default constructor.
        @remarks
            Temporary frustum and camera set up here.
        */
        ConcentricShadowCamera();

        /** Default destructor.
        @remarks
            Temporary frustum and camera destroyed here.
        */
        ~ConcentricShadowCamera() override;

        /** Returns a uniform shadow camera with a focused view.
         */
        void getShadowCamera( const SceneManager *sm, const Camera *cam, const Light *light,
                              Camera *texCam, size_t iteration,
                              const Vector2 &viewportRealSize ) const override;
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif  // __ShadowCameraSetupFocused_H__
