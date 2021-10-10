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

#include "Vct/OgreVctImageVoxelizer.h"

#include "Vct/OgreVctVoxelizer.h"
#include "Vct/OgreVoxelVisualizer.h"

#include "Compute/OgreComputeTools.h"
#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"
#include "OgreItem.h"
#include "OgreLogManager.h"
#include "OgreMesh2.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreRenderSystem.h"
#include "OgreSceneManager.h"
#include "OgreStringConverter.h"
#include "OgreSubMesh2.h"
#include "OgreTextureBox.h"
#include "OgreTextureGpuManager.h"
#include "Vao/OgreReadOnlyBufferPacked.h"
#include "Vao/OgreUavBufferPacked.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include "OgreProfiler.h"

#define TODO_deal_no_index_buffer

namespace Ogre
{
    VctImageVoxelizer::VctImageVoxelizer( IdType id, RenderSystem *renderSystem,
                                          HlmsManager *hlmsManager, bool correctAreaLightShadows ) :
        IdObject( id ),
        mMeshWidth( 64u ),
        mMeshHeight( 64u ),
        mMeshDepth( 64u ),
        mMeshMaxWidth( 64u ),
        mMeshMaxHeight( 64u ),
        mMeshMaxDepth( 64u ),
        mMeshDimensionPerPixel( 2.0f ),
        mBlankEmissive( 0 ),
        mAlbedoVox( 0 ),
        mEmissiveVox( 0 ),
        mNormalVox( 0 ),
        mAccumValVox( 0 ),
        mRenderSystem( renderSystem ),
        mVaoManager( renderSystem->getVaoManager() ),
        mHlmsManager( hlmsManager ),
        mTextureGpuManager( renderSystem->getTextureGpuManager() ),
        mComputeTools( new ComputeTools( hlmsManager->getComputeHlms() ) ),
        mSceneWidth( 128u ),
        mSceneHeight( 128u ),
        mSceneDepth( 128u ),
        mNeedsAlbedoMipmaps( correctAreaLightShadows ),
        mAutoRegion( true ),
        mRegionToVoxelize( Aabb::BOX_ZERO ),
        mMaxRegion( Aabb::BOX_INFINITE ),
        mDebugVisualizationMode( DebugVisualizationNone ),
        mDebugVoxelVisualizer( 0 )
    {
        createComputeJobs();
    }
    //-------------------------------------------------------------------------
    VctImageVoxelizer::~VctImageVoxelizer()
    {
        setDebugVisualization( DebugVisualizationNone, 0 );
        destroyVoxelTextures();
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::createComputeJobs( void ) {}
    //-------------------------------------------------------------------------
    inline bool isPowerOf2( uint32 x ) { return ( x & ( x - 1u ) ) == 0u; }
    inline bool getNextPowerOf2( uint32 x )
    {
        if( isPowerOf2( x ) )
            return x;

        x = x << 1u;
        x = x & ~( x - 1u );
        return x;
    }
    inline uint32 calculateMeshResolution( uint32 width, const float actualLength,
                                           const float referenceLength, uint32 maxWidth )
    {
        uint32 finalRes = static_cast<uint32>( std::round( width * actualLength / referenceLength ) );
        finalRes = getNextPowerOf2( finalRes );
        finalRes = std::min( finalRes, maxWidth );
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

    void VctImageVoxelizer::addMeshToCache( const MeshPtr &mesh, SceneManager *sceneManager )
    {
        bool bUpToDate = false;
        String meshName = mesh->getName();

        MeshCacheMap::const_iterator itor = mMeshes.find( meshName );
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
            }
            else if( itor->second.hash[0] == hash[0] && itor->second.hash[1] == hash[1] )
            {
                bUpToDate = true;
            }
        }

        if( !bUpToDate )
        {
            VctVoxelizer voxelizer( Ogre::Id::generateNewId<Ogre::VctVoxelizer>(), mRenderSystem,
                                    mHlmsManager, true );

            Item *tmpItem = sceneManager->createItem( mesh );
            sceneManager->getRootSceneNode()->attachObject( tmpItem );

            Aabb aabb = tmpItem->getLocalAabb();

            const uint32 actualWidth = calculateMeshResolution(
                mMeshWidth, aabb.getSize().x, mMeshDimensionPerPixel.x, mMeshMaxWidth );
            const uint32 actualHeight = calculateMeshResolution(
                mMeshHeight, aabb.getSize().y, mMeshDimensionPerPixel.y, mMeshMaxHeight );
            const uint32 actualDepth = calculateMeshResolution(
                mMeshDepth, aabb.getSize().z, mMeshDimensionPerPixel.z, mMeshMaxDepth );
            voxelizer.setResolution( actualWidth, actualHeight, actualDepth );

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
            voxelizedMesh.albedoVox = mTextureGpuManager->createTexture(
                "VctImage/" + mesh->getName() + "/Albedo", GpuPageOutStrategy::Discard,
                TextureFlags::ManualTexture, TextureTypes::Type3D );
            voxelizedMesh.normalVox = mTextureGpuManager->createTexture(
                "VctImage/" + mesh->getName() + "/Normal", GpuPageOutStrategy::Discard,
                TextureFlags::ManualTexture, TextureTypes::Type3D );
            if( bHasEmissive )
            {
                voxelizedMesh.emissiveVox = mTextureGpuManager->createTexture(
                    "VctImage/" + mesh->getName() + "/Emissive", GpuPageOutStrategy::Discard,
                    TextureFlags::ManualTexture, TextureTypes::Type3D );
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

            mMeshes[meshName] = voxelizedMesh;

            sceneManager->destroyItem( tmpItem );
        }
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::setCacheResolution( uint32 width, uint32 height, uint32 depth,
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
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::addItem( Item *item ) { mItems.push_back( item ); }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::removeItem( Item *item )
    {
        ItemArray::iterator itor = std::find( mItems.begin(), mItems.end(), item );
        if( itor == mItems.end() )
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "", "VctImageVoxelizer::removeItem" );

        efficientVectorRemove( mItems, itor );
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::removeAllItems( void ) { mItems.clear(); }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::createVoxelTextures( void )
    {
        if( mAlbedoVox && mAlbedoVox->getWidth() == mSceneWidth &&
            mAlbedoVox->getHeight() == mSceneHeight && mAlbedoVox->getDepth() == mSceneDepth )
        {
            mAccumValVox->scheduleTransitionTo( GpuResidency::Resident );
            return;
        }

        const bool hasTypedUavs = mRenderSystem->getCapabilities()->hasCapability( RSC_TYPED_UAV_LOADS );

        if( !mAlbedoVox )
        {
            uint32 texFlags = TextureFlags::Uav;
            if( !hasTypedUavs )
                texFlags |= TextureFlags::Reinterpretable;

            if( mNeedsAlbedoMipmaps )
                texFlags |= TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps;

            mAlbedoVox = mTextureGpuManager->createTexture(
                "VctImageVoxelizer" + StringConverter::toString( getId() ) + "/Albedo",
                GpuPageOutStrategy::Discard, texFlags, TextureTypes::Type3D );

            texFlags &= ~( uint32 )( TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps );

            mEmissiveVox = mTextureGpuManager->createTexture(
                "VctImageVoxelizer" + StringConverter::toString( getId() ) + "/Emissive",
                GpuPageOutStrategy::Discard, texFlags, TextureTypes::Type3D );
            mNormalVox = mTextureGpuManager->createTexture(
                "VctImageVoxelizer" + StringConverter::toString( getId() ) + "/Normal",
                GpuPageOutStrategy::Discard, texFlags, TextureTypes::Type3D );
            mAccumValVox = mTextureGpuManager->createTexture(
                "VctImageVoxelizer" + StringConverter::toString( getId() ) + "/AccumVal",
                GpuPageOutStrategy::Discard, TextureFlags::NotTexture | texFlags, TextureTypes::Type3D );
        }

        TextureGpu *textures[4] = { mAlbedoVox, mEmissiveVox, mNormalVox, mAccumValVox };
        for( size_t i = 0; i < sizeof( textures ) / sizeof( textures[0] ); ++i )
            textures[i]->scheduleTransitionTo( GpuResidency::OnStorage );

        mAlbedoVox->setPixelFormat( PFG_RGBA8_UNORM );
        mEmissiveVox->setPixelFormat( PFG_RGBA8_UNORM );
        mNormalVox->setPixelFormat( PFG_R10G10B10A2_UNORM );
        if( hasTypedUavs )
            mAccumValVox->setPixelFormat( PFG_R16_UINT );
        else
            mAccumValVox->setPixelFormat( PFG_R32_UINT );

        const uint8 numMipmaps =
            PixelFormatGpuUtils::getMaxMipmapCount( mSceneWidth, mSceneHeight, mSceneDepth );

        for( size_t i = 0; i < sizeof( textures ) / sizeof( textures[0] ); ++i )
        {
            if( textures[i] != mAccumValVox || hasTypedUavs )
                textures[i]->setResolution( mSceneWidth, mSceneHeight, mSceneDepth );
            else
                textures[i]->setResolution( mSceneWidth >> 1u, mSceneHeight, mSceneDepth );
            textures[i]->setNumMipmaps( mNeedsAlbedoMipmaps && i == 0u ? numMipmaps : 1u );
            textures[i]->scheduleTransitionTo( GpuResidency::Resident );
        }

        if( mDebugVoxelVisualizer )
        {
            setTextureToDebugVisualizer();
            mDebugVoxelVisualizer->setVisible( true );
        }
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::setTextureToDebugVisualizer( void )
    {
        TextureGpu *trackedTex = mAlbedoVox;
        switch( mDebugVisualizationMode )
        {
        default:
        case DebugVisualizationAlbedo:
            trackedTex = mAlbedoVox;
            break;
        case DebugVisualizationNormal:
            trackedTex = mNormalVox;
            break;
        case DebugVisualizationEmissive:
            trackedTex = mEmissiveVox;
            break;
        }
        mDebugVoxelVisualizer->setTrackingVoxel( mAlbedoVox, trackedTex,
                                                 mDebugVisualizationMode == DebugVisualizationEmissive );
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::destroyVoxelTextures( void )
    {
        if( mAlbedoVox )
        {
            mTextureGpuManager->destroyTexture( mAlbedoVox );
            mTextureGpuManager->destroyTexture( mEmissiveVox );
            mTextureGpuManager->destroyTexture( mNormalVox );
            mTextureGpuManager->destroyTexture( mAccumValVox );

            mAlbedoVox = 0;
            mEmissiveVox = 0;
            mNormalVox = 0;
            mAccumValVox = 0;

            if( mDebugVoxelVisualizer )
                mDebugVoxelVisualizer->setVisible( false );
        }
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::setRegionToVoxelize( bool autoRegion, const Aabb &regionToVoxelize,
                                                 const Aabb &maxRegion )
    {
        mAutoRegion = autoRegion;
        mRegionToVoxelize = regionToVoxelize;
        mMaxRegion = maxRegion;
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::autoCalculateRegion()
    {
        if( !mAutoRegion )
            return;

        mRegionToVoxelize = Aabb::BOX_NULL;

        ItemArray::const_iterator itor = mItems.begin();
        ItemArray::const_iterator end = mItems.end();

        while( itor != end )
        {
            Item *item = *itor;
            mRegionToVoxelize.merge( item->getWorldAabb() );
            ++itor;
        }

        Vector3 minAabb = mRegionToVoxelize.getMinimum();
        Vector3 maxAabb = mRegionToVoxelize.getMaximum();

        minAabb.makeCeil( mMaxRegion.getMinimum() );
        maxAabb.makeFloor( mMaxRegion.getMaximum() );

        mRegionToVoxelize.setExtents( minAabb, maxAabb );
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::dividideOctants( uint32 numOctantsX, uint32 numOctantsY, uint32 numOctantsZ )
    {
        mOctants.clear();
        mOctants.reserve( numOctantsX * numOctantsY * numOctantsZ );

        OGRE_ASSERT_LOW( mSceneWidth % numOctantsX == 0 );
        OGRE_ASSERT_LOW( mSceneHeight % numOctantsY == 0 );
        OGRE_ASSERT_LOW( mSceneDepth % numOctantsZ == 0 );

        Octant octant;
        octant.width = mSceneWidth / numOctantsX;
        octant.height = mSceneHeight / numOctantsY;
        octant.depth = mSceneDepth / numOctantsZ;

        const Vector3 voxelOrigin = mRegionToVoxelize.getMinimum();
        const Vector3 voxelCellSize =
            mRegionToVoxelize.getSize() / Vector3( numOctantsX, numOctantsY, numOctantsZ );

        for( uint32 x = 0u; x < numOctantsX; ++x )
        {
            octant.x = x * octant.width;
            for( uint32 y = 0u; y < numOctantsY; ++y )
            {
                octant.y = y * octant.height;
                for( uint32 z = 0u; z < numOctantsZ; ++z )
                {
                    octant.z = z * octant.depth;

                    Vector3 octantOrigin = Vector3( octant.x, octant.y, octant.z ) * voxelCellSize;
                    octantOrigin += voxelOrigin;
                    octant.region.setExtents( octantOrigin, octantOrigin + voxelCellSize );
                    mOctants.push_back( octant );
                }
            }
        }
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::clearVoxels( void )
    {
        OgreProfileGpuBegin( "VCT Voxelization Clear" );
        float fClearValue[4];
        uint32 uClearValue[4];
        memset( fClearValue, 0, sizeof( fClearValue ) );
        memset( uClearValue, 0, sizeof( uClearValue ) );

        mResourceTransitions.clear();
        mComputeTools->prepareForUavClear( mResourceTransitions, mAlbedoVox );
        mComputeTools->prepareForUavClear( mResourceTransitions, mEmissiveVox );
        mComputeTools->prepareForUavClear( mResourceTransitions, mNormalVox );
        mComputeTools->prepareForUavClear( mResourceTransitions, mAccumValVox );
        mRenderSystem->executeResourceTransition( mResourceTransitions );

        mComputeTools->clearUavFloat( mAlbedoVox, fClearValue );
        mComputeTools->clearUavFloat( mEmissiveVox, fClearValue );
        mComputeTools->clearUavFloat( mNormalVox, fClearValue );
        mComputeTools->clearUavUint( mAccumValVox, uClearValue );
        OgreProfileGpuEnd( "VCT Voxelization Clear" );
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::setSceneResolution( uint32 width, uint32 height, uint32 depth )
    {
        destroyVoxelTextures();
        mSceneWidth = width;
        mSceneHeight = height;
        mSceneDepth = depth;
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::build( SceneManager *sceneManager )
    {
        OgreProfile( "VctImageVoxelizer::build" );
        OgreProfileGpuBegin( "VCT build" );

        OGRE_ASSERT_LOW( !mOctants.empty() );

        mRenderSystem->endRenderPassDescriptor();

        if( mItems.empty() )
        {
            clearVoxels();
            return;
        }

        mRenderSystem->endRenderPassDescriptor();

        createVoxelTextures();

        const bool hasTypedUavs = mRenderSystem->getCapabilities()->hasCapability( RSC_TYPED_UAV_LOADS );

        HlmsCompute *hlmsCompute = mHlmsManager->getComputeHlms();
        clearVoxels();

        // const uint32 *threadsPerGroup = mComputeJobs[0]->getThreadsPerGroup();

        const Vector3 voxelOrigin = getVoxelOrigin();

        ShaderParams::Param paramInstanceRange;
        ShaderParams::Param paramVoxelOrigin;
        ShaderParams::Param paramVoxelCellSize;

        paramInstanceRange.name = "instanceStart_instanceEnd";
        paramVoxelOrigin.name = "voxelOrigin";
        paramVoxelCellSize.name = "voxelCellSize";

        paramVoxelCellSize.setManualValue( getVoxelCellSize() );

        uint32 instanceStart = 0;

        OgreProfileGpuBegin( "VCT Voxelization Jobs" );

        FastArray<Octant>::const_iterator itor = mOctants.begin();
        FastArray<Octant>::const_iterator endt = mOctants.end();

        while( itor != endt )
        {
            const Octant &octant = *itor;

            ++itor;
        }

        OgreProfileGpuEnd( "VCT Voxelization Jobs" );

        // This texture is no longer needed, it's not used for the injection phase. Save memory.
        mAccumValVox->scheduleTransitionTo( GpuResidency::OnStorage );

        if( mNeedsAlbedoMipmaps )
            mAlbedoVox->_autogenerateMipmaps();

        OgreProfileGpuEnd( "VCT build" );
    }
    //-------------------------------------------------------------------------
    void VctImageVoxelizer::setDebugVisualization( VctImageVoxelizer::DebugVisualizationMode mode,
                                                   SceneManager *sceneManager )
    {
        if( mDebugVoxelVisualizer )
        {
            SceneNode *sceneNode = mDebugVoxelVisualizer->getParentSceneNode();
            sceneNode->getParentSceneNode()->removeAndDestroyChild( sceneNode );
            OGRE_DELETE mDebugVoxelVisualizer;
            mDebugVoxelVisualizer = 0;
        }

        mDebugVisualizationMode = mode;

        if( mode != DebugVisualizationNone )
        {
            SceneNode *rootNode = sceneManager->getRootSceneNode( SCENE_STATIC );
            SceneNode *visNode = rootNode->createChildSceneNode( SCENE_STATIC );

            mDebugVoxelVisualizer = OGRE_NEW VoxelVisualizer(
                Ogre::Id::generateNewId<Ogre::MovableObject>(),
                &sceneManager->_getEntityMemoryManager( SCENE_STATIC ), sceneManager, 0u );

            setTextureToDebugVisualizer();

            visNode->setPosition( getVoxelOrigin() );
            visNode->setScale( getVoxelCellSize() );
            visNode->attachObject( mDebugVoxelVisualizer );
        }
    }
    //-------------------------------------------------------------------------
    VctImageVoxelizer::DebugVisualizationMode VctImageVoxelizer::getDebugVisualizationMode( void ) const
    {
        return mDebugVisualizationMode;
    }
    //-------------------------------------------------------------------------
    Vector3 VctImageVoxelizer::getVoxelOrigin( void ) const { return mRegionToVoxelize.getMinimum(); }
    //-------------------------------------------------------------------------
    Vector3 VctImageVoxelizer::getVoxelCellSize( void ) const
    {
        return mRegionToVoxelize.getSize() / getVoxelResolution();
    }
    //-------------------------------------------------------------------------
    Vector3 VctImageVoxelizer::getVoxelSize( void ) const { return mRegionToVoxelize.getSize(); }
    //-------------------------------------------------------------------------
    Vector3 VctImageVoxelizer::getVoxelResolution( void ) const
    {
        return Vector3( mSceneWidth, mSceneHeight, mSceneDepth );
    }
    //-------------------------------------------------------------------------
    TextureGpuManager *VctImageVoxelizer::getTextureGpuManager( void ) { return mTextureGpuManager; }
    //-------------------------------------------------------------------------
    RenderSystem *VctImageVoxelizer::getRenderSystem( void ) { return mRenderSystem; }
    //-------------------------------------------------------------------------
    HlmsManager *VctImageVoxelizer::getHlmsManager( void ) { return mHlmsManager; }
}  // namespace Ogre
