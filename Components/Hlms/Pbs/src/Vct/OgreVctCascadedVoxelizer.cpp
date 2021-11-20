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

#include "Vct/OgreVctCascadedVoxelizer.h"

#include "Math/Array/OgreBooleanMask.h"
#include "OgreItem.h"
#include "OgreSceneManager.h"
#include "Vct/OgreVctImageVoxelizer.h"
#include "Vct/OgreVctLighting.h"
#include "Vct/OgreVoxelizedMeshCache.h"

namespace Ogre
{
    VctCascadeSetting::VctCascadeSetting() :
        bCorrectAreaLightShadows( false ),
        bAutoMultiplier( true ),
        numBounces( 2u ),
        thinWallCounter( 1.0f ),
        rayMarchStepScale( 1.0f ),
        lightMask( 0xffffffff ),
        areaHalfSize( Vector3::ZERO ),
        voxelizer( 0 ),
        lastCameraPosition( Vector3::ZERO )
    {
        for( int i = 0; i < 3; ++i )
        {
            resolution[i] = 16u;
            octantSubdivision[i] = 1u;
            cameraStepSize[i] = 4;
        }
    }
    //-------------------------------------------------------------------------
    Vector3 VctCascadeSetting::getVoxelCellSize( void ) const
    {
        return 2.0f * this->areaHalfSize / Vector3( resolution[0], resolution[1], resolution[2] );
    }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    VctCascadedVoxelizer::VctCascadedVoxelizer() :
        mCameraPosition( Vector3::ZERO ),
        mMeshCache( 0 ),
        mFirstBuild( true )
    {
    }
    //-------------------------------------------------------------------------
    VctCascadedVoxelizer::~VctCascadedVoxelizer() { delete mMeshCache; }
    //-------------------------------------------------------------------------
    void VctCascadedVoxelizer::reserveNumCascades( size_t numCascades )
    {
        mCascadeSettings.reserve( numCascades );
        mCascades.reserve( numCascades );
    }
    //-------------------------------------------------------------------------
    void VctCascadedVoxelizer::addCascade( const VctCascadeSetting &cascadeSetting )
    {
        OGRE_ASSERT_LOW( !cascadeSetting.voxelizer );
        mCascadeSettings.push_back( cascadeSetting );
        mCascades.push_back( (VctLighting *)0 );
    }
    //-------------------------------------------------------------------------
    void VctCascadedVoxelizer::autoCalculateStepSizes( const int32 stepSize )
    {
        autoCalculateStepSizes( stepSize, stepSize, stepSize );
    }
    //-------------------------------------------------------------------------
    void VctCascadedVoxelizer::autoCalculateStepSizes( const int32 stepSizeX, const int32 stepSizeY,
                                                       const int32 stepSizeZ )
    {
        if( mCascadeSettings.empty() )
            return;

        VctCascadeSetting &lastCascade = mCascadeSettings.back();
        lastCascade.cameraStepSize[0] = stepSizeX;
        lastCascade.cameraStepSize[1] = stepSizeY;
        lastCascade.cameraStepSize[2] = stepSizeZ;

        const Vector3 lastVoxelCellSize = lastCascade.getVoxelCellSize();

        const size_t numCascades = mCascadeSettings.size() - 1u;
        for( size_t i = numCascades; i--; )
        {
            const Vector3 voxelCellSize = mCascadeSettings[i].getVoxelCellSize();
            const Vector3 factor = lastVoxelCellSize / voxelCellSize;

            mCascadeSettings[i].cameraStepSize[0] =
                static_cast<int32>( std::ceil( stepSizeX * factor.x ) );
            mCascadeSettings[i].cameraStepSize[1] =
                static_cast<int32>( std::ceil( stepSizeY * factor.y ) );
            mCascadeSettings[i].cameraStepSize[2] =
                static_cast<int32>( std::ceil( stepSizeZ * factor.z ) );

            for( int j = 0; j < 3; ++j )
            {
                // Step size should be <= resolution / 2
                mCascadeSettings[i].cameraStepSize[j] =
                    std::min( mCascadeSettings[i].cameraStepSize[0],
                              static_cast<int32>( mCascadeSettings[i].resolution[j] >> 1u ) );
            }
        }
    }
    //-------------------------------------------------------------------------
    struct Grid3D
    {
        int32 x;
        int32 y;
        int32 z;
    };
    inline Grid3D quantizePosition( const Vector3 position, const Vector3 voxelCellSize )
    {
        Vector3 cellPos = position / voxelCellSize;

        Grid3D retVal;
        retVal.x = static_cast<int32>( std::floor( cellPos.x ) );
        retVal.y = static_cast<int32>( std::floor( cellPos.y ) );
        retVal.z = static_cast<int32>( std::floor( cellPos.z ) );

        return retVal;
    }
    inline Vector3 quantToVec3( const Grid3D position, const Vector3 voxelCellSize )
    {
        Vector3 retVal;
        retVal.x = position.x * voxelCellSize.x;
        retVal.y = position.y * voxelCellSize.y;
        retVal.z = position.z * voxelCellSize.z;

        return retVal;
    }
    //-------------------------------------------------------------------------
    void VctCascadedVoxelizer::init( RenderSystem *renderSystem, HlmsManager *hlmsManager )
    {
        OGRE_ASSERT_LOW( !mMeshCache && "VctCascadedVoxelizer::init already called!" );

        mMeshCache = new VoxelizedMeshCache( Ogre::Id::generateNewId<VoxelizedMeshCache>(),
                                             renderSystem->getTextureGpuManager() );

        const size_t numCascades = mCascadeSettings.size();
        for( size_t i = 0u; i < numCascades; ++i )
        {
            mCascadeSettings[i].voxelizer =
                new VctImageVoxelizer( Ogre::Id::generateNewId<VctImageVoxelizer>(), renderSystem,
                                       hlmsManager, mMeshCache, true );

            mCascadeSettings[i].voxelizer->setSceneResolution( mCascadeSettings[i].resolution[0],
                                                               mCascadeSettings[i].resolution[1],
                                                               mCascadeSettings[i].resolution[2] );

            const Vector3 voxelCellSize = mCascadeSettings[i].getVoxelCellSize();
            const Grid3D quantCamPos = quantizePosition( mCameraPosition, voxelCellSize );

            mCascadeSettings[i].voxelizer->setRegionToVoxelize(
                false,
                Aabb( quantToVec3( quantCamPos, voxelCellSize ), mCascadeSettings[i].areaHalfSize ) );
        }

        mFirstBuild = true;
    }
    //-------------------------------------------------------------------------
    void VctCascadedVoxelizer::setCameraPosition( const Vector3 &cameraPosition )
    {
        mCameraPosition = cameraPosition;
    }
    //-------------------------------------------------------------------------
    void VctCascadedVoxelizer::removeAllItems()
    {
        FastArray<VctCascadeSetting>::const_iterator itor = mCascadeSettings.begin();
        FastArray<VctCascadeSetting>::const_iterator endt = mCascadeSettings.end();

        while( itor != endt )
        {
            itor->voxelizer->removeAllItems();
            ++itor;
        }
    }
    //-------------------------------------------------------------------------
    void VctCascadedVoxelizer::addAllItems( SceneManager *sceneManager,
                                            const uint32 sceneVisibilityFlags, const bool bStatic )
    {
        // TODO: We should be able to keep a list of Items ourselves, and in
        // VctCascadedVoxelizer::update when a voxelizer is moved, we detect
        // which items go into that voxelizer. What's faster? who knows

        Ogre::ObjectMemoryManager &objMemoryManager =
            sceneManager->_getEntityMemoryManager( bStatic ? SCENE_STATIC : SCENE_DYNAMIC );

        const ArrayInt sceneFlags = Mathlib::SetAll( sceneVisibilityFlags );

        const size_t numRenderQueues = objMemoryManager.getNumRenderQueues();

        for( size_t i = 0u; i < numRenderQueues; ++i )
        {
            Ogre::ObjectData objData;
            const size_t totalObjs = objMemoryManager.getFirstObjectData( objData, i );

            for( size_t j = 0; j < totalObjs; j += ARRAY_PACKED_REALS )
            {
                ArrayInt *RESTRICT_ALIAS visibilityFlags =
                    reinterpret_cast<ArrayInt * RESTRICT_ALIAS>( objData.mVisibilityFlags );

                ArrayMaskI isVisible = Mathlib::And(
                    Mathlib::TestFlags4( *visibilityFlags,
                                         Mathlib::SetAll( VisibilityFlags::LAYER_VISIBILITY ) ),
                    Mathlib::And( sceneFlags, *visibilityFlags ) );

                const uint32 scalarMask = BooleanMask4::getScalarMask( isVisible );

                for( size_t k = 0; k < ARRAY_PACKED_REALS; ++k )
                {
                    if( IS_BIT_SET( k, scalarMask ) )
                    {
                        // UGH! We're relying on dynamic_cast. TODO: Refactor it.
                        Item *item = dynamic_cast<Item *>( objData.mOwner[k] );
                        if( item )
                        {
                            FastArray<VctCascadeSetting>::const_iterator itor = mCascadeSettings.begin();
                            FastArray<VctCascadeSetting>::const_iterator endt = mCascadeSettings.end();

                            while( itor != endt )
                            {
                                itor->voxelizer->addItem( item );
                                ++itor;
                            }
                        }
                    }
                }
            }
        }
    }
    //-------------------------------------------------------------------------
    void VctCascadedVoxelizer::addItem( Item *item )
    {
        FastArray<VctCascadeSetting>::const_iterator itor = mCascadeSettings.begin();
        FastArray<VctCascadeSetting>::const_iterator endt = mCascadeSettings.end();

        while( itor != endt )
        {
            itor->voxelizer->addItem( item );
            ++itor;
        }
    }
    //-------------------------------------------------------------------------
    void VctCascadedVoxelizer::removeItem( Item *item )
    {
        FastArray<VctCascadeSetting>::const_iterator itor = mCascadeSettings.begin();
        FastArray<VctCascadeSetting>::const_iterator endt = mCascadeSettings.end();

        while( itor != endt )
        {
            itor->voxelizer->removeItem( item );
            ++itor;
        }
    }
    //-------------------------------------------------------------------------
    void VctCascadedVoxelizer::update( SceneManager *sceneManager )
    {
        const bool bFirstBuild = mFirstBuild;

        const size_t numCascades = mCascadeSettings.size();

        const Vector3 voxelCellSizeC0 = mCascadeSettings[0].getVoxelCellSize();

        // Iterate in reverse because multipe bounces needs the information of the higher cascades
        for( size_t i = numCascades; i--; )
        {
            VctCascadeSetting &cascade = mCascadeSettings[i];

            const Vector3 voxelCellSize = cascade.getVoxelCellSize();
            const Grid3D newPos = quantizePosition( mCameraPosition, voxelCellSize );
            const Grid3D oldPos = quantizePosition( cascade.lastCameraPosition, voxelCellSize );

            if( abs( newPos.x - oldPos.x ) >= cascade.cameraStepSize[0] ||  //
                abs( newPos.y - oldPos.y ) >= cascade.cameraStepSize[1] ||  //
                abs( newPos.z - oldPos.z ) >= cascade.cameraStepSize[2] ||  //
                bFirstBuild )
            {
                // Dirty. Must be updated
                cascade.voxelizer->setRegionToVoxelize(
                    false, Aabb( quantToVec3( newPos, voxelCellSize ), cascade.areaHalfSize ) );
                cascade.voxelizer->buildRelative( sceneManager, newPos.x - oldPos.x, newPos.y - oldPos.y,
                                                  newPos.z - oldPos.z, cascade.octantSubdivision[0],
                                                  cascade.octantSubdivision[1],
                                                  cascade.octantSubdivision[2] );

                if( !mCascades[i] )
                {
                    mCascades[i] = new VctLighting( Ogre::Id::generateNewId<Ogre::VctLighting>(),
                                                    cascade.voxelizer, true );

                    const size_t numExtraCascades = numCascades - i - 1u;
                    if( numExtraCascades > 0u )
                    {
                        mCascades[i]->reserveExtraCascades( numExtraCascades );

                        for( size_t j = i + 1u; j < numCascades; ++j )
                            mCascades[i]->addCascade( mCascades[j] );
                    }

                    mCascades[i]->setAllowMultipleBounces( cascade.numBounces > 0u );
                }
                else
                {
                    mCascades[i]->resetTexturesFromBuildRelative();
                }

                const Real volumeCi = voxelCellSize.x * voxelCellSize.y * voxelCellSize.z;
                const Real volumeC0 = voxelCellSizeC0.x * voxelCellSizeC0.y * voxelCellSizeC0.z;

                const Real factor = std::pow( volumeCi / volumeC0, Real( 1.0f / 3.0f ) );

                // OK so we have two issues that result in cascades getting darker:
                //
                //  - As cell volume increases, we get darker results; this makes
                //    makes higher cascades to get darker (i.e. i is brighter than i+1).
                //  - Cascade[i] may have more lighting information than Cascade[i+1]
                //    because of its finer resolution. The also makes higher cascades
                //    to get darker
                //
                // We need to stabilize brightness of all cascades otherwise there will be a
                // stark contrast when cascade i ends and i+1 starts.
                //
                // We do it in 3 ways:
                //
                //      1. Increasing num bounces to i + 1.
                //         More bounces, means brighter cascade (done here)
                //      2. Amplify the cascade i + 1's multiplier (done here)
                //      3. De-amplify cascade i + 1 when we have enough lighting information
                //         accumulated (via 1.0 - result.alpha). Done in shader
                //
                // We need all 3 because 1 & 2 will also amplify cascade i due to cascade
                // i using lighting information from i + 1 when bouncing.
                const uint32 numBounces =
                    cascade.numBounces == 0u ? 0u
                                             : static_cast<uint32>( std::round( std::sqrt(
                                                   ( ( cascade.numBounces + 1u ) * factor ) - 1.0f ) ) );

                mCascades[i]->mMultiplier = factor;

                mCascades[i]->update( sceneManager, numBounces, cascade.thinWallCounter,
                                      cascade.bAutoMultiplier, cascade.rayMarchStepScale,
                                      cascade.lightMask );

                cascade.lastCameraPosition = mCameraPosition;
            }
        }

        mFirstBuild = false;
    }
}  // namespace Ogre
