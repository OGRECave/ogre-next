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

#include "OgreLodInputProviderMesh.h"

#include "OgreBitwise.h"
#include "OgreLodData.h"
#include "OgreMesh.h"
#include "OgreSubMesh.h"

namespace Ogre
{
    LodInputProviderMesh::LodInputProviderMesh( v1::MeshPtr mesh ) : mMesh( mesh ) {}

    void LodInputProviderMesh::initData( LodData *data )
    {
        tuneContainerSize( data );
        initialize( data );
    }
    void LodInputProviderMesh::tuneContainerSize( LodData *data )
    {
        // Get Vertex count for container tuning.
        bool sharedVerticesAdded = false;
        size_t trianglesCount = 0;
        size_t vertexCount = 0;
        size_t vertexLookupSize = 0;
        size_t sharedVertexLookupSize = 0;
        unsigned submeshCount = mMesh->getNumSubMeshes();
        for( unsigned i = 0; i < submeshCount; i++ )
        {
            const v1::SubMesh *submesh = mMesh->getSubMesh( i );
            trianglesCount +=
                getTriangleCount( submesh->operationType, submesh->indexData[VpNormal]->indexCount );
            if( !submesh->useSharedVertices )
            {
                size_t count = submesh->vertexData[VpNormal]->vertexCount;
                vertexLookupSize = std::max<size_t>( vertexLookupSize, count );
                vertexCount += count;
            }
            else if( !sharedVerticesAdded )
            {
                sharedVerticesAdded = true;
                sharedVertexLookupSize = mMesh->sharedVertexData[VpNormal]->vertexCount;
                vertexCount += sharedVertexLookupSize;
            }
        }

        // Tune containers:
        data->mUniqueVertexSet.rehash(
            4 * vertexCount );  // less then 0.25 item/bucket for low collision rate
        data->mTriangleList.reserve( trianglesCount );
        data->mVertexList.reserve( vertexCount );
        mSharedVertexLookup.reserve( sharedVertexLookupSize );
        mVertexLookup.reserve( vertexLookupSize );
        data->mIndexBufferInfoList.resize( submeshCount );
    }

