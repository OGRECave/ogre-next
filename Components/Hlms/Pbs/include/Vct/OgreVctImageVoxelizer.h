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

#include "OgreHlmsPbsPrerequisites.h"

#include "Math/Simple/OgreAabb.h"
#include "OgreId.h"
#include "OgreIdString.h"
#include "Vao/OgreVertexBufferDownloadHelper.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class VoxelVisualizer;

    /**
    @class VctImageVoxelizer
        The image voxelizer aims to be faster than VctImageVoxelizer for the whole scene
        by following a different approach:

            1. For each single mesh, use VctImageVoxelizer to keep a voxelized texture version and
               save it to disk
            2. At build time, VctImageVoxelizer builds the voxel of the whole scene
               by iterating each mesh using those cached texture versions like a collage.
    */
    class _OgreHlmsPbsExport VctImageVoxelizer : public IdObject
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
        struct VoxelizedMesh
        {
            uint64 hash[2];
            String meshName;
            TextureGpu *albedoVox;
            TextureGpu *normalVox;
            TextureGpu *emissiveVox;
        };

        typedef map<IdString, VoxelizedMesh>::type MeshCacheMap;

        typedef FastArray<Item *> ItemArray;

        uint32 mMeshWidth;
        uint32 mMeshHeight;
        uint32 mMeshDepth;
        uint32 mMeshMaxWidth;
        uint32 mMeshMaxHeight;
        uint32 mMeshMaxDepth;
        Ogre::Vector3 mMeshDimensionPerPixel;

        MeshCacheMap mMeshes;
        /// Many meshes have no emissive at all, thus just use a 1x1x1 emissive
        /// texture for all those meshes instead of wasting ton of RAM.
        TextureGpu *mBlankEmissive;

        ItemArray mItems;

        HlmsComputeJob *mImageVoxelizerJob;

        TextureGpu *mAlbedoVox;
        TextureGpu *mEmissiveVox;
        TextureGpu *mNormalVox;
        TextureGpu *mAccumValVox;

        RenderSystem *mRenderSystem;
        VaoManager *mVaoManager;
        HlmsManager *mHlmsManager;
        TextureGpuManager *mTextureGpuManager;

        ComputeTools *mComputeTools;

        uint32 mSceneWidth;
        uint32 mSceneHeight;
        uint32 mSceneDepth;

        bool mNeedsAlbedoMipmaps;

        /// Whether mRegionToVoxelize is manually set or autocalculated
        bool mAutoRegion;
        Aabb mRegionToVoxelize;
        /// Limit to mRegionToVoxelize in case mAutoRegion is true
        Aabb mMaxRegion;

        struct Octant
        {
            uint32 x, y, z;
            uint32 width, height, depth;
            Aabb region;

            uint32 instances;                      // Used internally
            float *RESTRICT_ALIAS instanceBuffer;  // Temporary
        };

        FastArray<Octant> mOctants;

        float *mCpuInstanceBuffer;
        ReadOnlyBufferPacked *mInstanceBuffer;

        ResourceTransitionArray mResourceTransitions;

        DebugVisualizationMode mDebugVisualizationMode;
        VoxelVisualizer *mDebugVoxelVisualizer;

        void createComputeJobs( void );
        void clearComputeJobResources( void );

        void createVoxelTextures( void );
        void destroyVoxelTextures( void );
        void setTextureToDebugVisualizer( void );

        void createInstanceBuffers( void );
        void destroyInstanceBuffers( void );
        void fillInstanceBuffers( SceneManager *sceneManager );

        void clearVoxels( void );

    public:
        VctImageVoxelizer( IdType id, RenderSystem *renderSystem, HlmsManager *hlmsManager,
                           bool correctAreaLightShadows );
        ~VctImageVoxelizer();

        /**
        @brief addMeshToCache
            Checks if the mesh is already cached. If it's not, it gets voxelized.
        @param mesh
            Mesh to voxelize
        @param sceneManager
            We need it to temporarily create an Item
        */
        void addMeshToCache( const MeshPtr &mesh, SceneManager *sceneManager );

        /**
        @brief setCacheResolution
            When building the mesh cache, meshes must be voxelized at some arbitrary resolution
            This function lets you specify how many voxels per volume in units.

            e.g. if you call
            @code
                imageVoxelizer->setCacheResolution( 32u, 32u, 32u, maxWidth, maxHeight, maxDepth
                                                    Ogre::Vector3( 1.0f, 1.0f, 1.0f ) );
            @endcode

            And the mesh AABB is 2x2x2 in units, then we will voxelize at 64x64x64 voxels
            (unless this resolution exceeds maxWidth/maxHeight/maxDepth)

        @remarks
            This setting can be changed individually for each mesh.
            Call it as often as you need. e.g. a literal cube mesh
            only needs 1x1x1 of resolution.

        @param width
            The width in pixels per dimension.x in units
            i.e. the width / dimension.x
        @param height
            The height in pixels per dimension.y in units
            i.e. the height / dimension.y
        @param depth
            The height in pixels per dimension.z in units
            i.e. the depth / dimension.z
        @param maxWidth
            Width can never exceed this value
        @param maxHeight
            Height can never exceed this value
        @param maxDepth
            Depth can never exceed this value
        @param dimension
            Units to cover (see previous parameters)
        */
        void setCacheResolution( uint32 width, uint32 height, uint32 depth, uint32 maxWidth,
                                 uint32 maxHeight, uint32 maxDepth, const Ogre::Vector3 &dimension );

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
        void removeAllItems( void );

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
        void autoCalculateRegion( void );

        void dividideOctants( uint32 numOctantsX, uint32 numOctantsY, uint32 numOctantsZ );

        /** Changes resolution. Note that after calling this, you will need to call
            VctImageVoxelizer::build again, and VctLighting::build again.
        @param width
        @param height
        @param depth
        */
        void setSceneResolution( uint32 width, uint32 height, uint32 depth );

        void build( SceneManager *sceneManager );

        void setDebugVisualization( VctImageVoxelizer::DebugVisualizationMode mode,
                                    SceneManager *sceneManager );
        VctImageVoxelizer::DebugVisualizationMode getDebugVisualizationMode( void ) const;

        Vector3 getVoxelOrigin( void ) const;
        Vector3 getVoxelCellSize( void ) const;
        Vector3 getVoxelSize( void ) const;
        Vector3 getVoxelResolution( void ) const;

        TextureGpu *getAlbedoVox( void ) { return mAlbedoVox; }
        TextureGpu *getNormalVox( void ) { return mNormalVox; }
        TextureGpu *getEmissiveVox( void ) { return mEmissiveVox; }

        TextureGpuManager *getTextureGpuManager( void );
        RenderSystem *getRenderSystem( void );
        HlmsManager *getHlmsManager( void );
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
