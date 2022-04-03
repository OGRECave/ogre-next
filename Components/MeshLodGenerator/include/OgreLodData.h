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

#ifndef _LodData_H__
#define _LodData_H__

#include "OgreLodPrerequisites.h"

#include "OgreVector3.h"
#include "OgreVectorSet.h"
#include "OgreVectorSetImpl.h"

#include "ogrestd/map.h"
#include "ogrestd/unordered_set.h"
#include "ogrestd/vector.h"

#ifndef MESHLOD_QUALITY
/// MESHLOD_QUALITY=1 is fastest processing time.
/// MESHLOD_QUALITY=2 is balanced performance/quality (default)
/// MESHLOD_QUALITY=3 is best quality.
#    define MESHLOD_QUALITY 2
#endif

namespace Ogre
{
    struct _OgreLodExport LodData
    {
        static const Real NEVER_COLLAPSE_COST /*= std::numeric_limits<Real>::max()*/;
        static const Real UNINITIALIZED_COLLAPSE_COST /*= std::numeric_limits<Real>::infinity()*/;

        struct Edge;
        struct Vertex;
        struct Triangle;
        struct VertexHash;
        struct VertexEqual;

        typedef unsigned VertexI;    // offset in mVertexList
        typedef unsigned TriangleI;  // offset in mTriangleList
        enum
        {
            InvalidIndex = (unsigned)-1
        };

        typedef vector<Vertex>::type          VertexList;
        typedef vector<Triangle>::type        TriangleList;
        typedef multimap<Real, VertexI>::type CollapseCostHeap;
        typedef VectorSet<Edge, 8>            VEdges;
        typedef VectorSet<TriangleI, 7>       VTriangles;

        // Hash function for UniqueVertexSet.
        struct VertexHash
        {
            LodData *mGen;

            VertexHash() : mGen( 0 ) { assert( 0 ); }
            VertexHash( LodData *gen ) { mGen = gen; }
            size_t operator()( const VertexI v ) const;
        };

        // Equality function for UniqueVertexSet.
        struct VertexEqual
        {
            LodData *mGen;

            VertexEqual() : mGen( 0 ) { assert( 0 ); }
            VertexEqual( LodData *gen ) { mGen = gen; }
            bool operator()( const VertexI lhs, const VertexI rhs ) const;
        };

        // Directed edge
        struct Edge
        {
            VertexI dsti;          // destination vertex. (other end of the edge)
            Real    collapseCost;  // cost of the edge.
            int refCount;  // Reference count on how many triangles are using this edge. The edge will be
                           // removed when it gets 0.

            explicit Edge( VertexI destinationi );
            bool  operator==( const Edge &other ) const;
            Edge &operator=( const Edge &b );
            Edge( const Edge &b );
            bool operator<( const Edge &other ) const;
        };

        struct Vertex
        {
            Vector3    position;
            Vector3    normal;
            VEdges     edges;
            VTriangles triangles;

            VertexI                    collapseToi;
            bool                       seam;
            CollapseCostHeap::iterator costHeapPosition;  /// Iterator pointing to the position in the
                                                          /// mCollapseCostSet, which allows fast remove.

            void addEdge( const Edge &edge );
            void removeEdge( const Edge &edge );
        };

        struct Triangle
        {
            VertexI      vertexi[3];
            Vector3      normal;
            unsigned int submeshIDOrRemovedTag;  /// ID of the submesh. Usable with mMesh.getSubMesh()
                                                 /// function. Holds ~0U for removed triangles
            unsigned int vertexID[3];  /// Vertex ID in the buffer associated with the submeshID.

            unsigned int submeshID() const
            {
                OgreAssert( !isRemoved(), "" );
                return submeshIDOrRemovedTag;
            }
            bool isRemoved() const { return submeshIDOrRemovedTag == ~0U; }
            void setRemoved() { submeshIDOrRemovedTag = ~0U; }
            void computeNormal( const VertexList &vertexList );
            bool hasVertex( const VertexI vi ) const
            {
                return ( vi == vertexi[0] || vi == vertexi[1] || vi == vertexi[2] );
            }
            unsigned int getVertexID( const VertexI vi ) const;
            bool         isMalformed();
        };

        union IndexBufferPointer
        {
            unsigned short *pshort;
            unsigned int   *pint;
        };

        struct IndexBufferInfo
        {
            size_t             indexSize;
            size_t             indexCount;
            IndexBufferPointer buf;                 // Used by output providers only!
            size_t             prevOnlyIndexCount;  // Used by output providers only!
            size_t             prevIndexCount;      // Used by output providers only!
        };

        typedef vector<IndexBufferInfo>::type IndexBufferInfoList;

        typedef unordered_set<VertexI, VertexHash, VertexEqual>::type UniqueVertexSet;

        /// Provides position based vertex lookup. Position is the real identifier of a vertex.
        UniqueVertexSet mUniqueVertexSet;

        VertexList   mVertexList;
        TriangleList mTriangleList;

        /// Makes possible to get the vertices with the smallest collapse cost.
        CollapseCostHeap    mCollapseCostHeap;
        IndexBufferInfoList mIndexBufferInfoList;
#if OGRE_DEBUG_MODE
        /**
         * @brief The name of the mesh being processed.
         *
         * This is separate from mMesh in order to allow for access from background threads.
         */
        String mMeshName;
#endif
        Real mMeshBoundingSphereRadius;
        bool mUseVertexNormals;

        template <typename T, typename A>
        static size_t getVectorIDFromPointer( const std::vector<T, A> &vec, const T *pointer )
        {
            size_t id = static_cast<size_t>( pointer - &vec[0] );
            OgreAssert( id < vec.size() && ( &vec[id] == pointer ), "Invalid pointer" );
            return id;
        }

        UniqueVertexSet::iterator findUniqueVertexByPos( const Vector3 &pos )
        {
            UniqueVertexSet::iterator it = mUniqueVertexSet.begin();
            UniqueVertexSet::iterator itEnd = mUniqueVertexSet.end();
            for( ; it != itEnd; ++it )
            {
                Vertex *v = &mVertexList[*it];
                if( v->position == pos )
                    break;
            }
            return it;
        }

#if OGRE_COMPILER == OGRE_COMPILER_MSVC
// We know it's safe to pass this pointer to VertexHash and VertexEqual because it's not used there yet.
#    pragma warning( push )
#    pragma warning( disable : 4355 )
#endif
        LodData() :
            mUniqueVertexSet( (UniqueVertexSet::size_type)0,
                              (const UniqueVertexSet::hasher &)VertexHash( this ),
                              (const UniqueVertexSet::key_equal &)VertexEqual( this ) ),
            mMeshBoundingSphereRadius( 0.0f ),
            mUseVertexNormals( true )
        {
        }
#if OGRE_COMPILER == OGRE_COMPILER_MSVC
#    pragma warning( pop )
#endif
    };

}  // namespace Ogre
#endif
