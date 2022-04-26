
#ifndef _Demo_ImageVoxelizerGameState_H_
#define _Demo_ImageVoxelizerGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Ogre
{
    class VctCascadedVoxelizer;
    class VctImageVoxelizer;
    class VctLighting;
}  // namespace Ogre

namespace Demo
{
    class TestUtils;
    class ImageVoxelizerGameState : public TutorialGameState
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
            There are different ways to use GI:

                1. VctOnly (Voxel Cone Tracing). Requires mCascadedVoxelizer
                   Only hlmsPbs->setVctLighting( mVctLighting ) is called

            If tight on memory and data won't be regenerated again (e.g. due to changes in light),
            mVoxelizer can be deleted.

            Likewise when using mIrradianceField; once it's done generating,
            mVctLighting (when using voxels) is not needed anymore.
        */
        enum GiMode
        {
            NoGI,
            VctOnly,
            NumGiModes
        };

        size_t mCurrCascadeIdx;  /// Which cascade the controls are currently tweaking
        Ogre::VctCascadedVoxelizer *mCascadedVoxelizer;
        float mStepSize;
        float mThinWallCounter;

        Ogre::uint32 mDebugVisualizationMode;
        Ogre::uint32 mNumBounces;

        Ogre::FastArray<Ogre::Item *> mItems;

        Scenes mCurrentScene;

        TestUtils *mTestUtils;

        void cycleCascade( bool bPrev );
        void cycleVisualizationMode( bool bPrev );
        void toggletVctQuality();
        GiMode getGiMode() const;
        void cycleGiModes( bool bPrev );

        void cycleScenes( bool bPrev );
        void destroyCurrentScene();

        void createCornellScene();
        void createSibenikScene();
        void createStressScene();

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

    public:
        ImageVoxelizerGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void destroyScene() override;
        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
