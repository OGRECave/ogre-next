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
#ifndef _OgreVctImageVoxelizer_H_
#define _OgreVctImageVoxelizer_H_

#include "OgreVctVoxelizerSourceBase.h"

#include "OgreIdString.h"
#include "OgreResourceTransition.h"

#include <ogrestd/map.h>

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class VoxelVisualizer;
    class VoxelizedMeshCache;

    /**
    @class VctImageVoxelizer
        The image voxelizer aims to be faster than VctImageVoxelizer for the whole scene
        by following a different approach:

            1. For each single mesh, use VctImageVoxelizer to keep a voxelized texture version and
               save it to disk
            2. At build time, VctImageVoxelizer builds the voxel of the whole scene
               by iterating each mesh using those cached texture versions like a collage.
    */
    class _OgreHlmsPbsExport VctImageVoxelizer : public VctVoxelizerSourceBase
    {
    protected:
        struct BatchInstances
        {
            uint32 instanceOffset;
            uint32 numInstances;
            BatchInstances( uint32 _instanceOffset ) :
                instanceOffset( _instanceOffset ),
                numInstances( 0u )
            {
            }
            BatchInstances() : instanceOffset( 0u ), numInstances( 0u ) {}
        };

        struct Batch
        {
            FastArray<BatchInstances> instances;  // one per octant
            FastArray<TextureGpu *>   textures;
        };

        typedef FastArray<Batch>  BatchArray;
        typedef FastArray<Item *> ItemArray;

        VoxelizedMeshCache *mMeshCache;

        /// We split dispatch in batches because not all APIs/GPUs support
        /// binding (or dynamically indexing) enough textures per dispatch.
        ///
        /// On Desktop GPUs running Vulkan we'll likely have just one batch
        /// since we can bind all mesh textures at once and index them
        /// dynamically inside the compute shader
        ///
        /// On other API/GPUs we may need to split the dispatch into multiple
        /// ones; the worst case scenario we'll have 1 batch per mesh in scene.
        BatchArray mBatches;
        ItemArray  mItems;
        bool       mItemOrderDirty;

        uint32          mTexMeshesPerBatch;  // 3 * meshes per batch
        HlmsComputeJob *mImageVoxelizerJob;
        HlmsComputeJob *mPartialClearJob;

        ComputeTools *mComputeTools;

        bool mFullBuildDone;  /// When false, buildRelative must call build
        bool mNeedsAlbedoMipmaps;

        /// Whether mRegionToVoxelize is manually set or autocalculated
        bool mAutoRegion;
        /// Limit to mRegionToVoxelize in case mAutoRegion is true
        Aabb mMaxRegion;

        struct Octant
        {
            uint32 x, y, z;
            uint32 width, height, depth;
            Aabb   region;

            float *RESTRICT_ALIAS instanceBuffer;  // Temporary
            uint32                diffAxis;        // Used in buildRelative
        };

        FastArray<Octant> mOctants;
        FastArray<Octant> mTmpOctants;

        float *mCpuInstanceBuffer;
        /// Actually ReadOnlyBufferPacked is enough, but currently our only path
        /// to get structured buffers on D3D11 is through UavBufferPacked
        UavBufferPacked *mInstanceBuffer;

        /// When buildRelative is called, we need to move the voxels
        /// and we can't due it in-place (race condition) so we
        /// move it to to a copy and swap the pointers
        TextureGpu *mAlbedoVoxAlt;
        TextureGpu *mEmissiveVoxAlt;
        TextureGpu *mNormalVoxAlt;

        ResourceTransitionArray mResourceTransitions;

        void createComputeJobs();
        void clearComputeJobResources();

    public:
        /// Swaps mAlbedoVox & mAlbedoVoxAlt (and co.) if needed back to their originals.
        /// This is needed during shutdown since VctLighting otherwise will try to remove
        /// itself as a listener from mAlbedoVoxAlt
        void restoreSwappedVoxelTextures();

    protected:
        void createVoxelTextures();
        void createAltVoxelTextures();
        void setVoxelTexturesToJobs();

        void destroyVoxelTextures() override;

        void createInstanceBuffers();
        void destroyInstanceBuffers();
        void fillInstanceBuffers( SceneManager *sceneManager );

        void clearVoxels( const bool bPartialClear );

        void setOffsetOctants( const int32 diffX, const int32 diffY, const int32 diffZ );

    public:
        VctImageVoxelizer( IdType id, RenderSystem *renderSystem, HlmsManager *hlmsManager,
                           VoxelizedMeshCache *meshCache, bool correctAreaLightShadows );
        ~VctImageVoxelizer() override;

        /** Adds an item to voxelize.
        @param item
        */
        void addItem( Item *item );

        /** Removes an item added via VctImageVoxelizer::addItem
        @param item
            Item to remove
        */
        void removeItem( Item *item );

        /// Removes all items added via VctImageVoxelizer::addItem
        void removeAllItems();

        /** Call this function before VctImageVoxelizer::autoCalculateRegion
        @param autoRegion
            True to autocalculate region to cover all the added items
            False to use 'regionToVoxelize' instead
        @param regionToVoxelize
            When autoRegion = false, use this to manually provide the region
            When autoRegion = true, it is ignored as it will be overwritten by autoCalculateRegion
        @param maxRegion
            Maximum size of the regions are allowed to cover (mostly useful when autoRegion = true)
        */
        void setRegionToVoxelize( bool autoRegion, const Aabb &regionToVoxelize,
                                  const Aabb &maxRegion = Aabb::BOX_INFINITE );

        /// Does nothing if VctImageVoxelizer::setRegionToVoxelize( false, ... ) was called.
        void autoCalculateRegion();

        void dividideOctants( uint32 numOctantsX, uint32 numOctantsY, uint32 numOctantsZ );

        /** Changes resolution. Note that after calling this, you will need to call
            VctImageVoxelizer::build again, and VctLighting::build again.
        @param width
        @param height
        @param depth
        */
        void setSceneResolution( uint32 width, uint32 height, uint32 depth );

        void build( SceneManager *sceneManager );

        /** If the camera has moved by 1 voxel to the right (i.e. diffX = 1) we will
            "translate" the voxels by 1 and then partially rebuild those sections
        @remarks
            Do not call this function if diffX/diffY/diffZ are all == 0.
        @param sceneManager
        @param diffX
            How many voxels to translate to the left/right
        @param diffY
            How many voxels to translate up/down
        @param diffZ
            How many voxels to translate to the forward/backwards
        @param numOctantsX
            If we must perform a full rebuild, then the parameter passed to dividideOctants
        @param numOctantsY
            See numOctantsX
        @param numOctantsZ
            See numOctantsX
        */
        void buildRelative( SceneManager *sceneManager, const int32 diffX, const int32 diffY,
                            const int32 diffZ, const uint32 numOctantsX, const uint32 numOctantsY,
                            const uint32 numOctantsZ );

        void forceFullBuild() { mFullBuildDone = false; }
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
