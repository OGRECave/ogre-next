/*
 * -----------------------------------------------------------------------------
 * This source file is part of OGRE-Next
 * (Object-oriented Graphics Rendering Engine)
 * For the latest info, see http://www.ogre3d.org/
 *
 * Copyright (c) 2000-2014 Torus Knot Software Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -----------------------------------------------------------------------------
 */

#ifndef _LodInputProviderMeshV2_H__
#define _LodInputProviderMeshV2_H__

#include "OgreLodPrerequisites.h"

#include "OgreLodInputProvider.h"
#include "OgreSharedPtr.h"

namespace Ogre
{
    /** Reads geometry directly from a v2 Mesh's SubMesh VAOs (no v1 mesh involved at
        any point) and populates a LodData for MeshLodGenerator to collapse.
    @remarks
        Unlike LodInputProviderMesh, there is no shared-vertex-data concept to handle:
        v2 Mesh has no Mesh-level shared vertex buffer, only per-SubMesh VAOs. This
        removes the mSharedVertexLookup / useSharedVertexLookup branching entirely.
    @par
        Reads vertex/index data via VertexArrayObject::readRequests() +
        mapAsyncTickets() + unmapAsyncTickets() (the same pattern SubMesh::
        _dearrangeEfficient() already uses elsewhere in this codebase), which works
        synchronously regardless of whether the buffers are shadow-copied, so there
        is no special buffer-policy requirement on the source mesh.
    */
    class _OgreLodExport LodInputProviderMeshV2 : public LodInputProvider
    {
    public:
        LodInputProviderMeshV2( MeshPtr mesh );

        void initData( LodData *data ) override;

    protected:
        MeshPtr mMesh;

        /// Reused across submeshes; cleared at the start of each addVertexData() call.
        /// One entry per vertex in the submesh's LOD-0 vertex buffer, mapping its
        /// position in that buffer to the deduplicated LodData::VertexI.
        VertexLookupList mVertexLookup;

        void tuneContainerSize( LodData *data );
        void initialize( LodData *data );
        void addVertexData( LodData *data, SubMesh *subMesh, unsigned submeshID );
        void addIndexData( LodData *data, SubMesh *subMesh, unsigned submeshID );
    };

}  // namespace Ogre
#endif
