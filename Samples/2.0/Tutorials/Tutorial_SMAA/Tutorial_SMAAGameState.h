
#ifndef _Demo_Tutorial_SMAAGameState_H_
#define _Demo_Tutorial_SMAAGameState_H_

#include "OgreMesh2.h"
#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class Tutorial_SMAAGameState : public TutorialGameState
    {
    private:
        Ogre::SceneNode *mSceneNode[16];
        Ogre::SceneNode *mLightNodes[3];

        bool mAnimateObjects;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

    public:
        Tutorial_SMAAGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
