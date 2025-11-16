
#ifndef Demo_ImguiGameState_H
#define Demo_ImguiGameState_H

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class ImguiGameState : public TutorialGameState
    {
        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

        void keyEvent( const SDL_KeyboardEvent &arg, bool keyPressed );

    public:
        ImguiGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;

        void mouseMoved( const SDL_Event &arg ) override;
        void mousePressed( const SDL_MouseButtonEvent &arg, Ogre::uint8 id ) override;
        void mouseReleased( const SDL_MouseButtonEvent &arg, Ogre::uint8 id ) override;
        void textInput( const SDL_TextInputEvent &arg ) override;
        void keyPressed( const SDL_KeyboardEvent &arg ) override;
        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
