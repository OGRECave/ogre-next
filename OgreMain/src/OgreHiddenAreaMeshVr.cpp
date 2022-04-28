/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#include "OgreConfigFile.h"
#include "OgreLogManager.h"
#include "OgreMesh2.h"
#include "OgreMeshManager2.h"
#include "OgreRoot.h"
#include "OgreString.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreVaoManager.h"

namespace Ogre
{
    //-------------------------------------------------------------------------
    HiddenAreaVrSettings HiddenAreaMeshVrGenerator::loadSettings( const String &deviceName,
                                                                  ConfigFile &configFile )
    {
        HiddenAreaVrSettings retVal;
        silent_memset( &retVal, 0, sizeof( retVal ) );

        ConfigFile::SectionIterator itor = configFile.getSectionIterator();

        String bestMatch;
        String deviceConfigName;
        String deviceNameUpper = deviceName;
        StringUtil::toUpperCase( deviceNameUpper );
        while( itor.hasMoreElements() )
        {
            deviceConfigName = itor.peekNextKey();
            String deviceConfigNameUpper = deviceConfigName;
            StringUtil::toUpperCase( deviceConfigNameUpper );
            if( deviceNameUpper.find( deviceConfigNameUpper ) != String::npos &&
                deviceConfigName.size() > bestMatch.size() )
            {
                bestMatch = deviceConfigName;
            }

            itor.getNext();
        }

        if( bestMatch.empty() )
        {
            LogManager::getSingleton().logMessage( "No HiddenAreaMeshVr entry found for '" + deviceName +
                                                   "'. Disabling optimization." );
        }
        else
        {
            String value;
            value = configFile.getSetting( "leftEyeCenter", bestMatch, "" );
            retVal.leftEyeCenter = StringConverter::parseVector2( value );
            value = configFile.getSetting( "leftEyeRadius", bestMatch, "" );
            retVal.leftEyeRadius = StringConverter::parseVector2( value );
            value = configFile.getSetting( "leftNoseCenter", bestMatch, "" );
            retVal.leftNoseCenter = StringConverter::parseVector2( value );
            value = configFile.getSetting( "leftNoseRadius", bestMatch, "" );
            retVal.leftNoseRadius = StringConverter::parseVector2( value );

            value = configFile.getSetting( "rightEyeCenter", bestMatch, "" );
            retVal.rightEyeCenter = StringConverter::parseVector2( value );
            value = configFile.getSetting( "rightEyeRadius", bestMatch, "" );
            retVal.rightEyeRadius = StringConverter::parseVector2( value );
            value = configFile.getSetting( "rightNoseCenter", bestMatch, "" );
            retVal.rightNoseCenter = StringConverter::parseVector2( value );
            value = configFile.getSetting( "rightNoseRadius", bestMatch, "" );
            retVal.rightNoseRadius = StringConverter::parseVector2( value );

            value = configFile.getSetting( "tessellation", bestMatch, "" );
            retVal.tessellation = StringConverter::parseUnsignedInt( value );

            value = configFile.getSetting( "enabled", bestMatch, "" );
            if( !StringConverter::parseBool( value ) )
                retVal.tessellation = 0u;

            if( retVal.tessellation == 0u )
            {
                LogManager::getSingleton().logMessage( "HiddenAreaMeshVr optimization for '" +
                                                       bestMatch + "' explicitly disabled." );
            }
        }

        return retVal;
    }
    //-------------------------------------------------------------------------
    void HiddenAreaMeshVrGenerator::generate( const String &meshName,
                                              const HiddenAreaVrSettings &setting )
    {
        // Create now, because if it raises an exception, memory will leak
        MeshPtr mesh = MeshManager::getSingleton().createManual(
            meshName, ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME );

        size_t numVertices;
        numVertices = ( setting.tessellation + 1u ) * 2u * 3u;
        numVertices = numVertices * 3u;  // left + center + right

        float *vertexData = reinterpret_cast<float *>(
            OGRE_MALLOC_SIMD( numVertices * 4u * sizeof( float ), MEMCATEGORY_GEOMETRY ) );
        float *vertexDataStart = vertexData;
        FreeOnDestructor dataPtr( vertexDataStart );

        // clang-format off
        vertexData = generate( setting.leftEyeCenter, setting.leftEyeRadius, setting.tessellation,
                               -1, -1, 0, vertexData );
        vertexData = generate( setting.leftEyeCenter, setting.leftEyeRadius, setting.tessellation,
                               1, 1, 0, vertexData );
        vertexData = generate( setting.leftNoseCenter, setting.leftNoseRadius, setting.tessellation,
                               -1, 1, 0, vertexData );
        vertexData = generate( setting.rightNoseCenter, setting.rightNoseRadius, setting.tessellation,
                               1, -1, 1, vertexData );
        vertexData = generate( setting.rightEyeCenter, setting.rightEyeRadius, setting.tessellation,
                               1, 1, 1, vertexData );
        vertexData = generate( setting.rightEyeCenter, setting.rightEyeRadius, setting.tessellation,
                               -1, -1, 1, vertexData );
        // clang-format on

        const uint32 numUsedVertices = static_cast<uint32>( vertexData - vertexDataStart ) / 4u;

        OGRE_ASSERT_LOW( (size_t)( vertexData - vertexDataStart ) <= numVertices * 4u );

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
        mesh->_setBounds( Aabb::BOX_INFINITE, false );
    }
    //-------------------------------------------------------------------------
    float *HiddenAreaMeshVrGenerator::generate( Vector2 circleCenter, Vector2 radius,
                                                uint32 tessellation, float circleDir, float fillDir,
                                                float eyeIdx, float *RESTRICT_ALIAS vertexBuffer )
    {
        // We try to extend the triangle to infinity so that the triangle becomes a quad,
        // and rely on scissor clipping at the middle of the eyes.
        // However we can't:
        //  1. Use .w = 0; This is a very good trick, but it
        //     breaks with depth != 0 (i.e. breaks with Reverse Z)
        //  2. Use infinity. It is easy to generate NaNs.
        //  3. Use an extremely large value, it breaks GPUs (vertex ends up collapsing to origin).
        const float c_veryLargeValue = 65000.0f;

        const float fCircleStep = Math::PI / float( tessellation - 1u ) * circleDir;

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
                    *vertexBuffer++ = c_veryLargeValue;
                    *vertexBuffer++ = eyeIdx;
                    *vertexBuffer++ = 1.0f;
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
                    *vertexBuffer++ = -c_veryLargeValue;
                    *vertexBuffer++ = eyeIdx;
                    *vertexBuffer++ = 1.0f;
                }

                *vertexBuffer++ = c_veryLargeValue * fillDir;
                *vertexBuffer++ = vertexPoint[0].y;
                *vertexBuffer++ = eyeIdx;
                *vertexBuffer++ = 1.0f;
            }

            circlePos[0].swap( circlePos[1] );
        }

        return vertexBuffer;
    }
}  // namespace Ogre
