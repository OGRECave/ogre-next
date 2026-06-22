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

#ifndef _LodInputProvider_H__
#define _LodInputProvider_H__

#include "OgreLodPrerequisites.h"

#include "OgreLodData.h"

namespace Ogre
{
    class _OgreLodExport LodInputProvider
    {
    public:
        virtual ~LodInputProvider() {}
        /// Called when the data should be filled with the input.
        virtual void initData( LodData *data ) = 0;

    protected:
        // Helper functions
        void printTriangle( LodData *data, LodData::Triangle *triangle, stringstream &str );
        void addTriangleToEdges( LodData *data, LodData::Triangle *triangle );
        bool isDuplicateTriangle( LodData::Triangle *triangle, LodData::Triangle *triangle2 );
        LodData::Triangle *isDuplicateTriangle( LodData *data, LodData::Triangle *triangle );
        static size_t      getTriangleCount( OperationType renderOp, size_t indexCount );

        /// Typedef shared by every concrete provider: maps a raw vertex-buffer index
        /// (as found in the source index buffer) to the deduplicated LodData::VertexI
        /// produced while reading vertex data. See LodInputProviderMesh::addVertexData /
        /// LodInputProviderMeshV2::addVertexData for how this gets populated.
        typedef vector<LodData::VertexI>::type VertexLookupList;

        /** Builds a LodData::Triangle from three raw vertex-buffer indices (translated
            through 'lookup') and registers it with 'data', for both v1 and v2 source
            meshes alike -- once vertex/index data has been read into CPU-side arrays,
            triangle construction itself does not depend on the source mesh format.
        @remarks
            Moved here (promoted from LodInputProviderMesh, which used to be the only
            concrete provider) so LodInputProviderMeshV2 does not need to duplicate it.
            Templated on IndexType since callers read raw indices as either
            'unsigned short' or 'unsigned int' depending on the source index buffer's
            element size.
        */
        template <typename IndexType>
        void addTriangle( LodData *data, IndexType i0, IndexType i1, IndexType i2,
                          VertexLookupList &lookup, unsigned submeshID )
        {
            LodData::Triangle tri;
            tri.vertexID[0] = static_cast<unsigned int>( i0 );
            tri.vertexID[1] = static_cast<unsigned int>( i1 );
            tri.vertexID[2] = static_cast<unsigned int>( i2 );
            tri.vertexi[0] = lookup[i0];
            tri.vertexi[1] = lookup[i1];
            tri.vertexi[2] = lookup[i2];
            // No setter exists for this -- it's a plain public field. isRemoved()/
            // setRemoved() use the all-bits-set sentinel value on this same field, so
            // assigning a real submeshID here is also what marks the triangle as
            // "not removed" (see LodData::Triangle::isRemoved()).
            tri.submeshIDOrRemovedTag = submeshID;

            if( tri.isMalformed() )
            {
                // Degenerate after vertex dedup (e.g. two raw indices that pointed at
                // distinct, but identically-positioned, vertices). Exclude it from
                // collapse calculations the same way isDuplicateTriangle() does.
                data->mIndexBufferInfoList[submeshID].indexCount -= 3;
                return;
            }

            tri.computeNormal( data->mVertexList );

            data->mTriangleList.push_back( tri );
            addTriangleToEdges( data, &data->mTriangleList.back() );
        }
    };

}  // namespace Ogre
#endif