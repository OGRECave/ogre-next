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

#include "Compositor/OgreCompositorManager2.h"
#include "Math/Array/OgreBooleanMask.h"
#include "OgreItem.h"
#include "OgreSceneManager.h"
#include "Vct/OgreVctImageVoxelizer.h"
#include "Vct/OgreVctLighting.h"
#include "Vct/OgreVoxelizedMeshCache.h"

namespace Ogre
{
#if OGRE_COMPILER == OGRE_COMPILER_MSVC && OGRE_COMP_VER < 1800
    inline float roundf( float x ) { return x >= 0.0f ? floorf( x + 0.5f ) : ceilf( x - 0.5f ); }
#endif

    VctCascadeSetting::VctCascadeSetting() :
        bCorrectAreaLightShadows( false ),
        bAutoMultiplier( true ),
        thinWallCounter( 1.0f ),
        rayMarchStepScale( 1.0f ),
        lightMask( 0xffffffff ),
        areaHalfSize( Vector3::ZERO ),
        cameraStepSize( Vector3::UNIT_SCALE ),
        voxelizer( 0 ),
        lastCameraPosition( Vector3::ZERO )
    {
        for( int i = 0; i < 3; ++i )
        {
            resolution[i] = 16u;
            octantSubdivision[i] = 1u;
        }
    }
    //-------------------------------------------------------------------------
    Vector3 VctCascadeSetting::getVoxelCellSize() const
    {
        return 2.0f * this->areaHalfSize /
               Vector3( (Real)resolution[0], (Real)resolution[1], (Real)resolution[2] );
    }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    VctCascadedVoxelizer::VctCascadedVoxelizer() :
        mCameraPosition( Vector3::ZERO ),
        mMeshCache( 0 ),
        mSceneManager( 0 ),
        mCompositorManager( 0 ),
        mNumBounces( 2u ),
        mAnisotropic( true ),
        mFirstBuild( true ),
        mConsistentCascadeSteps( false )
    {
    }
    //-------------------------------------------------------------------------
    VctCascadedVoxelizer::~VctCascadedVoxelizer()
    {
        setAutoUpdate( 0, 0 );

        const size_t numCascades = mCascadeSettings.size();

        for( size_t i = 0u; i < numCascades; ++i )
        {
            if( mCascadeSettings[i].voxelizer )
            {
                mCascadeSettings[i].voxelizer->restoreSwappedVoxelTextures();
                if( mCascades[i] )
                    mCascades[i]->resetTexturesFromBuildRelative();

                delete mCascades[i];
                delete mCascadeSettings[i].voxelizer;
            }
        }

        delete mMeshCache;
    }
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
    void VctCascadedVoxelizer::popCascade()
    {
        if( !mCascades.empty() )
        {
            OGRE_ASSERT_LOW( !mCascadeSettings.back().voxelizer );
            OGRE_ASSERT_LOW( !mCascades.back() );
            mCascadeSettings.pop_back();
            mCascades.pop_back();
        }
    }
    //-------------------------------------------------------------------------
    void VctCascadedVoxelizer::autoCalculateStepSizes( const Vector3 stepSize )
    {
        if( mCascadeSettings.empty() )
            return;

        VctCascadeSetting &lastCascade = mCascadeSettings.back();
        lastCascade.cameraStepSize = stepSize;

        const Vector3 lastVoxelCellSize = lastCascade.getVoxelCellSize();

        const size_t numCascades = mCascadeSettings.size() - 1u;
        for( size_t i = numCascades; i--; )
        {
            const Vector3 voxelCellSize = mCascadeSettings[i].getVoxelCellSize();
            const Vector3 factor = lastVoxelCellSize / voxelCellSize;

            mCascadeSettings[i].cameraStepSize = stepSize * factor;

            // Must be integer and >= 1
            mCascadeSettings[i].cameraStepSize.x = std::ceil( mCascadeSettings[i].cameraStepSize.x );
            mCascadeSettings[i].cameraStepSize.y = std::ceil( mCascadeSettings[i].cameraStepSize.y );
            mCascadeSettings[i].cameraStepSize.z = std::ceil( mCascadeSettings[i].cameraStepSize.z );
            mCascadeSettings[i].cameraStepSize.makeCeil( Vector3::UNIT_SCALE );

            Ogre::Vector3 res( (Real)mCascadeSettings[i].resolution[0],
                               (Real)mCascadeSettings[i].resolution[1],
                               (Real)mCascadeSettings[i].resolution[2] );
            // Step size should be <= resolution / 2 (it must be <= resolution;
            // but we use half to avoid potential glitches / distortion)
            mCascadeSettings[i].cameraStepSize.makeFloor( res * 0.5f );
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
        retVal.x = Real( position.x ) * voxelCellSize.x;
        retVal.y = Real( position.y ) * voxelCellSize.y;
        retVal.z = Real( position.z ) * voxelCellSize.z;

        return retVal;
    }
    //-------------------------------------------------------------------------
    void VctCascadedVoxelizer::setConsistentCascadeSteps( bool bConsistentCascadeSteps )
    {
        mConsistentCascadeSteps = bConsistentCascadeSteps;
        if( mConsistentCascadeSteps )
        {
            mFirstBuild = true;

            const size_t numCascades = mCascadeSettings.size();
            for( size_t i = numCascades; i--; )
            {
                if( mCascadeSettings[i].voxelizer )
                    mCascadeSettings[i].voxelizer->forceFullBuild();
            }
        }
    }
    //-------------------------------------------------------------------------
    void VctCascadedVoxelizer::init( RenderSystem *renderSystem, HlmsManager *hlmsManager,
                                     uint32 numBounces, bool bAnisotropic )
    {
        OGRE_ASSERT_LOW( !mMeshCache && "VctCascadedVoxelizer::init already called!" );

        mNumBounces = numBounces;
        mAnisotropic = bAnisotropic;

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
    void VctCascadedVoxelizer::setNewSettings( uint32 numBounces, bool bAnisotropic )
    {
        if( mNumBounces == numBounces && mAnisotropic == bAnisotropic )
            return;

        const size_t numCascades = mCascades.size();

        // Iterate in reverse because multipe bounces needs the information of the higher cascades
        for( size_t i = numCascades; i--; )
        {
            VctLighting *vctLighting = mCascades[i];
            if( vctLighting )
            {
                // setAllowMultipleBounces must be called first or else mAnisoGeneratorStep0
                // may wreak havoc
                vctLighting->setAllowMultipleBounces( numBounces > 0u );
                vctLighting->setAnisotropic( bAnisotropic );
            }
        }

        mNumBounces = numBounces;
        mAnisotropic = bAnisotropic;
        mFirstBuild = true;
    }
    //-------------------------------------------------------------------------
    void VctCascadedVoxelizer::setCameraPosition( const Vector3 &cameraPosition )
    {
        mCameraPosition = cameraPosition;
    }
    //-------------------------------------------------------------------------
    void VctCascadedVoxelizer::setAutoUpdate( CompositorManager2 *compositorManager,
                                              SceneManager *sceneManager )
    {
        if( compositorManager && !mCompositorManager )
        {
            mSceneManager = sceneManager;
            mCompositorManager = compositorManager;
            compositorManager->addListener( this );
        }
        else if( !compositorManager && mCompositorManager )
        {
            mCompositorManager->removeListener( this );
            mSceneManager = 0;
            mCompositorManager = 0;
        }
    }
    //-------------------------------------------------------------------------
    void VctCascadedVoxelizer::allWorkspacesBeforeBeginUpdate() { update( mSceneManager ); }
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

        mFirstBuild = true;
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

                objData.advancePack();
            }
        }

