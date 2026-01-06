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

#ifndef _VertexRemapping_H__
#define _VertexRemapping_H__

#include "OgreCommon.h"
#include "OgreHardwareBufferManager.h"
#include "OgreKeyFrame.h"
#include "OgreMesh.h"
#include "OgreSubMesh.h"

#include "ogrestd/map.h"
#include "ogrestd/vector.h"

namespace Ogre
{
    struct VerticesRemapInfo
    {
    public:
        enum
        {
            UnusedIdx = (unsigned)-1
        };
        vector<unsigned>::type indexMap;  // returns new index if indexed by old index, or UnusedIdx
        unsigned               usedCount;
        bool                   skipReorderingIfNotStripping;

    public:
        VerticesRemapInfo() : usedCount( 0 ) {}
        void initialize( size_t originalVertexCount, bool skipReorderingIfNothingToStrip = true )
        {
            usedCount = 0;
            skipReorderingIfNotStripping = skipReorderingIfNothingToStrip;
            indexMap.resize( 0 );
            indexMap.resize( originalVertexCount, UnusedIdx );
        }
        template <typename IDX>
        void markUsedIndices( IDX *indices, size_t indexCount )
        {
            for( IDX *idx = indices, *idx_end = idx + indexCount; idx < idx_end; ++idx )
                if( indexMap[*idx] == UnusedIdx )
                    indexMap[*idx] = usedCount++;
        }
        void markUsedIndices( v1::IndexData *indexData )
        {
            v1::HardwareBufferLockGuard indexLock(
                indexData->indexBuffer, indexData->indexStart * indexData->indexBuffer->getIndexSize(),
                indexData->indexCount * indexData->indexBuffer->getIndexSize(),
                v1::HardwareBuffer::HBL_READ_ONLY );

            if( indexData->indexBuffer->getType() == v1::HardwareIndexBuffer::IT_32BIT )
                markUsedIndices( (uint32 *)indexLock.pData, indexData->indexCount );
            else
                markUsedIndices( (uint16 *)indexLock.pData, indexData->indexCount );
        }
        void renumerate()  // Preserves original vertex order.
        {
            usedCount = 0;
            for( size_t idx = 0, idx_end = indexMap.size(); idx < idx_end; ++idx )
                if( indexMap[idx] != UnusedIdx )
                    indexMap[idx] = usedCount++;
        }
        bool skipProcessing() const
        {
            return skipReorderingIfNotStripping && usedCount == indexMap.size();
        }
        IndexType minimalIndexType() const { return usedCount < 0xFFFF ? IT_16BIT : IT_32BIT; }

        void performIndexDataRemap( v1::HardwareBufferManagerBase *pHWBufferManager,
                                    v1::IndexData                 *indexData ) const;
        void performVertexDataRemap( v1::HardwareBufferManagerBase *pHWBufferManager,
                                     v1::VertexData                *vertexData ) const;
        v1::HardwareVertexBufferSharedPtr getRemappedVertexBuffer(
            v1::HardwareBufferManagerBase *pHWBufferManager, v1::HardwareVertexBufferSharedPtr vb,
            size_t srcStart, size_t srcCount ) const;
        template <class MeshOrSubmesh>
        void performBoneAssignmentRemap( MeshOrSubmesh *m ) const;
        void performBoneAssignmentRemap( v1::SubMesh *dst, v1::Mesh *src ) const;
        void performPoseRemap( v1::Pose *pose ) const;
        void performAnimationTrackRemap( v1::HardwareBufferManagerBase *pHWBufferManager,
                                         v1::VertexAnimationTrack      *track ) const;
    };

    // Implementation
    // ------------------------------------------------------------------------------------

