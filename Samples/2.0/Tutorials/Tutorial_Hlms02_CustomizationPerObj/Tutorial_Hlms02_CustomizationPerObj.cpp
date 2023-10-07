
#include "Tutorial_Hlms02_CustomizationPerObj.h"

#include "Tutorial_Hlms02_CustomizationPerObjGameState.h"

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
        Hlms02CustomizationPerObjGameState *gfxGameState = new Hlms02CustomizationPerObjGameState(
            "Shows how to customize an HLMS implementation using an HlmsListener,\n"
            "custom templates, and customizing HlmsPbs.\n"
            "This tutorial shows how apply a global wind to only a specific set of objects.\n"
            "\n"
            "This sample depends on the media files:\n"
            "   * Samples/Media/2.0/materials/Tutorial_Hlms02_CustomizationPerObj\n" );

        GraphicsSystem *graphicsSystem = new Hlms02CustomizationPerObjGraphicsSystem( gfxGameState );

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

    const char *MainEntryPoints::getWindowTitle() { return "Tutoroial: Hlms02 Customization"; }
}  // namespace Demo
