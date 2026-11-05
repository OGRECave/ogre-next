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

#ifndef _LodOutputProviderMeshV2_H__
#define _LodOutputProviderMeshV2_H__

#include "OgreLodPrerequisites.h"

#include "OgreLodOutputProvider.h"
#include "OgreSharedPtr.h"

namespace Ogre
{
    /** Writes generated LOD levels directly onto a v2 Mesh's SubMesh::mVao arrays.
    @remarks
        Unlike LodOutputProviderMesh (which builds a v1::SubMesh::LODFaceList), this
        appends a brand new VertexArrayObject per LOD level directly onto
        SubMesh::mVao[VpNormal] (and [VpShadow], see bakeLodLevel()), reusing the
        existing LOD-0 vertex buffers untouched -- LOD generation only ever changes
        the index buffer, never vertex data, matching the pattern already proven in
        SubMesh::importBuffersFromV1()'s own v1-LOD-import loop.
    @par
        Populating Mesh::_setLodValues() is intentionally NOT done here: just like
        v1's LodOutputProviderMesh leaves _configureMeshLodUsage() as a separate step
        MeshLodGenerator calls itself (it already has LodConfig in scope, this
        provider doesn't need to carry a copy of it), the v2 path has a parallel
        MeshLodGenerator::_configureMeshLodUsageV2() called the same way, after
        output->finalize().
    @par
        Manual (mesh-swap) LOD levels are intentionally not supported here; see
        bakeManualLodLevel().
    */
    class _OgreLodExport LodOutputProviderMeshV2 : public LodOutputProvider
    {
    public:
        LodOutputProviderMeshV2( MeshPtr mesh ) : mMesh( mesh ) {}

        void prepare( LodData *data ) override;
        void finalize( LodData *data ) override {}
        void bakeManualLodLevel( LodData *data, String &manualMeshName, int lodIndex ) override;
        void bakeLodLevel( LodData *data, int lodIndex ) override;

    protected:
        MeshPtr mMesh;

        /// Builds a new IndexBufferPacked from data's current triangle list for one
        /// submesh, exactly mirroring LodOutputProviderMesh::bakeLodLevel's v1 index
        /// construction (same "dummy triangle if empty" handling), but returning a
        /// v2 IndexBufferPacked instead of writing into a v1::IndexData.
        IndexBufferPacked *buildIndexBufferForSubmesh( LodData *data, unsigned submeshID );
    };

}  // namespace Ogre
#endif
