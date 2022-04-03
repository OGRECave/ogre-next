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

#include "OgreLodCollapseCostProfiler.h"

namespace Ogre
{
    Real LodCollapseCostProfiler::computeEdgeCollapseCost( LodData *data, LodData::VertexI srci,
                                                           LodData::Edge *dstEdge )
    {
        OgreAssert( 0, "Only computeVertexCollapseCost should call this function." );
        return 0;
    }

    void LodCollapseCostProfiler::computeVertexCollapseCost( LodData *data, LodData::VertexI vertexi,
                                                             Real &collapseCost,
                                                             LodData::VertexI &collapseToi )
    {
        LodData::Vertex *vertex = &data->mVertexList[vertexi];
        LodData::VEdges::iterator it = vertex->edges.begin();
        if( !mHasProfile[vertexi] )
        {
            for( ; it != vertex->edges.end(); ++it )
            {
                it->collapseCost = mCostCalculator->computeEdgeCollapseCost( data, vertexi, &*it );
                if( collapseCost > it->collapseCost )
                {
                    collapseCost = it->collapseCost;
                    collapseToi = it->dsti;
                }
            }
        }
        else
        {
            std::pair<ProfileLookup::iterator, ProfileLookup::iterator> ret =
                mProfileLookup.equal_range( vertexi );
            for( ; it != vertex->edges.end(); ++it )
            {
                it->collapseCost = LodData::UNINITIALIZED_COLLAPSE_COST;
                for( ProfileLookup::iterator it2 = ret.first; it2 != ret.second; ++it2 )
                {
                    if( it2->second.dsti == it->dsti )
                    {
                        it->collapseCost = it2->second.cost;
                        break;
                    }
                }
                if( it->collapseCost == LodData::UNINITIALIZED_COLLAPSE_COST )
                {
                    it->collapseCost = mCostCalculator->computeEdgeCollapseCost( data, vertexi, &*it );
                }
                if( collapseCost > it->collapseCost )
                {
                    collapseCost = it->collapseCost;
                    collapseToi = it->dsti;
                }
            }
        }
    }

    void LodCollapseCostProfiler::injectProfile( LodData *data )
    {
        mHasProfile.clear();
        mHasProfile.resize( data->mVertexList.size(), false );
        LodProfile::iterator it = mProfile.begin();
        LodProfile::iterator itEnd = mProfile.end();
        for( ; it != itEnd; it++ )
        {
            LodData::UniqueVertexSet::iterator src = data->findUniqueVertexByPos( it->src );
            OgreAssert( src != data->mUniqueVertexSet.end(), "Invalid vertex position in Lod profile" );
            mHasProfile[*src] = true;
            LodData::UniqueVertexSet::iterator dst = data->findUniqueVertexByPos( it->dst );
            OgreAssert( dst != data->mUniqueVertexSet.end(), "Invalid vertex position in Lod profile" );
            ProfiledEdge e;
            e.dsti = *dst;
            e.cost = it->cost;
            OgreAssert( e.cost >= 0 && e.cost != LodData::UNINITIALIZED_COLLAPSE_COST,
                        "Invalid collapse cost" );
            mProfileLookup.insert( ProfileLookup::value_type( *src, e ) );
        }
    }

    void LodCollapseCostProfiler::initCollapseCosts( LodData *data )
    {
        injectProfile( data );
        mProfile.clear();
        mCostCalculator->initCollapseCosts( data );
    }

}  // namespace Ogre
