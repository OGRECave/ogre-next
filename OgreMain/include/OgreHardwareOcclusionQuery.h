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
#ifndef _HardwareOcclusionQuery__
#define _HardwareOcclusionQuery__

// Precompiler options
#include "OgrePrerequisites.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup RenderSystem
     *  @{
     */
    /**
     * This is a abstract class that that provides the interface for the query class for
     * hardware occlusion.
     *
     * @author Lee Sandberg
     * Updated on 13/8/2005 by Tuan Kuranes email: tuan.kuranes@free.fr
     */
    class _OgreExport HardwareOcclusionQuery : public OgreAllocatedObj
    {
        //----------------------------------------------------------------------
        // Public methods
        //--
    public:
        /**
         * Object public member functions
         */

        /**
         * Default object constructor
         *
         */
        HardwareOcclusionQuery();

        /**
         * Object destructor
         */
        virtual ~HardwareOcclusionQuery();

        /**
         * Starts the hardware occlusion query
         * @remarks    Simple usage: Create one or more OcclusionQuery object one per outstanding query
         * or one per tested object OcclusionQuery* mOcclusionQuery; createOcclusionQuery(
         * &mOcclusionQuery ); In the rendering loop: Draw all occluders
         *             mOcclusionQuery->startOcclusionQuery();
         *             Draw the polygons to be tested
         *             mOcclusionQuery->endOcclusionQuery();
         *
         *             Results must be pulled using:
         *             UINT    mNumberOfPixelsVisable;
         *             pullOcclusionQuery( &mNumberOfPixelsVisable );
         *
         */
        virtual void beginOcclusionQuery() = 0;

        /**
         * Ends the hardware occlusion test
         */
        virtual void endOcclusionQuery() = 0;

        /**
         * Pulls the hardware occlusion query.
         * @note Waits until the query result is available; use isStillOutstanding
         *     if just want to test if the result is available.
         * @retval NumOfFragments will get the resulting number of fragments.
         * @return True if success or false if not.
         */
        virtual bool pullOcclusionQuery( unsigned int *NumOfFragments ) = 0;

        /**
         * Let's you get the last pixel count with out doing the hardware occlusion test
         * @return The last fragment count from the last test.
         * Remarks This function won't give you new values, just the old value.
         */
        unsigned int getLastQuerysPixelcount() const { return mPixelCount; }

        /**
         * Lets you know when query is done, or still be processed by the Hardware
         * @return true if query isn't finished.
         */
        virtual bool isStillOutstanding() = 0;

        //----------------------------------------------------------------------
        // protected members
        //--
    protected:
        /// Number of visible pixels determined by last query
        unsigned int mPixelCount;
        /// Has the query returned a result yet?
        bool mIsQueryResultStillOutstanding;
    };

    /** @} */
    /** @} */
}  // namespace Ogre
#endif
