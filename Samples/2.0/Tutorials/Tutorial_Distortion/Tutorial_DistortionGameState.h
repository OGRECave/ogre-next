
#ifndef _Demo_DistortionGameState_H_
#define _Demo_DistortionGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class DistortionGameState : public TutorialGameState
    {
        Ogre::SceneNode *mSceneNode[16];
        Ogre::SceneNode *mDistortionSceneNode[10];

        Ogre::SceneNode *mLightNodes[3];

        bool mAnimateObjects;
        bool mAnimateDistortion;

        float mDistortionStrenght;
        Ogre::Pass *mDistortionPass;

        size_t mNumSpheres;
        Ogre::uint8 mTransparencyMode;
        float mTransparencyValue;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

        void setTransparencyToMaterials();

    public:
        DistortionGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
