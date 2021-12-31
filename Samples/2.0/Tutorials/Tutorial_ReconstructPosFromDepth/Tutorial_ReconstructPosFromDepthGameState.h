
#ifndef _Demo_Tutorial_ReconstructPosFromDepthGameState_H_
#define _Demo_Tutorial_ReconstructPosFromDepthGameState_H_

#include "OgreMesh2.h"
#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class Tutorial_ReconstructPosFromDepthGameState : public TutorialGameState
    {
    public:
        Tutorial_ReconstructPosFromDepthGameState( const Ogre::String &helpDescription );

        void createScene01() override;
    };
}  // namespace Demo

#endif
