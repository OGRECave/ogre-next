
#ifndef _Demo_TutorialCompute01_UavTextureGameState_H_
#define _Demo_TutorialCompute01_UavTextureGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class TutorialCompute01_UavTextureGameState : public TutorialGameState
    {
        Ogre::SceneNode *mSceneNode;
        float mDisplacement;

    public:
        TutorialCompute01_UavTextureGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;
    };
}  // namespace Demo

#endif
