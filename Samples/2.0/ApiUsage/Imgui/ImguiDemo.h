#ifndef Demo_Imgui_H
#define Demo_Imgui_H

#include "GraphicsSystem.h"

#include "OgreImguiPrerequisites.h"

namespace Demo
{
    /** We override the Demo classes from OgreNext in order to:
            1. Create & setup ImguiManager.
            2. Register Imgui's compositor provider
            3. Use a compositor script that has the dear_imgui pass.
    */
    class ImguiGraphicsSystem final : public GraphicsSystem
    {
        Ogre::ImguiManager *mImguiManager;
        Ogre::CompositorPassImguiProvider *mImguiCompoProvider;

        Ogre::CompositorWorkspace *setupCompositor() override;

        void setupResources() override;

    public:
        ImguiGraphicsSystem( GameState *gameState );

        Ogre::ImguiManager *getImguiManager() { return mImguiManager; }

#if OGRE_USE_SDL2
        void handleRawSdlEvent( const SDL_Event &evt ) override;
        bool getGrabMousePointerOnStartup() const override { return false; }
#endif

        void deinitialize() override;
    };
}  // namespace Demo

#endif
