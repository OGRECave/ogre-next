/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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

/*
The nVidia occlusion query extension is defined in glext.h so you don't
need anything else. You do need to look up the function, we provide a
GL3PlusSupport class to do this, which has platform implementations for
getProcAddress. Check the way that extensions like glActiveTextureARB are
initialised and used in glRenderSystem and copy what is done there.

  To do: fix so dx7 and DX9 checks and flags if HW Occlusion is supported
  See the openGl dito for ideas what to do.
*/

#ifndef __GL3PlusHARDWAREOCCLUSIONQUERY_H__
#define __GL3PlusHARDWAREOCCLUSIONQUERY_H__

#include "OgreGL3PlusPrerequisites.h"

#include "OgreHardwareOcclusionQuery.h"

namespace Ogre
{
    // If you use multiple rendering passes you can test only the first pass and all other passes don't
    // have to be rendered if the first pass result has too few pixels visible.

    // Be sure to render all occluder first and what's out so the RenderQue don't switch places on
    // the occluding objects and the tested objects because it thinks it's more effective..

    /**
     * This is a class that is the base class of the query class for
     * hardware occlusion.
     *
     * @author Lee Sandberg email: lee@abcmedia.se
     * Updated on 13/9/2005 by Tuan Kuranes email: tuan.kuranes@free.fr
     */

    class _OgreGL3PlusExport GL3PlusHardwareOcclusionQuery final : public HardwareOcclusionQuery
    {
        //----------------------------------------------------------------------
        // Public methods
        //--
    public:
        /**
         * Default object constructor
         *
         */
        GL3PlusHardwareOcclusionQuery();
        /**
         * Object destructor
         */
        ~GL3PlusHardwareOcclusionQuery() override;

        //------------------------------------------------------------------
        // Occlusion query functions (see base class documentation for this)
        //--
        void beginOcclusionQuery() override;
        void endOcclusionQuery() override;
        bool pullOcclusionQuery( unsigned int *NumOfFragments ) override;
        bool isStillOutstanding() override;

    private:
        GLuint mQueryID;
    };

}  // namespace Ogre

#endif
