/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#include "OgreStableHeaders.h"

#include "OgreHiddenAreaMeshVr.h"
#include "OgreRoot.h"
#include "Vao/OgreVaoManager.h"

#include "OgreMesh2.h"
#include "OgreMeshManager2.h"
#include "OgreSubMesh2.h"

namespace Ogre
{
    void HiddenAreaMeshVrGenerator::generate( const String &meshName, Vector2 leftEyeCenter,
                                              Vector2 leftRadius, Vector2 rightEyeCenter,
                                              Vector2 rightRadius, float ipCenterY, Vector2 ipRadius,
                                              uint32 tessellation )
    {
        // Create now, because if it raises an exception, memory will leak
        MeshPtr mesh = MeshManager::getSingleton().createManual(
            meshName, ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME );

        size_t numVertices;
        numVertices = ( tessellation + 1u ) * 2u * 3u;
        numVertices = numVertices * 3u;  // left + center + right

        float *vertexData = reinterpret_cast<float *>(
            OGRE_MALLOC_SIMD( numVertices * 4u * sizeof( float ), MEMCATEGORY_GEOMETRY ) );
        float *vertexDataStart = vertexData;
        FreeOnDestructor dataPtr( vertexDataStart );

        const Vector2 ipCenterLeft( 1.0f, ipCenterY );
        const Vector2 ipCenterRight( -1.0f, ipCenterY );

        vertexData = generate( leftEyeCenter, leftRadius, tessellation, -1, -1, 0, vertexData );
        vertexData = generate( leftEyeCenter, leftRadius, tessellation, 1, 1, 0, vertexData );
        vertexData = generate( ipCenterLeft, ipRadius, tessellation, -1, 1, 0, vertexData );
        vertexData = generate( ipCenterRight, ipRadius, tessellation, 1, -1, 1, vertexData );
        vertexData = generate( rightEyeCenter, rightRadius, tessellation, 1, 1, 1, vertexData );
        vertexData = generate( rightEyeCenter, rightRadius, tessellation, -1, -1, 1, vertexData );

        const uint32 numUsedVertices = static_cast<uint32>( vertexData - vertexDataStart ) / 4u;

        OGRE_ASSERT_LOW( ( size_t )( vertexData - vertexDataStart ) <= numVertices * 4u );

        VaoManager *vaoManager = Root::getSingleton().getRenderSystem()->getVaoManager();
        VertexElement2Vec vertexElements;
        vertexElements.push_back( VertexElement2( VET_FLOAT4, VES_POSITION ) );
        VertexBufferPacked *vertexBuffer = vaoManager->createVertexBuffer(
            vertexElements, numVertices, BT_IMMUTABLE, vertexDataStart, false );
        VertexBufferPackedVec vertexBuffers;
        vertexBuffers.push_back( vertexBuffer );
        VertexArrayObject *vao =
            vaoManager->createVertexArrayObject( vertexBuffers, 0, OT_TRIANGLE_LIST );
        vao->setPrimitiveRange( 0u, numUsedVertices );
        SubMesh *submesh = mesh->createSubMesh();
        submesh->mVao[VpNormal].push_back( vao );
        submesh->mVao[VpShadow].push_back( vao );
        submesh->mMaterialName = "Ogre/VR/HiddenAreaMeshVr";
    }
    //-------------------------------------------------------------------------
    float *HiddenAreaMeshVrGenerator::generate( Vector2 circleCenter, Vector2 radius,
                                                uint32 tessellation, float circleDir, float fillDir,
                                                float eyeIdx, float *RESTRICT_ALIAS vertexBuffer )
    {
        const float fCircleStep = Math::PI / ( tessellation - 1u ) * circleDir;

        Vector2 circlePos[2];
        circlePos[0].x = sinf( 0 );
        circlePos[0].y = cosf( 0 );
        for( size_t i = 0u; i < tessellation + 1u; ++i )
        {
            const float fStep = static_cast<float>( std::min<size_t>( i, tessellation ) ) * fCircleStep;
            circlePos[1].x = sinf( fStep );
            circlePos[1].y = cosf( fStep );

            const Vector2 vertexPoint[2] = { circlePos[0] * radius + circleCenter,
                                             circlePos[1] * radius + circleCenter };

            const bool isInsideRect =
                ( Math::Abs( vertexPoint[0].x ) <= 1.0f && Math::Abs( vertexPoint[0].y ) <= 1.0f ) ||
                ( Math::Abs( vertexPoint[1].x ) <= 1.0f && Math::Abs( vertexPoint[1].y ) <= 1.0f );
            if( isInsideRect )
            {
                if( i != 0u )
                {
                    for( size_t j = 0u; j < 2u; ++j )
                        *vertexBuffer++ = vertexPoint[0][j];
                    *vertexBuffer++ = eyeIdx;
                    *vertexBuffer++ = 1.0f;
                }
                else
                {
                    *vertexBuffer++ = vertexPoint[0].x;
                    *vertexBuffer++ = 1.0f;
                    *vertexBuffer++ = eyeIdx;
                    *vertexBuffer++ = 0.0f;
                }
                if( i != tessellation )
                {
                    for( size_t j = 0u; j < 2u; ++j )
                        *vertexBuffer++ = vertexPoint[1][j];
                    *vertexBuffer++ = eyeIdx;
                    *vertexBuffer++ = 1.0f;
                }
                else
                {
                    *vertexBuffer++ = vertexPoint[0].x;
                    *vertexBuffer++ = -1.0f;
                    *vertexBuffer++ = eyeIdx;
                    *vertexBuffer++ = 0.0f;
                }

                *vertexBuffer++ = fillDir;
                *vertexBuffer++ = 0.0f;
                *vertexBuffer++ = eyeIdx;
                *vertexBuffer++ = 0.0f;
            }

            circlePos[0].swap( circlePos[1] );
        }

        return vertexBuffer;
    }
}  // namespace Ogre
