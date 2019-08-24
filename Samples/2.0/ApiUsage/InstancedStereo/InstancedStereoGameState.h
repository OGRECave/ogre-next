
#ifndef _Demo_InstancedStereoGameState_H_
#define _Demo_InstancedStereoGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
	class InstancedStereoGameState : public TutorialGameState
    {
        Ogre::SceneNode     *mSceneNode[16];

    public:
		InstancedStereoGameState( const Ogre::String &helpDescription );

        virtual void createScene01(void);

        virtual void update( float timeSinceLast );
    };
}

#endif
