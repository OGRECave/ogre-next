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
        voxelizer( 0 )
    {
        resolution[0] = 16u;
        resolution[1] = 16u;
        resolution[2] = 16u;
        octantSubdivision[0] = 1u;
        octantSubdivision[1] = 1u;
        octantSubdivision[2] = 1u;
    }
    //-------------------------------------------------------------------------
    VctCascadedVoxelizer::VctCascadedVoxelizer() :
        mCameraPosition( Vector3::ZERO ),
        mLastCameraPosition( Vector3::ZERO ),
        mFirstBuild( true )
    {
    }
    //-------------------------------------------------------------------------
    VctCascadedVoxelizer::~VctCascadedVoxelizer() {}
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
        const size_t numCascades = mCascadeSettings.size();
        for( size_t i = 0u; i < numCascades; ++i )
        {
            mCascadeSettings[i].voxelizer = new VctImageVoxelizer(
                Ogre::Id::generateNewId<VctImageVoxelizer>(), renderSystem, hlmsManager, true );

            mCascadeSettings[i].voxelizer->setSceneResolution( mCascadeSettings[i].resolution[0],
                                                               mCascadeSettings[i].resolution[1],
                                                               mCascadeSettings[i].resolution[2] );

            const Vector3 voxelCellSize = 2.0f * mCascadeSettings[i].areaHalfSize /
                                          mCascadeSettings[i].voxelizer->getVoxelResolution();
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
        for( size_t i = 0u; i < numCascades; ++i )
        {
            VctCascadeSetting &cascade = mCascadeSettings[i];

            // Do not use cascade.voxelizer->getVoxelCellSize() because it is bound to
            // floating point precision errors. This way is always consistent
            const Vector3 voxelCellSize =
                2.0f * mCascadeSettings[i].areaHalfSize / cascade.voxelizer->getVoxelResolution();
            const Grid3D newPos = quantizePosition( mCameraPosition, voxelCellSize );
            const Grid3D oldPos = quantizePosition( mLastCameraPosition, voxelCellSize );

            if( newPos.x != oldPos.x || newPos.y != oldPos.y || newPos.z != oldPos.z || bFirstBuild )
            {
                // Dirty. Must be updated
                cascade.voxelizer->setRegionToVoxelize(
                    false, Aabb( quantToVec3( newPos, voxelCellSize ), cascade.areaHalfSize ) );
                cascade.voxelizer->buildRelative( sceneManager, newPos.x - oldPos.x, newPos.y - oldPos.y,
                                                  newPos.z - oldPos.z, cascade.octantSubdivision[0],
                                                  cascade.octantSubdivision[1],
                                                  cascade.octantSubdivision[2] );

                mCascades[i]->update( sceneManager, cascade.numBounces, cascade.thinWallCounter,
                                      cascade.bAutoMultiplier, cascade.rayMarchStepScale,
                                      cascade.lightMask );
            }
        }

        mLastCameraPosition = mCameraPosition;
        mFirstBuild = false;
    }
}  // namespace Ogre
