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

#include "OgreStableHeaders.h"

#include "Vct/OgreVoxelizedMeshCache.h"

#include "Vct/OgreVctVoxelizer.h"

#include "OgreItem.h"
#include "OgreLogManager.h"
#include "OgreMesh2.h"
#include "OgreSceneManager.h"
#include "OgreStagingTexture.h"
#include "OgreSubMesh2.h"
#include "OgreTextureBox.h"
#include "OgreTextureGpuManager.h"

#include "OgreProfiler.h"

namespace Ogre
{
#if OGRE_COMPILER == OGRE_COMPILER_MSVC && OGRE_COMP_VER < 1800
    inline float roundf( float x ) { return x >= 0.0f ? floorf( x + 0.5f ) : ceilf( x - 0.5f ); }
#endif

    VoxelizedMeshCache::VoxelizedMeshCache( IdType id, TextureGpuManager *textureManager ) :
        IdObject( id ),
        mMeshWidth( 64u ),
        mMeshHeight( 64u ),
        mMeshDepth( 64u ),
        mMeshMaxWidth( 64u ),
        mMeshMaxHeight( 64u ),
        mMeshMaxDepth( 64u ),
        mMeshDimensionPerPixel( 2.0f ),
        mBlankEmissive( 0 )
    {
        mBlankEmissive = textureManager->createTexture(
            "VctImage/BlankEmissive" + StringConverter::toString( getId() ), GpuPageOutStrategy::Discard,
            TextureFlags::ManualTexture, TextureTypes::Type3D );
        mBlankEmissive->setResolution( 1u, 1u, 1u );
        mBlankEmissive->setPixelFormat( PFG_RGBA8_UNORM );
        mBlankEmissive->scheduleTransitionTo( GpuResidency::Resident );

        StagingTexture *stagingTexture =
            textureManager->getStagingTexture( 1u, 1u, 1u, 1u, PFG_RGBA8_UNORM, 100u );
        stagingTexture->startMapRegion();
        TextureBox box = stagingTexture->mapRegion( 1u, 1u, 1u, 1u, PFG_RGBA8_UNORM );
        box.setColourAt( Ogre::ColourValue::Black, 0u, 0u, 0u, PFG_RGBA8_UNORM );
        stagingTexture->stopMapRegion();
        stagingTexture->upload( box, mBlankEmissive, 0u );
        textureManager->removeStagingTexture( stagingTexture );
    }
    //-------------------------------------------------------------------------
    VoxelizedMeshCache::~VoxelizedMeshCache()
    {
        TextureGpuManager *textureManager = mBlankEmissive->getTextureManager();

        // Destroy cache
        MeshCacheMap::const_iterator itor = mMeshes.begin();
        MeshCacheMap::const_iterator endt = mMeshes.end();

        while( itor != endt )
        {
            textureManager->destroyTexture( itor->second.albedoVox );
            textureManager->destroyTexture( itor->second.normalVox );
            if( itor->second.emissiveVox != mBlankEmissive )
                textureManager->destroyTexture( itor->second.emissiveVox );
            ++itor;
        }
        mMeshes.clear();

        textureManager->destroyTexture( mBlankEmissive );
        mBlankEmissive = 0;
    }
    //-------------------------------------------------------------------------
    inline bool isPowerOf2( uint32 x ) { return ( x & ( x - 1u ) ) == 0u; }
    inline uint32 getNextPowerOf2( uint32 x )
    {
        if( isPowerOf2( x ) )
            return x;

        x = 1u << ( 32u - Bitwise::clz32( x ) );
        return x;
    }
    inline uint32 calculateMeshResolution( uint32 width, const float actualLength,
                                           const float referenceLength, uint32 maxWidth )
    {
        uint32 finalRes =
            static_cast<uint32>( roundf( float( width ) * actualLength / referenceLength ) );
        finalRes = getNextPowerOf2( finalRes );
        finalRes = std::min( finalRes, maxWidth );
        finalRes = std::max( finalRes, 1u );
        return finalRes;
    }
    /// Returns true if the Item has at least 1 submesh with emissive materials
    /// Note: This is a light check. If the submesh has an emissive texture
    /// but the texture is all black, we will mark it as emissive even though
    /// it shouldn't
    static bool hasEmissive( const Item *item )
    {
        const size_t numSubItems = item->getNumSubItems();
        for( size_t i = 0u; i < numSubItems; ++i )
        {
            const SubItem *subItem = item->getSubItem( i );
            const HlmsDatablock *datablock = subItem->getDatablock();

            if( datablock->getEmissiveTexture() )
                return true;

            if( datablock->getEmissiveColour() != ColourValue::Black )
                return true;
        }
        return false;
    }

