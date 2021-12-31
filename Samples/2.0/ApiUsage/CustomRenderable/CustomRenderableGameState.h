
#ifndef _Demo_CustomRenderableGameState_H_
#define _Demo_CustomRenderableGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Ogre
{
    class MyCustomRenderable;
}

namespace Demo
{
    class CustomRenderableGameState : public TutorialGameState
    {
        Ogre::MyCustomRenderable *mMyCustomRenderable;

    public:
        CustomRenderableGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void destroyScene() override;
    };
}  // namespace Demo

#endif
