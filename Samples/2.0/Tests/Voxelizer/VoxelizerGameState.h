
#ifndef _Demo_VoxelizerGameState_H_
#define _Demo_VoxelizerGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Ogre
{
    class VctVoxelizer;
    class VctLighting;
    class IrradianceField;
}  // namespace Ogre

namespace Demo
{
    class TestUtils;
    class VoxelizerGameState : public TutorialGameState
    {
        enum Scenes
        {
            SceneCornell,
            SceneSibenik,
            SceneStress,
            NumScenes
        };

        /**
        @brief The GiMode enum
            There are 4 different ways to use GI:

                1. IfdOnly (Irradiance Fields with Depth)
                    a. Using voxels. Requires mVoxelizer, mVctLighting and mIrradianceField
                       Only hlmsPbs->setIrradianceField( mIrradianceField ) is called
                    b. Using rasterization. Requires mIrradianceField only.
                       Only hlmsPbs->setIrradianceField( mIrradianceField ) is called
                2. VctOnly (Voxel Cone Tracing). Requires mVoxelizer, mVctLighting
                   Only hlmsPbs->setVctLighting( mVctLighting ) is called
                3. IfdVct (IFD + VCT hybrid). Requires mVoxelizer, mVctLighting and mIrradianceField
                   Rasterization may be used instead of voxels for generating IFD.
                   Both hlmsPbs->setVctLighting & hlmsPbs->setIrradianceField are called

            If tight on memory and data won't be regenerated again (e.g. due to changes in light),
            mVoxelizer can be deleted.

            Likewise when using mIrradianceField; once it's done generating,
            mVctLighting (when using voxels) is not needed anymore.
        */
        enum GiMode
        {
            NoGI,
            IfdOnly,
            VctOnly,
            IfdVct,
            NumGiModes
        };

        Ogre::VctVoxelizer *mVoxelizer;
        Ogre::VctLighting *mVctLighting;
        float mThinWallCounter;

        Ogre::IrradianceField *mIrradianceField;
        bool mUseRasterIrradianceField;

        Ogre::uint32 mDebugVisualizationMode;
        Ogre::uint32 mIfdDebugVisualizationMode;
        Ogre::uint32 mNumBounces;

        Ogre::FastArray<Ogre::Item *> mItems;

        Scenes mCurrentScene;

        TestUtils *mTestUtils;

        void cycleVisualizationMode( bool bPrev );
        void toggletVctQuality();
        GiMode getGiMode() const;
        void cycleIfdProbeVisualizationMode( bool bPrev );
        void cycleIrradianceField( bool bPrev );

        void voxelizeScene();

        void cycleScenes( bool bPrev );
        void destroyCurrentScene();

        void createCornellScene();
        void createSibenikScene();
        void createStressScene();

        bool needsVoxels() const;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

    public:
        VoxelizerGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void destroyScene() override;
        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
