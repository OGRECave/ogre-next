
#ifndef _Demo_TutorialGameState_H_
#define _Demo_TutorialGameState_H_

#include "OgrePrerequisites.h"

#include "GameState.h"

namespace Ogre
{
    namespace v1
    {
        class TextAreaOverlayElement;
    }
}  // namespace Ogre

namespace Demo
{
    class GraphicsSystem;
    class CameraController;

    /// Base game state for the tutorials. All it does is show a little text on screen :)
    class TutorialGameState : public GameState
    {
    protected:
        GraphicsSystem *mGraphicsSystem;

        /// Optional, for controlling the camera with WASD and the mouse
        CameraController *mCameraController;

        Ogre::String mHelpDescription;
        Ogre::uint16 mDisplayHelpMode;
        Ogre::uint16 mNumDisplayHelpModes;

        Ogre::v1::TextAreaOverlayElement *mDebugText;
        Ogre::v1::TextAreaOverlayElement *mDebugTextShadow;

        virtual void createDebugTextOverlay();
        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

#ifdef AUTO_TESTING
        int         frameCount = 0;
        std::string sampleName;
        int         maxFrames = 500;
#endif
    public:
        TutorialGameState( const Ogre::String &helpDescription );
        ~TutorialGameState() override;

        void _notifyGraphicsSystem( GraphicsSystem *graphicsSystem );

        void createScene01() override;
#ifdef AUTO_TESTING
        void setSampleName( std::string newSampleName );
        void setFrameCount( std::string frames );
#endif
        void update( float timeSinceLast ) override;

        void keyPressed( const SDL_KeyboardEvent &arg ) override;
        void keyReleased( const SDL_KeyboardEvent &arg ) override;

        void mouseMoved( const SDL_Event &arg ) override;
    };
}  // namespace Demo

#endif
