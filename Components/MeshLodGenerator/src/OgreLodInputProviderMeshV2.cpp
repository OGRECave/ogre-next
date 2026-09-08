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

#include "OgreLodInputProviderMeshV2.h"

#include "OgreBitwise.h"
#include "OgreLodData.h"
#include "OgreMesh2.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreAsyncTicket.h"
#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreVertexArrayObject.h"
#include "Vao/OgreVertexBufferPacked.h"

namespace Ogre
{
    LodInputProviderMeshV2::LodInputProviderMeshV2( MeshPtr mesh ) : mMesh( mesh ) {}

    void LodInputProviderMeshV2::initData( LodData *data )
    {
        tuneContainerSize( data );
        initialize( data );
    }

    void LodInputProviderMeshV2::tuneContainerSize( LodData *data )
    {
        size_t trianglesCount = 0;
        size_t vertexCount = 0;
        size_t vertexLookupSize = 0;
        unsigned submeshCount = mMesh->getNumSubMeshes();

        for( unsigned i = 0; i < submeshCount; ++i )
        {
            const SubMesh *subMesh = mMesh->getSubMesh( i );

            OgreAssert( !subMesh->mVao[VpNormal].empty(),
                        "SubMesh has no LOD-0 Vao. Was the mesh actually loaded/built?" );

            VertexArrayObject *vao = subMesh->mVao[VpNormal][0];
            const size_t vertexCountThisSub = vao->getVertexBuffers()[0]->getNumElements();
            // Non-indexed VAOs have no index buffer at all; in that case every vertex
            // is consumed directly by the operation, so the vertex count itself is
            // the right triangle-count input (there is no VertexArrayObject::
            // getNumElements() to call here).
            const size_t indexCountThisSub =
                vao->getIndexBuffer() ? vao->getIndexBuffer()->getNumElements() : vertexCountThisSub;

            trianglesCount += getTriangleCount( vao->getOperationType(), indexCountThisSub );
            vertexLookupSize = std::max<size_t>( vertexLookupSize, vertexCountThisSub );
            vertexCount += vertexCountThisSub;
        }

        // Less than 0.25 item/bucket for low collision rate, same tuning ratio
        // LodInputProviderMesh uses for v1 meshes.
        data->mUniqueVertexSet.rehash( 4 * vertexCount );
        data->mTriangleList.reserve( trianglesCount );
        data->mVertexList.reserve( vertexCount );
        mVertexLookup.reserve( vertexLookupSize );
        data->mIndexBufferInfoList.resize( submeshCount );
    }

    void LodInputProviderMeshV2::initialize( LodData *data )
    {
#if OGRE_DEBUG_MODE
        data->mMeshName = mMesh->getName();
#endif
        data->mMeshBoundingSphereRadius = mMesh->getBoundingSphereRadius();
        unsigned submeshCount = mMesh->getNumSubMeshes();

        for( unsigned i = 0; i < submeshCount; ++i )
        {
            SubMesh *subMesh = mMesh->getSubMesh( i );
            addVertexData( data, subMesh, i );

            const size_t indexCount =
                subMesh->mVao[VpNormal][0]->getIndexBuffer()
                    ? subMesh->mVao[VpNormal][0]->getIndexBuffer()->getNumElements()
                    : 0u;
            if( indexCount > 0u )
                addIndexData( data, subMesh, i );
        }

        // Only needed for addIndexData() within this submesh's iteration.
        mVertexLookup.clear();
    }

