
#ifndef _Demo_ScreenSpaceReflectionsGameState_H_
#define _Demo_ScreenSpaceReflectionsGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

#include "Utils/ScreenSpaceReflections.h"

#include "SdlEmulationLayer.h"
#if OGRE_USE_SDL2
#    include "SDL_keyboard.h"
#endif

namespace Ogre
{
    class HlmsPbsDatablock;
}

namespace Demo
{
    class ScreenSpaceReflectionsGameState : public TutorialGameState
    {
        ScreenSpaceReflections *mScreenSpaceReflections;
        Ogre::HlmsPbsDatablock *mMaterials[4];

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

    public:
        ScreenSpaceReflectionsGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void destroyScene() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
