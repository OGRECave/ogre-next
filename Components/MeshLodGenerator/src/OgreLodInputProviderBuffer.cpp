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

#include "OgreLodInputProviderBuffer.h"

#include "OgreLodData.h"
#include "OgreVector3.h"

namespace Ogre
{
    LodInputProviderBuffer::LodInputProviderBuffer( v1::MeshPtr mesh ) { mBuffer.fillBuffer( mesh ); }

    void LodInputProviderBuffer::initData( LodData *data )
    {
        tuneContainerSize( data );
        initialize( data );
    }

    void LodInputProviderBuffer::tuneContainerSize( LodData *data )
    {
        // Get Vertex count for container tuning.
        bool sharedVerticesAdded = false;
        size_t trianglesCount = 0;
        size_t vertexCount = 0;
        size_t vertexLookupSize = 0;
        size_t sharedVertexLookupSize = 0;
        const size_t submeshCount = mBuffer.submesh.size();
        for( size_t i = 0; i < submeshCount; i++ )
        {
            const LodInputBuffer::Submesh &submesh = mBuffer.submesh[i];
            trianglesCount += getTriangleCount( submesh.operationType, submesh.indexBuffer.indexCount );
            if( !submesh.useSharedVertexBuffer )
            {
                size_t count = submesh.vertexBuffer.vertexCount;
                vertexLookupSize = std::max( vertexLookupSize, count );
                vertexCount += count;
            }
            else if( !sharedVerticesAdded )
            {
                sharedVerticesAdded = true;
                sharedVertexLookupSize = mBuffer.sharedVertexBuffer.vertexCount;
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

    void LodInputProviderBuffer::initialize( LodData *data )
    {
#if OGRE_DEBUG_MODE
        data->mMeshName = mBuffer.meshName;
#endif
        data->mMeshBoundingSphereRadius = mBuffer.boundingSphereRadius;
        unsigned submeshCount = (unsigned)mBuffer.submesh.size();
        for( unsigned i = 0; i < submeshCount; ++i )
        {
            LodInputBuffer::Submesh &submesh = mBuffer.submesh[i];
            LodVertexBuffer &vertexBuffer =
                ( submesh.useSharedVertexBuffer ? mBuffer.sharedVertexBuffer : submesh.vertexBuffer );
            addVertexData( data, vertexBuffer, submesh.useSharedVertexBuffer );
            addIndexData( data, submesh.indexBuffer, submesh.useSharedVertexBuffer, i,
                          submesh.operationType );
        }

        // These were only needed for addIndexData() and addVertexData().
        mSharedVertexLookup.clear();
        mVertexLookup.clear();
        mBuffer.clear();
    }
    void LodInputProviderBuffer::addVertexData( LodData *data, LodVertexBuffer &vertexBuffer,
                                                bool useSharedVertexLookup )
    {
        if( useSharedVertexLookup && !mSharedVertexLookup.empty() )
        {
            return;  // We already loaded the shared vertex buffer.
        }

        VertexLookupList &lookup = useSharedVertexLookup ? mSharedVertexLookup : mVertexLookup;
        lookup.clear();

        Vector3 *pNormalOut = vertexBuffer.vertexNormalBuffer.get();
        data->mUseVertexNormals = data->mUseVertexNormals && ( pNormalOut != NULL );

        // Loop through all vertices and insert them to the Unordered Map.
        Vector3 *pOut = vertexBuffer.vertexBuffer.get();
        Vector3 *pEnd = pOut + vertexBuffer.vertexCount;
        for( ; pOut < pEnd; pOut++ )
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
            v->position = *pOut;
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
                if( data->mUseVertexNormals )
                {
                    if( v->normal.x != ( *pNormalOut ).x )
                    {
                        v->normal += *pNormalOut;
                        if( v->normal.isZeroLength() )
                        {
                            v->normal = Vector3( 1.0, 0.0, 0.0 );
                        }
                        v->normal.normalise();
                    }
                    pNormalOut++;
                }
            }
            else
            {
#if OGRE_DEBUG_MODE
                v->costHeapPosition = data->mCollapseCostHeap.end();
#endif
                v->seam = false;
                if( data->mUseVertexNormals )
                {
                    v->normal = *pNormalOut;
                    v->normal.normalise();
                    pNormalOut++;
                }
            }
            lookup.push_back( vi );
        }
    }

    void LodInputProviderBuffer::addIndexData( LodData *data, LodIndexBuffer &indexBuffer,
                                               bool useSharedVertexLookup, unsigned submeshID,
                                               OperationType op )
    {
        size_t isize = indexBuffer.indexSize;
        size_t numIndices = indexBuffer.indexCount;
        data->mIndexBufferInfoList[submeshID].indexSize = isize;
        data->mIndexBufferInfoList[submeshID].indexCount = numIndices;
        VertexLookupList &lookup = useSharedVertexLookup ? mSharedVertexLookup : mVertexLookup;

        // Lock the buffer for reading.
        void *iStart = indexBuffer.indexBuffer.get();
        if( !iStart )
            return;

        if( isize == sizeof( unsigned short ) )
        {
            unsigned short *indices = static_cast<unsigned short *>( iStart );
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
            unsigned int *indices = static_cast<unsigned int *>( iStart );
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
