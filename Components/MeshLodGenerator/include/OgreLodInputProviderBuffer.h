
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

#ifndef _LodInputProviderBuffer_H__
#define _LodInputProviderBuffer_H__

#include "OgreLodPrerequisites.h"

#include "OgreLodBuffer.h"
#include "OgreLodData.h"
#include "OgreLodInputProvider.h"
#include "OgreLogManager.h"

#include <sstream>

namespace Ogre
{
    class _OgreLodExport LodInputProviderBuffer : public LodInputProvider
    {
    public:
        LodInputProviderBuffer( v1::MeshPtr mesh );
        /// Called when the data should be filled with the input.
        void initData( LodData *data ) override;

    protected:
        LodInputBuffer mBuffer;

        typedef vector<LodData::VertexI>::type VertexLookupList;
        // This helps to find the vertex* in LodData for index buffer indices
        VertexLookupList mSharedVertexLookup;
        VertexLookupList mVertexLookup;

        void tuneContainerSize( LodData *data );
        void initialize( LodData *data );
        void addVertexData( LodData *data, LodVertexBuffer &vertexBuffer, bool useSharedVertexLookup );
        void addIndexData( LodData *data, LodIndexBuffer &indexBuffer, bool useSharedVertexLookup,
                           unsigned submeshID, OperationType op );
        template <typename IndexType>
        void addTriangle( LodData *data, IndexType i0, IndexType i1, IndexType i2,
                          VertexLookupList &lookup, unsigned submeshID )
        {
            // It should never reallocate or every pointer will be invalid.
            OgreAssert( data->mTriangleList.capacity() > data->mTriangleList.size(), "" );
            data->mTriangleList.push_back( LodData::Triangle() );
            LodData::Triangle *tri = &data->mTriangleList.back();
            tri->submeshIDOrRemovedTag = submeshID;
            // Invalid index: Index is bigger then vertex buffer size.
            OgreAssert( i0 < lookup.size() && i1 < lookup.size() && i2 < lookup.size(), "" );
            tri->vertexID[0] = i0;
            tri->vertexID[1] = i1;
            tri->vertexID[2] = i2;
            tri->vertexi[0] = lookup[i0];
            tri->vertexi[1] = lookup[i1];
            tri->vertexi[2] = lookup[i2];

            if( tri->isMalformed() )
            {
#if OGRE_DEBUG_MODE
                stringstream str;
                str << "In " << data->mMeshName << " malformed triangle found with ID: "
                    << LodData::getVectorIDFromPointer( data->mTriangleList, tri ) << ". " << std::endl;
                printTriangle( data, tri, str );
                str << "It will be excluded from Lod level calculations.";
                LogManager::getSingleton().stream() << str.str();
#endif
                data->mIndexBufferInfoList[tri->submeshID()].indexCount -= 3;
                tri->setRemoved();
                return;
            }
            tri->computeNormal( data->mVertexList );
            addTriangleToEdges( data, tri );
        }
    };

}  // namespace Ogre
#endif
