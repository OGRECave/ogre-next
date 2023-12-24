
#ifndef Demo_Hlms01CustomizationGameState_H
#define Demo_Hlms01CustomizationGameState_H

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class Hlms01CustomizationGameState : public TutorialGameState
    {
        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

    public:
        Hlms01CustomizationGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
