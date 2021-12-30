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
#ifndef _OgreVoxelizedMeshCache_H_
#define _OgreVoxelizedMeshCache_H_

#include "OgreVctVoxelizerSourceBase.h"

#include "OgreIdString.h"
#include "OgreResourceTransition.h"

#include <ogrestd/map.h>

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /**
    @class VoxelizedMeshCache
        Uses a VctVoxelizer to convert a single mesh made up of triangles into a voxel
        3D texture representation.

        This cache can be shared by multiple users (e.g. VctImageVoxelizer)
    */
    class _OgreHlmsPbsExport VoxelizedMeshCache : IdObject
    {
    public:
        struct VoxelizedMesh
        {
            uint64      hash[2];
            String      meshName;
            TextureGpu *albedoVox;
            TextureGpu *normalVox;
            TextureGpu *emissiveVox;
        };

    protected:
        typedef map<IdString, VoxelizedMesh>::type MeshCacheMap;

        uint32        mMeshWidth;
        uint32        mMeshHeight;
        uint32        mMeshDepth;
        uint32        mMeshMaxWidth;
        uint32        mMeshMaxHeight;
        uint32        mMeshMaxDepth;
        Ogre::Vector3 mMeshDimensionPerPixel;

        MeshCacheMap mMeshes;

        /// Many meshes have no emissive at all, thus just use a 1x1x1 emissive
        /// texture for all those meshes instead of wasting ton of RAM.
        TextureGpu *mBlankEmissive;

    public:
        /// The number of texture units GL can handle may exceed the hard limit in
        /// ShaderParams::ManualParam::dataBytes so we need to use EX and store
        /// the data here
        FastArray<int32> mGlslTexUnits;

    public:
        VoxelizedMeshCache( IdType id, TextureGpuManager *textureManager );
        ~VoxelizedMeshCache();

        /**
        @brief addMeshToCache
            Checks if the mesh is already cached. If it's not, it gets voxelized.
        @param mesh
            Mesh to voxelize
        @param sceneManager
            We need it to temporarily create an Item
        @param refItem
            Reference Item in case we need to copy its materials. Can be nullptr
        @returns
            Entry to VoxelizedMesh in cache
        */
        const VoxelizedMesh &addMeshToCache( const MeshPtr &mesh, SceneManager *sceneManager,
                                             RenderSystem *renderSystem, HlmsManager *hlmsManager,
                                             const Item *refItem );

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

        TextureGpu *getBlankEmissive() { return mBlankEmissive; }
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
