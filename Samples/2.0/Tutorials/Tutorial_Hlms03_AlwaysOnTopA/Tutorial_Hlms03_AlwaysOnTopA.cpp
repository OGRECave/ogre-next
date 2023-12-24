
#include "Tutorial_Hlms03_AlwaysOnTopA.h"

#include "Tutorial_Hlms03_AlwaysOnTopAGameState.h"

// Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/Android/AndroidSystems.h"
#include "System/MainEntryPoints.h"

#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
#    if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMainApp( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#    else
int mainApp( int argc, const char *argv[] )
#    endif
{
    return Demo::MainEntryPoints::mainAppSingleThreaded( DEMO_MAIN_ENTRY_PARAMS );
}
#endif

namespace Demo
{
    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState, LogicSystem **outLogicSystem )
    {
        Hlms03AlwaysOnTopAGameState *gfxGameState = new Hlms03AlwaysOnTopAGameState(
            "Shows how to keep an object rendered always on top using an HLMS customization.\n"
            "This is very useful in e.g. First Person Shooters, because they must always show the\n"
            "hand and weapon always on top and never clip through walls.\n"
            "\n"
            "This technique exploits the simple fact that the depth range [0; near_plane * 2)\n"
            "is heavily underutilized; thus we manipulate the vertex shader to put our weapon in\n"
            "that range. This means that 100% guaranteeing the weapon is always on top is not possible\n"
            "but 99.99% of the time it will be. The beauty of this trick is that:\n\n"
            " - It is extremely easy to implement.\n"
            " - It is very flexible (e.g. with transparent submeshes in the weapon).\n"
            " - It can be easily toggled on/off.\n"
            " - It is mobile friendly.\n"
            "\n"
            "Note however:\n"
            "\n"
            " - It doesn't always prevent culling, which makes it more suitable for TPS than FPS.\n"
            "\n"
            "This technique does not rely on RenderQueue groups because it is the depth buffer\n"
            "handling order for us.\n"
            "This sample depends on the media files:\n"
            "   * Samples/Media/2.0/materials/Tutorial_Hlms03_AlwaysOnTopA\n" );

        GraphicsSystem *graphicsSystem = new Hlms03AlwaysOnTopAGraphicsSystem( gfxGameState );

        gfxGameState->_notifyGraphicsSystem( graphicsSystem );

        *outGraphicsGameState = gfxGameState;
        *outGraphicsSystem = graphicsSystem;
    }

    void MainEntryPoints::destroySystems( GameState *graphicsGameState, GraphicsSystem *graphicsSystem,
                                          GameState *logicGameState, LogicSystem *logicSystem )
    {
        delete graphicsSystem;
        delete graphicsGameState;
    }

    const char *MainEntryPoints::getWindowTitle() { return "Tutoroial: Hlms03 Always on Top (A)"; }
}  // namespace Demo
