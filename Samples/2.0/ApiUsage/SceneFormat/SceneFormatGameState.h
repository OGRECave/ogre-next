
#ifndef _Demo_SceneFormatGameState_H_
#define _Demo_SceneFormatGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Ogre
{
    class InstantRadiosity;
    class IrradianceVolume;
    class ParallaxCorrectedCubemap;
}  // namespace Ogre

namespace Demo
{
    class SceneFormatGameState : public TutorialGameState
    {
        Ogre::String mFullpathToFile;
        Ogre::InstantRadiosity *mInstantRadiosity;
        Ogre::IrradianceVolume *mIrradianceVolume;
        Ogre::ParallaxCorrectedCubemap *mParallaxCorrectedCubemap;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

    public:
        void resetScene();
        void setupParallaxCorrectCubemaps();
        void destroyInstantRadiosity();
        void destroyParallaxCorrectCubemaps();

        Ogre::TextureGpu *createRawDecalDiffuseTex();
        void generateScene();
        void exportScene();
        void importScene();

    public:
        SceneFormatGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void destroyScene() override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