    void LodInputProviderMesh::initialize( LodData *data )
    {
#if OGRE_DEBUG_MODE
        data->mMeshName = mMesh->getName();
#endif
        data->mMeshBoundingSphereRadius = mMesh->getBoundingSphereRadius();
        unsigned submeshCount = mMesh->getNumSubMeshes();
        for( unsigned i = 0; i < submeshCount; ++i )
        {
            const v1::SubMesh *submesh = mMesh->getSubMesh( i );
            v1::VertexData *vertexData = ( submesh->useSharedVertices ? mMesh->sharedVertexData[VpNormal]
                                                                      : submesh->vertexData[VpNormal] );
            addVertexData( data, vertexData, submesh->useSharedVertices );
            if( submesh->indexData[VpNormal]->indexCount > 0 )
                addIndexData( data, submesh->indexData[VpNormal], submesh->useSharedVertices, i,
                              submesh->operationType );
        }

        // These were only needed for addIndexData() and addVertexData().
        mSharedVertexLookup.clear();
        mVertexLookup.clear();
    }
    void LodInputProviderMesh::addVertexData( LodData *data, v1::VertexData *vertexData,
                                              bool useSharedVertexLookup )
    {
        if( ( useSharedVertexLookup &&
              !mSharedVertexLookup.empty() ) )  // We already loaded the shared vertex buffer.
        {
            return;
        }
        OgreAssert( vertexData->vertexCount != 0, "" );

        // Locate position element and the buffer to go with it.
        const v1::VertexElement *elemPos =
            vertexData->vertexDeclaration->findElementBySemantic( VES_POSITION );

        // Only float supported.
        OgreAssert( ( elemPos->getBaseType( elemPos->getType() ) == VET_FLOAT1 &&
                      elemPos->getTypeCount( elemPos->getType() ) >= 3u ) ||
                        elemPos->getType() == VET_HALF4,
                    "Position must be VET_FLOAT3, VET_FLOAT4 or VET_HALF4" );

        const bool bPositionIsHalf = elemPos->getType() == VET_HALF4;

        if( elemPos->getTypeCount( elemPos->getType() ) == 4u && !bPositionIsHalf )
        {
            LogManager::getSingleton().logMessage(
                "LodInputProviderMesh::addVertexData: Position is VET_FLOAT4. "
                "The 4th component will be ignored. This warning is safe to "
                "ignore if it is just filled with 1.0" );
        }

        v1::HardwareVertexBufferSharedPtr vbuf =
            vertexData->vertexBufferBinding->getBuffer( elemPos->getSource() );

        // Lock the buffer for reading.
        v1::HardwareBufferLockGuard vbufLock( vbuf, v1::HardwareBuffer::HBL_READ_ONLY );
        unsigned char *vStart = static_cast<unsigned char *>( vbufLock.pData );
        unsigned char *vertex = vStart;
        size_t vSize = vbuf->getVertexSize();
        unsigned char *vEnd = vertex + vertexData->vertexCount * vSize;

        VertexLookupList &lookup = useSharedVertexLookup ? mSharedVertexLookup : mVertexLookup;
        lookup.clear();

        v1::HardwareVertexBufferSharedPtr vNormalBuf;
        v1::HardwareBufferLockGuard vNormalBufLock;
        unsigned char *vNormal = NULL;
        size_t vNormSize = 0;
        const v1::VertexElement *elemNormal =
            vertexData->vertexDeclaration->findElementBySemantic( VES_NORMAL );

        const bool bNormalIsQTangent = elemNormal->getType() == VET_SHORT4_SNORM;

        data->mUseVertexNormals &= ( elemNormal != NULL );
        if( data->mUseVertexNormals )
        {
            if( elemNormal->getSource() == elemPos->getSource() )
            {
                // Don't open the same buffer twice. Just copy the pointer.
                vNormalBuf = vbuf;
                vNormal = vStart;
            }
            else
            {
                vNormalBuf = vertexData->vertexBufferBinding->getBuffer( elemNormal->getSource() );
                vNormalBufLock.lock( vNormalBuf, v1::HardwareBuffer::HBL_READ_ONLY );
                vNormal = static_cast<unsigned char *>( vNormalBufLock.pData );
            }
            vNormSize = vNormalBuf->getVertexSize();
        }

        // Loop through all vertices and insert them to the Unordered Map.
        for( ; vertex < vEnd; vertex += vSize )
        {
            LodData::VertexI vi = (LodData::VertexI)data->mVertexList.size();
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
                uint16 *pHalf;
                elemPos->baseVertexPointerToElement( vertex, &pHalf );
                v->position.x = Bitwise::halfToFloat( pHalf[0] );
                v->position.y = Bitwise::halfToFloat( pHalf[1] );
                v->position.z = Bitwise::halfToFloat( pHalf[2] );
            }
            else
            {
                float *pFloat;
                elemPos->baseVertexPointerToElement( vertex, &pFloat );
                v->position.x = pFloat[0];
                v->position.y = pFloat[1];
                v->position.z = pFloat[2];
            }
            v->collapseToi = LodData::InvalidIndex;
            std::pair<LodData::UniqueVertexSet::iterator, bool> ret;
            ret = data->mUniqueVertexSet.insert( vi );
            if( !ret.second )
            {
                // Vertex position already exists.
                data->mVertexList.pop_back();
                vi = *ret.first;
                v = &data->mVertexList[vi];  // Point to the existing vertex.
                v->seam = true;
            }
            else
            {
#if OGRE_DEBUG_MODE
                // Needed for an assert, don't remove it.
                v->costHeapPosition = data->mCollapseCostHeap.end();
#endif
                v->seam = false;
            }
            lookup.push_back( vi );

            if( data->mUseVertexNormals )
            {
                float *pFloat;
                float tmpFloat[3];
                if( bNormalIsQTangent )
                {
                    int16 *srcData16;
                    elemNormal->baseVertexPointerToElement( vNormal,
                                                            reinterpret_cast<uint16 **>( &srcData16 ) );

                    Quaternion qTangent;
                    qTangent.x = Bitwise::snorm16ToFloat( srcData16[0] );
                    qTangent.y = Bitwise::snorm16ToFloat( srcData16[1] );
                    qTangent.z = Bitwise::snorm16ToFloat( srcData16[2] );
                    qTangent.w = Bitwise::snorm16ToFloat( srcData16[3] );
                    float reflection = 1.0f;
                    if( qTangent.w < 0 )
                        reflection = -1.0f;

                    Vector3 decompressedNormal = qTangent.xAxis();

                    pFloat = tmpFloat;
                    tmpFloat[0] = decompressedNormal.x;
                    tmpFloat[1] = decompressedNormal.y;
                    tmpFloat[2] = decompressedNormal.z;
                }
                else
                {
                    elemNormal->baseVertexPointerToElement( vNormal, &pFloat );
                }

                if( !ret.second )
                {
                    if( v->normal.x != pFloat[0] )
                    {
                        v->normal.x += pFloat[0];
                        v->normal.y += pFloat[1];
                        v->normal.z += pFloat[2];
                        if( v->normal.isZeroLength() )
                        {
                            v->normal = Vector3( 1.0, 0.0, 0.0 );
                        }
                        v->normal.normalise();
                    }
                }
                else
                {
                    v->normal.x = pFloat[0];
                    v->normal.y = pFloat[1];
                    v->normal.z = pFloat[2];
                    v->normal.normalise();
                }
                vNormal += vNormSize;
            }
        }
    }
    void LodInputProviderMesh::addIndexData( LodData *data, v1::IndexData *indexData,
                                             bool useSharedVertexLookup, unsigned submeshID,
                                             OperationType op )
    {
        const v1::HardwareIndexBufferSharedPtr &ibuf = indexData->indexBuffer;
        size_t isize = ibuf->getIndexSize();
        size_t numIndices = indexData->indexCount;
        data->mIndexBufferInfoList[submeshID].indexSize = isize;
        data->mIndexBufferInfoList[submeshID].indexCount = numIndices;
        if( numIndices == 0 )
        {
            // Locking a zero length buffer on Linux with nvidia cards fails.
            return;
        }
        VertexLookupList &lookup = useSharedVertexLookup ? mSharedVertexLookup : mVertexLookup;

        // Lock the buffer for reading.
        v1::HardwareBufferLockGuard ibufLock( ibuf, v1::HardwareBuffer::HBL_READ_ONLY );
        if( isize == sizeof( unsigned short ) )
        {
            unsigned short *indices = static_cast<unsigned short *>( ibufLock.pData );
            switch( op )
            {
            case OT_TRIANGLE_LIST:  // (0,1,2),(3,4,5),(6,7,8),...
                for( size_t i0 = 0; i0 + 2 < numIndices; i0 += 3 )
                    addTriangle( data, indices[i0], indices[i0 + 1], indices[i0 + 2], lookup,
                                 submeshID );
                break;

            case OT_TRIANGLE_STRIP:  // (0,1,2),(2,1,3),(2,3,4),...
                for( size_t i0 = 0, i1 = 1, i2 = 2; i2 < numIndices;
                     ( i2 & 1 ) ? i1 = i2 : i0 = i2, ++i2 )
                    addTriangle( data, indices[i0], indices[i1], indices[i2], lookup, submeshID );
                break;

            case OT_TRIANGLE_FAN:  // (0,1,2),(0,2,3),(0,3,4),...
                for( size_t i1 = 1; i1 + 1 < numIndices; ++i1 )
                    addTriangle( data, indices[0], indices[i1], indices[i1 + 1], lookup, submeshID );
                break;

            default:
                break;
            }
        }
        else
        {
            // Unsupported index size.
            OgreAssert( isize == sizeof( unsigned int ), "" );
            unsigned int *indices = static_cast<unsigned int *>( ibufLock.pData );
            switch( op )
            {
            case OT_TRIANGLE_LIST:  // (0,1,2),(3,4,5),(6,7,8),...
                for( size_t i0 = 0; i0 + 2 < numIndices; i0 += 3 )
                    addTriangle( data, indices[i0], indices[i0 + 1], indices[i0 + 2], lookup,
                                 submeshID );
                break;

            case OT_TRIANGLE_STRIP:  // (0,1,2),(2,1,3),(2,3,4),...
                for( size_t i0 = 0, i1 = 1, i2 = 2; i2 < numIndices;
                     ( i2 & 1 ) ? i1 = i2 : i0 = i2, ++i2 )
                    addTriangle( data, indices[i0], indices[i1], indices[i2], lookup, submeshID );
                break;

            case OT_TRIANGLE_FAN:  // (0,1,2),(0,2,3),(0,3,4),...
                for( size_t i1 = 1; i1 + 1 < numIndices; ++i1 )
                    addTriangle( data, indices[0], indices[i1], indices[i1 + 1], lookup, submeshID );
                break;

            default:
                break;
            }
        }
    }

}  // namespace Ogre
