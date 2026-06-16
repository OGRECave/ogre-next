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

#include "OgreLodCollapseCost.h"

#include "OgreLogManager.h"

#include <sstream>

namespace Ogre
{
    void LodCollapseCost::initCollapseCosts( LodData *data )
    {
        data->mCollapseCostHeap.clear();
        LodData::VertexList::iterator it = data->mVertexList.begin();
        LodData::VertexList::iterator itEnd = data->mVertexList.end();
        LodData::VertexI vi = 0;
        for( ; it != itEnd; ++it, ++vi )
        {
            if( !it->edges.empty() )
            {
                initVertexCollapseCost( data, vi );
            }
            else
            {
#if OGRE_DEBUG_MODE
                LogManager::getSingleton().stream()
                    << "In " << data->mMeshName
                    << " never used vertex found with ID: " << data->mCollapseCostHeap.size() << ". "
                    << "Vertex position: (" << it->position.x << ", " << it->position.y << ", "
                    << it->position.z << ") " << "It will be excluded from Lod level calculations.";
#endif
            }
        }
    }

    void LodCollapseCost::computeVertexCollapseCost( LodData *data, LodData::VertexI vertexi,
                                                     Real &collapseCost, LodData::VertexI &collapseToi )
    {
        LodData::Vertex *vertex = &data->mVertexList[vertexi];
        LodData::VEdges::iterator it = vertex->edges.begin();
        for( ; it != vertex->edges.end(); ++it )
        {
            it->collapseCost = computeEdgeCollapseCost( data, vertexi, &*it );
            if( collapseCost > it->collapseCost )
            {
                collapseCost = it->collapseCost;
                collapseToi = it->dsti;
            }
        }
    }
    void LodCollapseCost::initVertexCollapseCost( LodData *data, LodData::VertexI vertexi )
    {
        LodData::Vertex *vertex = &data->mVertexList[vertexi];
        OgreAssert( !vertex->edges.empty(), "" );

        Real collapseCost = LodData::UNINITIALIZED_COLLAPSE_COST;
        LodData::VertexI collapseToi = LodData::InvalidIndex;
        computeVertexCollapseCost( data, vertexi, collapseCost, collapseToi );

        vertex->collapseToi = collapseToi;
        vertex->costHeapPosition = data->mCollapseCostHeap.emplace( collapseCost, vertexi );
    }

    void LodCollapseCost::updateVertexCollapseCost( LodData *data, LodData::VertexI vertexi )
    {
        Real collapseCost = LodData::UNINITIALIZED_COLLAPSE_COST;
        LodData::VertexI collapseToi = LodData::InvalidIndex;
        computeVertexCollapseCost( data, vertexi, collapseCost, collapseToi );

        LodData::Vertex *vertex = &data->mVertexList[vertexi];
        if( vertex->collapseToi != collapseToi || collapseCost != vertex->costHeapPosition->first )
        {
            OgreAssert( vertex->costHeapPosition != data->mCollapseCostHeap.end(), "" );
            data->mCollapseCostHeap.erase( vertex->costHeapPosition );
            if( collapseCost != LodData::UNINITIALIZED_COLLAPSE_COST )
            {
                vertex->collapseToi = collapseToi;
                vertex->costHeapPosition = data->mCollapseCostHeap.emplace( collapseCost, vertexi );
            }
            else
            {
#if OGRE_DEBUG_MODE
                vertex->collapseToi = LodData::InvalidIndex;
                vertex->costHeapPosition = data->mCollapseCostHeap.end();
#endif
            }
        }
    }

    bool LodCollapseCost::isBorderVertex( const LodData::Vertex *vertex ) const
    {
        LodData::VEdges::const_iterator it = vertex->edges.begin();
        LodData::VEdges::const_iterator itEnd = vertex->edges.end();
        for( ; it != itEnd; ++it )
        {
            if( it->refCount == 1 )
            {
                return true;
            }
        }
        return false;
    }
}  // namespace Ogre
