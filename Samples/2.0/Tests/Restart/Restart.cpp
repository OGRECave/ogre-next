
#include "GraphicsSystem.h"
#include "RestartGameState.h"

#include "Compositor/OgreCompositorManager2.h"
#include "OgreConfigFile.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreWindow.h"

// Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/MainEntryPoints.h"

namespace Demo
{
    class RestartGraphicsSystem final : public GraphicsSystem
    {
        Ogre::CompositorWorkspace *setupCompositor() override
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(), mCamera,
                                                    "PbsMaterialsWorkspace", true );
        }

        void setupResources() override
        {
            GraphicsSystem::setupResources();

            Ogre::ConfigFile cf;
            cf.load( mResourcePath + "resources2.cfg" );

            Ogre::String dataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

            if( dataFolder.empty() )
                dataFolder = "./";
            else if( *( dataFolder.end() - 1 ) != '/' )
                dataFolder += "/";

            dataFolder += "2.0/scripts/materials/PbsMaterials";

            addResourceLocation( dataFolder, getMediaReadArchiveType(), "General" );
        }

    public:
        RestartGraphicsSystem( GameState *gameState ) : GraphicsSystem( gameState )
        {
            mAlwaysAskForConfig = false;
        }
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState, LogicSystem **outLogicSystem )
    {
        RestartGameState *gfxGameState = new RestartGameState(
            "Tests Ogre can successfully shutdown and reinitialize several times without crashing" );

        GraphicsSystem *graphicsSystem = new RestartGraphicsSystem( gfxGameState );

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

    const char *MainEntryPoints::getWindowTitle()
    {
        return "Tests Ogre can successfully shutdown and reinitialize several times without crashing.";
    }
}  // namespace Demo

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMainApp( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#else
int mainApp( int argc, const char *argv[] )
#endif
{
    for( int i = 0; i < 5; ++i )
    {
        int result = Demo::MainEntryPoints::mainAppSingleThreaded( DEMO_MAIN_ENTRY_PARAMS );

        if( result != 0 )
            return result;
    }

    return 0;
}
