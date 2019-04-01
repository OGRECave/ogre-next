/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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
#ifndef _OgreVctVoxelizer_H_
#define _OgreVctVoxelizer_H_

#include "OgreHlmsPbsPrerequisites.h"
#include "OgreId.h"
#include "Math/Simple/OgreAabb.h"
#include "Vao/OgreVertexBufferDownloadHelper.h"
#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class VctMaterial;

    namespace VoxelizerJobSetting
    {
        enum VoxelizerJobSetting
        {
            HasDiffuseTex           = 1u << 0u,
            HasEmissiveTex          = 1u << 1u,
            EmissiveIsDiffuseTex    = 1u << 2u,
            Index32bit              = 1u << 3u,
            CompressedVertexFormat  = 1u << 4u,
        };
    }

    struct VoxelizerBucket
    {
        HlmsComputeJob      *job;
        ConstBufferPacked   *materialBuffer;
        UavBufferPacked     *vertexBuffer;
        UavBufferPacked     *indexBuffer;
        TextureGpu          *diffuseTex;
        TextureGpu          *emissiveTex;
        //TexBufferPacked     *instanceBuffer;
    };

    class _OgreHlmsPbsExport VctVoxelizer : public IdObject
    {
        struct MappedBuffers
        {
            float * RESTRICT_ALIAS uncompressedVertexBuffer;
            size_t index16BufferOffset;
            size_t index32BufferOffset;
        };

        struct QueuedSubMesh
        {
            VertexBufferDownloadHelper downloadHelper;
        };

        typedef FastArray<QueuedSubMesh> QueuedSubMeshArray;

        struct QueuedMesh
        {
            bool                bCompressed;
            QueuedSubMeshArray  submeshes;
        };

        typedef map<v1::MeshPtr, bool>::type v1MeshPtrMap;
        typedef map<MeshPtr, QueuedMesh>::type MeshPtrMap;
        typedef FastArray<Item*> ItemArray;

        v1MeshPtrMap    mMeshesV1;
        MeshPtrMap      mMeshesV2;

        ItemArray       mItems;

        /// HlmsComputeJob have internal caches, thus we could dynamically change properties
        /// and let the internal cache handle whether a compute job needs to be compiled.
        ///
        /// However the way we will be using may abuse the cache too much, thus we pre-set
        /// all variants as long as the number of variants is manageable.
        HlmsComputeJob  *mComputeJobs[1u<<5u];

        UavBufferPacked *mVertexBufferCompressed;
        UavBufferPacked *mVertexBufferUncompressed;
        UavBufferPacked *mIndexBuffer16;
        UavBufferPacked *mIndexBuffer32;

        uint32 mNumVerticesCompressed;
        uint32 mNumVerticesUncompressed;
        uint32 mNumIndices16;
        uint32 mNumIndices32;

        TextureGpu  *mAlbedoVox;
        TextureGpu  *mEmissiveVox;
        TextureGpu  *mNormalVox;
        TextureGpu  *mAccumValVox;

        VaoManager  *mVaoManager;
        HlmsManager *mHlmsManager;
        TextureGpuManager *mTextureGpuManager;

        VctMaterial *mVctMaterial;

        uint32  mWidth;
        uint32  mHeight;
        uint32  mDepth;

        /// Whether mRegionToVoxelize is manually set or autocalculated
        bool    mAutoRegion;
        Aabb    mRegionToVoxelize;
        /// Limit to mRegionToVoxelize in case mAutoRegion is true
        Aabb    mMaxRegion;

        void createComputeJobs();

        void countBuffersSize( const MeshPtr &mesh, QueuedMesh &queuedMesh );
        void convertMeshUncompressed( const MeshPtr &mesh, QueuedMesh &queuedMesh,
                                      MappedBuffers &mappedBuffers );

        void freeBuffers(void);

        void buildMeshBuffers(void);
        void createVoxelTextures(void);
        void destroyVoxelTextures(void);

        void calculateRegion();

        void placeItemsInBuckets();

    public:
        VctVoxelizer( IdType id, VaoManager *vaoManager, HlmsManager *hlmsManager,
                      TextureGpuManager *textureGpuManager );
        ~VctVoxelizer();

        /**
        @param item
        @param bCompressed
            True if we should compress:
                position to 16-bit SNORM
                normal to 16-bit SNORM
                uv as 16-bit Half
            False if we should use everything as 32-bit float
            If multiple Items using the same Mesh are added and one of them asks
            to not use compression, then not using compression takes precedence.
        */
        void addItem( Item *item, bool bCompressed );

        void build(void);
    };
}

#include "OgreHeaderSuffix.h"

#endif
