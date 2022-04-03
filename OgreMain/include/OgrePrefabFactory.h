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

#ifndef __PrefabFactory_H__
#define __PrefabFactory_H__

#include "OgrePrerequisites.h"

namespace Ogre
{
    namespace v1
    {
        /** \addtogroup Core
         *  @{
         */
        /** \addtogroup Resources
         *  @{
         */
        /** A factory class that can create various mesh prefabs.
        @remarks
            This class is used by OgreMeshManager to offload the loading of various prefab types
            to a central location.
        */
        class _OgreExport PrefabFactory
        {
        public:
            /** If the given mesh has a known prefab resource name (e.g "Prefab_Plane")
                then this prefab will be created as a submesh of the given mesh.

                @param mesh The mesh that the potential prefab will be created in.
                @return true if a prefab has been created, otherwise false.
            */
            static bool createPrefab( Mesh *mesh );

        private:
            /// Creates a plane as a submesh of the given mesh
            static void createPlane( Mesh *mesh );

            /// Creates a 100x100x100 cube as a submesh of the given mesh
            static void createCube( Mesh *mesh );

            /// Creates a sphere with a diameter of 100 units as a submesh of the given mesh
            static void createSphere( Mesh *mesh );
        };
        /** @} */
        /** @} */

    }  // namespace v1
}  // namespace Ogre

#endif
