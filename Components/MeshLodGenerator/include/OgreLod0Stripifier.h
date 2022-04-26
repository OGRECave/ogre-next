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

#ifndef _Lod0Stripifier_H__
#define _Lod0Stripifier_H__

#include "OgreLodPrerequisites.h"

#include "OgreVertexRemapping.h"

namespace Ogre
{
    class Lod0Stripifier
    {
    public:
        Lod0Stripifier(){};
        ~Lod0Stripifier(){};
        bool StripLod0Vertices( const v1::MeshPtr &mesh, bool stableVertexOrder = false );

    private:
        void generateRemapInfo( const v1::MeshPtr &mesh, bool stableVertexOrder );

    private:
        vector<VerticesRemapInfo>::type remapInfos;  // 0 for shared geometry, 1+ for submesh index + 1
    };

    // Implementation
    // ------------------------------------------------------------------------------------

    inline void Lod0Stripifier::generateRemapInfo( const v1::MeshPtr &mesh, bool stableVertexOrder )
    {
        unsigned submeshCount = mesh->getNumSubMeshes();
        remapInfos.resize( 1 + submeshCount );
        remapInfos[0].initialize(
            mesh->sharedVertexData[VpNormal] ? mesh->sharedVertexData[VpNormal]->vertexCount : 0 );
        for( unsigned i = 0; i < submeshCount; i++ )
        {
            const v1::SubMesh *submesh = mesh->getSubMesh( i );
            remapInfos[1 + i].initialize(
                submesh->useSharedVertices ? 0 : submesh->vertexData[VpNormal]->vertexCount );

            VerticesRemapInfo &remapInfo = remapInfos[submesh->useSharedVertices ? 0 : 1 + i];

            for( ushort lod = mesh->getNumLodLevels() - 1; lod != 0;
                 --lod )  // intentionally skip lod0, visit in reverse order to improve vertex locality
                          // for high lods
            {
                v1::IndexData              *lodIndexData = submesh->mLodFaceList[VpNormal][lod - 1];
                v1::HardwareBufferLockGuard indexLock(
                    lodIndexData->indexBuffer,
                    lodIndexData->indexStart * lodIndexData->indexBuffer->getIndexSize(),
                    lodIndexData->indexCount * lodIndexData->indexBuffer->getIndexSize(),
                    v1::HardwareBuffer::HBL_READ_ONLY );

                if( lodIndexData->indexBuffer->getType() == v1::HardwareIndexBuffer::IT_32BIT )
                    remapInfo.markUsedIndices( (uint32 *)indexLock.pData, lodIndexData->indexCount );
                else
                    remapInfo.markUsedIndices( (uint16 *)indexLock.pData, lodIndexData->indexCount );
            }

            if( stableVertexOrder )
                remapInfos[1 + i].renumerate();
        }
        if( stableVertexOrder )
            remapInfos[0].renumerate();
    }

    inline bool Lod0Stripifier::StripLod0Vertices( const v1::MeshPtr &mesh, bool stableVertexOrder )
    {
        // we need at least one lod level except lod0 that would be stripped
        ushort numLods = mesh->hasManualLodLevel() ? 1 : mesh->getNumLodLevels();
        if( numLods <= 1 )
            return false;

        bool edgeListWasBuilt = mesh->isEdgeListBuilt();
        mesh->freeEdgeList();
        mesh->destroyShadowMappingGeom();

        generateRemapInfo( mesh, stableVertexOrder );

        if( mesh->sharedVertexData[VpNormal] )
            remapInfos[0].performVertexDataRemap( mesh->getHardwareBufferManager(),
                                                  mesh->sharedVertexData[VpNormal] );
        remapInfos[0].performBoneAssignmentRemap( mesh.get() );

        unsigned submeshCount = mesh->getNumSubMeshes();
        for( unsigned i = 0; i < submeshCount; i++ )
        {
            v1::SubMesh             *submesh = mesh->getSubMesh( i );
            const VerticesRemapInfo &remapInfo = remapInfos[submesh->useSharedVertices ? 0 : 1 + i];

            if( !submesh->useSharedVertices )
                remapInfo.performVertexDataRemap( mesh->getHardwareBufferManager(),
                                                  submesh->vertexData[VpNormal] );
            remapInfo.performBoneAssignmentRemap( submesh );

            for( ushort lod = numLods - 1; lod != 0; --lod )  // intentionally skip lod0
            {
                v1::IndexData *lodIndexData =
                    submesh->mLodFaceList[VpNormal][lod - 1];  // lod0 is stored separately
                remapInfo.performIndexDataRemap( mesh->getHardwareBufferManager(), lodIndexData );
            }

            OGRE_DELETE submesh->indexData[VpNormal];
            submesh->indexData[VpNormal] = submesh->mLodFaceList[VpNormal][0];
            submesh->mLodFaceList[VpNormal].erase( submesh->mLodFaceList[VpNormal].begin() );
        }

        for( ushort lod = 1; lod < numLods - 1; ++lod )
            mesh->_setLodUsage( lod, mesh->getLodLevel( lod + 1 ) );
        mesh->_setLodInfo( numLods - 1 );

        v1::Mesh::PoseIterator poseIterator = mesh->getPoseIterator();
        while( poseIterator.hasMoreElements() )
        {
            v1::Pose *pose = poseIterator.getNext();
            remapInfos[pose->getTarget()].performPoseRemap( pose );
        }

        for( unsigned short a = 0; a < mesh->getNumAnimations(); ++a )
        {
            v1::Animation                     *anim = mesh->getAnimation( a );
            v1::Animation::VertexTrackIterator trackIt = anim->getVertexTrackIterator();
            while( trackIt.hasMoreElements() )
            {
                v1::VertexAnimationTrack *track = trackIt.getNext();
                remapInfos[track->getHandle()].performAnimationTrackRemap(
                    mesh->getHardwareBufferManager(), track );
            }
        }

        mesh->prepareForShadowMapping( false );
        if( edgeListWasBuilt )
            mesh->buildEdgeList();

        return true;
    }
}  // namespace Ogre

#endif
