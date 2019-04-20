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

        bool operator < ( const VoxelizerBucket &other ) const
        {
            if( this->job != other.job )
                return this->job < other.job;
            if( this->materialBuffer != other.materialBuffer )
                return this->materialBuffer < other.materialBuffer;
            if( this->vertexBuffer != other.vertexBuffer )
                return this->vertexBuffer < other.vertexBuffer;
            if( this->indexBuffer != other.indexBuffer )
                return this->indexBuffer < other.indexBuffer;
            if( this->diffuseTex != other.diffuseTex )
                return this->diffuseTex < other.diffuseTex;

            return this->emissiveTex < other.emissiveTex;
        }
    };

    /**
    @class VctVoxelizer
        The voxelizer consists in several stages. The main ones that requre explanation are:

        1. Download the vertex buffers to CPU, then upload it again to GPU in an homogeneous
           format our compute shader understands. We do the same with the index buffer, except
           we perform GPU -> GPU copies. We can't use the buffers directly because in many
           APIs we can't bind the index buffer as an UAV easily.
           This step is handled by VctVoxelizer::buildMeshBuffers.
           Not implemented yet: when rebuilding the voxelized scene, this step can be skipped
           if no meshes were added since the last change (and the buffers weren't freed
           to save memory)

        2. Iterate through every Item and convert the datablocks to the simplified version
           our compute shader uses. VctMaterial handles this; done in
           VctVoxelizer::placeItemsInBuckets.

        3. During step 2, we also group the items into buckets. Each bucket can be batched
           together to dispatch a single compute shader execution; because they share all
           the same settings (and we haven't run out of material buffer space)

        4. The items grouped in buckets may be split into 8 instance buffers; in order to
           cull each octant of the voxel; thus avoiding having all voxels try to check
           for all instances (performance optimization).
    */
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
            uint32  vbOffset;
            uint32  ibOffset;
            uint32  numIndices;
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

        float           *mCpuInstanceBuffer;
        TexBufferPacked *mInstanceBuffer;
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

        RenderSystem*mRenderSystem;
        VaoManager  *mVaoManager;
        HlmsManager *mHlmsManager;
        TextureGpuManager *mTextureGpuManager;

        ComputeTools *mComputeTools;

        struct QueuedInstance
        {
            MovableObject   *movableObject;
            uint32          vertexBufferStart;
            uint32          indexBufferStart;
            uint32          numIndices;
            uint32          materialIdx;
        };
        typedef map< VoxelizerBucket, FastArray<QueuedInstance> >::type VoxelizerBucketMap;
        VoxelizerBucketMap mBuckets;

        VctMaterial *mVctMaterial;

        uint32  mWidth;
        uint32  mHeight;
        uint32  mDepth;

        /// Whether mRegionToVoxelize is manually set or autocalculated
        bool    mAutoRegion;
        Aabb    mRegionToVoxelize;
        /// Limit to mRegionToVoxelize in case mAutoRegion is true
        Aabb    mMaxRegion;

        struct Octant
        {
            uint32 x, y, z;
            uint32 width, height, depth;
            Aabb region;
        };

        FastArray<Octant> mOctants;

        void createComputeJobs();

        void countBuffersSize( const MeshPtr &mesh, QueuedMesh &queuedMesh );
        void convertMeshUncompressed( const MeshPtr &mesh, QueuedMesh &queuedMesh,
                                      MappedBuffers &mappedBuffers );

        void freeBuffers(void);

        void buildMeshBuffers(void);
        void createVoxelTextures(void);
        void destroyVoxelTextures(void);

        void placeItemsInBuckets(void);
        void createInstanceBuffers(void);
        void destroyInstanceBuffers(void);
        void fillInstanceBuffers(void);

    public:
        VctVoxelizer( IdType id, RenderSystem *renderSystem, HlmsManager *hlmsManager );
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

        void autoCalculateRegion(void);

        void dividideOctants( uint32 numOctantsX, uint32 numOctantsY, uint32 numOctantsZ );

        void build(void);

        TextureGpu* getAlbedoVox(void)          { return mAlbedoVox; }
        TextureGpu* getEmissiveVox(void)        { return mEmissiveVox; }
        TextureGpu* getNormalVox(void)          { return mNormalVox; }

        TextureGpuManager* getTextureGpuManager(void);
        RenderSystem* getRenderSystem(void);
    };
}

#include "OgreHeaderSuffix.h"

#endif
