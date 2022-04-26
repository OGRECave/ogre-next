
#ifndef _Demo_BillboardTestGameState_H_
#define _Demo_BillboardTestGameState_H_

#include "OgrePrerequisites.h"

#include "TutorialGameState.h"

namespace Demo
{
    class BillboardTestGameState : public TutorialGameState
    {
    public:
        BillboardTestGameState( const Ogre::String &helpDescription );

        void createScene01() override;
    };
}  // namespace Demo

#endif
