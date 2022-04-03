
#ifndef _Demo_NearFarProjectionGameState_H_
#define _Demo_NearFarProjectionGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class NearFarProjectionGameState : public TutorialGameState
    {
        Ogre::SceneNode *mSceneNode;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

    public:
        NearFarProjectionGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
