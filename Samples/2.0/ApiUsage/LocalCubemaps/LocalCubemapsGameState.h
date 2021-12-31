
#ifndef _Demo_LocalCubemapsGameState_H_
#define _Demo_LocalCubemapsGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Ogre
{
    class ParallaxCorrectedCubemap;
    class ParallaxCorrectedCubemapAuto;
    class ParallaxCorrectedCubemapBase;
    class HlmsPbsDatablock;
}  // namespace Ogre

namespace Demo
{
    class LocalCubemapsGameState : public TutorialGameState
    {
        Ogre::SceneNode *mLightNodes[3];

        Ogre::ParallaxCorrectedCubemapBase *mParallaxCorrectedCubemap;
        Ogre::ParallaxCorrectedCubemapAuto *mParallaxCorrectedCubemapAuto;
        Ogre::ParallaxCorrectedCubemap *mParallaxCorrectedCubemapOrig;
        Ogre::HlmsPbsDatablock *mMaterials[4];
        bool mUseMultipleProbes;
        bool mRegenerateProbes;
        bool mPerPixelReflections;
        bool mUseDpm2DArray;
        bool mRoughnessDirty;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

        void setupParallaxCorrectCubemaps();
        void forceUpdateAllProbes();

    public:
        LocalCubemapsGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void destroyScene() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
