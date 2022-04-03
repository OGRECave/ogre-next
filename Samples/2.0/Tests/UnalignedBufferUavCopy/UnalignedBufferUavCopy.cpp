
#include "GraphicsSystem.h"
#include "UnalignedBufferUavCopyGameState.h"

#include "Compositor/OgreCompositorManager2.h"
#include "OgreConfigFile.h"
#include "OgreRoot.h"
#include "OgreWindow.h"

// Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/MainEntryPoints.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMainApp( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#else
int mainApp( int argc, const char *argv[] )
#endif
{
    return Demo::MainEntryPoints::mainAppSingleThreaded( DEMO_MAIN_ENTRY_PARAMS );
}

namespace Demo
{
    /**
        The purpose of this test is to avoid regressions when copying from 2-byte buffers
        (like index buffers) to 4-byte UAV buffers, as D3D11 and Metal are picky about alignment
        copies.

        Particularly, D3D11 is hard to workaround right.

        It throws when it detects the buffer hasn't been copied correctly
    */
    class UnalignedBufferUavCopyGraphicsSystem final : public GraphicsSystem
    {
    public:
        UnalignedBufferUavCopyGraphicsSystem( GameState *gameState ) : GraphicsSystem( gameState )
        {
            mAlwaysAskForConfig = false;
        }
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState, LogicSystem **outLogicSystem )
    {
        UnalignedBufferUavCopyGameState *gfxGameState = new UnalignedBufferUavCopyGameState( "" );

        UnalignedBufferUavCopyGraphicsSystem *graphicsSystem =
            new UnalignedBufferUavCopyGraphicsSystem( gfxGameState );

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

    const char *MainEntryPoints::getWindowTitle() { return "UnalignedBufferUavCopy"; }
}  // namespace Demo
