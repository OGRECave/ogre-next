
#ifndef _Demo_V2MeshGameState_H_
#define _Demo_V2MeshGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class V2MeshGameState : public TutorialGameState
    {
    public:
        V2MeshGameState( const Ogre::String &helpDescription );

        void createScene01() override;
    };
}  // namespace Demo

#endif