        mFirstBuild = true;
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

        mFirstBuild = true;
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

        mFirstBuild = true;
    }
    //-------------------------------------------------------------------------
    bool VctCascadedVoxelizer::needsRebuild( const VctCascadeSetting &cascade ) const
    {
        if( mConsistentCascadeSteps )
        {
            const Vector3 voxelCellSize = cascade.getVoxelCellSize();
            const Grid3D newPosSized =
                quantizePosition( mCameraPosition, voxelCellSize * cascade.cameraStepSize );
            const Grid3D oldPosSized =
                quantizePosition( cascade.lastCameraPosition, voxelCellSize * cascade.cameraStepSize );

            return newPosSized.x != oldPosSized.x ||  //
                   newPosSized.y != oldPosSized.y ||  //
                   newPosSized.z != oldPosSized.z;
        }
        else
        {
            const Vector3 voxelCellSize = cascade.getVoxelCellSize();
            const Grid3D newPos = quantizePosition( mCameraPosition, voxelCellSize );
            const Grid3D oldPos = quantizePosition( cascade.lastCameraPosition, voxelCellSize );

            return Real( abs( newPos.x - oldPos.x ) ) >= cascade.cameraStepSize.x ||  //
                   Real( abs( newPos.y - oldPos.y ) ) >= cascade.cameraStepSize.y ||  //
                   Real( abs( newPos.z - oldPos.z ) ) >= cascade.cameraStepSize.z;
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

            const bool bNeedsRebuild = needsRebuild( cascade );

            if( bNeedsRebuild || bFirstBuild )
            {
                // Dirty. Must be updated
                const Vector3 voxelCellSize = cascade.getVoxelCellSize();
                const Grid3D newPos = quantizePosition( mCameraPosition, voxelCellSize );
                const Grid3D oldPos = quantizePosition( cascade.lastCameraPosition, voxelCellSize );

                cascade.voxelizer->setRegionToVoxelize(
                    false, Aabb( quantToVec3( newPos, voxelCellSize ), cascade.areaHalfSize ) );
                cascade.voxelizer->buildRelative( sceneManager, newPos.x - oldPos.x, newPos.y - oldPos.y,
                                                  newPos.z - oldPos.z, cascade.octantSubdivision[0],
                                                  cascade.octantSubdivision[1],
                                                  cascade.octantSubdivision[2] );

                if( !mCascades[i] )
                {
                    mCascades[i] = new VctLighting( Ogre::Id::generateNewId<Ogre::VctLighting>(),
                                                    cascade.voxelizer, mAnisotropic );

                    const size_t numExtraCascades = numCascades - i - 1u;
                    if( numExtraCascades > 0u )
                    {
                        mCascades[i]->reserveExtraCascades( numExtraCascades );

                        for( size_t j = i + 1u; j < numCascades; ++j )
                            mCascades[i]->addCascade( mCascades[j] );
                    }

                    mCascades[i]->setAllowMultipleBounces( mNumBounces > 0u );
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
                // We do it in 2 ways:
                //
                //      1. Increasing num bounces to i + 1.
                //         More bounces, means brighter cascade (done here)
                //      2. De-amplify cascade i + 1 when we have enough lighting information
                //         accumulated (via 1.0 - result.alpha). Done in shader
                const uint32 numBounces =
                    mNumBounces == 0u
                        ? 0u
                        : static_cast<uint32>(
                              roundf( std::sqrt( ( Real( mNumBounces + 1u ) * factor ) - 1.0f ) ) );

                mCascades[i]->update( sceneManager, numBounces, cascade.thinWallCounter,
                                      cascade.bAutoMultiplier, cascade.rayMarchStepScale,
                                      cascade.lightMask );

                cascade.lastCameraPosition = mCameraPosition;
            }
        }

        mFirstBuild = false;
    }
}  // namespace Ogre
