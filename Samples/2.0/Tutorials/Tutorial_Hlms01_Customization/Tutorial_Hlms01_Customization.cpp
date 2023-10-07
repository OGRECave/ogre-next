
#include "Tutorial_Hlms01_Customization.h"

#include "Tutorial_Hlms01_CustomizationGameState.h"

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
        Hlms01CustomizationGameState *gfxGameState = new Hlms01CustomizationGameState(
            "Shows how to customize an HLMS implementation using an HlmsListener\n"
            "and custom templates.\n"
            "This customization applies to all objects using the HlmsPbs implementation.\n"
            "\n"
            "This sample depends on the media files:\n"
            "   * Samples/Media/2.0/materials/Tutorial_Hlms01_Customization\n" );

        GraphicsSystem *graphicsSystem = new Hlms01CustomizationGraphicsSystem( gfxGameState );

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

    const char *MainEntryPoints::getWindowTitle() { return "Tutoroial: Hlms01 Customization"; }
}  // namespace Demo
