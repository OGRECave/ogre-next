
#ifndef _Demo_MyGameState_H_
#define _Demo_MyGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class MyGameState : public TutorialGameState
    {
        Ogre::SceneNode *mSceneNode;
        float mDisplacement;

    public:
        MyGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;
    };
}  // namespace Demo

#endif
