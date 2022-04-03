
#ifndef _Demo_AnimationTagPointGameState_H_
#define _Demo_AnimationTagPointGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Ogre
{
    class SkeletonAnimation;
}

namespace Demo
{
    class AnimationTagPointGameState : public TutorialGameState
    {
        Ogre::SceneNode *mSphereNodes[5];
        Ogre::SceneNode *mCubesNode;
        Ogre::SceneNode *mCubeNodes[5];

        Ogre::SkeletonAnimation *mWalkAnimation;

    public:
        AnimationTagPointGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void update( float timeSinceLast ) override;
    };
}  // namespace Demo

#endif
