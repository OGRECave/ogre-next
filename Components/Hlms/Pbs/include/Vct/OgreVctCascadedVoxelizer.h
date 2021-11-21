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
#ifndef _OgreVctCascadedVoxelizer_H_
#define _OgreVctCascadedVoxelizer_H_

#include "Vct/OgreVctVoxelizerSourceBase.h"

#include "Compositor/OgreCompositorWorkspaceListener.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class VctImageVoxelizer;
    class VoxelizedMeshCache;

    /// Contains VctLighting settings about each cascade that cannot be
    /// set by setting them directly via VctCascadedVoxelizer::getVctLighting
    struct _OgreHlmsPbsExport VctCascadeSetting
    {
        /// See VctImageVoxelizer::VctImageVoxelizer
        ///
        /// Recommended setting is true for the first cascade, false for the rest
        /// Or just false if you don't plan on using area lights (saves memory and
        /// performance)
        bool bCorrectAreaLightShadows;

        bool bAutoMultiplier;     /// @see VctLighting::update
        float thinWallCounter;    /// @see VctLighting::update
        float rayMarchStepScale;  /// @see VctLighting::update
        uint32 lightMask;         /// @see VctLighting::update

        /// The resolution to set to voxelizer
        uint32 resolution[3];
        /// The area around the camera to voxelize.
        /// The camera is at the center of this AABB
        Vector3 areaHalfSize;
        /// See VctImageVoxelizer::dividideOctants
        /// These are the values passed to it when we have to
        /// perform a full rebuild
        uint32 octantSubdivision[3];

        /// How much we let the camera move before updating the cascade
        /// Value is in range [1; inf)
        /// Camera is evaluated in quantized steps. i.e.
        ///     stepSize = cameraStepSize * 2.0 * areaHalfSize / resolution
        ///     stepSize = cameraStepSize * getVoxelCellSize()
        ///
        /// If cameraStepSize = 1, after the camera moves stepSize, we will move the cascades
        /// If cameraStepSize = 2, after the camera moves 2 * stepSize, we will move the cascades
        ///
        ///	Small step sizes may cause too much brightness jumping as VCT may not be stable
        /// Very big step sizes may cause periodic performance spikes or sudden changes
        /// in brightness
        Vector3 cameraStepSize;

        /// Valid ptr after VctCascadedVoxelizer::init
        ///
        /// You can call:
        ///     - VctImageVoxelizer::dividideOctants
        ///     - VctImageVoxelizer::setCacheResolution
        VctImageVoxelizer *voxelizer;

        Vector3 lastCameraPosition;

        VctCascadeSetting();

        void setResolution( uint32 res )
        {
            for( int i = 0; i < 3; ++i )
                this->resolution[i] = res;
        }

        void setOctantSubdivision( uint32 subdiv )
        {
            for( int i = 0; i < 3; ++i )
                this->octantSubdivision[i] = subdiv;
        }

        // Same as doing voxelizer->getVoxelCellSize() but is much better on floating point
        // precision errors. This method is always consistent for all cascades
        Vector3 getVoxelCellSize( void ) const;
    };

    /**
    @class VctCascadedVoxelizer
        This class manages multiple cascades of Voxels and each cascade can be
        composed of VctImageVoxelizer
    */
    class _OgreHlmsPbsExport VctCascadedVoxelizer : public CompositorWorkspaceListener
    {
    protected:
        FastArray<VctCascadeSetting> mCascadeSettings;
        FastArray<VctLighting *> mCascades;

        Vector3 mCameraPosition;

        VoxelizedMeshCache *mMeshCache;

        SceneManager *mSceneManager;
        CompositorManager2 *mCompositorManager;

        uint32 mNumBounces;  /// @see VctLighting::update

        bool mFirstBuild;

        bool isInitialized( void ) const;

    public:
        VctCascadedVoxelizer();
        virtual ~VctCascadedVoxelizer();

        /// Tells beforehand how many cascades there will be.
        /// It's not necessary to call this, but recommended
        void reserveNumCascades( size_t numCascades );

        /// Adds a new cascade. VctCascadeSetting::voxelizer must be nullptr
        /// Cannot be called after VctCascadedVoxelizer::init
        void addCascade( const VctCascadeSetting &cascadeSetting );

        /// Alters each cascade's step size. The last cascade is set to stepSize.
        ///
        /// The rest of the cascades are set to step sizes that are >= stepSize
        /// automatically
        void autoCalculateStepSizes( const Vector3 stepSize );

        size_t getNumCascades( void ) const { return mCascadeSettings.size(); }
        VctCascadeSetting &getCascade( size_t idx ) { return mCascadeSettings[idx]; }

        /** Call this function after adding all cascades.
            You can no longer add cascades after this
        @param numBounces
            Number of bounces for cascade 0. The rest of the cascades are autocalculated
            to maintain even brightness levels.

            Range is [0; inf) but a value of 0 is strongly discouraged if you have more than 1 cascade
            as you can end up with very uneven brightness levels between cascades

            See VctLighting::update
        */
        void init( RenderSystem *renderSystem, HlmsManager *hlmsManager, uint32 numBounces );

        /// Removes all items from all cascades
        void removeAllItems();

        /** Adds all items in SceneManager that match visibilityFlags
        @param sceneManager
            SceneManager ptr
        @param visibilityFlags
            Visibility Mask to exclude Items
        @param bStatic
            True to add only static Items
            False to add only dynamic Items
        */
        void addAllItems( SceneManager *sceneManager, const uint32 visibilityFlags, const bool bStatic );

        /// Adds the given item to the voxelizer in all cascades
        void addItem( Item *item );

        /// Removes the given item to the voxelizer in all cascades
        void removeItem( Item *item );

        /// You can call this directly with sceneManager, but we assume all objects
        /// are already updated (i.e. SceneManager::updateSceneGraph already called)
        ///
        /// See VctCascadedVoxelizer::setAutoUpdate
        void update( SceneManager *sceneManager );

        /// Register against the CompositorManager to call VctCascadedVoxelizer::update
        /// automatically when the CompositorManager is about to render (recommended)
        ///
        /// You still must call setCameraPosition every frame though (or whenever the camera
        /// changes)
        ///
        /// Set to nullptr to disable auto update
        void setAutoUpdate( CompositorManager2 *compositorManager, SceneManager *sceneManager );

        /// CompositorWorkspaceListener override
        virtual void allWorkspacesBeforeBeginUpdate( void );

        void setCameraPosition( const Vector3 &cameraPosition );
        const Vector3 &getCameraPosition( void ) const { return mCameraPosition; }

        VctLighting *getVctLighting( size_t idx ) { return mCascades[idx]; }
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