    void LodInputProviderMeshV2::addVertexData( LodData *data, SubMesh *subMesh, unsigned submeshID )
    {
        OGRE_UNUSED_VAR( submeshID );

        VertexArrayObject *vao = subMesh->mVao[VpNormal][0];
        const size_t vertexCount = vao->getVertexBuffers()[0]->getNumElements();
        OgreAssert( vertexCount != 0, "" );

        VertexArrayObject::ReadRequestsVec requests;
        requests.push_back( VertexArrayObject::ReadRequests( VES_POSITION ) );
        requests.push_back( VertexArrayObject::ReadRequests( VES_NORMAL ) );

        vao->readRequests( requests );
        vao->mapAsyncTickets( requests );

        const bool bPositionIsHalf = requests[0].type == VET_HALF4;
        const bool bHasNormal = requests[1].data != 0;
        const bool bNormalIsQTangent = bHasNormal && requests[1].type == VET_SHORT4_SNORM;
        const bool bNormalIsHalf = bHasNormal && requests[1].type == VET_HALF4;

        data->mUseVertexNormals &= bHasNormal;

        mVertexLookup.clear();

        for( size_t vi = 0; vi < vertexCount; ++vi )
        {
            LodData::VertexI lvi = (LodData::VertexI)data->mVertexList.size();
            {
                LodData::Vertex tmp;
                tmp.position = Vector3::ZERO;
                tmp.normal = Vector3::ZERO;
                tmp.seam = false;
                data->mVertexList.push_back( tmp );
            }
            LodData::Vertex *v = &data->mVertexList.back();

            if( bPositionIsHalf )
            {
                const uint16 *p = reinterpret_cast<const uint16 *>( requests[0].data );
                v->position.x = Bitwise::halfToFloat( p[0] );
                v->position.y = Bitwise::halfToFloat( p[1] );
                v->position.z = Bitwise::halfToFloat( p[2] );
            }
            else
            {
                const float *p = reinterpret_cast<const float *>( requests[0].data );
                v->position.x = p[0];
                v->position.y = p[1];
                v->position.z = p[2];
            }
            v->collapseToi = LodData::InvalidIndex;

            std::pair<LodData::UniqueVertexSet::iterator, bool> ret;
            ret = data->mUniqueVertexSet.insert( lvi );
            if( !ret.second )
            {
                // Vertex position already exists.
                data->mVertexList.pop_back();
                lvi = *ret.first;
                v = &data->mVertexList[lvi];
                v->seam = true;
            }
            else
            {
#if OGRE_DEBUG_MODE
                v->costHeapPosition = data->mCollapseCostHeap.end();
#endif
                v->seam = false;
            }
            mVertexLookup.push_back( lvi );

            if( data->mUseVertexNormals )
            {
                Vector3 normal;
                if( bNormalIsQTangent )
                {
                    const int16 *q = reinterpret_cast<const int16 *>( requests[1].data );
                    Quaternion qTangent;
                    qTangent.x = Bitwise::snorm16ToFloat( q[0] );
                    qTangent.y = Bitwise::snorm16ToFloat( q[1] );
                    qTangent.z = Bitwise::snorm16ToFloat( q[2] );
                    qTangent.w = Bitwise::snorm16ToFloat( q[3] );
                    normal = qTangent.xAxis();
                }
                else if( bNormalIsHalf )
                {
                    const uint16 *n = reinterpret_cast<const uint16 *>( requests[1].data );
                    normal.x = Bitwise::halfToFloat( n[0] );
                    normal.y = Bitwise::halfToFloat( n[1] );
                    normal.z = Bitwise::halfToFloat( n[2] );
                }
                else
                {
                    const float *n = reinterpret_cast<const float *>( requests[1].data );
                    normal.x = n[0];
                    normal.y = n[1];
                    normal.z = n[2];
                }

                if( !ret.second )
                {
                    if( v->normal.x != normal.x || v->normal.y != normal.y || v->normal.z != normal.z )
                    {
                        v->normal += normal;
                        if( v->normal.isZeroLength() )
                            v->normal = Vector3( 1.0, 0.0, 0.0 );
                        v->normal.normalise();
                    }
                }
                else
                {
                    v->normal = normal;
                    v->normal.normalise();
                }
            }

            requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
            if( bHasNormal )
                requests[1].data += requests[1].vertexBuffer->getBytesPerElement();
        }

        vao->unmapAsyncTickets( requests );
    }

    void LodInputProviderMeshV2::addIndexData( LodData *data, SubMesh *subMesh, unsigned submeshID )
    {
        VertexArrayObject *vao = subMesh->mVao[VpNormal][0];
        IndexBufferPacked *ibuf = vao->getIndexBuffer();
        const size_t numIndices = ibuf->getNumElements();
        const bool is32Bit = ibuf->getIndexType() == IndexBufferPacked::IT_32BIT;

        data->mIndexBufferInfoList[submeshID].indexSize = is32Bit ? 4u : 2u;
        data->mIndexBufferInfoList[submeshID].indexCount = numIndices;

        if( numIndices == 0 )
            return;

        // Prefer the shadow copy (instant, synchronous) when present; only fall back
        // to the async ticket round-trip for non-shadowed (e.g. BT_IMMUTABLE without a
        // shadow buffer) index buffers. Mirrors the pattern already used elsewhere in
        // this codebase for reading back index data on the CPU.
        const void *raw = ibuf->getShadowCopy();
        AsyncTicketPtr ticket;
        if( !raw )
        {
            ticket = ibuf->readRequest( 0u, numIndices );
            raw = ticket->map();
        }

        const OperationType op = vao->getOperationType();

        if( is32Bit )
        {
            const uint32 *indices = reinterpret_cast<const uint32 *>( raw );
            switch( op )
            {
            case OT_TRIANGLE_LIST:
                for( size_t i0 = 0; i0 + 2 < numIndices; i0 += 3 )
                    addTriangle( data, indices[i0], indices[i0 + 1], indices[i0 + 2], mVertexLookup,
                                 submeshID );
                break;
            case OT_TRIANGLE_STRIP:
                for( size_t i0 = 0, i1 = 1, i2 = 2; i2 < numIndices;
                     ( i2 & 1 ) ? i1 = i2 : i0 = i2, ++i2 )
                    addTriangle( data, indices[i0], indices[i1], indices[i2], mVertexLookup, submeshID );
                break;
            case OT_TRIANGLE_FAN:
                for( size_t i1 = 1; i1 + 1 < numIndices; ++i1 )
                    addTriangle( data, indices[0], indices[i1], indices[i1 + 1], mVertexLookup,
                                 submeshID );
                break;
            default:
                break;
            }
        }
        else
        {
            const uint16 *indices = reinterpret_cast<const uint16 *>( raw );
            switch( op )
            {
            case OT_TRIANGLE_LIST:
                for( size_t i0 = 0; i0 + 2 < numIndices; i0 += 3 )
                    addTriangle( data, indices[i0], indices[i0 + 1], indices[i0 + 2], mVertexLookup,
                                 submeshID );
                break;
            case OT_TRIANGLE_STRIP:
                for( size_t i0 = 0, i1 = 1, i2 = 2; i2 < numIndices;
                     ( i2 & 1 ) ? i1 = i2 : i0 = i2, ++i2 )
                    addTriangle( data, indices[i0], indices[i1], indices[i2], mVertexLookup, submeshID );
                break;
            case OT_TRIANGLE_FAN:
                for( size_t i1 = 1; i1 + 1 < numIndices; ++i1 )
                    addTriangle( data, indices[0], indices[i1], indices[i1 + 1], mVertexLookup,
                                 submeshID );
                break;
            default:
                break;
            }
        }

        if( ticket )
            ticket->unmap();
    }

}  // namespace Ogre