    inline void VerticesRemapInfo::performIndexDataRemap(
        v1::HardwareBufferManagerBase *pHWBufferManager, v1::IndexData *indexData ) const
    {
        if( skipProcessing() )
            return;

        size_t                             indexCount = indexData->indexCount;
        v1::HardwareIndexBuffer           *indexBuffer = indexData->indexBuffer.get();
        v1::HardwareIndexBuffer::IndexType indexType = indexBuffer->getType();
        v1::HardwareIndexBuffer::IndexType newIndexType = minimalIndexType();
        v1::HardwareIndexBufferSharedPtr   newIndexBuffer = pHWBufferManager->createIndexBuffer(
            newIndexType, indexCount, indexBuffer->getUsage(), indexBuffer->hasShadowBuffer() );

        v1::HardwareBufferLockGuard srcLock(
            indexBuffer, indexData->indexStart * indexBuffer->getIndexSize(),
            indexCount * indexBuffer->getIndexSize(), v1::HardwareBuffer::HBL_READ_ONLY );
        v1::HardwareBufferLockGuard dstLock( newIndexBuffer, v1::HardwareBuffer::HBL_DISCARD );

        if( indexType == v1::HardwareIndexBuffer::IT_32BIT &&
            newIndexType == v1::HardwareIndexBuffer::IT_32BIT )
        {
            uint32 *pSrc32 = (uint32 *)srcLock.pData, *pDst32 = (uint32 *)dstLock.pData;
            for( size_t i = 0; i < indexCount; ++i )
                pDst32[i] = indexMap[pSrc32[i]];
        }
        else if( indexType == v1::HardwareIndexBuffer::IT_32BIT &&
                 newIndexType == v1::HardwareIndexBuffer::IT_16BIT )
        {
            uint32 *pSrc32 = (uint32 *)srcLock.pData;
            uint16 *pDst16 = (uint16 *)dstLock.pData;
            for( size_t i = 0; i < indexCount; ++i )
                pDst16[i] = (uint16)indexMap[pSrc32[i]];
        }
        else if( indexType == v1::HardwareIndexBuffer::IT_16BIT &&
                 newIndexType == v1::HardwareIndexBuffer::IT_32BIT )
        {
            uint16 *pSrc16 = (uint16 *)srcLock.pData;
            uint32 *pDst32 = (uint32 *)dstLock.pData;
            for( size_t i = 0; i < indexCount; ++i )
                pDst32[i] = indexMap[pSrc16[i]];
        }
        else  // indexType == v1::HardwareIndexBuffer::IT_16BIT && newIndexType ==
              // v1::HardwareIndexBuffer::IT_16BIT
        {
            uint16 *pSrc16 = (uint16 *)srcLock.pData, *pDst16 = (uint16 *)dstLock.pData;
            for( size_t i = 0; i < indexCount; ++i )
                pDst16[i] = (uint16)indexMap[pSrc16[i]];
        }

        srcLock.unlock();
        dstLock.unlock();

        indexData->indexBuffer = newIndexBuffer;
        indexData->indexStart = 0;
    }

    inline void VerticesRemapInfo::performVertexDataRemap(
        v1::HardwareBufferManagerBase *pHWBufferManager, v1::VertexData *vertexData ) const
    {
        if( skipProcessing() )
            return;

        // Copy vertex buffers in turn
        typedef map<v1::HardwareVertexBufferSharedPtr, v1::HardwareVertexBufferSharedPtr>::type VBMap;
        VBMap alreadyProcessed;  // prevent duplication of the same buffer bound under several indices
        const v1::VertexBufferBinding::VertexBufferBindingMap &bindings =
            vertexData->vertexBufferBinding->getBindings();
        v1::VertexBufferBinding::VertexBufferBindingMap::const_iterator vbi, vbend;
        for( vbi = bindings.begin(), vbend = bindings.end(); vbi != vbend; ++vbi )
        {
            v1::HardwareVertexBufferSharedPtr srcbuf = vbi->second;
            VBMap::iterator                   it = alreadyProcessed.find( srcbuf );
            if( it != alreadyProcessed.end() )
            {
                vertexData->vertexBufferBinding->setBinding( vbi->first, it->second );
                continue;
            }

            v1::HardwareVertexBufferSharedPtr dstbuf = getRemappedVertexBuffer(
                pHWBufferManager, srcbuf, vertexData->vertexStart, vertexData->vertexCount );
            vertexData->vertexBufferBinding->setBinding( vbi->first, dstbuf );
            alreadyProcessed[srcbuf] = dstbuf;
        }

        vertexData->vertexStart = 0;
        vertexData->vertexCount = usedCount;

        vertexData->hardwareShadowVolWBuffer = v1::HardwareVertexBufferSharedPtr();  // TODO: check this
        vertexData->hwAnimationDataList.clear();                                     // TODO: check this
        vertexData->hwAnimDataItemsUsed = 0;                                         // TODO: check this
    }

