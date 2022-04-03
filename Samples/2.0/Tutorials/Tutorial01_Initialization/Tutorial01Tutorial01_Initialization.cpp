
//---------------------------------------------------------------------------------------
// This tutorial shows nothing fancy. Only how to setup Ogre to render to
// a window, rolling your own game loop, single threaded.
//
// You only need the basics from the Common framework. Just derive from GameState to
// perform your own scene and update it accordingly.
// Scene setup is divided in two stages (createScene01 & createScene02) because it
// is prepared for multithreading; although it is not strictly necessary in this case.
// See the multithreading tutorial for a better explanation
//---------------------------------------------------------------------------------------

#include "GraphicsSystem.h"

#include "GameState.h"

#include "OgreTimer.h"
#include "OgreWindow.h"

#include "Threading/OgreThreads.h"

// Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/MainEntryPoints.h"

using namespace Demo;

namespace Demo
{
    class MyGraphicsSystem final : public GraphicsSystem
    {
        // No resources. They're not needed and a potential point of failure.
        // This is a very simple project
        void setupResources() override {}

    public:
        MyGraphicsSystem( GameState *gameState ) : GraphicsSystem( gameState ) {}
    };

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS || \
    OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState, LogicSystem **outLogicSystem )
    {
        GameState *gfxGameState = new GameState();
        GraphicsSystem *graphicsSystem = new MyGraphicsSystem( gfxGameState );

        *outGraphicsGameState = gfxGameState;
        *outGraphicsSystem = graphicsSystem;
    }

    void MainEntryPoints::destroySystems( GameState *graphicsGameState, GraphicsSystem *graphicsSystem,
                                          GameState *logicGameState, LogicSystem *logicSystem )
    {
        delete graphicsSystem;
        delete graphicsGameState;
    }

#    if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
    const char *MainEntryPoints::getWindowTitle() { return "Tutorial 01: Initialization"; }
#    endif
#endif
}  // namespace Demo

#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
#    if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMainApp( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#    else
int mainApp( int argc, const char *argv[] )
#    endif
{
    GameState gameState;
    MyGraphicsSystem graphicsSystem( &gameState );

    // MyGraphicsSystem::setupResources overrode the folder setup process to minimize errors.
    // But this also means you'll see "WARNING: LTC matrix textures could not be loaded.
    // Accurate specular IBL reflections and LTC area lights won't be available or may not
    // function properly!" message in the Log
    graphicsSystem.initialize( "Tutorial 01: Initialization" );

    if( graphicsSystem.getQuit() )
    {
        graphicsSystem.deinitialize();
        return 0;  // User cancelled config
    }

    Ogre::Window *renderWindow = graphicsSystem.getRenderWindow();

    graphicsSystem.createScene01();
    graphicsSystem.createScene02();

    Ogre::Timer timer;
    Ogre::uint64 startTime = timer.getMicroseconds();

    double timeSinceLast = 1.0 / 60.0;

    while( !graphicsSystem.getQuit() )
    {
        graphicsSystem.beginFrameParallel();
        graphicsSystem.update( static_cast<float>( timeSinceLast ) );
        graphicsSystem.finishFrameParallel();
        graphicsSystem.finishFrame();

        if( !renderWindow->isVisible() )
        {
            // Don't burn CPU cycles unnecessary when we're minimized.
            Ogre::Threads::Sleep( 500 );
        }

        Ogre::uint64 endTime = timer.getMicroseconds();
        timeSinceLast = ( endTime - startTime ) / 1000000.0;
        timeSinceLast = std::min( 1.0, timeSinceLast );  // Prevent from going haywire.
        startTime = endTime;
    }

    graphicsSystem.destroyScene();
    graphicsSystem.deinitialize();

    return 0;
}
#endif
