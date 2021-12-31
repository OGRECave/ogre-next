
#ifndef _Demo_Tutorial_OpenVRGameState_H_
#define _Demo_Tutorial_OpenVRGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class Tutorial_OpenVRGameState : public TutorialGameState
    {
        Ogre::SceneNode *mSceneNode[16];

        Ogre::SceneNode *mLightNodes[3];

        bool mAnimateObjects;

        size_t mNumSpheres;
        Ogre::uint8 mTransparencyMode;
        float mTransparencyValue;

        Ogre::Item *mHiddenAreaMeshVr;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

        void setTransparencyToMaterials();

    public:
        Tutorial_OpenVRGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
