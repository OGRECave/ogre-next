
#ifndef Demo_InternalCoreGameState_H
#define Demo_InternalCoreGameState_H

#include "OgrePrerequisites.h"

#include "TutorialGameState.h"

namespace Demo
{
    class InternalCoreGameState : public TutorialGameState
    {
    public:
        InternalCoreGameState( const Ogre::String &helpDescription );

        void createScene01() override;
    };
}  // namespace Demo

#endif
