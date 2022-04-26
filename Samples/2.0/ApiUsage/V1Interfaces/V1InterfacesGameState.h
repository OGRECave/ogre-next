
#ifndef _Demo_V1InterfacesGameState_H_
#define _Demo_V1InterfacesGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class V1InterfacesGameState : public TutorialGameState
    {
    public:
        V1InterfacesGameState( const Ogre::String &helpDescription );

        void createScene01() override;
    };
}  // namespace Demo

#endif