    const VoxelizedMeshCache::VoxelizedMesh &VoxelizedMeshCache::addMeshToCache(
        const MeshPtr &mesh, SceneManager *sceneManager, RenderSystem *renderSystem,
        HlmsManager *hlmsManager, const Item *refItem )
    {
        bool bUpToDate = false;
        const String &meshName = mesh->getName();

        TextureGpuManager *textureManager = mBlankEmissive->getTextureManager();

        MeshCacheMap::iterator itor = mMeshes.find( meshName );
        if( itor != mMeshes.end() )
        {
            const uint64 *hash = mesh->getHashForCaches();

            if( hash[0] == 0u && hash[1] == 0u )
            {
                LogManager::getSingleton().logMessage(
                    "WARNING: Mesh '" + mesh->getName() +
                        "' has no hash and thus cannot determine if cache is stale. Make sure "
                        "mesh->_setHashForCaches is set, or save the Mesh again using "
                        "OgreMeshTool or set Mesh::msUseTimestampAsHash to true",
                    LML_CRITICAL );

                // Cache won't save to disk if there is no hash info, thus we must
                // have created it recently. Let's assume it's not stale otherwise
                // every Item will revoxelize the mesh!
                if( itor->second.hash[0] == 0u && itor->second.hash[1] == 0u )
                {
                    bUpToDate = true;
                    LogManager::getSingleton().logMessage(
                        "WARNING: Mesh '" + mesh->getName() + "' assuming cache entry is not stale",
                        LML_CRITICAL );
                }
                else
                {
                    // Erase entry
                    textureManager->destroyTexture( itor->second.albedoVox );
                    textureManager->destroyTexture( itor->second.normalVox );
                    if( itor->second.emissiveVox != mBlankEmissive )
                        textureManager->destroyTexture( itor->second.emissiveVox );

                    MeshCacheMap::iterator toErase = itor++;
                    mMeshes.erase( toErase );
                }
            }
            else if( itor->second.hash[0] == hash[0] && itor->second.hash[1] == hash[1] )
            {
                bUpToDate = true;
            }
        }

        if( !bUpToDate )
        {
            VctVoxelizer voxelizer( Ogre::Id::generateNewId<Ogre::VctVoxelizer>(), renderSystem,
                                    hlmsManager, true );
            voxelizer._setNeedsAllMipmaps( true );

            Item *tmpItem = sceneManager->createItem( mesh );

            if( refItem )
            {
                const size_t numSubItems = tmpItem->getNumSubItems();
                for( size_t i = 0u; i < numSubItems; ++i )
                    tmpItem->getSubItem( i )->setDatablock( refItem->getSubItem( i )->getDatablock() );
            }

            sceneManager->getRootSceneNode()->attachObject( tmpItem );

            Aabb aabb = tmpItem->getLocalAabb();

            const uint32 actualWidth = calculateMeshResolution(
                mMeshWidth, aabb.getSize().x, mMeshDimensionPerPixel.x, mMeshMaxWidth );
            const uint32 actualHeight = calculateMeshResolution(
                mMeshHeight, aabb.getSize().y, mMeshDimensionPerPixel.y, mMeshMaxHeight );
            const uint32 actualDepth = calculateMeshResolution(
                mMeshDepth, aabb.getSize().z, mMeshDimensionPerPixel.z, mMeshMaxDepth );
            voxelizer.setResolution( actualWidth, actualHeight, actualDepth );

            tmpItem->getWorldAabbUpdated();  // Force AABB calculation

            voxelizer.addItem( tmpItem, false );
            voxelizer.autoCalculateRegion();
            voxelizer.dividideOctants( 1u, 1u, 1u );
            voxelizer.build( sceneManager );

            VoxelizedMesh voxelizedMesh;

            voxelizedMesh.hash[0] = mesh->getHashForCaches()[0];
            voxelizedMesh.hash[1] = mesh->getHashForCaches()[1];
            voxelizedMesh.meshName = meshName;

            const bool bHasEmissive = hasEmissive( tmpItem );

            // Copy the results to a 3D texture that is optimized for sampling
            voxelizedMesh.albedoVox = textureManager->createTexture(
                "VctImage/" + mesh->getName() + "/Albedo" + StringConverter::toString( getId() ),
                GpuPageOutStrategy::Discard, TextureFlags::ManualTexture, TextureTypes::Type3D );
            voxelizedMesh.normalVox = textureManager->createTexture(
                "VctImage/" + mesh->getName() + "/Normal" + StringConverter::toString( getId() ),
                GpuPageOutStrategy::Discard, TextureFlags::ManualTexture, TextureTypes::Type3D );
            if( bHasEmissive )
            {
                voxelizedMesh.emissiveVox = textureManager->createTexture(
                    "VctImage/" + mesh->getName() + "/Emissive" + StringConverter::toString( getId() ),
                    GpuPageOutStrategy::Discard, TextureFlags::ManualTexture, TextureTypes::Type3D );
            }
            else
            {
                voxelizedMesh.emissiveVox = mBlankEmissive;
            }
            voxelizedMesh.albedoVox->copyParametersFrom( voxelizer.getAlbedoVox() );
            voxelizedMesh.albedoVox->scheduleTransitionTo( GpuResidency::Resident );
            voxelizedMesh.normalVox->copyParametersFrom( voxelizer.getNormalVox() );
            voxelizedMesh.normalVox->scheduleTransitionTo( GpuResidency::Resident );
            if( bHasEmissive )
            {
                voxelizedMesh.emissiveVox->copyParametersFrom( voxelizer.getEmissiveVox() );
                voxelizedMesh.emissiveVox->scheduleTransitionTo( GpuResidency::Resident );
            }

            const uint8 numMipmaps = voxelizedMesh.albedoVox->getNumMipmaps();
            for( uint8 i = 0u; i < numMipmaps; ++i )
            {
                voxelizer.getAlbedoVox()->copyTo( voxelizedMesh.albedoVox,
                                                  voxelizedMesh.albedoVox->getEmptyBox( i ), i,
                                                  voxelizedMesh.albedoVox->getEmptyBox( i ), i );
                voxelizer.getNormalVox()->copyTo( voxelizedMesh.normalVox,
                                                  voxelizedMesh.normalVox->getEmptyBox( i ), i,
                                                  voxelizedMesh.normalVox->getEmptyBox( i ), i );
                if( bHasEmissive )
                {
                    voxelizer.getEmissiveVox()->copyTo( voxelizedMesh.emissiveVox,
                                                        voxelizedMesh.emissiveVox->getEmptyBox( i ), i,
                                                        voxelizedMesh.emissiveVox->getEmptyBox( i ), i );
                }
            }

            itor = mMeshes.emplace( meshName, voxelizedMesh ).first;

            sceneManager->destroyItem( tmpItem );
        }

        return itor->second;
    }
    //-------------------------------------------------------------------------
    void VoxelizedMeshCache::setCacheResolution( uint32 width, uint32 height, uint32 depth,
                                                 uint32 maxWidth, uint32 maxHeight, uint32 maxDepth,
                                                 const Ogre::Vector3 &dimension )
    {
        mMeshWidth = width;
        mMeshHeight = height;
        mMeshDepth = depth;
        mMeshMaxWidth = maxWidth;
        mMeshMaxHeight = maxHeight;
        mMeshMaxDepth = maxDepth;
        mMeshDimensionPerPixel = dimension;
    }
}  // namespace Ogre
