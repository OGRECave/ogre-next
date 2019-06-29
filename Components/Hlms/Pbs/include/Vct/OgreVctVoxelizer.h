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

#ifdef OGRE_FORCE_VCT_VOXELIZER_DETERMINISTIC
    #include "OgreMesh2.h"
#endif

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class VctMaterial;
    class VoxelVisualizer;

    namespace VoxelizerJobSetting
    {
        enum VoxelizerJobSetting
        {
            Index32bit              = 1u << 0u,
            CompressedVertexFormat  = 1u << 1u,
            HasDiffuseTex           = 1u << 2u,
            HasEmissiveTex          = 1u << 3u,
        };
    }

    struct VoxelizerBucket
    {
        HlmsComputeJob      *job;
        ConstBufferPacked   *materialBuffer;
        UavBufferPacked     *vertexBuffer;
        UavBufferPacked     *indexBuffer;
        bool                needsTexPool;

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
            return this->needsTexPool < other.needsTexPool;
        }
    };

#ifdef OGRE_FORCE_VCT_VOXELIZER_DETERMINISTIC
    struct DeterministicMeshPtrOrder
    {
        bool operator()( const MeshPtr &a, const MeshPtr &b ) const
        {
            return a->getName() < b->getName();
        }
    };
#endif

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
    public:
        enum DebugVisualizationMode
        {
            DebugVisualizationAlbedo,
            DebugVisualizationNormal,
            DebugVisualizationEmissive,
            DebugVisualizationNone
        };
    protected:
        struct MappedBuffers
        {
            float * RESTRICT_ALIAS uncompressedVertexBuffer;
            size_t index16BufferOffset;
            size_t index32BufferOffset;
        };

        struct PartitionedSubMesh
        {
            uint32  vbOffset;
            uint32  ibOffset;
            uint32  numIndices;
            uint32  partSubMeshIdx;
        };

        struct QueuedSubMesh
        {
            //Due to an infrastructure bug, we're commenting out the 'STREAM_DOWNLOAD' path
            //The goal was to queue up several transfer GPU -> staging area, then map
            //the staging area. However this backfired as each AsyncTicket will hold its own
            //StagingBuffer (and each one is at least 4MB) instead of sharing it. This balloons
            //memory consumption and still needs to map staging buffers a lot, defeating part of
            //its purpose.
            //Since fixing it would take a lot of time, it has been ifdef'ed out and instead
            //we download the data and immediately map it. This causes more stalls but is
            //far more memory friendly.
#ifdef STREAM_DOWNLOAD
            VertexBufferDownloadHelper downloadHelper;
#else
            size_t  downloadVertexStart;
            size_t  downloadNumVertices;
#endif
            FastArray<PartitionedSubMesh> partSubMeshes;
        };

        typedef FastArray<QueuedSubMesh> QueuedSubMeshArray;

        struct QueuedMesh
        {
            bool                bCompressed;
            uint32              numItems;
            uint32              indexCountSplit;
            QueuedSubMeshArray  submeshes;
        };

        typedef map<v1::MeshPtr, bool>::type v1MeshPtrMap;
#ifdef OGRE_FORCE_VCT_VOXELIZER_DETERMINISTIC
        typedef map<MeshPtr, QueuedMesh, DeterministicMeshPtrOrder>::type MeshPtrMap;
#else
        typedef map<MeshPtr, QueuedMesh>::type MeshPtrMap;
