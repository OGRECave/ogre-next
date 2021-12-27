
#ifndef _Demo_NearFarProjectionGameState_H_
#define _Demo_NearFarProjectionGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class NearFarProjectionGameState : public TutorialGameState
    {
        Ogre::SceneNode *mSceneNode;

        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

    public:
        NearFarProjectionGameState( const Ogre::String &helpDescription );

        virtual void createScene01();

        virtual void keyReleased( const SDL_KeyboardEvent &arg );
    };
}  // namespace Demo

#endif
