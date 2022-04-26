
#include "GraphicsSystem.h"
#include "NearFarProjectionGameState.h"

// Declares WinMain / main
#include "MainEntryPointHelper.h"
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
        NearFarProjectionGameState *gfxGameState = new NearFarProjectionGameState(
            "Tests whether objects are only rendered between (near; far) planes\n"
            "This test must ensure it works for all APIs, with and without Reverse Z" );

        GraphicsSystem *graphicsSystem = new GraphicsSystem( gfxGameState );

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

    const char *MainEntryPoints::getWindowTitle() { return "Test near and far plane"; }
}  // namespace Demo
