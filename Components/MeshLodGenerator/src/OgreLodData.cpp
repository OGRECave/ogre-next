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
#include "OgreLodData.h"

namespace Ogre
{
    // Use float limits instead of Real limits, because LodConfigSerializer may convert them to float.
    const Real LodData::NEVER_COLLAPSE_COST = std::numeric_limits<float>::max();
    const Real LodData::UNINITIALIZED_COLLAPSE_COST = std::numeric_limits<float>::infinity();

    void LodData::Vertex::addEdge( const LodData::Edge &edge )
    {
        // OgreAssert(edge.dst != this, "");
        VEdges::iterator it;
        it = edges.add( edge );
        if( it == edges.end() )
        {
            edges.back().refCount = 1;
        }
        else
        {
            it->refCount++;
        }
    }

    void LodData::Vertex::removeEdge( const LodData::Edge &edge )
    {
        VEdges::iterator it = edges.findExists( edge );
        if( it->refCount == 1 )
        {
            edges.remove( it );
        }
        else
        {
            it->refCount--;
        }
    }

    bool LodData::VertexEqual::operator()( const LodData::VertexI lhs, const LodData::VertexI rhs ) const
    {
        return mGen->mVertexList[lhs].position == mGen->mVertexList[rhs].position;
    }

    size_t LodData::VertexHash::operator()( const LodData::VertexI vi ) const
    {
        // Stretch the values to an integer grid.
        const LodData::Vertex *v = &mGen->mVertexList[vi];
        Real stretch = (Real)0x7fffffff / mGen->mMeshBoundingSphereRadius;
        int hash = (int)( v->position.x * stretch );
        hash ^= (int)( v->position.y * stretch ) * 0x100;
        hash ^= (int)( v->position.z * stretch ) * 0x10000;
        return (size_t)hash;
    }

    void LodData::Triangle::computeNormal( const VertexList &vertexList )
    {
        // Cross-product 2 edges
        Vector3 e1 = vertexList[vertexi[1]].position - vertexList[vertexi[0]].position;
        Vector3 e2 = vertexList[vertexi[2]].position - vertexList[vertexi[1]].position;

        normal = e1.crossProduct( e2 );
        normal.normalise();
    }

    unsigned int LodData::Triangle::getVertexID( const LodData::VertexI vi ) const
    {
        for( int i = 0; i < 3; i++ )
        {
            if( vertexi[i] == vi )
            {
                return vertexID[i];
            }
        }
        OgreAssert( 0, "" );
        return 0;
    }
    bool LodData::Triangle::isMalformed()
    {
        return vertexi[0] == vertexi[1] || vertexi[0] == vertexi[2] || vertexi[1] == vertexi[2];
    }

    LodData::Edge::Edge( LodData::VertexI destinationi ) :
        dsti( destinationi )
#if OGRE_DEBUG_MODE
        ,
        collapseCost( UNINITIALIZED_COLLAPSE_COST )
#endif
        ,
        refCount( 0 )
    {
    }

    LodData::Edge::Edge( const LodData::Edge &b ) { *this = b; }

    bool LodData::Edge::operator<( const LodData::Edge &other ) const
    {
        return dsti < other.dsti;  // Comparing pointers for uniqueness.
    }

    LodData::Edge &LodData::Edge::operator=( const LodData::Edge &b )
    {
        dsti = b.dsti;
        collapseCost = b.collapseCost;
        refCount = b.refCount;
        return *this;
    }

    bool LodData::Edge::operator==( const LodData::Edge &other ) const { return dsti == other.dsti; }

}  // namespace Ogre
