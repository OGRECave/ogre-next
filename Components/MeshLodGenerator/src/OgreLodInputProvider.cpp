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

#include "OgreLodInputProvider.h"

#include "OgreLodData.h"
#include "OgreLogManager.h"

#include <sstream>

namespace Ogre
{
    void LodInputProvider::printTriangle( LodData *data, LodData::Triangle *triangle, stringstream &str )
    {
        for( int i = 0; i < 3; i++ )
        {
            const LodData::Vertex *v = &data->mVertexList[triangle->vertexi[i]];
            str << ( i + 1 ) << ". vertex position: (" << v->position.x << ", " << v->position.y << ", "
                << v->position.z << ") " << "vertex ID: " << triangle->vertexID[i] << std::endl;
        }
    }

    bool LodInputProvider::isDuplicateTriangle( LodData::Triangle *triangle,
                                                LodData::Triangle *triangle2 )
    {
        for( int i = 0; i < 3; i++ )
        {
            if( triangle->vertexi[i] != triangle2->vertexi[0] ||
                triangle->vertexi[i] != triangle2->vertexi[1] ||
                triangle->vertexi[i] != triangle2->vertexi[2] )
            {
                return false;
            }
        }
        return true;
    }

    LodData::Triangle *LodInputProvider::isDuplicateTriangle( LodData *data,
                                                              LodData::Triangle *triangle )
    {
        // duplicate triangle detection (where all vertices has the same position)
        LodData::Vertex *v0 = &data->mVertexList[triangle->vertexi[0]];
        LodData::VTriangles::iterator itEnd = v0->triangles.end();
        LodData::VTriangles::iterator it = v0->triangles.begin();
        for( ; it != itEnd; ++it )
        {
            LodData::Triangle *t = &data->mTriangleList[*it];
            if( isDuplicateTriangle( triangle, t ) )
            {
                return t;
            }
        }
        return NULL;
    }
    void LodInputProvider::addTriangleToEdges( LodData *data, LodData::Triangle *triangle )
    {
        if( MESHLOD_QUALITY >= 3 )
        {
            LodData::Triangle *duplicate = isDuplicateTriangle( data, triangle );
            if( duplicate != NULL )
            {
#if OGRE_DEBUG_MODE
                stringstream str;
                str << "In " << data->mMeshName << " duplicate triangle found." << std::endl;
                str << "Triangle " << LodData::getVectorIDFromPointer( data->mTriangleList, triangle )
                    << " positions:" << std::endl;
                printTriangle( data, triangle, str );
                str << "Triangle " << LodData::getVectorIDFromPointer( data->mTriangleList, duplicate )
                    << " positions:" << std::endl;
                printTriangle( data, duplicate, str );
                str << "Triangle " << LodData::getVectorIDFromPointer( data->mTriangleList, triangle )
                    << " will be excluded from Lod level calculations.";
                LogManager::getSingleton().stream() << str.str();
#endif
                data->mIndexBufferInfoList[triangle->submeshID()].indexCount -= 3;
                triangle->setRemoved();
                return;
            }
        }
        LodData::TriangleI trianglei =
            (LodData::TriangleI)LodData::getVectorIDFromPointer( data->mTriangleList, triangle );
        for( int i = 0; i < 3; i++ )
        {
            data->mVertexList[triangle->vertexi[i]].triangles.addNotExists( trianglei );
        }
        for( int i = 0; i < 3; i++ )
        {
            LodData::Vertex *v = &data->mVertexList[triangle->vertexi[i]];
            for( int n = 0; n < 3; n++ )
            {
                if( i != n )
                {
                    v->addEdge( LodData::Edge( triangle->vertexi[n] ) );
                }
            }
        }
    }
    size_t LodInputProvider::getTriangleCount( OperationType renderOp, size_t indexCount )
    {
        if( renderOp == OT_TRIANGLE_LIST )
            return indexCount / 3;
        else if( renderOp == OT_TRIANGLE_STRIP || renderOp == OT_TRIANGLE_FAN )
            return indexCount >= 3 ? indexCount - 2 : 0;
        return 0;
    }
}  // namespace Ogre
