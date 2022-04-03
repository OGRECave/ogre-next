
#ifndef _Demo_TutorialUav01_SetupGameState_H_
#define _Demo_TutorialUav01_SetupGameState_H_

#include "OgreMesh2.h"
#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class TutorialUav01_SetupGameState : public TutorialGameState
    {
    public:
        TutorialUav01_SetupGameState( const Ogre::String &helpDescription );

        void createScene01() override;
    };
}  // namespace Demo

#endif
