
#ifndef _Demo_LocalCubemapsGameState_H_
#define _Demo_LocalCubemapsGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

#include "OgreTexture.h"

namespace Ogre
{
    class ParallaxCorrectedCubemap;
    class ParallaxCorrectedCubemapAuto;
    class ParallaxCorrectedCubemapBase;
    class HlmsPbsDatablock;
}

namespace Demo
{
    class LocalCubemapsGameState : public TutorialGameState
    {
        Ogre::SceneNode     *mLightNodes[3];

        Ogre::ParallaxCorrectedCubemapBase  *mParallaxCorrectedCubemap;
        Ogre::ParallaxCorrectedCubemapAuto  *mParallaxCorrectedCubemapAuto;
        Ogre::ParallaxCorrectedCubemap      *mParallaxCorrectedCubemapOrig;
        Ogre::HlmsPbsDatablock          *mMaterials[4];
        bool                            mUseMultipleProbes;
        bool                            mRegenerateProbes;
        bool                            mPerPixelReflections;
        bool                            mUseDpm2DArray;
        bool                            mRoughnessDirty;

        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

        void setupParallaxCorrectCubemaps(void);
        void forceUpdateAllProbes(void);

    public:
        LocalCubemapsGameState( const Ogre::String &helpDescription );

        virtual void createScene01(void);
        virtual void destroyScene(void);

        virtual void update( float timeSinceLast );

        virtual void keyReleased( const SDL_KeyboardEvent &arg );
    };
}

#endif