    inline v1::HardwareVertexBufferSharedPtr VerticesRemapInfo::getRemappedVertexBuffer(
        v1::HardwareBufferManagerBase *pHWBufferManager, v1::HardwareVertexBufferSharedPtr srcbuf,
        size_t srcStart, size_t srcCount ) const
    {
        assert( !skipProcessing() );  // till now there are no use cases that triggers this assert

        size_t                            vertexSize = srcbuf->getVertexSize();
        v1::HardwareVertexBufferSharedPtr dstbuf = pHWBufferManager->createVertexBuffer(
            vertexSize, usedCount, srcbuf->getUsage(), srcbuf->hasShadowBuffer() );

        v1::HardwareBufferLockGuard srcLock( srcbuf, srcStart * vertexSize, srcCount * vertexSize,
                                             v1::HardwareBuffer::HBL_READ_ONLY );
        v1::HardwareBufferLockGuard dstLock( dstbuf, v1::HardwareBuffer::HBL_DISCARD );

        for( size_t oldIdx = 0, oldIdxEnd = indexMap.size(); oldIdx < oldIdxEnd; ++oldIdx )
        {
            unsigned newIdx = indexMap[oldIdx];
            if( newIdx != UnusedIdx )
            {
                memcpy( (uint8 *)dstLock.pData + newIdx * vertexSize,
                        (uint8 *)srcLock.pData + oldIdx * vertexSize, vertexSize );
            }
        }

        return dstbuf;
    }

    template <class MeshOrSubmesh>
    inline void VerticesRemapInfo::performBoneAssignmentRemap( MeshOrSubmesh *m ) const
    {
        if( skipProcessing() || m->getBoneAssignments().empty() )
            return;

        v1::Mesh::VertexBoneAssignmentList tmp = m->getBoneAssignments();
        m->clearBoneAssignments();
        for( v1::Mesh::VertexBoneAssignmentList::const_iterator it = tmp.begin(), it_end = tmp.end();
             it != it_end; ++it )
        {
            v1::VertexBoneAssignment vba = it->second;
            unsigned                 newIdx = indexMap[vba.vertexIndex];
            if( newIdx != UnusedIdx )
            {
                vba.vertexIndex = newIdx;
                m->addBoneAssignment( vba );
            }
        }
    }

    inline void VerticesRemapInfo::performBoneAssignmentRemap( v1::SubMesh *dst, v1::Mesh *src ) const
    {
        assert( !skipProcessing() );
        if( src->getBoneAssignments().empty() )
            return;

        for( v1::Mesh::VertexBoneAssignmentList::const_iterator it = src->getBoneAssignments().begin(),
                                                                it_end = src->getBoneAssignments().end();
             it != it_end; ++it )
        {
            v1::VertexBoneAssignment vba = it->second;
            unsigned                 newIdx = indexMap[vba.vertexIndex];
            if( newIdx != UnusedIdx )
            {
                vba.vertexIndex = newIdx;
                dst->addBoneAssignment( vba );
            }
        }
    }

    inline void VerticesRemapInfo::performPoseRemap( v1::Pose *pose ) const
    {
        if( skipProcessing() || ( pose->getVertexOffsets().empty() && pose->getNormals().empty() ) )
            return;

        v1::Pose::VertexOffsetMap vv = pose->getVertexOffsets();
        v1::Pose::NormalsMap      nn = pose->getNormals();
        pose->clearVertices();
        for( v1::Pose::VertexOffsetMap::const_iterator vit = vv.begin(), vit_end = vv.end();
             vit != vit_end; ++vit )
        {
            unsigned newIdx = indexMap[vit->first];
            if( newIdx != UnusedIdx )
            {
                if( pose->getIncludesNormals() )
                    pose->addVertex( newIdx, vit->second, nn[vit->first] );
                else
                    pose->addVertex( newIdx, vit->second );
            }
        }
    }

    inline void VerticesRemapInfo::performAnimationTrackRemap(
        v1::HardwareBufferManagerBase *pHWBufferManager, v1::VertexAnimationTrack *track ) const
    {
        if( skipProcessing() )
            return;

        if( track->getAnimationType() == v1::VAT_MORPH )
        {
            for( unsigned short i = 0; i < track->getNumKeyFrames(); ++i )
            {
                v1::VertexMorphKeyFrame          *kf = track->getVertexMorphKeyFrame( i );
                v1::HardwareVertexBufferSharedPtr VB = kf->getVertexBuffer();
                kf->setVertexBuffer(
                    getRemappedVertexBuffer( pHWBufferManager, VB, 0, VB->getNumVertices() ) );
            }
        }
    }

}  // namespace Ogre

#endif
