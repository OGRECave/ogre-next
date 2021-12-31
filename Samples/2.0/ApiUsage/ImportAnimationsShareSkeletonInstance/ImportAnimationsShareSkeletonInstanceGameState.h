
#ifndef _Demo_ImportAnimationsShareSkeletonInstanceGameState_H_
#define _Demo_ImportAnimationsShareSkeletonInstanceGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Ogre
{
    class SkeletonAnimation;
}

namespace Demo
{
    class ImportAnimationsShareSkeletonInstanceGameState : public TutorialGameState
    {
        Ogre::SkeletonAnimation *mAnyAnimation;

    public:
        ImportAnimationsShareSkeletonInstanceGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void update( float timeSinceLast ) override;
    };
}  // namespace Demo

#endif
