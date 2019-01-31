/*
 * -----------------------------------------------------------------------------
 * This source file is part of OGRE
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
#include "OgreMesh.h"
#include "OgreSubMesh.h"
#include "OgreKeyFrame.h"
#include "OgreHardwareBufferManager.h"

namespace Ogre
{
    class Lod0Stripifier
    {
    public:
        Lod0Stripifier();
        ~Lod0Stripifier();
        bool StripLod0Vertices(const v1::MeshPtr& mesh, bool stableVertexOrder = false);

    private:
        struct RemapInfo;
        void generateRemapInfo(const v1::MeshPtr& mesh, bool stableVertexOrder);
        static void performIndexDataRemap(v1::HardwareBufferManagerBase* pHWBufferManager, v1::IndexData* indexData, const RemapInfo& remapInfo);
        static void performVertexDataRemap(v1::HardwareBufferManagerBase* pHWBufferManager, v1::VertexData* vertexData, const RemapInfo& remapInfo);
        static v1::HardwareVertexBufferSharedPtr getRemappedVertexBuffer(v1::HardwareBufferManagerBase* pHWBufferManager, v1::HardwareVertexBufferSharedPtr vb, size_t srcStart, size_t srcCount, const RemapInfo& remapInfo);
        template<class MeshOrSubmesh> static void performBoneAssignmentRemap(MeshOrSubmesh* m, const RemapInfo& remapInfo);
        static void performPoseRemap(v1::Pose* pose, const RemapInfo& remapInfo);
        static void performAnimationTrackRemap(v1::HardwareBufferManagerBase* pHWBufferManager, v1::VertexAnimationTrack* track, const RemapInfo& remapInfo);

    private:
        vector<RemapInfo>::type		remapInfos;	// 0 for shared geometry, 1+ for submesh index + 1
    };


// Implementation ------------------------------------------------------------------------------------

    struct Lod0Stripifier::RemapInfo
    {
        RemapInfo() : usedCount(0) { }
        void prepare(size_t originalSize)
        {
            usedCount = 0;
            indexMap.resize(originalSize, UnusedIdx);
        }
        template<typename IDX> void markUsedIndices(IDX* indices, size_t indexCount)
        {
            for(IDX *idx = indices, *idx_end = idx + indexCount; idx < idx_end; ++idx)
                if(indexMap[*idx] == UnusedIdx)
                    indexMap[*idx] = usedCount++;
        }
        void renumerate()
        {
            usedCount = 0;
            for(size_t idx = 0, idx_end = indexMap.size(); idx < idx_end; ++idx)
                if(indexMap[idx] != UnusedIdx)
                    indexMap[idx] = usedCount++;
        }
        bool nothingToStrip() const
        {
            return usedCount == indexMap.size();
        }
        v1::HardwareIndexBuffer::IndexType minimalIndexType() const
        {
            return usedCount < 0xFFFF ? v1::HardwareIndexBuffer::IT_16BIT : v1::HardwareIndexBuffer::IT_32BIT;
        }

    public:
        enum { UnusedIdx = (unsigned)-1 };
        vector<unsigned>::type indexMap;	// returns new index if indexed by old index, or UnusedIdx
        unsigned usedCount;
    };

    inline Lod0Stripifier::Lod0Stripifier()
    {
    }
    inline Lod0Stripifier::~Lod0Stripifier()
    {
    }

    inline void Lod0Stripifier::generateRemapInfo(const v1::MeshPtr& mesh, bool stableVertexOrder)
    {
        ushort submeshCount = mesh->getNumSubMeshes();
        remapInfos.resize(1 + submeshCount);
        remapInfos[0].prepare(mesh->sharedVertexData[VpNormal] ? mesh->sharedVertexData[VpNormal]->vertexCount : 0);
        for(ushort i = 0; i < submeshCount; i++)
        {
            const v1::SubMesh* submesh = mesh->getSubMesh(i);
            remapInfos[1 + i].prepare(submesh->useSharedVertices ? 0 : submesh->vertexData[VpNormal]->vertexCount);

            RemapInfo& remapInfo = remapInfos[submesh->useSharedVertices ? 0 : 1 + i];

            for(ushort lod = mesh->getNumLodLevels() - 1; lod != 0; --lod) // intentionally skip lod0, visit in reverse order to improve vertex locality for high lods
            {
                v1::IndexData *lodIndexData = submesh->mLodFaceList[VpNormal][lod - 1];
                void* ptr = lodIndexData->indexBuffer->lock(
                                lodIndexData->indexStart * lodIndexData->indexBuffer->getIndexSize(),
                                lodIndexData->indexCount * lodIndexData->indexBuffer->getIndexSize(),
                                v1::HardwareBuffer::HBL_READ_ONLY);

                if(lodIndexData->indexBuffer->getType() == v1::HardwareIndexBuffer::IT_32BIT)
                    remapInfo.markUsedIndices((uint32*)ptr, lodIndexData->indexCount);
                else
                    remapInfo.markUsedIndices((uint16*)ptr, lodIndexData->indexCount);

                lodIndexData->indexBuffer->unlock();
            }

            if(stableVertexOrder)
                remapInfos[1 + i].renumerate();
        }
        if(stableVertexOrder)
            remapInfos[0].renumerate();
    }

    inline void Lod0Stripifier::performIndexDataRemap(v1::HardwareBufferManagerBase* pHWBufferManager, v1::IndexData* indexData, const RemapInfo& remapInfo)
    {
        if(remapInfo.nothingToStrip())
            return;

        size_t indexCount = indexData->indexCount;
        v1::HardwareIndexBuffer::IndexType indexType = indexData->indexBuffer->getType();
        v1::HardwareIndexBuffer::IndexType newIndexType = remapInfo.minimalIndexType();
        v1::HardwareIndexBufferSharedPtr newIndexBuffer =
            pHWBufferManager->createIndexBuffer(
                newIndexType, indexCount, indexData->indexBuffer->getUsage(), indexData->indexBuffer->hasShadowBuffer());

        void* pSrc = indexData->indexBuffer->lock(
                         indexData->indexStart * indexData->indexBuffer->getIndexSize(),
                         indexData->indexCount * indexData->indexBuffer->getIndexSize(),
                         v1::HardwareBuffer::HBL_READ_ONLY);
        void* pDst = newIndexBuffer->lock(v1::HardwareBuffer::HBL_DISCARD);

        if(indexType == v1::HardwareIndexBuffer::IT_32BIT && newIndexType == v1::HardwareIndexBuffer::IT_32BIT)
        {
            uint32 *pSrc32 = (uint32*)pSrc, *pDst32 = (uint32*)pDst;
            for(size_t i = 0; i < indexCount; ++i)
                pDst32[i] = remapInfo.indexMap[pSrc32[i]];
        }
        else if(indexType == v1::HardwareIndexBuffer::IT_32BIT && newIndexType == v1::HardwareIndexBuffer::IT_16BIT)
        {
            uint32 *pSrc32 = (uint32*)pSrc; uint16 *pDst16 = (uint16*)pDst;
            for(size_t i = 0; i < indexCount; ++i)
                pDst16[i] = (uint16)remapInfo.indexMap[pSrc32[i]];
        }
        else if(indexType == v1::HardwareIndexBuffer::IT_16BIT && newIndexType == v1::HardwareIndexBuffer::IT_32BIT)
        {
            uint16 *pSrc16 = (uint16*)pSrc; uint32 *pDst32 = (uint32*)pDst;
            for(size_t i = 0; i < indexCount; ++i)
                pDst32[i] = remapInfo.indexMap[pSrc16[i]];
        }
        else // indexType == v1::HardwareIndexBuffer::IT_16BIT && newIndexType == v1::HardwareIndexBuffer::IT_16BIT
        {
            uint16 *pSrc16 = (uint16*)pSrc, *pDst16 = (uint16*)pDst;
            for(size_t i = 0; i < indexCount; ++i)
                pDst16[i] = (uint16)remapInfo.indexMap[pSrc16[i]];
        }

        indexData->indexBuffer->unlock();
        newIndexBuffer->unlock();

        indexData->indexBuffer = newIndexBuffer;
        indexData->indexStart = 0;
    }

    inline void Lod0Stripifier::performVertexDataRemap(v1::HardwareBufferManagerBase* pHWBufferManager, v1::VertexData* vertexData, const RemapInfo& remapInfo)
    {
        if(remapInfo.nothingToStrip())
            return;

        // Copy vertex buffers in turn
        typedef std::map<v1::HardwareVertexBufferSharedPtr, v1::HardwareVertexBufferSharedPtr> VBMap;
        VBMap alreadyProcessed; // prevent duplication of the same buffer bound under several indices
        const v1::VertexBufferBinding::VertexBufferBindingMap& bindings = vertexData->vertexBufferBinding->getBindings();
        v1::VertexBufferBinding::VertexBufferBindingMap::const_iterator vbi, vbend;
        for(vbi = bindings.begin(), vbend = bindings.end(); vbi != vbend; ++vbi)
        {
            v1::HardwareVertexBufferSharedPtr srcbuf = vbi->second;
            VBMap::iterator it = alreadyProcessed.find(srcbuf);
            if(it != alreadyProcessed.end())
            {
                vertexData->vertexBufferBinding->setBinding(vbi->first, it->second);
                continue;
            }

            v1::HardwareVertexBufferSharedPtr dstbuf = getRemappedVertexBuffer(pHWBufferManager, srcbuf, vertexData->vertexStart, vertexData->vertexCount, remapInfo);
            vertexData->vertexBufferBinding->setBinding(vbi->first, dstbuf);
            alreadyProcessed[srcbuf] = dstbuf;
        }

        vertexData->vertexStart = 0;
        vertexData->vertexCount = remapInfo.usedCount;

        vertexData->hardwareShadowVolWBuffer = v1::HardwareVertexBufferSharedPtr(); // TODO: check this
        vertexData->hwAnimationDataList.clear(); // TODO: check this
        vertexData->hwAnimDataItemsUsed = 0; // TODO: check this
    }

    inline v1::HardwareVertexBufferSharedPtr Lod0Stripifier::getRemappedVertexBuffer(v1::HardwareBufferManagerBase* pHWBufferManager,
        v1::HardwareVertexBufferSharedPtr srcbuf, size_t srcStart, size_t srcCount, const RemapInfo& remapInfo)
    {
        assert(!remapInfo.nothingToStrip());

        size_t vertexSize = srcbuf->getVertexSize();
        v1::HardwareVertexBufferSharedPtr dstbuf =
            pHWBufferManager->createVertexBuffer(
                vertexSize, remapInfo.usedCount, srcbuf->getUsage(), srcbuf->hasShadowBuffer());

        char* pSrc = (char*)srcbuf->lock(srcStart * vertexSize, srcCount * vertexSize, v1::HardwareBuffer::HBL_READ_ONLY);
        char* pDst = (char*)dstbuf->lock(v1::HardwareBuffer::HBL_DISCARD);

        for(size_t oldIdx = 0, oldIdxEnd = remapInfo.indexMap.size(); oldIdx < oldIdxEnd; ++oldIdx)
        {
            unsigned newIdx = remapInfo.indexMap[oldIdx];
            if(newIdx != RemapInfo::UnusedIdx)
                memcpy(pDst + newIdx * vertexSize, pSrc + oldIdx * vertexSize, vertexSize);
        }

        srcbuf->unlock();
        dstbuf->unlock();
        return dstbuf;
    }

    template<class MeshOrSubmesh>
    inline void Lod0Stripifier::performBoneAssignmentRemap(MeshOrSubmesh* m, const RemapInfo& remapInfo)
    {
        if(remapInfo.nothingToStrip() || m->getBoneAssignments().empty())
            return;

        v1::Mesh::VertexBoneAssignmentList tmp = m->getBoneAssignments();
        m->clearBoneAssignments();
        for(v1::Mesh::VertexBoneAssignmentList::const_iterator it = tmp.begin(), it_end = tmp.end(); it != it_end; ++it)
        {
            v1::VertexBoneAssignment vba = it->second;
            unsigned newIdx = remapInfo.indexMap[vba.vertexIndex];
            if(newIdx != RemapInfo::UnusedIdx)
            {
                vba.vertexIndex = newIdx;
                m->addBoneAssignment(vba);
            }
        }
    }

    inline void Lod0Stripifier::performPoseRemap(v1::Pose* pose, const RemapInfo& remapInfo)
    {
        if(remapInfo.nothingToStrip() || pose->getVertexOffsets().empty() && pose->getNormals().empty())
            return;

        v1::Pose::VertexOffsetMap vv = pose->getVertexOffsets();
        v1::Pose::NormalsMap nn = pose->getNormals();
        pose->clearVertices();
        for(v1::Pose::VertexOffsetMap::const_iterator vit = vv.begin(), vit_end = vv.end(); vit != vit_end; ++vit)
        {
            unsigned newIdx = remapInfo.indexMap[vit->first];
            if(newIdx != RemapInfo::UnusedIdx)
            {
                if(pose->getIncludesNormals())
                    pose->addVertex(newIdx, vit->second, nn[vit->first]);
                else
                    pose->addVertex(newIdx, vit->second);
            }
        }
    }

    inline void Lod0Stripifier::performAnimationTrackRemap(v1::HardwareBufferManagerBase* pHWBufferManager, v1::VertexAnimationTrack* track, const RemapInfo& remapInfo)
    {
        if(remapInfo.nothingToStrip())
            return;

        if(track->getAnimationType() == v1::VAT_MORPH)
        {
            for(unsigned short i = 0; i < track->getNumKeyFrames(); ++i)
            {
                v1::VertexMorphKeyFrame* kf = track->getVertexMorphKeyFrame(i);
                v1::HardwareVertexBufferSharedPtr VB = kf->getVertexBuffer();
                kf->setVertexBuffer(getRemappedVertexBuffer(pHWBufferManager, VB, 0, VB->getNumVertices(), remapInfo));
            }
        }
    }

    inline bool Lod0Stripifier::StripLod0Vertices(const v1::MeshPtr& mesh, bool stableVertexOrder)
    {
        // we need at least one lod level except lod0 that would be stripped
        ushort numLods = mesh->hasManualLodLevel() ? 1 : mesh->getNumLodLevels();
        if(numLods <= 1)
            return false;

        bool edgeListWasBuilt = mesh->isEdgeListBuilt();
        mesh->freeEdgeList();

        generateRemapInfo(mesh, stableVertexOrder);

        if(mesh->sharedVertexData[VpNormal])
            performVertexDataRemap(mesh->getHardwareBufferManager(), mesh->sharedVertexData[VpNormal], remapInfos[0]);
        performBoneAssignmentRemap(mesh.get(), remapInfos[0]);

        ushort submeshCount = mesh->getNumSubMeshes();
        for(ushort i = 0; i < submeshCount; i++)
        {
            v1::SubMesh* submesh = mesh->getSubMesh(i);
            const RemapInfo& remapInfo = remapInfos[submesh->useSharedVertices ? 0 : 1 + i];

            if(!submesh->useSharedVertices)
                performVertexDataRemap(mesh->getHardwareBufferManager(), submesh->vertexData[VpNormal], remapInfo);
            performBoneAssignmentRemap(submesh, remapInfo);

            for(ushort lod = numLods - 1; lod != 0; --lod) // intentionally skip lod0
            {
                v1::IndexData *lodIndexData = submesh->mLodFaceList[VpNormal][lod - 1]; // lod0 is stored separately
                performIndexDataRemap(mesh->getHardwareBufferManager(), lodIndexData, remapInfo);
            }

            OGRE_DELETE submesh->indexData[VpNormal];
            submesh->indexData[VpNormal] = submesh->mLodFaceList[VpNormal][0];
            submesh->mLodFaceList[VpNormal].erase(submesh->mLodFaceList[VpNormal].begin());
        }

        for(ushort lod = 1; lod < numLods - 1; ++lod)
            mesh->_setLodUsage(lod, mesh->getLodLevel(lod + 1));
        mesh->_setLodInfo(numLods - 1);


        v1::Mesh::PoseIterator poseIterator = mesh->getPoseIterator();
        while(poseIterator.hasMoreElements())
        {
            v1::Pose* pose = poseIterator.getNext();
            performPoseRemap(pose, remapInfos[pose->getTarget()]);
        }

        for(unsigned short a = 0; a < mesh->getNumAnimations(); ++a)
        {
            v1::Animation* anim = mesh->getAnimation(a);
            v1::Animation::VertexTrackIterator trackIt = anim->getVertexTrackIterator();
            while(trackIt.hasMoreElements())
            {
                v1::VertexAnimationTrack* track = trackIt.getNext();
                performAnimationTrackRemap(mesh->getHardwareBufferManager(), track, remapInfos[track->getHandle()]);
            }
        }

        if(edgeListWasBuilt)
            mesh->buildEdgeList();

        return true;
    }
}

#endif