#endif
        typedef FastArray<Item*> ItemArray;

        v1MeshPtrMap    mMeshesV1;
        MeshPtrMap      mMeshesV2;

        ItemArray       mItems;

        /// HlmsComputeJob have internal caches, thus we could dynamically change properties
        /// and let the internal cache handle whether a compute job needs to be compiled.
        ///
        /// However the way we will be using may abuse the cache too much, thus we pre-set
        /// all variants as long as the number of variants is manageable.
        HlmsComputeJob  *mComputeJobs[1u<<4u];
        HlmsComputeJob  *mAabbCalculator[1u<<2u];
        HlmsComputeJob  *mAabbWorldSpaceJob;

        uint32			mTotalNumInstances;
        float           *mCpuInstanceBuffer;
        UavBufferPacked *mInstanceBuffer;
        TexBufferPacked *mInstanceBufferAsTex;
        UavBufferPacked *mVertexBufferCompressed;
        UavBufferPacked *mVertexBufferUncompressed;
        UavBufferPacked *mIndexBuffer16;
        UavBufferPacked *mIndexBuffer32;
        //Aabb Calculator
        uint32			mNumUncompressedPartSubMeshes16;
        uint32			mNumUncompressedPartSubMeshes32;
        uint32			mNumCompressedPartSubMeshes16;
        uint32			mNumCompressedPartSubMeshes32;
        TexBufferPacked *mGpuPartitionedSubMeshes;
        UavBufferPacked *mMeshAabb;
        /// Normally once we're done we free all memory that isn't needed. However
        /// we leave mGpuPartitionedSubMeshes & mMeshAabb around because they occupy very little memory
        bool            mGpuMeshAabbDataDirty;

        bool    mNeedsAlbedoMipmaps;

        uint32 mNumVerticesCompressed;
        uint32 mNumVerticesUncompressed;
        uint32 mNumIndices16;
        uint32 mNumIndices32;

        uint32 mDefaultIndexCountSplit;

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
            uint32          partSubMeshIdx;
            uint32          materialIdx;
            bool            needsAabbUpdate;
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

        ResourceTransition mStartupTrans;
        ResourceTransition mAfterClearTrans;
        ResourceTransition mAfterAabbCalculatorTrans;
        ResourceTransition mAfterAabbWorldUpdateTrans;
        ResourceTransition mVoxelizerInterDispatchTrans;
        ResourceTransition mVoxelizerPrepareForSamplingTrans;

        DebugVisualizationMode  mDebugVisualizationMode;
        VoxelVisualizer         *mDebugVoxelVisualizer;

        /** 16-bit buffer values must always be even since the UAV buffer
            is internally packed uint32 and BufferPacked::copyTo doesn't
            like copying with odd-starting offsets in some APIs.
        @returns
            True if indexStart was decremented
        */
        static bool adjustIndexOffsets16( uint32 &indexStart, uint32 &numIndices );

        void createComputeJobs();

        void countBuffersSize( const MeshPtr &mesh, QueuedMesh &queuedMesh );
        void prepareAabbCalculatorMeshData(void);
        void destroyAabbCalculatorMeshData(void);
        void convertMeshUncompressed( const MeshPtr &mesh, QueuedMesh &queuedMesh,
                                      MappedBuffers &mappedBuffers );

        void freeBuffers( bool bForceFree );

        void buildMeshBuffers(void);
        void createVoxelTextures(void);
        void destroyVoxelTextures(void);
        void setTextureToDebugVisualizer(void);

        void placeItemsInBuckets(void);
        size_t countSubMeshPartitionsIn( Item *item ) const;
        void createInstanceBuffers(void);
        void destroyInstanceBuffers(void);
        void fillInstanceBuffers(void);

        void computeMeshAabbs(void);

        void createBarriers(void);
        void destroyBarriers(void);

        void clearVoxels(void);

    public:
        VctVoxelizer( IdType id, RenderSystem *renderSystem, HlmsManager *hlmsManager,
                      bool correctAreaLightShadows );
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
        @param indexCountSplit
            0 to use mDefaultIndexCountSplit. Use a different value to override
            This value is ignored if the mesh had already been added.

            Use std::numeric_limits<uint32>::max to avoid partitioning at all.
        */
        void addItem( Item *item, bool bCompressed, uint32 indexCountSplit=0u );

        /** Removes an item added via VctVoxelizer::addItem
        @remarks
            Once the last item that shares the same mesh is removed, the entry about
            that mesh is also removed.
            That means informations such as 'compressed' setting is forgot.

            Will throw if Item is not found.
        @param item
            Item to remove
        */
        void removeItem( Item *item );

        /// Removes all items added via VctVoxelizer::addItem
        void removeAllItems(void);

        void autoCalculateRegion(void);

        void dividideOctants( uint32 numOctantsX, uint32 numOctantsY, uint32 numOctantsZ );

        /** Changes resolution. Note that after calling this, you will need to call
            VctVoxelizer::build again, and VctLighting::build again.
        @param width
        @param height
        @param depth
        */
        void setResolution( uint32 width, uint32 height, uint32 depth );

        void build( SceneManager *sceneManager );

        void setDebugVisualization( VctVoxelizer::DebugVisualizationMode mode,
                                    SceneManager *sceneManager );
        VctVoxelizer::DebugVisualizationMode getDebugVisualizationMode(void) const;

        Vector3 getVoxelOrigin(void) const;
        Vector3 getVoxelCellSize(void) const;
        Vector3 getVoxelSize(void) const;
        Vector3 getVoxelResolution(void) const;

        TextureGpu* getAlbedoVox(void)          { return mAlbedoVox; }
        TextureGpu* getNormalVox(void)          { return mNormalVox; }
        TextureGpu* getEmissiveVox(void)        { return mEmissiveVox; }

        TextureGpuManager* getTextureGpuManager(void);
        RenderSystem* getRenderSystem(void);
        HlmsManager* getHlmsManager(void);
    };
}

#include "OgreHeaderSuffix.h"

#endif
