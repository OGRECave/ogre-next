
#include "GraphicsSystem.h"
#include "TutorialCompute02_UavBufferGameState.h"

#include "OgreWindow.h"

#include "Compositor/OgreCompositorManager2.h"
#include "OgreConfigFile.h"
#include "OgreRoot.h"

// Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/Android/AndroidSystems.h"
#include "System/MainEntryPoints.h"

namespace Demo
{
    class TutorialCompute02_UavBufferGraphicsSystem final : public GraphicsSystem
    {
        Ogre::CompositorWorkspace *setupCompositor() override
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(), mCamera,
                                                    "TutorialComputeTest02_UavBufferWorkspace", true );
        }

        void setupResources() override
        {
            GraphicsSystem::setupResources();

            Ogre::ConfigFile cf;
            cf.load( AndroidSystems::openFile( mResourcePath + "resources2.cfg" ) );

            Ogre::String originalDataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

            if( originalDataFolder.empty() )
                originalDataFolder = AndroidSystems::isAndroid() ? "/" : "./";
            else if( *( originalDataFolder.end() - 1 ) != '/' )
                originalDataFolder += "/";

            const char *c_locations[4] = { "2.0/scripts/materials/TutorialCompute02_UavBuffer",
                                           "2.0/scripts/materials/TutorialCompute02_UavBuffer/GLSL",
                                           "2.0/scripts/materials/TutorialCompute02_UavBuffer/HLSL",
                                           "2.0/scripts/materials/TutorialCompute02_UavBuffer/Metal" };

            for( size_t i = 0; i < 4; ++i )
            {
                Ogre::String dataFolder = originalDataFolder + c_locations[i];
                addResourceLocation( dataFolder, getMediaReadArchiveType(), "General" );
            }
        }

    public:
        TutorialCompute02_UavBufferGraphicsSystem( GameState *gameState ) : GraphicsSystem( gameState )
        {
        }
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState, LogicSystem **outLogicSystem )
    {
        TutorialCompute02_UavBufferGameState *gfxGameState = new TutorialCompute02_UavBufferGameState(
            "This sample shows how to setup and use UAV Buffers with compute shaders." );

        GraphicsSystem *graphicsSystem = new TutorialCompute02_UavBufferGraphicsSystem( gfxGameState );

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

    const char *MainEntryPoints::getWindowTitle() { return "Tutorial: Compute 02"; }
}  // namespace Demo

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
