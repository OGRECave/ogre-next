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
#ifndef _KfTransformArrayMemoryManager_H_
#define _KfTransformArrayMemoryManager_H_

#include "OgreArrayMemoryManager.h"

namespace Ogre
{
    struct KfTransform;

    /** Implementation to create the KfTransform variables needed by SkeletonTrack
        @See SkeletonAnimationDef and @see SkeletonTrack
    @author
        Matias N. Goldberg
    @version
        1.0
    */
    class _OgreExport KfTransformArrayMemoryManager : public ArrayMemoryManager
    {
    public:
        enum MemoryTypes
        {
            KfTransformType = 0,
            NumMemoryTypes
        };

        static const size_t ElementsMemSize[NumMemoryTypes];

        /// @copydoc ArrayMemoryManager::ArrayMemoryManager
        KfTransformArrayMemoryManager( uint16 depthLevel, size_t hintMaxNodes,
                                       size_t          cleanupThreshold = 100,
                                       size_t          maxHardLimit = MAX_MEMORY_SLOTS,
                                       RebaseListener *rebaseListener = 0 );

        virtual ~KfTransformArrayMemoryManager() {}

        /** Requests memory for a new KfTransofrm (for the Array vectors & matrices)
        @remarks
            Uses all slots.
            Deletion is assumed to take place when the memory manager is destroyed;
            as this manager is run in a controlled environment.
        @param outTransform
            Out: The transform with filled memory pointers
        */
        void createNewNode( KfTransform *RESTRICT_ALIAS *outTransform );
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#endif
